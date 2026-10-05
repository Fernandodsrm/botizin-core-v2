# WROOM 0.0.6 — build e limites de validação

Fonte: fb6b2197b1f408a898e0eb957b9bbc309d064e6e; GitHub Actions 37291183776, job 111701686888, artifact 11336034454.
Arduino ESP32 3.3.12; mesma FQBN e bibliotecas OLED fixadas da 0.0.5. Compilação concluída com sucesso.

Binário: 1224256 bytes; SHA256 f85308d75759bab3533ac89f724cb4422632a352f8fe47e0443f81be93d66e8c; capacidade OTA 1310720 bytes. Global RAM 65440 bytes (19%). Header ESP32 clássico e identidade WROOM verificados. Tabela de partições byte a byte idêntica à 0.0.5.

Testes host: parser S3; confirmação vinculada a candidata/versão/SHA/boot; validade e overflow de millis. Testes do widget: gate de versão, correlação, candidata correta e ausência de sucesso prematuro. Nenhuma chave da conta ou credencial Wi-Fi compilada/publicada.

Esta evidência não comprova a OLED física, botões ou instalação manual real. A 0.0.5 validada permanece preservada em refs próprias. A publicação da 0.0.6 permite a primeira instalação automática; consulta/recusa/estado são então testados no dispositivo. Um ciclo manual completo exige uma candidata posterior e confirmação correlacionada.
