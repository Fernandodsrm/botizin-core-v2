# BOTIZIN S3 — ACEITE DO MÓDULO WI-FI 2.2
**CORE 0.0.28 · Testes na ESP32-S3 · 17/11/2026**

## Decisão
Módulo Wi-Fi **encerrado funcionalmente e aceito para continuidade do projeto**. A CORE 0.0.28 passa a ser o checkpoint operacional do módulo Wi-Fi, sem alterar a estabilidade histórica da CORE 0.0.25 nem habilitar atualizações automáticas pela branch main.

### Evidências confirmadas em placa / screenshots do teste
| Área | Resultado observado |
| --- | --- |
| Inicialização / web | CORE 0.0.28 no slot app1 / ota_1, estado VALID; painel HTTP abriu, RAM interna ~234 KiB livres |
| Rede doméstica | Conectou à Thor, IP 192.168.0.32, leitura de RSSI e detalhes de rede funcionando |
| Autenticação | Login administrativo único no painel; sessão solicitou nova autenticação após reconexão |
| Busca Wi-Fi | Lista de redes de 2,4 GHz exibida; SSID Fernando's Galaxy A22 encontrado |
| Teste sem salvar | Conectou à rede do Galaxy A22, recebeu 10.72.11.1 e retornou à Thor automaticamente |
| Acesso alternativo | Painel apresentou IP do teste com botões para copiar e abrir; a cópia foi confirmada na interface |
| Duas redes lembradas | Após salvar, painel exibiu **2/5**: Thor e Fernando's Galaxy A22, sem exibir senhas |
| Conexão ao hotspot | Ao conectar ao hotspot do A22, o painel abriu em http://10.72.11.1/ e a lista indicou A22 conectado |
| Fallback automático | **Confirmado pelo usuário**: após desligar o hotspot, a S3 voltou sozinha à Thor, sem redigitar senha, sem F5 nem nova configuração; painel abriu com login |
| Seleção por sinal | A regra de não trocar só porque outra rede tem sinal melhor está implementada; teste de longa duração ainda não realizado |

### Limites e pendências não bloqueantes
1. **Nova tela do portal de recuperação ainda não validada fisicamente**: quando nenhuma rede conhecida está disponível, a S3 deve abrir a rede temporária BOTIZIN-S3-XXXX e, depois de configurar outra rede, mostrar IP e botões de acesso durante um período curto. A recuperação tradicional já havia funcionado em teste real anterior na CORE 0.0.27. Não apresentar esse detalhe novo como validado.
2. **mDNS no hotspot**: http://botizin-s3.local/ não foi resolvido pelo Windows na rede do A22 em teste anterior. A S3 informou MDNS READY, porém isso não é garantia de resolução pelo cliente. A alternativa pelo IP direto funcionou.
3. **IP por DHCP** pode mudar; IP copiado é o endereço obtido no teste, não uma reserva permanente. Se a S3 ficar conectada e o IP mudar enquanto mDNS não resolve, consultar clientes do roteador ou recuperar via manutenção USB.
4. **Segurança**: controle por senha e sessão local, sem HTTPS; senhas Wi-Fi na NVS sem criptografia da flash. Não expor o HTTP local diretamente à internet. Guardar a senha exclusiva do AP de emergência em local seguro.

### Arquitetura aprovada
- **Até cinco redes** na NVS. A preferida conecta primeiro; outras são candidatas apenas quando a ativa fica indisponível. Não trocar automaticamente por RSSI maior.
- **Teste** conecta à rede candidata, verifica, retorna à antiga e mostra resultado. **Salvar** é separado e a rede fica disponível após reiniciar.
- **Rede temporária de recuperação** só é iniciada quando nenhuma rede é alcançada; acesso http://192.168.4.1/ com senha individual da placa.
- Wi-Fi independente do Galaxy A22 como controlador e do notebook para operação normal.
- Mantidos MQTT observador, OTA, status, painel e mDNS; ThingsBoard/RPC antigos desativados nesta candidata.

### Identidade / preservação
- Branch de desenvolvimento: `candidate/wifi2-final-0.0.28`
- Compilação CI bem-sucedida: https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/44012438325
- BIN da candidata: `packages/wifi2-0.0.28/firmware.bin`
- SHA-256: `2e6269f79f559a63f36d7c3f225832dbb62bcf255eb57da764958eeb5edeb624`
- Tamanho: **1.292.560 bytes**
- Flash na placa: **app1 / ota_1**. Manter **CORE 0.0.25 no app0**.
- `main/manifest.json` segue em **0.0.25**; a 0.0.28 não foi publicada em produção nem definida para atualização automática.

### Próxima fase
**Congelar o módulo Wi-Fi**, sem reescrevê-lo ao começar outro. Escolher e implementar o próximo módulo separadamente, preservando diagnóstico HTTP, recursos OTA e os cinco comportamentos já verificados. Se surgir regressão, consultar este relatório e o manual `docs/WIFI22-FECHAMENTO-028.md` antes de alterar o Wi-Fi.
