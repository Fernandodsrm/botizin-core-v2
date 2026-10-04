# BOTIZIN CORE 0.0.7 — BASE CONGELADA

Regra expressa de Fernando: não alterar, recompilar sobre os arquivos salvos,
sobrescrever o binário, mover esta referência ou atualizar o manifesto desta base.
Todo desenvolvimento seguinte ocorre em outra versão e em outra pasta.

Binário físico validado: `releases/0.0.7/firmware.bin`.
Tamanho: 1.145.648 bytes.
SHA256: `08eea6677702ef8b0fc166da018e5485bd3ebd54355e91701680515ff1fc8929`.
Commit de publicação: `5716a3d55e586b09eba88be8739393e3a171f606`.
O conteúdo desse commit é uma referência fixa; a branch de arquivo é um marcador
e não equivale a um bloqueio de permissões no GitHub.

## Evidência física enviada por Fernando em 04/10/2026

- USB 0.0.1, OTA local 0.0.2 e 0.0.3; persistência após cortar alimentação.
- 0.0.4 apresentou estouro de pilha ao iniciar TLS; não usar 0.0.4/0.0.5.
- ELF identificou `checkInternetOTA()` → `HTTPClient::GET()` → handshake TLS.
- Correção única: buffer de 4096 bytes passou a armazenamento static.
- 0.0.6 realizou pelo menos quatro consultas HTTPS sem PANIC/reboot.
- OTA automático pela internet: 0.0.6 / ota_0 → 0.0.7 / ota_1.
- Transporte: HTTPS com bundle de CAs autenticado.
- Update.begin / Update.setSHA256 / Update.end: TRUE.
- Bytes escritos: 1.145.648; SHA esperado = recebido = leitura da flash.
- Biblioteca Update: erro 0 / No Error; descrição da imagem ESP_OK.
- Boot pós-OTA: 0.0.7, SOFTWARE (3), app1/ota_1, estado VALID.
- Boot após corte de alimentação: 0.0.7, POWERON (1), uptime 20 segundos,
  running app1/ota_1, boot app1/ota_1, próxima app0/ota_0, estado VALID.
- Wi-Fi OK; INTERNET_OTA UP_TO_DATE 0.0.7; consulta a cada 60 segundos.

ESP32-S3 N16R8: flash 16.777.216 bytes, PSRAM 8.388.608 bytes, CPU 240 MHz,
core Arduino ESP32 3.3.12, QIO/80MHz, OPI PSRAM, dois slots de 3 MB,
particionamento app3M_fat9M_16MB.

Os fontes arquivados incluem a correção exata aplicada pelo script de reparo
e a versão 0.0.7. Não incluem senha Wi-Fi: WiFi.begin() reutiliza NVS.
O binário recebido, seu SHA e o teste físico são a evidência autoritativa;
uma recompilação futura pode produzir outro SHA devido a metadados/toolchain.

O core Arduino confirma a imagem durante o início. A validação comprova OTA e
persistência, mas não testa rollback deliberado por autoteste da aplicação.
