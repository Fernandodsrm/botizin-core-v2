# ESTUDO DA BASE S3 0.0.17 — mapa do sistema atual

**Data:** 22/10/2026. **Status:** auditoria sem modificar firmware, OTA, partições ou dispositivos.
**Fonte exata do binário:** build GitHub Actions 39702807605, commit da2de855fe8d9bd86bbddbbc7327e0f4f62439ed, branch s3-mqtt-readonly-0.0.17. Foi essa compilação que gerou a 0.0.17, 1.207.616 bytes, SHA-256 7a7c16bcb2acd885c74c7d8ad77fe15f9d0e1e55d634dfd23d1ebb7abdb6884f.
**Evidência física:** captura de http://botizin-s3.local/status fornecida em 22/10/2026 mostra firmware 0.0.17, running/boot app1/ota_1, ota_state VALID, OTA READY, Wi-Fi OK, MDNS READY, e histórico da instalação a partir de 0.0.16. **Evidência MQTT:** GitHub run 39712807513 recebeu estado 0.0.17, ota_1, ota_valid true no broker público; não há autenticação criptográfica da identidade MQTT.

Marcadores conceituais: [PLACA] observado na S3 física, [CÓDIGO] função implementada, [GITHUB] fonte/build/manifests comprovados, [NÃO TESTADO] caminho ainda não exercitado.

## A. Hardware e memória

- ESP32-S3 N16R8, 16 MiB de Flash, 8 MiB PSRAM, CPU 240 MHz [PLACA].
- Firmware aplicativo compilado 1.207.616 bytes, slot 3.145.728 bytes, margem nominal 1.938.112 bytes [GITHUB/PLACA].
- Slot em execução app1/ota_1 em 0x00310000; próximo OTA app0/ota_0 em 0x00010000, ambos 3 MiB [PLACA].
- Partição otadata no endereço 0xE000, tamanho 0x2000, usada pelo bootloader para escolha do app e estados de OTA [CÓDIGO].
- Mapa de partições do build app3M_fat9M_16MB contém região de dados FAT/FFAT ~9 MiB, MAS o aplicativo S3 0.0.17 não a utiliza no código analisado [CÓDIGO].
- Após OTA, o outro slot provavelmente conserva a imagem anterior 0.0.16, mas não houve leitura independente do seu conteúdo. NUNCA presumir backup perfeito sem verificá-lo.

## B. Arquivos e ferramentas

[firmware/firmware.ino](firmware/firmware.ino): principal setup 557–609, loop 611–644, OTA 210–468, status 127–162, ThingsBoard 470–540.
[firmware/version.h](firmware/version.h): versão 0.0.17.
[firmware/internet_config.h](firmware/internet_config.h): manifesto GitHub da main e prefixo permitido para binários.
[firmware/local_name.h](firmware/local_name.h): recuperação mDNS botizin-s3.local.
[firmware/mqtt_observer.h](firmware/mqtt_observer.h): cliente MQTT experimental de publicação, sem comando.
[firmware/diagnostics.h](firmware/diagnostics.h): memória e métricas temporais.
[firmware/peer_auth.h](firmware/peer_auth.h): chave HMAC compartilhada com WROOM, NVS.
[firmware/peer_ota_server.h](firmware/peer_ota_server.h): protocolo local protegido S3-WROOM.
[firmware/ping_rpc.h](firmware/ping_rpc.h): recebe comandos ThingsBoard sobre HTTPS/polling.
[firmware/ota_confirmation.h](firmware/ota_confirmation.h): amarra instalação manual a candidato/versão/SHA/boot_id/prazo.
[ci/build.py](ci/build.py): verifica core, compilação, tamanho e hash e monta manifest, sem instalar.
[tests/ota_confirmation_test.cpp](tests/ota_confirmation_test.cpp): valida regras de confirmação sem placa.

Bibliotecas diretas principais: Arduino, WiFi, WebServer, Update, Preferences, esp_ota_ops, esp_system, esp_partition, esp_psram, mbedtls/sha256, NetworkClientSecure, HTTPClient, cJSON, time, ESPmDNS, esp_heap_caps, esp_timer, mbedtls/md e PubSubClient 2.8. Core Arduino ESP32 3.3.12; Arduino CLI 1.5.1; compilação para esp32s3 com Flash16M, PSRAM opi, partição app3M_fat9M_16MB, USB CDC hwcdc.

O arquivo telemetry_config.h é gerado no GitHub Actions com o segredo BOTIZIN_TB_DEVICE_TOKEN, então o binário publicado inclui o token de dispositivo ThingsBoard. O código não compila SSID/senha do Wi-Fi.

## C. Boot e setup — em ordem EXATA

Código-fonte firmware.ino 557–609:
1. Bootloader lê otadata e escolhe um slot; atualmente S3 executa app1/ota_1 [PLACA].
2. Inicia Serial USB em 115200, espera 1500ms.
3. loadPeerKey() lê chave do namespace NVS botizin-peer, valida 64 caracteres hexadecimais.
4. Abre NVS botizin-v2, carrega string last (último log de OTA).
5. Imprime partições; compara Flash16MB, PSRAM8MB, endereços e tamanhos dos dois slots, otadata e slot atual; se incompatível otaReady é FALSE.
6. Mede tamanho do sketch e margem OTA UMA VEZ, amostra memória RAM/PSRAM e imprime status serial.
7. WiFi.mode(WIFI_STA), WiFi.setAutoReconnect(true), WiFi.begin() SEM PARÂMETROS. Reusa configuração de Wi-Fi previamente salva na própria placa pelo stack do ESP-IDF/Arduino (NVS). NÃO procura usuário/senha no GitHub, ThingsBoard, MQTT ou planilha. Espera até 30s por conexão.
8. Imprime novo status, registra HTTP na porta 80: GET /, GET /status, POST /update, POST /reboot; registra também /peer/challenge e /peer/ota.
9. Se o Wi-Fi conectou, chama configTime() (NTP via pool.ntp.org/time.cloudflare.com). Se Wi-Fi só conectar depois, não há retry explícito dessa chamada no loop [PONTO DE ESTUDO].
10. Gera identificador do boot, agenda primeiras chamadas ThingsBoard RPC +30s, ThingsBoard telemetria +20s e GitHub OTA +10s.
11. server.begin(), mantém nome botizin-s3.local com mDNS, configura MQTT experimental (+35s).

## D. O loop — tudo que repete

Código-fonte firmware.ino 611–644:
- Em toda iteração mantém mDNS, mede duração entre iterações, processa servidor web e solicitações de reboot; delay(2).
- Amostra RAM/PSRAM no máximo a cada 5s, quando não está gravando/reiniciando.
- Verifica se há instalação/candidato OTA manual enfileirado.
- GitHub OTA automático: agenda a cada 60s, primeiro teste ~10s após boot, desde que não esteja ocupado nem numa janela manual, e sem erro terminal.
- POST de telemetria ThingsBoard: a cada 60s, começa ~20s depois do boot; para após três falhas até reboot.
- Polling de comando ThingsBoard RPC: a cada 15s, começa ~30s depois do boot; para após três falhas até reboot.
- Tentativa MQTT: a cada 90s, começa ~35s depois do boot; abre sessão TLS, publica uma mensagem e desconecta, se Wi-Fi/relógio/heap e OTA permitirem.
- Operações de rede HTTP/TLS são majoritariamente síncronas no mesmo loop, podendo provocar atrasos de segundos. Não confundir com CPU ocupada 100%.

## E. OTA GitHub — 100% automático quando elegível

Manifesto: https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/manifest.json.
Prefixo permitido: https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/releases/.

Fases firmware.ino 304–468:
1. Verifica otaReady, Wi-Fi e hora sincronizada.
2. GET HTTPS do manifesto, com CA bundle, timeout de conexão/leitura ~15s, HTTP 200, Content-Length entre 1 e 2048 e dados completos.
3. JSON exige board exatamente esp32s3-n16r8, versão, URL, sha256 hexadecimal de 64 posições, tamanho inteiro de 1 até 3145728 e URL com prefixo exato permitido. Não aceitar redirect.
4. Compara versão de manifesto contra 0.0.17 compilada. Se igual => UP_TO_DATE; se mais antiga ou inválida => STOPPED (suspende consulta até reboot); se mais nova => atualiza automaticamente, exceto caso de consulta manual vigente.
5. GET HTTPS do firmware e comparação do tamanho HTTP com manifesto.
6. Determina partição inativa, inicia Update.begin, configura SHA esperado, baixa em buffer estático de 4096 bytes e faz Update.write; compara SHA incremental dos bytes recebidos.
7. Timeout se >15s sem dados ou >180s download total. Processa servidor HTTP durante download.
8. Update.end(false) seleciona novo app; lê de volta partição em blocos e recalcula SHA da Flash, confere novo boot target e descrição de imagem ESP_OK. Se falha pós-escrita, tenta restaurar boot target original, não reinicia.
9. Registra último andamento na NVS botizin-v2/last, aguarda 2s e chama ESP.restart; o bootloader escolhe app novo.
10. Na próxima inicialização Arduino core 3.3.12 confirma imagem VALID por padrão logo no começo, se o rollback estiver habilitado e seu gancho padrão não for substituído.

**Para atualizar pelo GitHub:** é preciso compilar binário do dispositivo, conferir integridade, PUBLICÁ-LO num caminho permitido e alterar o manifesto de produção para versão MAIS NOVA com SHA e tamanho. **Subir qualquer commit ou código fonte sem atualizar manifesto não altera placa**; publicar binário isolado também não. Uma alteração no manifesto pode disparar instalação automaticamente a partir do próximo ciclo de consulta. A WROOM não é necessária para OTA autônomo.

**O que significa rollback:** bootloader pode voltar de imagem nova que falhou ANTES de ser confirmada; nesta 0.0.17 o Arduino confirma automaticamente no boot. Não há contador de 5min com retorno, botão remoto de escolha de slots, nem rollback por simples perda de MQTT depois da imagem VALID.

## F. OTA manual e outros comandos

**Via ThingsBoard HTTPS** [ping_rpc.h]: polling GET /rpc?timeout=1000; métodos ping, ota_status, ota_check, ota_confirm, ota_cancel, peer_pair. Comandos trazem ID e issued_at_ms com janela -90s/+30s. Consulta manual gera candidato com ID/versão/SHA/boot_id por 5min; só confirmando candidato exato dentro do prazo a atualização pode ser enfileirada. Cancelamento reserva 5min antes de reativar atualização automática. Os CINCO MINUTOS NÃO são rollback. A API ThingsBoard está indisponível após limite de ingestão notificado: não supor que botões remotos funcionem.

**Via WROOM local** [peer_auth.h e peer_ota_server.h]: chave compartilhada de 32 bytes armazenada em NVS botizin-peer, HMAC-SHA256, nonce único por 10s, controle /peer/ota do tipo status/check/confirm/cancel. Resposta assinada. PEER_PAIRED:YES significa que chave válida foi encontrada, NÃO que WROOM esteja atualmente conectada. Status transportado por HTTP LAN sem criptografia de conteúdo; MAC autentica mensagens.

**Via LAN direto**: GET / e /status mostram texto completo. POST /update recebe binário, com tamanho e SHA; POST /reboot reinicia após upload verificado. ATENÇÃO: handlers /update, /reboot e /status não implementam verificação de usuário ou HMAC; NÃO publicar porta 80 diretamente na Internet. Diferem da rota protegida /peer/ota.

## G. MQTT 0.0.17 — publica, NÃO controla

firmware/mqtt_observer.h:
- Broker público de testes broker.emqx.io, TLS porta 8883, biblioteca PubSubClient 2.8, tópico de teste fernando-lab/botizin-lab-s3-mqtt-readonly-20261021/state.
- Começa +35s depois do boot, depois tenta cada 90s. Não mantém sessão TCP permanente.
- Só tenta se Wi-Fi OK, hora/NTP plausível, heap >140KB, sem OTA/gravação/pedido pendente.
- Publica JSON contendo firmware, uptime_s, slot ota_0/ota_1, ota_valid, ota_ready, heap_kb, attempt, role; NÃO inclui boot_id nem timestamp de parede ou hash.
- Publicação QoS0 padrão, NÃO retained, desconecta logo após transmitir. Não faz subscribe; não recebe PING, comando, pedido OTA ou pedido de retorno de slot.
- Guarda somente na RAM botizinMqttLast, botizinMqttAttempts e botizinMqttSuccesses; log pela Serial USB. Esses contadores NÃO aparecem em /status, nem NVS, nem GitHub automaticamente.
- Broker público aceita mensagens de remetentes não autenticados; uma mensagem sozinha não prova identidade. Não usar para comandos de produção, senhas, controle ou telemetria sensível.
- O GitHub Actions apenas observou quando uma execução esteve ativa, não é painel contínuo 24h nem sistema de histórico.

## H. ThingsBoard antigo — HTTPS, independente do MQTT

firmware.ino 470–540: envia JSON periódico com versão, boot_id, memória, slots, motivo de reset e journal OTA via POST HTTPS para ThingsBoard com token de dispositivo embutido. Interrompe emissão após 3 falhas até reboot: na captura TELEMERY=PAUSED_AFTER_3_FAILURES_UNTIL_REBOOT. Isso NÃO significa que MQTT tenha parado.

ping_rpc.h: consulta RPC ThingsBoard a cada 15s, aceita seis métodos, só instala após validação e confirmação do serviço. Três falhas suspendem polling até reboot de forma independente. ThingsBoard Device API atingiu limite no serviço; comandos podem não funcionar.

## I. Diagnósticos e observações físicas

firmware/diagnostics.h mede RAM interna, mínimo livre, maior bloco de heap, PSRAM livre, espaço no próximo slot, máximo tempo entre loop e máximos de latência por serviço. O custo de ESP.getSketchSize foi movido para uma execução no boot; coleta de RAM a cada 5s não verifica Flash.

Na captura [PLACA]:
- UPTIME_SECONDS 7935 (~2h12m); RESET_REASON BROWNOUT (9), mostra causa do último reset detectado. Não informa automaticamente origem elétrica e não prova reinícios recorrentes.
- DIAG_FREE 248752, DIAG_MIN 190100, DIAG_LARGEST 155636, DIAG_PSRAM 8373408, DIAG_OTA_FREE 1938112 bytes.
- DIAG_LOOP_MAX_US 10642042 (~10,64s); DIAG_GIT_MAX_US 10639173 (~10,64s); DIAG_TB_MAX_US 1247858 (~1,25s); DIAG_RPC_MAX_US 6070623 (~6,07s); DIAG_SAMPLE_MAX_US 165μs. São **máximos desde boot**, NÃO uso da CPU nem médias.
- DIAG_PEER_MAX_US 0 e DIAG_OLED_MAX_US 0 não provam que WROOM está desconectada ou que há OLED na S3.
- TEST_RESULT: Deu certo na atualização é uma **frase fixa** em snapshot(), não teste dinâmico. A validação real usa versão, slot, SHA e estado OTA.

## J. Onde ficam dados persistentes

- Flash OTA app0/app1: imagens do firmware; somente um app executado por vez.
- Flash otadata: escolha do slot/estado de boot.
- NVS Wi-Fi: credenciais previamente provisionadas na placa, reutilizadas por WiFi.begin() SEM parâmetros.
- NVS botizin-v2 / last: último log OTA; não é banco histórico amplo.
- NVS botizin-peer / key: segredo local HMAC S3–WROOM, não expor.
- RAM: variáveis, estado atual, contadores MQTT, buffers TLS e medições temporárias. Desaparecem após reboot.
- GitHub: código, binários, manifesto, histórico dos commits e logs de CI; não recebe eventos da placa automaticamente.
- Broker MQTT atual: transporte de uma mensagem sem retained, não histórico persistente.
- ThingsBoard: antigo armazenamento/RPC; disponível historicamente, Device API atualmente limitada.

## K. Quais funções NÃO existem nessa S3

- Nenhuma interface OLED na S3, nenhum menu físico funcional da S3; WROOM tem sua própria interface separada.
- Sem microfone, alto-falante, áudio I2S/UART, fone HFP, motor XiaoZhi ou animação de rosto na 0.0.17.
- Não há MQTT subscribe, controle remoto MQTT, web painel com histórico permanente ou broker privado.
- Não há portal na S3 para cadastrar novas redes Wi-Fi.
- Não há rollback automático temporizado de 5 minutos, botão remoto de alternar slots ou garantia de retorno após firmware já VALID.
- Não existe serviço automático que salve todos os eventos da S3 no GitHub.

## L. Decisões que este inventário permite — SEM implementar ainda

Estudo OTA: bootloader, SHA, slots, critérios de confirmação, rollback antes/depois de VALID, autenticação do upload LAN, histórico de atualizações, teste reversível.
Estudo MQTT: autenticação/ACL e broker privado, persistência, boot_id, timestamps, estados reais, painel, perda de mensagens, controle seguro depois.
Estudo CONECTIVIDADE: provisionar Wi-Fi alternativo/NVS, recuperação de NTP após Wi-Fi tardio, mDNS, TLS e fallback seguro.
Estudo DESEMPENHO: 10+ segundos de lacuna no loop incompatíveis com requisitos exigentes de áudio conversacional de baixa latência; testar não-bloqueio antes de XiaoZhi.
Estudo WROOM: inventário próprio depois, separado desta placa.

**Regras de evidência:** código previsto não é teste; build não é instalação; imagem VALID não garante recuperação arbitrária; mensagem no broker público não comprova identidade. Não modificar a S3 por causa deste estudo.
