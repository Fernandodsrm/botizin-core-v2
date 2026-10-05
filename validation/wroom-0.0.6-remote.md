# WROOM 0.0.6 — implantação e comandos reais

2026-10-05: receptor ThingsBoard autenticado retornou telemetria real 0.0.6, novo boot 54341164272d5434, app1 / ota_1 / address=0x00150000 / size=1310720 e OTA VALID. Automático explicitamente true. A primeira instalação foi automática, após propagação do manifesto GitHub. PONG correlacionado confirma firmware/boot/slots; Wi-Fi/telemetria/RPC continuam ativos.

Consulta OTA aceita por RPC e completada pela placa: UP_TO_DATE 0.0.6; candidata ausente, automático true e janela restante 286 s. Confirmação com id/versão/SHA/boot inválidos foi recusada com CONFIRMATION_REJECTED. Não houve pedido de instalação manual válido nem gravação manual neste teste.

Limites: ainda precisam de teste físico os botões/página OTA e o progresso OLED durante uma próxima instalação. Ciclo manual completo exige próxima candidata. Leitura LAN da S3 falhou antes/depois da atualização (HTTP_-11 / HTTP_-1); não foi atribuída causa. A S3 manteve relato cloud 0.0.10 VALID durante os testes, e seu manifesto/canal permaneceu intacto. Diagnosticar IP/rede local antes do relay LED. Não desligar o automático.

## Respostas verificadas

### ping

```json
{
  "result": "PONG",
  "origin": "ESP32_REAL",
  "command_id": "ping-1791193712879-064ffd4e",
  "request_id": 4,
  "firmware_version": "0.0.6",
  "boot_id": "54341164272d5434",
  "uptime_seconds": 67,
  "running_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "boot_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "ota_state": "VALID",
  "reset_reason": "SOFTWARE (3)"
}
```

### ota_confirm

```json
{
  "result": "CONFIRMATION_REJECTED",
  "origin": "ESP32_REAL",
  "command_id": "ota_confirm-1791193742891-fc1516f7",
  "request_id": 6,
  "firmware_version": "0.0.6",
  "boot_id": "54341164272d5434",
  "uptime_seconds": 97,
  "running_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "boot_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "ota_state": "VALID",
  "reset_reason": "SOFTWARE (3)",
  "automatic_enabled": true,
  "manual_status": "READY",
  "internet_status": "UP_TO_DATE 0.0.6",
  "candidate_ready": false,
  "candidate_id": "",
  "target_version": "",
  "sha256": "",
  "size": 0,
  "expires_in_seconds": 0
}
```

### ota_check

```json
{
  "result": "CHECK_ACCEPTED",
  "origin": "ESP32_REAL",
  "command_id": "ota_check-1791193757432-f4da7811",
  "request_id": 7,
  "firmware_version": "0.0.6",
  "boot_id": "54341164272d5434",
  "uptime_seconds": 112,
  "running_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "boot_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "ota_state": "VALID",
  "reset_reason": "SOFTWARE (3)",
  "automatic_enabled": true,
  "manual_status": "READY",
  "internet_status": "UP_TO_DATE 0.0.6",
  "candidate_ready": false,
  "candidate_id": "",
  "target_version": "",
  "sha256": "",
  "size": 0,
  "expires_in_seconds": 0
}
```

### ota_status

```json
{
  "result": "OTA_STATUS",
  "origin": "ESP32_REAL",
  "command_id": "ota_status-1791193772504-71fb11a7",
  "request_id": 8,
  "firmware_version": "0.0.6",
  "boot_id": "54341164272d5434",
  "uptime_seconds": 127,
  "running_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "boot_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "ota_state": "VALID",
  "reset_reason": "SOFTWARE (3)",
  "automatic_enabled": true,
  "manual_status": "UP_TO_DATE 0.0.6",
  "internet_status": "UP_TO_DATE 0.0.6",
  "candidate_ready": false,
  "candidate_id": "",
  "target_version": "",
  "sha256": "",
  "size": 0,
  "expires_in_seconds": 286
}
```

