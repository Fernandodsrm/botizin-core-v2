# ESTUDO OTA-02 — confirmação tardia, rollback e testes da S3

**Data:** 22/10/2026. **Status:** experimentação de código concluída; nenhum firmware instalado fisicamente.

## O que fizemos

- Criamos a branch `study/ota-02-deferred-health` partindo do OTA-01.
- Criamos uma política pura e testável em `firmware/ota_boot_health.h` e testes em `tests/ota02_boot_health_test.cpp`.
- Acrescentamos ao firmware experimental `0.0.18` um override `extern "C" bool verifyRollbackLater(void) { return true; }` que impede a confirmação antecipada padrão do core Arduino.
- Em execução pendente, aguardamos Wi-Fi conectado, NTP, servidor HTTP inicializado, partições prontas e ao menos um envio MQTT bem-sucedido. Quando todos os critérios ficam verdadeiros por **15 segundos consecutivos**, tentamos `esp_ota_mark_app_valid_cancel_rollback()`.
- Se partições inválidas ou a janela de **180 segundos** termina sem passar, solicitamos `esp_ota_mark_app_invalid_rollback_and_reboot()`. Se esse rollback falhar, registramos o erro e não repetimos a operação num loop apertado.
- Suspendemos temporariamente consultas OTA GitHub, ThingsBoard telemetria e RPC enquanto há imagem `PENDING_VERIFY`; mantivemos HTTP local e envio MQTT.
- Evitamos bloquear o OTA ao consultar um manifesto mais antigo do que uma versão experimental instalada: informa `MANIFEST_NOT_NEWER` sem iniciar downgrade.
- Criamos workflow de laboratório `ota02-lab-build.yml`: ele **não modifica `main`, não publica manifestos e não grava ESP32**. O build usa credencial ThingsBoard de placeholder: **`publishable=false`**; não é pacote de produção.

## O que testamos

- Política de saúde em **15 cenários**: ausência de firmware pendente, boot frio, confirmação prematura, 15s saudável, perda de Wi-Fi, falta de NTP, ausência de HTTP ou MQTT, timeout por rede/broker, timeout com sinais atrasados, partições inválidas e contador com wrap.
- Testes anteriores de identificação de candidato manual (SHA, versão, boot_id e prazo) recompilados e executados.
- Confirmamos por rede a versão `0.0.17` e o hash correto **do manifesto principal em produção**, independentemente do manifesto histórico da branch de desenvolvimento.
- Fizemos build ESP32-S3 completo com **Arduino core 3.3.12, Arduino CLI 1.5.1 e PubSubClient 2.8**, sem token de produção.
- Inspecionamos o arquivo ELF gerado por `nm`: símbolo `T verifyRollbackLater` em vez de `W` (forte, linkado corretamente), confirmando que a função experimental realmente substitui o hook fraco.

## O que aconteceu

**GitHub Actions:** execução https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/39733987530 — `success`.

- `OTA02_POLICY_TESTS_OK 15 cases`.
- `OTA_CONFIRMATION_TESTS_OK`.
- `OTA02_SOURCE_AND_MANIFEST_PASS`.
- `OTA02_LAB_COMPILE_OK 1209136 PUBLISHABLE_FALSE` — **1.209.136 bytes**, abaixo dos 3.145.728 bytes de cada slot.
- `420c26e8 T verifyRollbackLater`.
- `OTA02_STRONG_C_LINKAGE_HOOK_OK`.

**Atenção:** esta evidência prova a política e o firmware compilado, NÃO o efeito físico do bootloader ao reiniciar uma ESP32-S3. A S3 física permanece na 0.0.17 conforme última verificação do `/status`. O manifesto público também continua ofertando 0.0.17.

## O que aprendemos

1. A confirmação tardia é tecnicamente possível usando o hook existente do core Arduino — não precisamos modificar bootloader nem tabela de partições.
2. O firmware candidato 0.0.18 ainda cabe com folga nos slots atuais.
3. O teste com MQTT público como requisito obrigatório prova conectividade fim a fim, mas é frágil como regra final: indisponibilidade do broker pode reverter firmware saudável. A política definitiva deve usar critérios locais e/ou canal autenticado controlado por nós.
4. Caso 0.0.18 falhe e retorne a 0.0.17, manter o manifesto principal em 0.0.17 evita que a própria S3 volte a oferecer automaticamente a versão falha.
5. Se não houver imagem anterior válida disponível, o rollback pode falhar; o firmware mostra `ROLLBACK_FAILED_...` e a recuperação pode exigir USB. Ainda não testamos essa condição em placa.
6. HTTP local foi inicializado, mas não recebe uma requisição ativa como condição de saúde. Isso pode ser um requisito futuro.
7. O arquivo `manifest.json` *dentro da branch de estudo* possui versão histórica e NÃO é o manifesto efetivamente consumido pela S3. Consultamos diretamente `main/manifest.json` no teste.

## Próxima ação (OTA-02 físico, ainda não iniciada)

**Preparar teste físico controlado por Wi-Fi local**: antes de qualquer upload, consultar o `/status` da S3 (versão 0.0.17, app1, `VALID`, `OTA READY`), confirmar 0.0.17 preservada no manifesto; compilar candidata autorizada com credenciais apropriadas (o build atual é VALIDATION_ONLY); transferir via canal local de atualização para app0; conferir progresso e reboot; comprovar a transição `PENDING_VERIFY` → `VALID` após saúde, depois conduzir um teste separado de falha que exija retorno a app1. Não ofertar versão deliberadamente falha no manifesto público. Se houver dúvida sobre disponibilidade do firmware fallback, testar somente a candidata saudável e não provocar falha.

**Arquivos alterados exclusivamente na branch experimental:** `firmware/firmware.ino`, `firmware/version.h`, `firmware/ota_boot_health.h`, `tests/ota02_boot_health_test.cpp`, `.github/workflows/ota02-lab-build.yml` e este relatório.

## Evidência física posterior — 22/10/2026 às 04h04 BRT

Captura do `/status` local entregue pelo usuário:
- A placa respondia `BOTIZIN CORE V0.0.17` no IP local `192.168.0.18`, `app1/ota_1`, imagem `VALID`, `OTA: READY`; uptime **22 segundos**, `RESET_REASON: BROWNOUT (9)`.
- `LAST_PERSISTED_OTA_ATTEMPT` mostrava origem 0.0.17, destino app0/ota_0, versão 0.0.18, **1.209.136 bytes**, SHA verificado dos bytes e da Flash `a360836ac1d886e53ab1db90fe4046675c6904d3635815a0aab88696ff03300d`, `Update.end: TRUE`, `DESTINATION_OTA_STATE: NEW`, `OTA_WRITE_VERIFIED: YES` e reinício solicitado.
- Ao mesmo tempo, `INTERNET_OTA: DOWNLOADING 0.0.18` e `CURRENT_OTA_ATTEMPT` informavam uma NOVA tentativa de instalar a mesma imagem. Isso demonstra **reoferta/reinstalação**, mas não diferencia com certeza rollback, reinício por queda de tensão ou combinação das duas causas.
- Observações remotas subsequentes da fase inicial receberam somente um estado MQTT 0.0.17 e depois zero mensagens; **não houve prova de execução física da 0.0.18**.
- Para evitar ciclo, em seguida o manifesto `main/manifest.json` foi **restaurado para 0.0.17**, mantendo os binários 0.0.18 e 0.0.19 armazenados em `main/releases/`. O manifesto antigo de versão 0.0.17 é detectado como "não mais novo" no código experimental 0.0.18, sem forçar downgrade.
- Um observador MQTT em execução foi lançado em GitHub Actions para tentar diferenciar estados posteriores, mas as mensagens de broker público não autenticado são indício, não atestado criptográfico.

**Diagnóstico em aberto:** não afirmar que rollback inteligente já foi validado. Necessário observar `OTA_BOOT_HEALTH` da 0.0.18 ou obter logs seriais durante a inicialização para comprovar a transição `PENDING_VERIFY -> VALID` ou `PENDING_VERIFY -> ABORTED`; investigar brownout e depender menos de broker público para confirmação da imagem.
