# WROOM 0.0.7 — OLED confirmada e diagnóstico de PING

2026-10-05: usuário confirmou na OLED WROOM 0.0.7 e S3 0.0.10, mas o painel exibiu ausência de confirmação de PING. Consulta autenticada mostrou relato real recente de 0.0.7, OTA VALID, automático true, S3_LINK OK. Chamada direta à mesma rota RPC retornou PONG correlacionado do boot cf0206162ef664af, app0/ota_0, VALID e reset POWERON (1). Isso comprova recebimento/resposta nessa chamada; não determina a causa do timeout anterior do painel.

Ajuste somente do widget PING: timeout 30→60 s; preservada correlação, exigência de relato real e janela de contato recente de 180 s. Mensagem passa a explicar dados ausentes ou desatualizados. Testes do widget aprovados. Usuário precisa recarregar e repetir o PING no painel para comprovar o resultado de ponta a ponta. Firmware, partições, NVS e automático permanecem inalterados. Prova de instalação manual segue pendente.

```json
{
  "result": "PONG",
  "origin": "ESP32_REAL",
  "command_id": "diagnose007-1791195827858-1fbddf7b",
  "request_id": 19,
  "firmware_version": "0.0.7",
  "boot_id": "cf0206162ef664af",
  "uptime_seconds": 144,
  "running_partition": "app0 / ota_0 / address=0x00010000 / size=1310720",
  "boot_partition": "app0 / ota_0 / address=0x00010000 / size=1310720",
  "ota_state": "VALID",
  "reset_reason": "POWERON (1)"
}
```
