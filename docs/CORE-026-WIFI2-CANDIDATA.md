# BOTIZIN CORE 0.0.26 — Wi-Fi 2.0 sem perder a base 0.0.25

## Entrega atual

Fonte da CORE 0.0.26 candidata em `candidate/wifi2-core-0.0.26`. Derivada da branch Wi-Fi 2.0, preservando os módulos existentes da CORE 0.0.25 e restabelecendo o fluxo de OTA automática/manual original, MQTT e ThingsBoard no código. Não modifica o manifesto `main/manifest.json`, que continua na `0.0.25`. A S3 física não foi gravada por esse trabalho.

### O novo Wi-Fi

- Leitura do estado atual, IP, RSSI, canal, gateway, DNS e mDNS, sem entregar a senha.
- Descoberta sob demanda (até 20 redes) na LAN autenticada.
- Testar nova rede sem gravar NVS; conexão candidata em RAM. Confirmar até 120 s após conectar.
- Em erro de conexão ou falta de confirmação, tentar voltar à rede anterior. Se a credencial antiga não estiver disponível em RAM, recusar teste.
- Mostra `WIFI_TRIAL_STATE`, `WIFI_TRIAL_RESULT` e `WIFI_TRIAL_REMAINING_S` em `/status`; painel separado com botões e status visível.
- Suspende MQTT, telemetria ThingsBoard e operações OTA enquanto a rede está em teste, para evitar competição por recursos/reconexões. Retoma comportamento normal após finalizar.
- Rota `/control/wifi-save` (gravação direta antiga) responde `WIFI2_USE_TEST_AND_CONFIRM`. Portal físico de recuperação AP continua disponível.

### Preservações e diferenças

| Componente | CORE 0.0.26 candidata |
|---|---|
| N16R8 + layout flash app0/app1 3 MiB | Mantidos |
| Painel status, eventos, diagnóstico | Mantidos e ampliados |
| MQTT observador readonly | Código preservado; ainda não testado fisicamente |
| ThingsBoard | Código preservado; **credencial real depende de compilação privada** |
| OTA HTTPS automática/manual, HTTP POST /update, peer OTA | Código habilitado; não publicado no manifesto |
| S3 física CORE 0.0.25 | Intocada |
| WROOM | Intocada |

### Compilação verificada

- [GitHub Actions #43963452072](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/43963452072): **success**.
- Base histórica recompilada: 1.264.224 bytes.
- CORE 0.0.26 candidata com Wi-Fi 2.0 + OTA habilitada, token fictício de CI: **1.281.536 bytes**, crescimento líquido **+17.312 bytes**.
- Artefato `WIFI2-CORE-026-TEST-CANDIDATE`, **somente teste**. Não é substituto equivalente do .25 para ThingsBoard, pois não contém o token real.
- Memória RAM/PSRAM, desempenho e reconexão física pendentes de medição na S3.

### Como gerar candidato funcional de forma privada no Windows

A CORE 0.0.25 compilava um *device token* do ThingsBoard no firmware; esse segredo não está no GitHub e não deve ser enviado ao chat. Por isso, baixar o pacote de CI e instalar como produção faria a telemetria falhar. O código disponibiliza `tools/s3/Compilar-CORE026-WIFI2-Privado.ps1`, que solicita o token **localmente**, compila via Arduino CLI `esp32:esp32@3.3.12` + PubSubClient 2.8 + Python 3.11 e coloca o binário em `Downloads/BOTIZIN-RECUPERACAO-S3/FIRMWARE-PRIVADO`, sem publicar ou atualizar S3. O script ainda **não foi executado no Windows do usuário**, portanto sua operação prática também será validada.

### Próximo teste físico, sem ciclo extra de backups

- Conferir SHA256 de imagem compilada e identidade 0.0.26.
- Transferir manualmente via mecanismo de OTA existente da 0.0.25 apenas mediante ação explícita do operador (gravará o próximo slot OTA).
- Verificar `BOTIZIN CORE V0.0.26`, `RUNNING_PARTITION`, Wi-Fi, HTTP, recursos, MQTT e telemetria ThingsBoard antes de alterar qualquer manifesto de publicação.
- Testar Wi-Fi correto, incorreto, timeout de confirmação e retorno. Em caso de falha, usar recuperação USB 0.0.25 já testada no procedimento prévio, sem refazer inventário/backups do outro slot.

**Aprovação de compilação não equivale a aprovação de hardware, OTA ou telemetria.**
