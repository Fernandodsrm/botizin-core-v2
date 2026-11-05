#!/usr/bin/env python3
"""BOTIZIN Command Center - instalador da descoberta automatica de ESP32.
Somente consultas HTTP locais, sem OTA ou acesso remoto generico.
Executar no Debian do A22 com /root/botizin-mcp/.venv/bin/python.
"""
import ast
import os
import py_compile
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT=Path("/root/botizin-mcp")
ACTIVE=ROOT/"servidor.py"
TOOLS={"consultar_status_botizin","consultar_status_wroom","consultar_status_ambas","consultar_status_a22"}
PATTERN=r"^/root/botizin-mcp/\.venv/bin/python servidor\.py$"
MARKER="# BOTIZIN_DISCOVERY_V1"
HELPERS=r'''
# BOTIZIN_DISCOVERY_V1
def botizin_descobrir_status(placa):
    """Usa ultimo IP confirmado; se necessario procura ESP32 na LAN /24."""
    import concurrent.futures
    import ipaddress
    import json
    import os
    import socket
    import tempfile
    import threading
    import time
    import urllib.request
    from pathlib import Path

    if placa not in ("s3", "wroom"):
        raise ValueError("Placa desconhecida")

    arquivo=Path("/root/botizin-mcp/botizin_ips.json")
    padrao={"s3":"192.168.0.2","wroom":"192.168.0.10"}
    assinaturas={"s3":"BOTIZIN CORE V","wroom":"BOTIZIN WROOM V"}
    # A busca so ocorre quando nao ha resposta valida no endereco salvo.
    if not hasattr(botizin_descobrir_status, "_trava"):
        botizin_descobrir_status._trava=threading.RLock()

    def carregar():
        try:
            dados=json.loads(arquivo.read_text(encoding="utf-8"))
            if not isinstance(dados,dict):
                return {}
            return dados
        except (OSError,ValueError,TypeError):
            return {}

    def salvar(dados):
        fd,nome=tempfile.mkstemp(prefix=".botizin-ips-",dir=arquivo.parent)
        try:
            with os.fdopen(fd,"w",encoding="utf-8") as arq:
                json.dump(dados,arq,sort_keys=True)
                arq.flush()
                os.fsync(arq.fileno())
            os.chmod(nome,0o600)
            os.replace(nome,arquivo)
        finally:
            if os.path.exists(nome):
                os.unlink(nome)

    def ip_valido(ip):
        try:
            obj=ipaddress.ip_address(ip)
            return obj.version==4 and obj.is_private and not obj.is_loopback
        except ValueError:
            return False

    def obter(ip,tempo=1.5):
        if not ip_valido(ip):
            return None,None
        try:
            opener=urllib.request.build_opener(urllib.request.ProxyHandler({}))
            req=urllib.request.Request("http://"+ip+"/status",
                headers={"User-Agent":"Botizin-Discovery/1.0"})
            with opener.open(req,timeout=tempo) as resposta:
                if resposta.status != 200:
                    return None,None
                texto=resposta.read(16384).decode("utf-8","replace")
            for nome,inicio in assinaturas.items():
                if texto.startswith(inicio):
                    return nome,texto
        except Exception:
            pass
        return None,None

    def subnet_local():
        # Consulta somente a rota do Wi-Fi domestico; nao envia pacote UDP.
        try:
            with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as sock:
                sock.connect(("192.168.0.1",80))
                origem=sock.getsockname()[0]
            addr=ipaddress.ip_address(origem)
            if addr.is_private and not addr.is_loopback and not addr.is_link_local:
                rede=ipaddress.ip_network(origem+"/24",strict=False)
                if rede.num_addresses==256:
                    return rede
        except (OSError,ValueError):
            pass
        return None

    def sondar(ip):
        try:
            with socket.create_connection((ip,80),timeout=0.4):
                return ip
        except OSError:
            return None

    with botizin_descobrir_status._trava:
        dados=carregar()
        lista=[dados.get(placa),padrao[placa]]
        for endereco in dict.fromkeys(x for x in lista if x):
            nome,texto=obter(endereco)
            if nome==placa:
                if dados.get(placa)!=endereco:
                    dados[placa]=endereco
                    salvar(dados)
                return texto
        # Evita repetir uma busca completa a cada pedido quando a placa cai.
        if time.time()-float(dados.get("ultima_varredura",0))<40:
            raise ConnectionError("BOTIZIN "+placa.upper()+" sem resposta; aguardando nova busca")

        rede=subnet_local()
        if rede is None:
            raise ConnectionError("Nao foi possivel identificar a rede local do A22")
        ips=[str(ip) for ip in rede.hosts()]
        encontrados={}
        # 24 verificacoes TCP simultaneas; apenas LAN local; nunca portas de escrita.
        with concurrent.futures.ThreadPoolExecutor(max_workers=24) as pool:
            abertos=[ip for ip in pool.map(sondar,ips) if ip]
            for ip in abertos:
                nome,texto=obter(ip,tempo=1.2)
                if nome:
                    encontrados[nome]=(ip,texto)
        for nome,(ip,_) in encontrados.items():
            dados[nome]=ip
        dados["ultima_varredura"]=time.time()
        salvar(dados)
        if placa in encontrados:
            return encontrados[placa][1]
        raise ConnectionError("BOTIZIN "+placa.upper()+" nao encontrado na rede "+str(rede))
'''

REPLACEMENTS={
    "consultar_status_botizin":'''@mcp.tool()
def consultar_status_botizin() -> dict:
    """Consulta somente a ESP32-S3 usando IP verificado e descoberta LAN."""
    return {"origin":"BOTIZIN_CORE","status_texto":botizin_descobrir_status("s3")}
''',
    "consultar_status_wroom":'''@mcp.tool()
def consultar_status_wroom() -> dict:
    """Consulta somente a ESP32-WROOM usando IP verificado e descoberta LAN."""
    return {"origin":"BOTIZIN_WROOM","status_texto":botizin_descobrir_status("wroom")}
'''
}

def tools_and_nodes(source):
    tree=ast.parse(source)
    found={}
    for node in tree.body:
        if not isinstance(node,ast.FunctionDef):
            continue
        if any(isinstance(d,ast.Call) and isinstance(d.func,ast.Attribute)
               and isinstance(d.func.value,ast.Name)
               and d.func.value.id=="mcp" and d.func.attr=="tool" for d in node.decorator_list):
            found[node.name]=node
    return found

def construir(source):
    if MARKER in source:
        return None
    nodes=tools_and_nodes(source)
    if set(nodes)!=TOOLS:
        raise ValueError("Conjunto MCP inesperado: "+", ".join(sorted(nodes)))
    for name in REPLACEMENTS:
        node=nodes[name]
        if len(node.decorator_list)!=1 or node.args.args or node.args.kwonlyargs:
            raise ValueError("Assinatura inesperada na consulta "+name)
    linhas=source.splitlines(keepends=True)
    # Insere helpers antes do primeiro decorator das consultas.
    insertion=min(min([n.lineno]+[d.lineno for d in n.decorator_list])
                  for n in [nodes["consultar_status_botizin"],nodes["consultar_status_wroom"]])
    edits=[]
    for name,text in REPLACEMENTS.items():
        node=nodes[name]
        start=min([node.lineno]+[d.lineno for d in node.decorator_list])
        edits.append((start,node.end_lineno,text+"\n"))
    for start,end,text in sorted(edits,reverse=True):
        linhas[start-1:end]=[text]
    linhas[insertion-1:insertion-1]=["\n"+HELPERS+"\n"]
    novo="".join(linhas)
    if set(tools_and_nodes(novo))!=TOOLS or novo.count(MARKER)!=1:
        raise ValueError("Falha na validacao estrutural")
    return novo

def tcp_local(porta):
    try:
        with socket.create_connection(("127.0.0.1",porta),timeout=1):
            return True
    except OSError:
        return False

def aguardar_mcp(segundos=45):
    ate=time.monotonic()+segundos
    while time.monotonic()<ate:
        if tcp_local(8765):
            return True
        time.sleep(2)
    return False

def reiniciar_mcp():
    subprocess.run(["pkill","-f",PATTERN],check=False)

def validar(candidato):
    import runpy
    funcoes=runpy.run_path(str(candidato))
    if set(tools_and_nodes(candidato.read_text(encoding="utf-8"))) != TOOLS:
        raise ValueError("Ferramentas registradas invalidas")
    if not all(callable(funcoes[n]) for n in TOOLS):
        raise ValueError("Ferramenta nao executavel")
    diagnostico=funcoes["consultar_status_a22"]()
    if diagnostico.get("origin")!="BOTIZIN_A22":
        raise ValueError("Diagnostico A22 inesperado")
    # Teste estrutural: sem exigir que a ESP32 esteja online durante instalacao.
    d=funcoes["botizin_descobrir_status"]
    if not callable(d):
        raise ValueError("Descoberta nao registrada")
    print("VALIDACAO OK: quatro ferramentas, helper de descoberta e A22.",flush=True)

def main():
    if not ACTIVE.is_file():
        print("PARADO: servidor.py nao encontrado")
        return 2
    try:
        src=ACTIVE.read_text(encoding="utf-8-sig")
        if MARKER in src:
            print("JA INSTALADO: descoberta automatica presente; sem alteracoes")
            return 0
        new=construir(src)
    except Exception as e:
        print("PARADO, SEM ALTERAR:",type(e).__name__,str(e)[:200])
        return 2

    fd,tempname=tempfile.mkstemp(prefix=".botizin-discovery-",suffix=".py",dir=ROOT)
    temp=Path(tempname)
    backup=None
    installed=False
    try:
        with os.fdopen(fd,"w",encoding="utf-8") as f:
            f.write(new)
            f.flush()
            os.fsync(f.fileno())
        os.chmod(temp,0o600)
        py_compile.compile(str(temp),doraise=True)
        if not tcp_local(8765):
            print("PARADO: MCP nao responde na porta 8765; nada instalado")
            return 2
        validar(temp)
        backup=ROOT/("servidor.py.backup-discovery-"+time.strftime("%Y%m%d-%H%M%S")+"-"+str(os.getpid()))
        shutil.copy2(ACTIVE,backup)
        os.chmod(backup,0o600)
        print("BACKUP:",backup.name,flush=True)
        os.replace(temp,ACTIVE)
        installed=True
        print("INSTALADO. Reiniciando apenas MCP...",flush=True)
        reiniciar_mcp()
        time.sleep(2)
        if aguardar_mcp():
            print("CONCLUIDO: descoberta automatica instalada, MCP ONLINE.",flush=True)
            print("As quatro ferramentas continuam com os mesmos nomes.",flush=True)
            return 0
        raise RuntimeError("MCP nao voltou apos atualizacao")
    except Exception as e:
        print("FALHA:",type(e).__name__,str(e)[:180],flush=True)
        if installed and backup and backup.is_file():
            shutil.copy2(backup,ACTIVE)
            os.chmod(ACTIVE,0o600)
            reiniciar_mcp()
            print("BACKUP RESTAURADO.", "MCP ONLINE." if aguardar_mcp() else "MCP INDISPONIVEL.",flush=True)
        return 1
    finally:
        temp.unlink(missing_ok=True)

if __name__=="__main__":
    sys.exit(main())
