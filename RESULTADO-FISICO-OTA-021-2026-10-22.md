# BOTIZIN S3 — Instalação OTA 0.0.21: confirmação física

Data: 22/10/2026. Evidência: texto integral da resposta HTTP `/status` da própria ESP32-S3, fornecido após ativação da oferta GitHub da 0.0.21 corrigida.

## O que fizemos
- Comparado o `initArduino()` do Arduino ESP32 Core 3.3.12 com as versões BOTIZIN 0.0.17 e 0.0.21.
- Retirado o override forte `verifyRollbackLater() { return true; }` e a confirmação tardia por saúde (15 s), restaurando confirmação original do Arduino.
- Firmware compilado e verificado no GitHub Actions: `39813168126`. O ELF comprovou símbolo fraco `W verifyRollbackLater`, sem sobrescrita forte.
- Publicado binário corrigido em `main/releases/0.0.21-arduino-default/firmware.bin`, preservando o binário experimental 0.0.21 anterior.
- Atualizado `main/manifest.json` para oferecer a imagem corrigida via GitHub OTA.

## O que testamos / resultado observado

**Prova de instalação na placa física:**
- `BOTIZIN CORE V0.0.21`
- `RUNNING_PARTITION: app0 / ota_0 / address=0x00010000 / size=3145728`
- `BOOT_PARTITION: app0 / ota_0`
- `NEXT_UPDATE_PARTITION: app1 / ota_1`
- `RUNNING_OTA_STATE: VALID`
- `OTA_CONFIRMATION_MODE: ARDUINO_3_3_12_STARTUP_DEFAULT`
- `ROLLBACK_CONFIG: ENABLED`
- `WIFI: OK`, `MDNS: READY`, `OTA: READY`, `INTERNET_OTA: UP_TO_DATE 0.0.21`
- `UPTIME_SECONDS: 354` no momento da captura
- `MQTT_LAST: PUBLISHED`, `MQTT_ATTEMPTS: 4`, `MQTT_SUCCESSES: 4`
- `WIFI_SETUP: OFF` (comportamento correto enquanto rede STA conhecida está conectada; portal ainda não validado pelo celular)

**Prova de integridade e instalação do binário corrigido:**
- `SOURCE_VERSION: 0.0.17`, slot de origem `app1 / ota_1`, destino `app0 / ota_0`
- `EXPECTED_BYTES: 1223504`, `BYTES_WRITTEN: 1223504`
- SHA-256 de imagem recebida, da Flash e esperado **iguais**: `5d313f298a2404ecdcc6e8a9edb91a3ff7c5f68ed4c55a4b6479591be6283d77`
- `Update.begin: TRUE`, `Update.end: TRUE`, `UPDATE_LIBRARY_ERROR: 0 / No Error`
- `DESTINATION_OTA_STATE: NEW` antes do reboot, seguido do estado `VALID` no app0 após o boot.

## O que aprendemos
- A regressão OTA 0.0.18–0.0.21 resultava em `ABORTED` no `otadata` de app0; tínhamos substituído a confirmação automática do Arduino por uma confirmação tardia. A restauração da função padrão foi seguida por instalação OTA real e confirmação `VALID`.
- A instalação bem-sucedida demonstra que GitHub HTTPS, escrita/validação da Flash, seleção de slot e confirmação do novo firmware estão funcionando no caminho testado.
- **Não prova isoladamente a causa exata de cada reset anterior**: a S3 ainda informa `RESET_REASON: BROWNOUT (9)`, que pode ser um registro do último reset; verificar alimentação se houver novos resets não planejados.
- **Pendência separada:** `TELEMETRY: PAUSED_AFTER_3_FAILURES_UNTIL_REBOOT` indica erro de telemetria ThingsBoard, apesar de o MQTT público publicar 4/4; diagnosticar depois, sem afetar OTA.
- **Pendência separada:** portal SoftAP do Wi-Fi Manager da 0.0.21 não foi testado com celular em rede desconhecida. `WIFI_SETUP: OFF` é o estado esperado enquanto STA está conectada.

**Veredito: OTA da 0.0.17 para 0.0.21 CORRIGIDA e CONFIRMADA NA PLACA.**
