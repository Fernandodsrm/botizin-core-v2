# S3 — Recuperação USB, dois slots 0.0.17 — 22/10/2026

## O que fizemos
- Executado no Windows o script `RECUPERAR-S3-DOIS-SLOTS.ps1` fornecido na conversa, via PowerShell, utilizando `esptool.py 4.5.1` e porta COM4.
- Identificada ESP32-S3 revisão 0.2, flash 16 MB e mapa de partições esperado.
- Baixado binário estável S3 versão 0.0.17, 1.207.616 bytes; SHA-256 `7a7c16bcb2acd885c74c7d8ad77fe15f9d0e1e55d634dfd23d1ebb7abdb6884f` confere.
- Gravado o MESMO binário OTA em `app0` (0x00010000) e `app1` (0x00310000).
- **Não houve `erase_flash` completo.** NVS, `otadata`, tabela de partições e bootloader foram preservados; o esptool apagou e regravou os setores-alvo dos dois aplicativos.

## O que testamos
- Confirmação de Flash 16 MB, app0/app1 de 3 MiB e `otadata` em 0xE000.
- `esptool write_flash` de app0 e app1; cada um reportou `Hash of data verified.`
- `esptool verify_flash` individual dos dois endereços; ambos reportaram `verify OK (digest matched)`.
- Reinicialização automática via `--after hard_reset`.
- Consulta `/status` pela rede local depois do reboot.

## O que aconteceu
- App0: 0.0.17 gravada, hash verificado.
- App1: 0.0.17 gravada, hash verificado; é a partição em execução.
- Status emitido **pela S3 física**: `BOTIZIN CORE V0.0.17`; IP local privado; `RUNNING_PARTITION: app1 / ota_1`; `NEXT_UPDATE_PARTITION: app0 / ota_0`; `RUNNING_OTA_STATE: VALID`; `WIFI: OK`; `OTA: READY`; `INTERNET_OTA: UP_TO_DATE 0.0.17`.
- `RESET_REASON: BROWNOUT (9)` apareceu; isso é o motivo do último reset informado e, por si só, não prova nova queda durante o teste.
- Confirmado também que `main/manifest.json` continua ofertando 0.0.17 com SHA esperado. **Nenhum manifesto foi modificado por esta recuperação.**

## O que aprendemos
1. **Recuperação USB funcionou integralmente**: firmware conhecido nos dois slots, Wi-Fi e consulta automática ao GitHub reestabelecidos.
2. Ambos os slots com o mesmo binário **não são teste de rollback entre versões diferentes**, embora os dois tenham passado por verificação de escrita.
3. Estado `VALID` foi observado no slot ativo app1; a execução do firmware em app0 após escolher esse slot **não foi testada nesta sessão**.
4. Firmware gravado e consulta do manifesto não provam que uma versão futura será instalada e aceita sem falhas: isso é o objetivo do próximo teste incremental, com `/status` antes/depois e registro de boot.
5. Investigar alimentação caso `BROWNOUT` volte a ser registrado com novos reinícios. Não recomendar apagar NVS ou `otadata` sem objetivo específico.

**Último estado físico confirmado:** S3 0.0.17, app1 ativo VALID, Wi-Fi OK e GitHub OTA UP_TO_DATE. **Próxima versão não instalada.**
