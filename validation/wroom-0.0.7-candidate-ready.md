# WROOM 0.0.7 — candidata real aguardando confirmação

A placa permaneceu em 0.0.6, boot 54341164272d5434, OTA VALID e automático true. Após publicação do manifesto e consulta manual, retornou AVAILABLE, candidate_ready true, candidata 0.0.7 com SHA e tamanho iguais ao build verificado. Expiração restante 286 segundos no momento da resposta. Nenhum ota_confirm foi enviado pelo assistente. A confirmação física/pelo painel do usuário e a prova do novo boot ainda estão pendentes. Se a reserva expirar, a instalação automática pode ocorrer e não comprova o caminho manual.

```json
{
  "result": "OTA_STATUS",
  "origin": "ESP32_REAL",
  "command_id": "manual007-ota_status-1791194552449-a55b0920",
  "request_id": 14,
  "firmware_version": "0.0.6",
  "boot_id": "54341164272d5434",
  "uptime_seconds": 907,
  "running_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "boot_partition": "app1 / ota_1 / address=0x00150000 / size=1310720",
  "ota_state": "VALID",
  "reset_reason": "SOFTWARE (3)",
  "automatic_enabled": true,
  "manual_status": "AVAILABLE",
  "internet_status": "UP_TO_DATE 0.0.6",
  "candidate_ready": true,
  "candidate_id": "54341164272d5434-35f11915",
  "target_version": "0.0.7",
  "sha256": "407fde034f75761a47671e81e71952a91b85e4195d3da4b89f2afb13233be115",
  "size": 1224256,
  "expires_in_seconds": 286
}
```
