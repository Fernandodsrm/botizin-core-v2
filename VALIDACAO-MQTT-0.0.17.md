# BOTIZIN S3 0.0.17 — registro verificável do teste MQTT

**Data da solicitação:** 2026-10-21 (BRT)  
**Autorização:** usuário autorizou instalar S3 mesmo sem telemetria anterior, aceitando possível recuperação futura via USB.  
**Mecanismo existente preservado:** OTA por HTTPS no `botizin-core-v2/main/manifest.json`, partições S3 app0/app1 de 3 MiB; não houve reparticionamento.  
**Origem do código:** branch `s3-peer-discovery-20261008`, versão 0.0.16, com módulo `firmware/mqtt_observer.h` e versão 0.0.17.

## Preparação e validação

- **CI aprovada:** https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/39702807605
- Compilou com Arduino core 3.3.12, PubSubClient 2.8, partição `app3M_fat9M_16MB`.
- Imagem aplicativo: **1.207.616 bytes**; SHA-256 **7a7c16bcb2acd885c74c7d8ad77fe15f9d0e1e55d634dfd23d1ebb7abdb6884f**.
- Testes de lógica OTA anteriores e invariantes de partição aprovados. Teste TLS do GitHub até broker público em 8883 aprovado.
- Módulo MQTT é **somente publicação** sem inscrição/comandos. Publica `firmware`, `slot`, `ota_valid`, `ota_ready`, `uptime_s`, `heap_kb`. Sem endereço IP, MAC ou segredos. O broker público serve exclusivamente ao primeiro teste e NÃO autentica o dispositivo.
- O ThingsBoard anterior permaneceu no código, embora a Device API da conta tenha sido desativada por limite de ingestão em 21/10/2026.

## Publicação e ativação

- **Execução:** https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/39703234006
- Artifact compilado verificado por tamanho/versão/SHA e `publishable=true`.
- Pacote publicado em `main/releases/0.0.17/firmware.bin`.
- Binário remoto comparado com cópia compilada (`cmp` aprovado).
- Manifesto **main/manifest.json** alterado de 0.0.16 para **0.0.17** após verificação dos bytes. Isto pode disparar OTA automático na S3 se ela estiver com a linha de firmware correspondente ativa e conectada.
- Os binários antigos, inclusive `main/releases/0.0.16/firmware.bin`, NÃO foram excluídos.
- Nenhum comando foi enviado diretamente via USB/COM nem via ThingsBoard.
- **Este registro NÃO afirma que a S3 física instalou ou reiniciou.** A observação MQTT precisa confirmar mensagem recente do novo firmware; broker público não constitui autenticação criptográfica de identidade.

## Pendências

- Evidência fresca de que a S3 se conectou e enviou mensagem de versão 0.0.17.
- Leitura remota independente da versão/boot_id/slot e `OTA VALID` após reboot real.
- Verificação do retorno remoto/rollback em um teste físico controlado.
- Configuração futura de broker PRIVADO para operação permanente. NÃO usar canal público para executar comandos, senhas, OTA ou informações pessoais.

**Risco aceito:** se o firmware falhar, manter cópia binária no GitHub não possibilita reversão remota sem canal de controle; USB/esptool poderá ser necessário quando usuário estiver na bancada.

## Resultado da janela inicial de observação — 2026-10-21, depois da oferta

- O workflow [39703234006](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/39703234006) terminou **success** em todos os passos de publicação e monitoramento.
- `S3_0_0_17_PACKAGE_PUBLISHED_NOT_ACTIVE`, `REMOTE_RELEASE_BYTES_VERIFIED` e `S3_0_0_17_OFFER_ACTIVE_NO_DEVICE_CONFIRMATION` comprovam o estado do servidor GitHub, **não** da ESP32 física.
- Observador conectou ao broker: `OBSERVER_MQTT_CONNECTED`.
- Durante aproximadamente 190 segundos de escuta após a ativação, recebeu **zero mensagens**: `FINAL_MQTT_OBSERVATION {\"connected\": true, \"received\": 0, \"firmware_017\": false, \"ota_valid_reported\": false}`.
- **Resultado físico:** NÃO VERIFICADO. Não sabemos se a S3 está executando 0.0.16, 0.0.17 ou outro firmware, está online no Wi-Fi ou teve falha no OTA/MQTT. O teste não distingue essas hipóteses.
- **Importante:** manifesto 0.0.17 permanece ofertado enquanto esta entrada é escrita. Isso significa que o download pode ocorrer futuramente se uma S3 elegível voltar à internet. Para suspender essa oferta, é preciso fazer nova alteração consciente de manifesto para a versão 0.0.16, sem apagar o binário da 0.0.17. Não confundir volta do manifesto com rollback de placa já atualizada.
- A verificação independente final requer leitura real de `/status` na rede local, serial (COM) ou um canal seguro de telemetria operacional. GitHub não acessa automaticamente COM do Windows ou Wi-Fi residencial.

## Segunda consulta — sinal MQTT fresco recebido, 21/10/2026 23h43 BRT

**Execução GitHub Actions:** https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/39712807513

- Observador MQTT read-only conectado em `broker.emqx.io:8883`: `BROKER_CONNECTED_LISTENING`.
- **2026-10-22 02:43:17 UTC** (2026-10-21 23:43:17 BRT): `FRESH_TOPIC_REPORT`, **não retido**, com:
  - `firmware: 0.0.17`
  - `uptime_s: 5892` (~1h38min)
  - `slot: ota_1`
  - `ota_valid: true`
  - `ota_ready: true`
  - `heap_kb: 242`
  - `attempt: 66`
- Resultado da execução: `broker_connected: true`; `fresh_count: 1`; `retained_count: 0`.
- **Interpretação:** forte evidência de firmware BOTIZIN S3 0.0.17 rodando e publicando telemetria após OTA, já que o pacote 0.0.17 passou na compilação e a mensagem tem o esquema esperado.
- **Limite crítico:** o tópico está em **broker público e não autenticado**. Ter recebido uma mensagem recente com campos consistentes não prova criptograficamente que veio da S3 física, nem que a credencial/OTA opera ponta a ponta após novo teste. Antes de uso como monitor confiável, migrar para broker privado com autenticação e correlacionar boot_id/slot/hash com outro canal.
- O observador **não publicou nenhum comando MQTT**, não acionou OTA, não alterou manifestos ou firmware. Foi apenas uma segunda consulta do canal já ofertado.
- Com a evidência nova, o bloqueio anterior `nenhuma mensagem` está superado; o próximo trabalho é profissionalizar identidade/autenticação e histórico, não presumir falha do MQTT.


## Confirmação física local — captura fornecida pelo usuário (22/10/2026, ~00h17 BRT)

Usuário abriu via navegador na mesma rede da S3 o endpoint HTTP `http://botizin-s3.local/status` e compartilhou captura da resposta textual emitida pela placa. Evidência **independente do tópico MQTT público**.

| Campo exibido pela S3 | Valor |
| --- | --- |
| Firmware | `BOTIZIN CORE V0.0.17` |
| `LOCAL_NAME` / `MDNS` | `botizin-s3.local` / `READY` |
| `RUNNING_PARTITION` | `app1 / ota_1` (0x00310000, 3145728 bytes) |
| `BOOT_PARTITION` | `app1 / ota_1` (mesma partição) |
| `NEXT_UPDATE_PARTITION` | `app0 / ota_0` (0x00010000, 3145728 bytes) |
| `RUNNING_OTA_STATE` | `VALID` |
| `ROLLBACK_CONFIG` | `ENABLED` (core Arduino confirma a imagem na inicialização) |
| `OTA` | `READY` |
| `OTA_AUTOMATIC_ENABLED` | `YES` |
| `INTERNET_OTA` | `UP_TO_DATE 0.0.17` |
| `UPTIME_SECONDS` | 7935 |
| `RESET_REASON` | `BROWNOUT (9)` |
| `TELEMETRY` | `PAUSED_AFTER_3_FAILURES_UNTIL_REBOOT` (canal ThingsBoard antigo, NÃO MQTT) |

Histórico `LAST_PERSISTED_OTA_ATTEMPT` informado pela S3: origem `0.0.16`, execução anterior em `app0/ota_0`, destino `app1/ota_1`, candidato `0.0.17`, 1.207.616 bytes, SHA calculado nos bytes recebidos e na Flash igual ao esperado, `Update.end: TRUE`, `UPDATE_LIBRARY_ERROR: 0 / No Error`, `IMAGE_DESCRIPTION: ESP_OK`, `OTA_WRITE_VERIFIED: YES`, reboot via `INTERNET_OTA`.

**Resultado:** confirmação local direta de que a **S3 executa 0.0.17** no slot `ota_1` com OTA `VALID` e verificação remota de MQTT por mensagem recente em 21/10 às 23:43 BRT (workflow 39712807513). Isto comprova os aspectos essenciais da integração OTA + telemetria MQTT em teste. O broker MQTT de teste continua público e não autenticado: não interpretar mensagem MQTT como comprovação criptográfica de identidade. O mecanismo de retorno temporizado ao slot anterior **não foi testado fisicamente** e o core Arduino já confirma imagem automaticamente.

**Pontos de atenção:** reset `BROWNOUT (9)` significa detecção de queda de tensão; investigar alimentação. ThingsBoard foi pausado depois de três falhas — manter separado do novo MQTT. Para estabilização, migrar MQTT para broker privado com autenticação e adicionar ao status local as métricas `mqtt_last`, `mqtt_attempts`, `mqtt_successes` (não são exibidas nessa versão). Dados de rede local e identificadores únicos omitidos desta nota por privacidade.
