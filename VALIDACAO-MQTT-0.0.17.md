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
