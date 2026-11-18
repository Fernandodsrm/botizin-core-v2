# BOTIZIN S3 — OTA 3.0 / CORE 0.0.31 — TESTADA E CONFIRMADA

**Aceite do responsável pelo projeto:** 18/11/2026. **Escopo:** funcionalidades principais do gerenciador OTA 3.0 testadas fisicamente em ESP32-S3 N16R8, incluindo ida e volta entre versões compatíveis, sem reinstalar ou conectar USB.

## Referências verificadas

- **Imagem:** `packages/ota3-0.0.31/firmware.bin`, commit imutável de publicação candidata `232cc5040b00afb72c7e44eb91628c980e3d6cab`.
- **Tamanho:** `1.300.480` bytes, dentro da APP de 3 MiB.
- **SHA-256:** `6e526a8299dd24c112c5324878954c1dea81d0f15e99ae49d6484d4ee57dda82`.
- **CI:** [GitHub Actions #44253160947](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/44253160947) — SUCCESS, testes estáticos, JavaScript e compilação do Arduino core 3.3.12.
- **Acompanhamento e evidências:** [Issue #2](https://github.com/Fernandodsrm/botizin-core-v2/issues/2), testes físicos 01–10.
- **Catálogo OTA manual:** `candidate/ota3-0.0.29/ota3/catalog.json`; CORE 0.0.31 marcada `TESTADA_E_CONFIRMADA`, `physical_test:true`, `approved:true`, `experimental:false`. O firmware já compilado pode continuar exibindo a palavra `EXPERIMENTAL` no campo de build por origem histórica; não altera o resultado do aceite físico dentro do escopo.

## Testes físicos efetivamente confirmados

| Área | Evidência | Aceite |
|---|---|---|
| Consulta ao catálogo GitHub | 2, depois 4 e 5 versões carregadas sob demanda | CONFIRMADO |
| Sem consulta automática | `CHECK_INTERVAL_SECONDS: 0`; `DIAG_GIT_MAX_US: 0` nas sessões observadas | CONFIRMADO |
| Gravação no slot inativo | APP1 e APP0 gravados enquanto a outra APP permanecia em execução | CONFIRMADO |
| Instalar sem mudar próximo boot | `APP*_GRAVADA_SEM_TROCAR_BOOT` e `BOOT_PARTITION` inalterado até comando manual | CONFIRMADO |
| SHA-256 após escrita | `OTA3_LAST_BYTES: 1300480/1300480`; SHA acima coincide com CI e catálogo | CONFIRMADO |
| Selecionar APP0/APP1 | Boot por comando do painel, `RESET_REASON: SOFTWARE (3)` | CONFIRMADO |
| Voltar sem reinstalar | Fluxo 0.0.29↔0.0.30 e 0.0.31 APP0→0.0.30 APP1, com ambas imagens válidas | CONFIRMADO |
| Identificar versões e estados | Painel 0.0.31 exibiu CORE .31 APP0 e CORE .30 APP1, `VALID`, próxima APP e destino claro | CONFIRMADO |
| Wi-Fi/mDNS/HTTP | `WIFI OK`, `MDNS READY`, `HTTP_CONFIRMED_BY_REQUEST YES` após boots | CONFIRMADO |
| MQTT | `MQTT_LAST PUBLISHED`, `MQTT_ATTEMPTS 4`, `MQTT_SUCCESSES 4` **na CORE 0.0.30** depois do retorno | CONFIRMADO NA 0.0.30 |
| RAM na .31 | `DIAG_FREE 239200`, `DIAG_MIN 229968`, `DIAG_LARGEST 196596`; uptime 38 s no registro | BASELINE CURTA |

## Último estado físico relatado

- `APP 0 / ota_0`: CORE **0.0.31**, `VALID`, instalada, **inativa**.
- `APP 1 / ota_1`: CORE **0.0.30**, `VALID`, **executando** e selecionada para próximo boot.
- Nenhuma versão foi regravada na alternância final.
- A CORE **0.0.25 ORIGINAL** permanece no GitHub; **não** há partição dedicada a ela.
- `main/manifest.json` permaneceu na 0.0.25; OTA3 só consulta catálogo quando solicitado e não instala automaticamente.
- **Não alterar a tabela de partições nem o bootloader** no checkpoint.

## Limites reconhecidos (não impedem este aceite)

1. MQTT pós-primeiro ciclo foi comprovado na CORE **0.0.30**, mas **não** houve captura de publicação MQTT prolongada na **0.0.31**; não declarar teste que não ocorreu.
2. A interface refinada .31 ainda não recebeu um teste **de gravação iniciada pela própria .31** para comprovar a atualização dos dois cartões imediatamente após a escrita. O mecanismo de gravação feito pela .30 e a versão visual após boot estão confirmados.
3. Falhas extremas de firmware/otadata podem exigir recuperação USB. Não existe garantia de rollback automático por cinco minutos.
4. Em `/status`, o bloco `LAST_PERSISTED_OTA_ATTEMPT` refere-se ao instalador OTA legado (0.0.28→0.0.29), enquanto `OTA3_LAST_RESULT/BYTES/SHA256` são a evidência do OTA 3.0.
5. Este checkpoint não afirma testes de stress, queda de energia durante flash, catálogo indisponível ou recuperação de todas as falhas de boot.

## Decisão

**CORE 0.0.31 = TESTADA E CONFIRMADA NA BANCADA PARA O OTA 3.0.**
**Módulo OTA 3.0 = ENCERRADO** para o fluxo principal.
Não criar outra revisão apenas para reexecutar uma gravação de demonstração, não mexer na APP ativa sem necessidade e não disparar atualização automática.
Próximo trabalho: módulos do painel e comunicação; **Recursos e Limites será revisitado no final**, conforme prioridade do usuário.
