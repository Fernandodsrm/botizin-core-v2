# OTA-04 — Teste do firmware 0.0.20 com verificação de saúde local

Data: 22/10/2026 (BRT).

## O que fizemos

- Ajustamos a confirmação tardia para exigir partições prontas, Wi-Fi conectado e servidor HTTP iniciado por 15 s contínuos; MQTT público e NTP deixam de ser condições obrigatórias de validação do boot.
- Mantivemos o hook forte `verifyRollbackLater` e prazo de rollback de 180 s.
- Mantivemos MQTT somente-publicação e diagnóstico local de tentativas/sucesso no `/status`.
- Criamos o firmware S3 0.0.20, sem mudar mapa de partições ou credenciais Wi-Fi.
- Publicamos o binário aprovado em `main/releases/0.0.20/firmware.bin` e oferecemos via `main/manifest.json`.
- Workflow automático observou o broker MQTT por 320 s; na falta de confirmação da 0.0.20, pausou a oferta ao restaurar manifesto da 0.0.17.

## O que testamos

- Testes de política local: 15/15 passaram (incluindo ausência de MQTT, ausência de NTP, perda de Wi-Fi, falta de HTTP, timeout e confirmação saudável).
- Teste de candidato manual OTA anterior passou.
- Compilação real com Arduino ESP32 core 3.3.12 e PubSubClient 2.8 aprovada; símbolo de override forte de `verifyRollbackLater` verificado no ELF.
- Firmware compilado: 1.209.392 bytes, SHA-256 `09dcc1af1a83b1995ad4468a80e19d3dbd284516d8dd4eebb72e746e9a5825e8`.
- Oferta OTA remota: manifesto `main` 0.0.20 temporariamente, pacote baixável verificado; observação MQTT GitHub Actions foi executada.

## O que aconteceu

- Compilação PASS: https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/39739006648.
- Publicação/observação com pausa automática PASS: https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/39739382906.
- Durante a janela de escuta, recebida mensagem MQTT pública (não autenticada) em 22/10 às 07:18:20 UTC: firmware 0.0.17, slot ota_1, ota_valid true, ota_ready true, uptime 672s.
- **NÃO recebemos prova de execução da 0.0.20 em hardware**. Uma única observação com 0.0.17 não prova que a 0.0.20 foi sequer baixada; pode haver OTA suspenso, reinicializações elétricas, conexão intermitente ou janela não coincidente.
- Conforme configurado, o workflow restaurou automaticamente `main/manifest.json` para versão 0.0.17 quando não confirmou a 0.0.20. Confirmado por leitura direta do manifesto depois da execução.
- A imagem 0.0.20 permanece salva em `releases/0.0.20/firmware.bin`; 0.0.17, 0.0.18 e 0.0.19 também continuam disponíveis.

## O que aprendemos

- A dependência de um broker MQTT público não era um bom requisito para aceitar firmware; a política de saúde local já foi corrigida, testada e compilada.
- Ainda não temos evidência física de que a S3 iniciou a 0.0.18 ou 0.0.20. A captura prévia mostrou 0.0.18 gravada e verificada, mas S3 novamente em 0.0.17 com `RESET_REASON:BROWNOUT (9)` e nova tentativa de download.
- Evidência remota pelo MQTT informa telemetria, não estado completo do mecanismo OTA; precisamos ler `/status` local em tempo útil ou logs serial durante boot.
- Próximo diagnóstico necessário, sem mexer em partições: capturar `/status` na rede local e analisar `INTERNET_OTA`, `LAST_PERSISTED_OTA_ATTEMPT`, `CURRENT_OTA_ATTEMPT` e `RESET_REASON`. Se OTA parar ou reinicializar por brownout, estabilizar alimentação; USB serve como diagnóstico/recuperação, não precisa ser primeira via enquanto `/status` funciona.

**Estado comprovado ao fechar este relatório:** manifesto `main` 0.0.17; 0.0.20 compilada e preservada, sem confirmação de instalação física.