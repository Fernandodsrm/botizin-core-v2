# S3: instalação e retorno automático comprovados

Em 2026-10-07, completamos na placa real: **0.0.13 (app1) → 0.0.15 (app0) → 0.0.13 (app1)**.

## Entrega e comando

A oferta curta anterior não demonstrou instalação. Nesta rodada, a oferta ficou disponível por até cinco minutos. Durante a oferta, duas consultas explícitas retornaram `UP_TO_DATE 0.0.13`; uma consulta posterior encontrou a 0.0.15 com tamanho/hash corretos. Isso comprova atraso em encontrar o manifesto novo; cache é uma explicação consistente com o endpoint anunciar max-age=300, mas não identificamos precisamente a camada responsável.

Após `AVAILABLE` e `candidate_ready: true`, retiramos o manifesto experimental antes de confirmar a instalação. A confirmação vinculou candidate_id, versão, SHA e boot_id. A S3 respondeu `INSTALL_ACCEPTED`. O watchdog de publicação confirmou que a oferta original já estava restaurada.

## Evidência física

- Fonte compilada: 5918d60127aa2a3688df3968dee614480ba202d4.
- BIN 0.0.15: 1.164.880 bytes; SHA256 64d3ab72abafc46cadbe4b6b6092624fbdf182f780765dbcdf2b9cc323de3c71.
- Experimental: app0 em 0x10000; boot 40a729e7711f169; próximo boot app1 em 0x310000.
- Status real: `ARMED_600_SECONDS_RETURN_0.0.13`. Armar exige validar o hash exato da 0.0.13 preservada.
- Dez relatórios do experimental, uptime de 25 a 565 segundos, mesmo boot e nenhuma falha de telemetria registrada.
- Retorno: 0.0.13 no app1; novo boot 946d0bbf4b6962dd; reset SOFTWARE.
- Estimativa pelos timestamps recebidos menos uptime: 600,065 segundos entre os boots (aproximadamente dez minutos, não uma medição instrumental de precisão).
- Consulta RPC após retorno: estado VALID, automático true, manual READY, internet UP_TO_DATE 0.0.13, sem candidato pendente.
- Manifesto main permanece 0.0.13. A WROOM não recebeu oferta nem firmware novo.

## Recursos durante o experimental

Mínimo de heap interno observado: 200.872 bytes. Heap livre estabilizou em cerca de 259.612 bytes. Maior intervalo entre passagens do loop: 1.287.014 microssegundos. Esse intervalo inclui espera de rede e não é percentual de uso de CPU. Git e RPC estavam desativados no experimental para preservar o slot de retorno; telemetria continuou em intervalos de aproximadamente 60 segundos.

## Limites e política

Comprovamos uma rodada remota de execução e retorno normal por temporizador na S3. Não injetamos falhas de energia, travamento antes da armação ou perda de Wi-Fi; não declarar esses casos garantidos. Os testes nativos de guardas passaram e a compilação ESP32 também.

O Menu pode ficar fixo em um slot se todas as gravações de módulos ocorrerem a partir dele no outro slot. Módulos mantêm retorno local ao Menu e bloqueiam gravação no slot preservado. Atualizar o próprio Menu exige outro fluxo de manutenção. O Menu mínimo final ainda não foi implementado; nesta prova a referência preservada era a 0.0.13.

O retorno sem download pode dispensar Wi-Fi, HTTPS e OTA de download nos módulos que não usam esses recursos. Para comandos remotos com um módulo aberto, é necessário manter rede ou um retorno local/programado ao Menu.

O ThingsBoard mantém a última leitura de cada chave. Após o retorno, as chaves timed_trial_* antigas ainda constam com timestamps do experimental: elas não descrevem o estado atual da 0.0.13. A evidência JSON abaixo inclui apenas campos do mesmo timestamp de cada relatório, evitando mistura de boots.

## Registros

- [Build aprovado](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/37664816919)
- [Oferta com retirada automática](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/37665212699)
- [Evidência](validation/trial15-proof.json)
- [Projeto do Menu de dois slots](MENU_TWO_SLOT_DESIGN.md)

O workflow que disparava a oferta ao editar a branch foi removido após esta rodada para evitar novas ofertas acidentais. Código e histórico permanecem disponíveis.
