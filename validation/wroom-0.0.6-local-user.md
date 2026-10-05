# WROOM 0.0.6 — teste local fornecido pelo usuário

2026-10-05, aproximadamente 06:54 America/Sao_Paulo. GET /status pelo Windows respondeu nos dois IPs.

WROOM 192.168.0.38: versão 0.0.6, uptime 400 s, reset SOFTWARE (3), app1/ota_1 de 1310720 bytes como execução e boot, OTA VALID, Wi-Fi OK, telemetria HTTP_200, OLED READY_128x64_SDA21_SCL22, página OTA. OTA_AUTOMATIC_ENABLED YES; estado manual CANCELLED_AUTO_IN_5MIN. S3_LINK OK, versão remota 0.0.10. Journal registra 0.0.5→0.0.6 com bytes 1224256 e SHA recebido/flash/esperado f85308d75759bab3533ac89f724cb4422632a352f8fe47e0443f81be93d66e8c; sem erro Update.

S3 192.168.0.36: versão 0.0.10, uptime 728 s, último reset BROWNOUT (9), app0/ota_0 de 3145728 bytes como execução e boot, OTA VALID, Wi-Fi OK, telemetria HTTP_200; manifesto atual 0.0.10, intervalo 60 s.

Resultado: os IPs atuais são corretos e a leitura local entre placas voltou a funcionar. Falha anterior é intermitente; causa não determinada. O timeout de 400 ms da WROOM e as operações HTTPS síncronas da S3 são uma hipótese a medir, não uma causa comprovada. A recuperação aconteceu sem nova alteração de firmware além da instalação da 0.0.6 já registrada.

Próximo teste: 0.0.7 repete funções da 0.0.6 para comprovar consulta/candidata/confirmacão/download/hash/reboot pela rota manual. O automático permanece habilitado; a publicação do manifesto será coordenada dentro da reserva manual. Instalação automática não vale como prova da rota manual.
