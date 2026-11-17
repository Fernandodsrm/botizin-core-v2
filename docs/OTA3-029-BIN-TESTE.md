# OTA 3.0 — BIN candidato fixado para primeiro teste na rede

**ATENÇÃO: não é release; ainda não foi testado fisicamente.** O firmware atual da S3 continua em 0.0.28 app1 até o usuário iniciar manualmente um instalador.

- Fonte do binário: commit de compilação `d79cac0db1285588f51166da5baa42448d191896`.
- Publicação **somente em branch candidata**: commit imutável `f79d42b0565d32fb935d6508125741d132cf1119`.
- Arquivo: `packages/ota3-0.0.29/firmware.bin`.
- SHA-256: `1e35e161df445aca08efa92d3adf42eedda57a6ebda0aadad263ae33fce0c8cf`.
- Tamanho: **1.290.208 bytes**.
- CI: https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/44046511132
- Código e arquitetura: https://github.com/Fernandodsrm/botizin-core-v2/blob/candidate/ota3-0.0.29/docs/OTA3-CANDIDATO-029.md

## Modo de instalação inicial (único uso de OTA legado)
O firmware 0.0.28 em APP1 não possui OTA3, mas seu `POST /update?size=...&sha256=...` já está funcional. O instalador deve:
1. Conferir `/status`: CORE 0.0.28, APP1 RUNNING/BOOT, APP0 NEXT, OTA VALID/READY e Wi-Fi estável.
2. Conferir o manifesto de produção em 0.0.25 (não alterar manifesto).
3. Baixar este binário pelo commit imutável e verificar comprimento/SHA antes de enviar.
4. Exigir confirmação explícita para gravar APP0 e enviar multipart/form-data para `/update`.
5. Confirmar `OTA_WRITE_VERIFIED: YES` e `SHA_CALCULATED_FLASH`.
6. Exigir segunda confirmação para `POST /reboot`. A 0.0.28 já programa APP0 como próximo boot após Update.end, mesmo antes de reiniciar.
7. Confirmar `/status` CORE 0.0.29, RUNNING APP0, estado VALID.

**Risco:** a 0.0.29 ainda não passou pelo teste físico. Caso o painel não abra, pode ser necessário USB. APP1 0.0.28 continuará gravada se a instalação seguir o fluxo correto. Não reexecutar uma gravação após falha sem ler /status e logs. A partir do OTA3, as instalações usam `esp_ota_begin/write/end` para não alterar boot até o clique do usuário.

**Sem publicação em `main`, sem disparar OTA automática, sem modificações no firmware da WROOM.**
