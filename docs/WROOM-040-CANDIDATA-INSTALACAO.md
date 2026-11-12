# BOTIZIN de bancada — WROOM 0.0.40 (candidata de teste)
**Status:** compilada/testada no GitHub Actions; **não gravada no hardware**, **não publicada no manifesto `main`**, S3/A22/WROOM 0.0.38 de produção preservados.

## Decisão: não reparticionar
`app0`: 0x10000 / 0x140000 (1.310.720 bytes); `app1`: 0x150000 / 0x140000. Layout atual preservado, NVS e coredump preservados. 0.0.38 original: 1.288.144 B. Primeira compilação aprovada 0.0.40: **1.069.872 B**, economia **218.272 B**, margem **240.848 B** no mesmo slot, 81% de ocupação. Registros no workflow do branch `wroom/botizin-base-040`.

## O que funciona no CÓDIGO (não equivale a teste físico)
1. **BOTIZIN base no OTA0:** rosto procedural OLED 128x64, botões OK25/UP27/BACK32/DOWN33, Wi-Fi STA, reconexão, HTTP local `GET /status`, mDNS, GPIO26 LED temporizado.
2. **Menu de módulos:** rosto -> botão abre menu -> "Modulos" -> selecionar LED/servo/PS4/A22 Expresso -> confirmação por botão -> WROOM abre autorização de 120s (a confirmação é local e explícita).
3. **A22 agente:** `tools/a22/wroom_module_agent.py`. Lê `dispositivos.json`, confirma `WROOM 0.0.40`, `OTA READY` e `RUNNING ota0`, obtém `GET /module/request`, busca metadados via HTTPS GitHub, compara ID/versão/tamanho/SHA com o pedido, baixa e guarda a imagem em `/root/botizin-mcp/wroom-module-cache/`, verifica hash e envia por multipart `POST /update`; envia `POST /reboot` só após confirmação de gravação.
4. **WROOM verifica de novo** tamanho/SHA e destino app1, só aceita upload depois da autorização física, grava âncora de retorno NVS da base app0, persiste metadata do módulo e arma boot no OTA1. Rotas de atualização sem confirmação física são rejeitadas.
5. **Voltar:** os módulos legados `0.0.30/31/32` com protocolo 2 implementam retorno ao menu por botão, verificando âncora da imagem do BOTIZIN. Testar fisicamente o módulo LED primeiro antes de PS4, servo ou A22 Expresso.

## O que SAIU da WROOM 040
- Cliente de telemetria e comandos ThingsBoard (não é mais parte do executável).
- Consulta direta ao GitHub via HTTPS, download de firmware TLS na WROOM e atualização automática de manifesto.
- Página de atualização automática; agora só há o menu de módulos manual e ajuda.
- Configuração USB aceita apenas Wi-Fi; campos antigos de serviço são ignorados (credenciais legadas já salvas podem permanecer em NVS até migração/limpeza explicitamente autorizada).
- **S3 0.0.25 e A22** não foram modificados; funções antigas de telemetria na S3 não devem ser apagadas sem plano específico.

## LIMITAÇÃO V40: lista inicial fixa
O catálogo inicial é um instantâneo **fixo com SHA das quatro releases existentes**: PS4 0.0.30, servo 0.0.31, LED 0.0.32, A22 Expresso 0.0.34. **Não atualiza automaticamente** quando um novo módulo surge no GitHub. Isso preserva o limite de confiança do firmware: um arquivo recebido por LAN sem autenticação forte não pode autorizar um executável arbitrário. A22 pode baixar e armazenar o pacote, mas novos IDs/SHA exigirão uma próxima evolução de catálogo com autenticação/assinatura para atualização dinâmica, antes de declarar o sistema de módulos universal.

## Segurança, pressupostos e pontos de parada
- **Não há autenticação criptográfica própria do transporte HTTP LAN nesta candidata.** A proteção atual de instalação é a confirmação física na WROOM, janela de 120s, SHA/size imutáveis no catálogo compilado e validação no A22 e na WROOM. LAN não pode ser exposta na internet; testes em rede doméstica confiável. Proteger contra tráfego indesejado ainda exige revisão antes de chamar de produto seguro para redes não confiáveis.
- Se A22 ou túnel cair, o rosto e as funções locais da WROOM continuam. Instalação de módulo aguarda A22; nenhuma instalação por conta própria.
- O rosto do BOTIZIN **não roda** enquanto um firmware de módulo ocupa a execução do OTA1. A conversa via A22 pode continuar por dispositivo separado.
- **Voz ChatGPT no OLED/WROOM não foi concluída nesta versão**; não confundir com testes anteriores de voz XiaoZhi no notebook.
- OTA0 deve executar a base 040. A WROOM **atual está no OTA0 com 038**, logo um OTA comum grava no OTA1; não usar OTA comum para instalar esta base por cima da partição errada.
- A instalação inicial da base 040 requer uma operação USB validada com **backup integral de 4 MB**, preservando bootloader/tabela de partições/NVS/OTA1. O script `tools/wroom040/install_usb_040.py` valida pacote e MAC conhecido, lê o backup privado e exige `--confirm WRITE_BASE_040` antes de escrever **somente OTA0**. O script antigo `wroom/install_usb.py` é para 0.0.1 e NÃO deve ser usado para 040.
- Depois de provar a base WROOM 040 no OTA0, o script `tools/a22/ativar_modulos_wroom_040.sh` instala o agente e o integra ao **supervisor Linux existente** do A22. A instalação aborta se a WROOM ainda estiver na 0.0.38.
- Firmware e scripts ainda precisam do ciclo físico 040 base → LED 032 → BACK → 040 base antes de publicação no manifesto `main` ou confiança em módulos de motor.

## Critérios de aceite
- [x] Construção de imagem 0.0.40 sem aumentar partições.
- [x] Build isolado e testes de navegação, OTA, face, agente e segurança básica de autorização.
- [ ] Backup integral da WROOM 0.0.38 criado e validado localmente.
- [ ] Gravação USB da base 040 no app0, Wi-Fi/status/rosto e LED comprovados.
- [ ] Agente A22 habilitado no supervisor e monitorado.
- [ ] Comando pelo botão WROOM solicita LED 032, A22 baixa e valida SHA, OTA1 gravado.
- [ ] BACK do módulo retorna ao BOTIZIN 040 no OTA0 após reboot e confirma hash da base.
- [ ] Reinício físico controlado e recuperação de conexão validados.
- [ ] Dynamic catalog securely authenticated (fase seguinte, não pré-requisito do piloto LED com SHA fixo).
