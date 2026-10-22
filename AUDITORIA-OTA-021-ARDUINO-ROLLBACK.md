# Diagnóstico de regressão OTA S3 0.0.17 → 0.0.21 e correção da confirmação

Data: 22/10/2026. Escopo: auditoria de código Arduino ESP32 core 3.3.12, branch firmware 0.0.21, logs de OTA local e leitura física da otadata, **sem escrever na placa**.

## O que sabemos diretamente da placa
- O firmware 0.0.17 (app1 / ota_1) iniciou, respondeu por HTTP, Wi-Fi OK, OTA READY e fez consulta ao GitHub.
- A 0.0.21 foi baixada por HTTPS/local OTA e gravada em app0 / ota_0: 1.224.864 bytes, SHA-256 recebido e lido da Flash iguais a `486ea4740e598b6917d7207a45f12beb1d7d645b4caec71cc8ddc727244c73c2`. `Update.begin=TRUE`, `Update.end=TRUE`, `IMAGE_DESCRIPTION=ESP_OK`, `DESTINATION_OTA_STATE=NEW` e `BOOT_PARTITION_AFTER_Update.end=app0`.
- Depois do reboot, o status HTTP e serial voltaram a apresentar 0.0.17 no app1, `RESET_REASON: BROWNOUT (9)`.
- A leitura física de `otadata` 0xE000 (8192 bytes) exibiu setor 0, seq=17, ota_0 = `ABORTED`; setor 1, seq=16, ota_1 = `VALID`.
- Não foi capturado um log UART claro da primeira execução da 0.0.21, então não se pode atribuir o reset exclusivamente a energia, watchdog ou panic a partir do código; `BROWNOUT` reportado pelo dispositivo é uma evidência relevante de reset de alimentação.

## Implementação oficial Arduino ESP32 3.3.12
Fonte: [cores/esp32/esp32-hal-misc.c](https://github.com/espressif/arduino-esp32/blob/3.3.12/cores/esp32/esp32-hal-misc.c):
- `verifyRollbackLater()` é função `weak` cujo retorno padrão é `false`.
- `verifyOta()` é `weak` cujo retorno padrão é `true`.
- Durante `initArduino()`, se estado é `ESP_OTA_IMG_PENDING_VERIFY` e `verifyRollbackLater()` é falso, a plataforma chama `esp_ota_mark_app_valid_cancel_rollback()`. O rollback do bootloader continua habilitado para falhas antes desse ponto.
Fonte IDF: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html
- Um app novo sai de `NEW` para `PENDING_VERIFY` no primeiro boot; se reiniciar ainda pendente, bootloader grava `ABORTED` e seleciona slot anterior `VALID`.
- `INVALID` é usado quando a aplicação solicita rejeição explícita via API, diferente do `ABORTED` observado.

## O que fizemos errado no BOTIZIN
- As versões estáveis 0.0.16 e 0.0.17 não sobrescrevem `verifyRollbackLater()` e deixam a plataforma confirmar no início do Arduino.
- Nossa versão experimental 0.0.18 introduziu `extern "C" bool verifyRollbackLater(void) { return true; }` e a 0.0.21 herdou essa mudança.
- Isso suprime a confirmação automática. Em vez disso, `otaBootHealthTick()` exige partições válidas, Wi-Fi ou SoftAP de recuperação e servidor HTTP iniciado por **15 segundos contínuos**, com prazo total de 180 segundos; só então chama `esp_ota_mark_app_valid_cancel_rollback()`.
- O timer de 15s/180s é nossa implementação; **não é timer automático do bootloader**. Como o registro físico final é `ABORTED`, é altamente consistente com um novo reset antes da confirmação da imagem; não comprova que a regra de timeout de 180 s chegou a ser executada.
- Uma versão em `PENDING_VERIFY` não deve depender de Wi-Fi externo nem MQTT para confirmar o boot de firmware básico. Isso amplia a janela em que brownout/panic/wdt devolve ao firmware antigo.
- A publicação da versão 0.0.21 foi temporária e o fluxo de publicação revogou o manifesto por falta de confirmação MQTT, agravando visibilidade. Download e SHA eram bons; MQTT não é prova do motivo do rollback.

## Correção aplicada na branch de reparo
`fix/s3-021-arduino-default-ota-confirmation`
- Removida a função `verifyRollbackLater` sobrescrita. O link será resolvido pela função fraca oficial do core (`false`) e o core confirmará via `verifyOta()` (`true`) durante `initArduino()`.
- Removidos `otaBootHealthBegin()`, `otaBootHealthTick()` e a dependência de Wi-Fi, MQTT, SoftAP e janelas de 15/180 s para marcar a imagem `VALID`.
- Mantidos mapa de partições app0/app1, `otadata`, OTA por GitHub, SHA-256, Wi-Fi manager 0.0.21, mDNS e MQTT.
- Campo de status `OTA_CONFIRMATION_MODE: ARDUINO_3_3_12_STARTUP_DEFAULT` torna o mecanismo explícito.
- **Não desabilitar CONFIG_APP_ROLLBACK_ENABLE**. Somente restaurar momento da confirmação para o padrão da versão que funcionava.
- Limitação: nenhuma alteração de código de boot corrige instabilidade real da fonte de energia; se outro reset ocorrer antes do `initArduino()`, o bootloader ainda pode abortar. A correção encurta drasticamente a janela vulnerável, sem esconder a necessidade de corrigir alimentação se persistir.
- Este patch ainda não foi gravado na placa nem incorporado ao `main`; a versão 0.0.17 atual permanece como oferta estável. O build de bancada deve comprovar que o ELF usa símbolo `weak` para `verifyRollbackLater`.

## Critérios para futura entrega
1. CI compilar binário S3 com core fixo 3.3.12, verificar SHA e espaço (<= 3MiB), sem credencial real de produção neste artefato de bancada.
2. `nm -C firmware.elf` deve exibir `W verifyRollbackLater()` ou `W verifyRollbackLater`, nunca símbolo `T` definido pelo BOTIZIN.
3. Empacotar binário com token ThingsBoard de produção apenas no fluxo autorizado de publicação (não publicar nem enviar credenciais).
4. Uma única atualização real deverá validar `RUNNING_PARTITION=app0`, `RUNNING_OTA_STATE=VALID`, versão instalada e estado de `otadata`; se brownout voltar, revisar alimentação antes de novos ciclos.
