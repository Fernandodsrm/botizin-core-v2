# Central provisória BOTIZIN no notebook (A22 desligado)

**13/11/2026 — desenvolvimento isolado, não instalado no notebook automaticamente.**

## Escopo
- S3 XiaoZhi LAB 0.0.41, endereço local observado `192.168.0.24`: voz/transcrição seguem pelo Chrome, **não são transferidos**.
- WROOM 0.0.40, endereço local observado `192.168.0.23`: rosto procedural e status/LED; **a sincronização dos olhos com estados reais exige firmware WROOM LAB preparado separadamente**.
- Windows usa nova venv em `%USERPROFILE%\BOTIZIN_WINDOWS_TEMP\.venv`, MCP **só em loopback** `127.0.0.1:8766/mcp`. O servidor 8765 existente, A22, túnel e firmwares **não são modificados**.
- Ferramentas do novo MCP: `consultar_status_botizin`, `consultar_telemetria_s3`, `consultar_status_wroom`, `controlar_led_wroom` (GPIO26 com off automático em 30s) e `consultar_central_windows`.
- Segurança: sem shell remoto, sem escaneamento LAN, sem leitura de chaves, sem credenciais no GitHub, sem abrir portas de roteador, sem instalar serviço persistente no Windows. O usuário inicia manualmente o script ao ligar notebook.
- O novo MCP em loopback **não** restabelece sozinho a conexão do ChatGPT via plugin: o plugin anterior continua apontando para o túnel do A22. A integração remota exige um túnel HTTPS com autenticação configurado para este MCP e reconfiguração explícita da conexão do plugin.
- `MacroDroid`, abertura de apps no A22 e recursos do Android **não** são reproduzidos pelo Windows.
- Tailscale Serve é alternativa futura para acesso do **A35** no tailnet, mas não substitui diretamente um túnel público/autenticado para chamadas do serviço ChatGPT em nuvem; a configuração exige HTTPS, Tailscale conectado e validação de WebSocket da voz.

## Ordem de ativação
1. Manter a aba Chrome atual no XiaoZhi S3. Verificar acesso LAN da S3 e WROOM.
2. Executar inicializador PowerShell a partir da origem de commit fixada, que baixa o `windows_mcp.py`, cria venv isolada e inicia só na porta 8766.
3. Validar `WINDOWS_HUB_SELF_TEST_OK` e `MCP WINDOWS LOCAL`. Sem reiniciar 8765.
4. Fazer teste local das ferramentas via cliente MCP, antes de expor HTTPS.
5. Para recuperar recursos remotos do A35/ChatGPT, levantar a configuração do túnel **existente no notebook** e validar autenticação, TLS e conectividade. Não usar túnel público sem proteção.
6. Quando A22 voltar, parar só a central provisória usando `stop_windows_hub.ps1` e conferir serviços antigos; não duplicar um túnel permanente.
7. Sincronização OLED: exigir receptor WROOM LAB já gravado e validado (base app0 preservada), depois adaptar transmissor Windows ao IP autorizado. **Não ativar polling contra firmware 0.0.40.**

## Critério de aceitação da fase local
- S3 responde `XIAOZHI_REAL`; WROOM responde `BOTIZIN WROOM V0.0.40` (ou LAB validada).
- `windows_mcp.py --self-test` passa; Python SDK v2 `MCPServer` disponível.
- Servidor 127.0.0.1:8766 responde protocolo MCP; funcionalidade de rede não confundida com teste do browser.
- Nenhuma porta `0.0.0.0` nem firewall exposto; túnel **ainda não configurado**.
- Caso a bateria A22 volte inesperadamente, as duas centrais não conflitam na porta, e o plugin antigo permanece apontando para A22.
