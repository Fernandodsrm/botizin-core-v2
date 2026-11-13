# BOTIZIN — mapa de arquitetura e auditoria executiva (13/11/2026)

**Base:** leitura das fontes reais das duas placas, status MCP no hardware, rastreio do fluxo de áudio, OLED, OTA, Windows e túnel. **Esta nota não muda nenhuma placa.** A fonte S3 está em `Fernandodsrm/botizin-core` branch `lab/s3-voice-transcript-mcp-041`; a fonte WROOM testada está em `Fernandodsrm/botizin-core-v2` branch `wroom/voice-face-direct-s3-042`. O `main` do core-v2 contém firmware CORE legado e documentação; **não é o firmware XiaoZhi .41 ativo**. As 2.291 entradas do snapshot XiaoZhi incluem arquivos upstream e assets; a auditoria de execução concentrou-se no caminho efetivo da placa e seus módulos principais, não validou cada dependência upstream.

## Diagrama mental único

```text
Microfone Chrome notebook -> /botizin/audio WS -> S3 0.0.41 (XiaoZhi)
          ^                                  | codec PCM/Opus, estados, IA externa
          |<-- som PCM para navegador <------|
          |
          +--- página /botizin e transcrição STT/TTS na S3

S3 GET /status -- Wi-Fi local --> WROOM 0.0.42, a cada ~650 ms
                                      | estado 3/4/5/6, TTL 4 s
                                      +--> OLED SSD1306 olhos + botões/menu
                                      +--> LED GPIO26, diagnósticos
                                      +--> OTA0 base .40 / OTA1 rosto .42

ChatGPT (A35 ou notebook) <--> Secure MCP Tunnel (saída TLS)
                              <--> tunnel-client Windows
                                   <--> MCP 127.0.0.1:8766
                                        <--> S3 e WROOM via LAN
```

**Não confundir as três conversas:** ChatGPT (nuvem), XiaoZhi (backend de IA próprio) e MCP (ações/consulta). O ChatGPT **não roda dentro da placa**; seu plugin remoto **não carrega áudio da voz**.

## S3 — código revisado e papel atual

**Hardware:** ESP32-S3 N16R8, 16 MB flash e 8 MB PSRAM, IP observado `192.168.0.24`.

- `main/application.cc`, `device_state_machine.cc` e `device_state.h`: inicia Wi-Fi, ativa XiaoZhi, alterna repouso/conectando/ouvindo/falando, trata eventos servidor/usuário, stop e autosilêncio; estado 3=idle, 4=connecting, 5=listening, 6=speaking.
- `boards/botizin-web-s3/botizin_web_board.cc`: adapta a placa ao áudio navegador, registra ações MCP internas `self.botizin.test_action` (comprovada) e `self.chatgpt.open_voice` (abertura Android ainda não comprovada).
- `web_audio_server.cc` + `web_audio_codec.h`: servidor HTTP `/botizin`, `/status`, `/lab/telemetry`, WebSocket `/botizin/audio`, uma sessão de navegador, filas PCM de microfone/alto-falante, contadores, STT/TTS enviados ao navegador, limitação de frames e buffers.
- `web_panel.h`: interface, permissão de microfone Chrome, WebSocket, botões continuar/encerrar, transcrição efêmera e estados apresentados.
- `audio/audio_service.cc`: filas de codificação/decodificação Opus, tarefas de áudio e troca com backend; `protocols/websocket_protocol.cc` é o transporte remoto XiaoZhi.
- `mcp_server.cc`: protocolo de ferramentas **dentro da S3**, distinto do servidor MCP do notebook; `phone_bridge.h` busca Android/A22 por IP fixo e ainda não validou abertura real de voz.
- `lab_return_025.cc` e `ota.cc`: mecanismo de OTA e retorno ao CORE 0.0.25; não confundir `lab_confirmed:false` com firmware inválido.
- **No hardware:** S3 XiaoZhi LAB **0.0.41 em OTA1**, CORE **0.0.25 em OTA0** preservado; voz, transcrição e turno ouvindo/falando comprovados. Áudio descartado PCM teve contador acumulado; investigar deltas, não interpretá-lo como perda atual de pacotes sem medição.

## WROOM — código revisado e papel atual

**Hardware:** ESP32-WROOM-32 clássico, flash 4 MB, **sem PSRAM**, IP observado `192.168.0.23`, OLED SSD1306 I²C 128×64 SDA21/SCL22.

- `firmware.ino`: `setup()` inicia OLED, botões, Wi-Fi, diagnósticos, endpoint HTTP, OTA; `loop()` gerencia LEDs, menu, polling S3, catálogo e HTTP.
- `voice_poll_s3.h` + `voice_sync_state.h`: lê **somente o estado** da S3 por HTTP na LAN (650 ms, timeouts 200/220 ms), verifica `XIAOZHI_REAL`, mantém estado atômico com TTL de 4s; não envia microfone nem áudio.
- `face_engine.h` + `face_state.h` + `oled_status.h`: gera o rosto procedural, piscar/olhar, substitui expressão conforme estado real e desenha OLED em tarefa de ~16 FPS protegida por mutex I²C.
- `navigation.h`, `navigation_model.h`, `catalog_state.h`, `environment_catalog.h` e `module_bridge.h`: botões/menu, manifesto de módulos, seleção, aprovação, retorno e launcher em **único slot OTA1** (módulos não executam em paralelo).
- `remote_led.h`: LED GPIO26 com desligamento automático após 30 s; `diagnostics.h`: heap/tempos/memória; `provisioning.h`: Wi-Fi salvo em NVS; `local_name.h`: mDNS.
- **No hardware:** OTA0 **0.0.40** intacta, OTA1 **LAB 0.0.42** executando e recebendo estados 5/6 reais; próximo boot escolhido OTA0 (proteção de teste). **Não reiniciar por engano.**

## Windows, A22 e túnel — quem faz o quê

- **Notebook ligado:** Chrome faz microfone+áudio da S3. Python `tools/windows_hub/windows_mcp.py` em `127.0.0.1:8766/mcp` oferece consultas S3/WROOM, telemetria e LED físico; não instala firmware nem acessa A22. O Secure MCP Tunnel conecta *ChatGPT na nuvem* ao MCP privado usando saída HTTPS autenticada. O túnel **não substitui** servidor local, rede Wi-Fi nem transporte WebSocket do XiaoZhi.
- **A35:** cliente ChatGPT. Com túnel ativo no notebook, consultas/ações MCP seguem funcionando de fora, mas abrir o áudio local `192.168.0.24` de fora de casa exige rota segura adicional (VPN/HTTPS).
- **A22:** tinha Termux/PRoot, MCP 8765, tunnel-client, supervisor, MacroDroid e ponte Android; está fora de operação por problema físico. O notebook substituiu **status e controle MCP**, não todas as automações Android. Não afirmar A22 restaurado.
- **Segurança:** S3 HTTP `/status` inclui informação de ativação para painel local; não publicar porta 80 na internet. Windows MCP filtra esse campo e usa loopback. Chaves do tunnel-client permanecem locais, nunca no repositório.

## Estabilização em elaboração (não instalada)

A **0.0.43 candidata estável** deriva da .42 e mantém os olhos da S3. Em todo início pela OTA1, seleciona primeiro a OTA0 como próximo boot. Somente após 60 s com OLED, Wi-Fi, memória e leituras S3 verificadas seleciona novamente a OTA1 para os reinícios futuros. A imagem de 0.0.40 em OTA0 não é regravada. Isso evita regressão imediata no primeiro teste, mas não garante recuperação contra toda falha de hardware/bootloader.

**Conflito arquitetural sem decisão:** ocupando OTA1 permanentemente com rosto, o menu atual não tem um segundo slot livre para instalar e executar outros módulos. Caminhos futuros: (A) consolidar rosto/menu como **base OTA0** com rollback/backup testados, deixando OTA1 para módulos; (B) manter rosto como firmware único OTA1 e instalar módulos noutra placa/arquitetura. Não colocar rosto na OTA0 sem aceitação do usuário e plano de recuperação.

## Próximas decisões, em ordem

1. Instalar e validar .43 somente se compilação, SHA e testes passarem, com backup 4MB e retorno testado à .40; teste de voz contínua, consumo e reconexão.
2. Escolher desenho OTA0/OTA1 para o BOTIZIN base + módulos antes de aumentar o menu.
3. Escolher central ligada 24h: A22 reparado, notebook sob demanda ou host barato/reciclado. O **túnel não exige notebook**, exige um cliente sempre ativo ao lado do MCP.
4. Planejar microfone e saída de áudio própria para liberar o notebook, sem comprar hardware antes de validar alternativas.
5. Só então ações reais por voz entre XiaoZhi e MCP/ChatGPT, com permissões e confirmação de ações físicas.


## Preparacao efetiva para estabilizar os olhos (sem gravacao remota)
- Firmware WROOM **candidata 0.0.43** compilado com SHA256 `d462bec420b8ca8c30e9d86be78ad29e8472257c6c62d9c0947037f59650f705`, 1.104.992 bytes; imagem verificada publicada no branch isolado [wroom/voice-face-stable-043](https://github.com/Fernandodsrm/botizin-core-v2/tree/wroom/voice-face-stable-043), com [README de instalacao](https://github.com/Fernandodsrm/botizin-core-v2/blob/wroom/voice-face-stable-043/tools/wroom043/README.md).
- CI de compilacao ESP32 3.3.12 [43484881807](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/43484881807), instalador Windows 5.1 + testes OTA [43485352096](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/43485352096) e arquivamento de binario [43485364367](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/43485364367) **aprovados**.
- Nova protecao da .43: app0 .40 armado primeiro, promocao app1 apenas apos 60s com leitura S3/ OLED/RAM saudaveis, maximo tres tentativas espacadas de 5s; **nao testada fisicamente ainda**. Nenhuma placa foi gravada durante preparacao.
- O servidor MCP Windows foi preparado em fonte para ler `WROOM V0.0.43` ([branch da central](https://github.com/Fernandodsrm/botizin-core-v2/tree/lab/windows-hub-a22-offline-20261113)); instalacao local e reinicio do processo continuam pendentes. Ultima consulta MCP posteriormente falhou por falta de polling do cliente de tunel; **nao inferir WROOM offline**.
