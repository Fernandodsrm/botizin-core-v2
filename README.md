# BOTIZIN — Central do projeto (atualizada em 11/11/2026)

## Checkpoint de recuperação S3 — 15/11/2026

**Situação observada fisicamente (somente S3):** a placa ESP32-S3 N16R8 voltou a executar o aplicativo original **CORE 0.0.25**, publicado em `releases/0.0.25/firmware.bin` (1.259.872 bytes; SHA-256 `bc68a1c45c43f4b0e63d40767521b50290a664725568cd0e6acfae24c37e049a`). A gravação USB em `app0` (`0x10000`) e a verificação por leitura foram concluídas; `otadata` foi apagada para selecionar `app0`. O boot reportou `RUNNING_PARTITION: app0`, estado `VALID`, 16 MB flash, 8 MB PSRAM e `OTA: READY`. **O bootloader e a tabela de partições não foram reinstalados: permanecem os do pacote CORE.1**; compatibilidade observada nesta inicialização, sem declarar equivalência completa.

**Teste novo de recuperação de Wi-Fi:** após não encontrar a rede conhecida, a S3 ativou `BOTIZIN-S3-...` e apresentou o portal `http://192.168.4.1/`. O usuário selecionou a rede doméstica 2,4 GHz, salvou a senha localmente e verificou reconexão. Depois `http://botizin-s3.local/status` respondeu, com `WIFI: OK`, `MDNS: READY`, `HTTP_SERVER_STARTED: YES`, `HTTP_CONFIRMED_BY_REQUEST: YES`, `INTERNET_OTA: UP_TO_DATE 0.0.25`, `TELEMETRY: HTTP_200` e MQTT publicado. A janela de espera inicial do firmware inclui tentativa STA de até 30 s e recuperação AP por tempo de desconexão de ~35 s; **os tempos reais ponta a ponta ainda precisam ser medidos**, não deduzidos das constantes.

**Limites e cuidados:** `PEER_PAIRED: NO`; comunicação S3↔WROOM e troca/retorno físico entre versões OTA **não foram validadas nesta recuperação**. O backup integral de 16 MB tentou ler a flash, mas falhou com erro USB de transferência em ~25%; **não existe backup integral válido deste procedimento**. A NVS não foi apagada na restauração. `OTA_AUTOMATIC_ENABLED: YES`: o manifesto publicado segue na 0.0.25, mas **mudar o manifesto de produção pode provocar OTA automática**; proteger esse caminho antes de ensaios. Não modificar `manifest.json`, `app0`, slots nem WROOM para fins de auditoria.

**Direção aprovada:** manter o hardware na CORE 0.0.25 e auditar os subsistemas um a um (boot/partições; Wi-Fi/portal/mDNS; HTTP/painel/auth; eventos/diagnósticos; OTA GitHub/local e rollback; telemetria ThingsBoard; MQTT; RPC/pareamento; comunicação entre placas). Classificar **confirmado / parcialmente testado / não testado / candidato a substituir**, sempre medindo uso de RAM/flash e latência antes de remover. O painel já tem abas e recursos de controle: reaproveitar a implementação existente. Futuro BOTIZIN 2.0: S3 como central local, CORE estável preservado, A22 auxiliar e XiaoZhi integrado por etapas, sem tornar a comunicação local dependente de voz, internet ou serviços externos. Nenhuma regravação de firmware foi feita para registrar este checkpoint.


**A ideia em 30 segundos:** um ecossistema de cinco placas com resultados úteis, não mais uma coleção de testes. ESP32-S3 + A22 fazem a central; ESP32-WROOM é o **BOTIZIN de bancada**, rosto OLED, menu e lançador de módulos pelo GitHub; duas ESP32-C3 com telas serão um par de comunicação; uma ESP8266 com OLED será ferramenta portátil de IR/Wi-Fi. A35 é o cliente de conversa com ChatGPT. O carrinho 2WD e o mini robô ficam em fila, sem receber uma placa por impulso.

**[VISÃO MESTRE — decisões, prioridades, arquitetura, resultados e limites técnicos](docs/BOTIZIN-VISAO-MESTRE-2026-11-11.md)**

**[INVENTÁRIO — peças físicas, estado e chegada de placas](docs/inventario-hardware-botizin-lab-2026-11.md)**

**[AUDITORIA TÉCNICA DA WROOM 0.0.38 — tamanho, fontes exatas, SPIFFS e histórico 0–38](docs/AUDITORIA-WROOM-038-HISTORICO-E-OTIMIZACAO-2026-11-11.md)**

**Checkpoint validado em 13/11/2026:** [LAB S3 0.0.41 — voz, transcrição, MCP, áudio e plano dos olhos OLED WROOM](docs/BOTIZIN-CHECKPOINT-VOZ-MCP-OLED-2026-11-13.md). S3 LAB 0.0.41 testada pelo proprietário; WROOM 0.0.40 preservada; próximas verificações incluem perdas PCM e handoff A22.

## Onde estamos realmente
- **ESP32-S3:** CORE 0.0.25, OTA/Wi-Fi, comunicação e diagnósticos confirmados. A voz XiaoZhi foi testada pelo proprietário no notebook com LAB; o CORE de produção atual não é a LAB de voz.
- **ESP32-WROOM:** 0.0.38, OLED com rosto BOTIZIN, menu/catálogo e LED GPIO26 controlado pelo ChatGPT; partições de app apertadas. Ainda não existe launcher enxuto OTA0↔OTA1 consolidado.
- **A22:** Termux, Debian PRoot, MCP privado, túnel e supervisor 2.1; diagnóstico via ChatGPT funcionando. Recuperação total após quedas de energia continua sob observação.
- **Duas C3 + ESP8266 OLED:** relatadas como encomendadas, ainda sem confirmação física ou testes.
- **Carrinho:** chassi 2WD montado, motor/driver/alimentação/PS4 ainda sem prova final.

## Próxima entrega, sem expandir infraestrutura
**Auditar o firmware exato da WROOM 0.0.38** versus Menu 0.0.29 e ambientes existentes; identificar ocupação de flash; criar em branch isolada a menor base possível BOTIZIN + OLED + menu + catálogo + OTA de módulo + retorno seguro. Preservar firmware atual até build, testes, backups e autorização para gravar.

Depois: **BOTIZIN → módulo LED → BOTIZIN** sem notebook. O próximo módulo só começa quando esse ciclo estiver fisicamente comprovado.

## Restrições que impedem confusão
- O modelo ChatGPT não roda dentro das ESP32; a conversa inteligente usa serviços conectados (A22/IA) e poderá acionar o hardware.
- Ao inicializar um firmware de módulo no segundo slot, o programa do rosto no outro slot **não executa simultaneamente**. Voz independente no A22 pode continuar, mas OLED/rosto na WROOM só volta quando ela reiniciar no BOTIZIN.
- A WROOM tem Bluetooth Clássico; C3, S3 e ESP8266 não têm. Não prometer PS4 direto na C3.
- Compilação, pacote arquivado, teste físico e uso real são estados diferentes. Nunca regravar slot estável nem alterar o túnel por interpretar um plano como autorização.
