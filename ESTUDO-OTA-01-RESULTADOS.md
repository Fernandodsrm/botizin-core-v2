# ESTUDO OTA-01 — S3 0.0.17: testes e conclusões

**Data:** 22/10/2026. **Escopo:** auditoria real de código; sem gravação da S3, sem mudar partições, sem alterar o manifesto principal e sem publicar nova versão.

## O que fizemos

Criamos a branch isolada study/ota-01-rollback-recovery derivada da S3 0.0.17. Conferimos o firmware original, a função de comparação de versões, a validação manual de candidato, o gravador Update e as rotinas de consulta GitHub. Conferimos também o código do Arduino ESP32 core 3.3.12, especialmente verifyRollbackLater, verifyOta e initArduino.

Criamos tests/ota01_real_firmware_test.py, que extrai e executa a função C++ original newerVersion, executa os testes nativos existentes para otaConfirmationMatches e verifica o restante das condições diretamente no código. Automatizamos a execução no GitHub Actions.

- Teste: https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/39728707658
- Core verificado: https://github.com/espressif/arduino-esp32/blob/3.3.12/cores/esp32/esp32-hal-misc.c
- Fonte: https://github.com/Fernandodsrm/botizin-core-v2/tree/study/ota-01-rollback-recovery

## O que testamos

| Verificação | Resultado | Tipo |
|---|---|---|
| Firmware estudado é 0.0.17 | Aprovado | Estático |
| Comparação de versão, 11 casos incluindo 0.0.16, 0.0.17 e 0.0.18 | Aprovado | Execução da função real em C++ |
| ID, versão, SHA, boot e validade temporal do candidato | Aprovado | Execução do helper C++ existente |
| Só versão maior aciona OTA automático | Aprovado | Estrutural |
| URL de manifesto e prefixo de binário permitidos | Aprovado | Estrutural |
| SHA recebido e SHA lido da Flash previstos no caminho de gravação | Aprovado | Estrutural |
| Dois slots OTA de 3MiB exigidos no setup | Aprovado | Estrutural |
| Arduino valida imagem cedo por padrão | Confirmado | Fonte core 3.3.12 |
| Não há rollback do aplicativo ao perder MQTT/Wi-Fi depois da confirmação | Confirmado | Fonte atual |
| Janela manual de cinco minutos não é rollback | Confirmado | Fonte atual |
| Recuperação de espera Wi-Fi/NTP difere de STOPPED fatal | Confirmado | Fonte atual |
| MQTT não transporta o firmware OTA | Confirmado | Fonte atual |

**Resultado: 12/12 testes passaram** na execução 39728707658. Os ensaios estruturais confirmam o código, NÃO simulam uma queda real de energia nem provam um rollback físico. O caminho OTA normal foi demonstrado anteriormente pela instalação 0.0.16 → 0.0.17 e pelo /status real.

## O que aconteceu

A função original aceita 0.0.18 e versões posteriores, e não considera novas 0.0.17 ou versões anteriores. Quando encontra no manifesto uma versão mais antiga, o programa chama stopInternet e suspende a busca OTA até reiniciar; apenas publicar uma versão menor não provoca downgrade automático.

O core Arduino define verifyRollbackLater como falso e verifyOta como verdadeiro. Quando uma imagem nova entra em PENDING_VERIFY, o core pode confirmá-la imediatamente como VALID durante initArduino, antes do setup e antes de testar conexão Wi-Fi ou MQTT. A S3 0.0.17 não substitui esses hooks. Por isso o rollback habilitado pelo bootloader NÃO oferece um retorno por falha de rede depois da confirmação.

A janela de cinco minutos implementada no cancelamento manual só adia a atualização automática. NÃO é um temporizador de rollback.

## O que aprendemos

1. A instalação normal verifica tamanho, SHA recebido, SHA da partição gravada e destino de boot. A confirmação de saúde depois do boot é a parte que falta.
2. O firmware antigo continua no outro slot durante o update, mas não há botão seguro implementado para voltar a ele depois que o novo é VALID.
3. Se oferecermos uma versão ruim 0.0.18 no manifesto principal e ela voltar para 0.0.17, a 0.0.17 pode detectar a 0.0.18 novamente e entrar em CICLO DE REINSTALAÇÃO. Não testar firmware deliberadamente defeituoso por alteração do manifesto principal.
4. Uma confirmação tardia requer substituir o hook verifyRollbackLater e confirmar VALID explicitamente só depois de cumprir um critério de saúde.

## Próxima tarefa: OTA-02 — experimento controlado

**A. Projeto e testes sem tocar hardware:** preparar branch com hook de verificação tardia, estado PENDING_VERIFY, limite de tempo e evidência de saúde. Testar em host a política de estados e compilar candidato S3 com a toolchain real.

**B. Teste controlado na placa:** instalar apenas por upload LAN a candidata no slot inativo, mantendo o manifesto principal 0.0.17. Confirmar leitura do estado PENDING_VERIFY, depois permitir que candidato saudável fique VALID. Numa execução de falha controlada, provar a volta à 0.0.17; não provocar ciclos de oferta OTA.

**Critérios:** saudável só vira VALID após autotestes; falho reverte sem intervenção; firmware 0.0.17 segue disponível; nenhum endereço de diagnóstico é exposto na internet.

**Sem atualização física neste OTA-01.** A última leitura física verificada permaneceu S3 0.0.17, app1/ota_1, OTA VALID.
