# AUDITORIA DA WROOM 0.0.38 — tamanho, histórico e proposta de base modular
**Data:** 11/11/2026. **Natureza:** auditoria somente-leitura, com fontes de releases e logs do GitHub Actions, código do commit de build, estado consultado da placa. **Nenhum firmware alterado, flash gravado, manifesto publicado ou serviço reiniciado.**

## Resumo executivo
- A WROOM 0.0.38 está ativa no hardware, `app0/ota_0`, OTA READY, OLED e rosto prontos, catálogo CACHED, `ENVIRONMENT_INSTALLED: NONE`, AUTO OTA NO. Versão física e hash de release concordam com `wroom/releases/0.0.38/build-record.json` (a consulta física confirma versão, não mede SHA do firmware inteiro a distância).
- **Não é o LED que deixou a WROOM pesada**: pacote 0.0.29 = 1.286.144 bytes; 0.0.35 = 1.286.176; 0.0.38 = 1.288.144. Diferença líquida 29→38 = +2.000 bytes.
- Capacidade de cada partição OTA é **1.310.720 bytes** (0x140000, 1,25 MiB). Margem atual da .38 = **22.576 bytes**, ~1,72%. O log oficial do build relata **Sketch uses 1.287.988 bytes (98%)** e **Global variables use 59.856 bytes (18%)**. A diferença entre `Sketch uses` e comprimento exato do firmware.bin é normal; são métricas distintas.
- A WROOM 0.0.38 informou `DIAG_FREE=153144`, `DIAG_MIN=12416`, `DIAG_LARGEST=110580`, `DIAG_PSRAM=0`: espaço estático de app e picos de RAM interna são DOIS problemas distintos.
- O grande candidato para **simplificar** é a sobreposição de infraestrutura: Wi-Fi + HTTPS GitHub (manifesto/catálogo/download) + HTTPS ThingsBoard (telemetria/consulta RPC) + HTTP servidor local + OTA + diagnósticos + NVS + OLED/menu. Não há prova de qual biblioteca consome quantos bytes compilados; falta mapa de símbolos ELF e builds comparativos. O rosto é procedural (~3,3 KB de código-fonte) e não carrega bitmaps volumosos.
- A tabela de partições contém uma área **SPIFFS de 1.441.792 bytes (0x160000, 1,375 MiB)** e duas apps de 1,25 MiB. **Não encontrei montagem/uso de SPIFFS no código WROOM 0.0.38 examinado**, mas essa área é flash reservada, NÃO parte do firmware.bin. Reparticionar poderia ampliar ambas apps para aproximadamente 1,94 MiB sem ocupar o coredump; **é operação estrutural por USB, com backups/recovery, não OTA normal. Não alterar agora.**
- 0.0.39 do **repositório S3** é `esp32s3-n16r8` de voz LAB (1.982.640 bytes); **não encontrei release de WROOM 0.0.39**. Não cruzar linhas de versões. O manifesto da LAB S3 0.0.39 declara `physical_test_pending`; isso não apaga a conversa por voz que o usuário confirmou ter realizado em seus testes anteriores.
- **Direção:** preservar conexão/recuperação, fazer WROOM BOTIZIN de bancada (rosto + menu + catálogo + OTA/retorno). Módulos experimentais separados. Investigar tamanho com experimentos de compilação sem gravar hardware. NÃO reiniciar numeração em 0.0.0.

## 1. Proveniência exata
| WROOM | Fonte do build (Git SHA) | Pacote tamanho (bytes) | Margem de 1.310.720 B |
|---|---|---:|---:|
| 0.0.04 | `354c639dcd` | 1.210.528 | 100.192 |
| 0.0.07 | `dad207f4a1` | 1.224.256 | 86.464 |
| 0.0.09 | `fe058ef2e0` | 1.227.440 | 83.280 |
| 0.0.17 (ensaio módulo) | `c2eda9ccfc` | 1.162.544 | 148.176 |
| 0.0.18 | `c47cd9dc47` | 1.293.232 | 17.488 |
| 0.0.20 | `0df8098416` | 1.293.232 | 17.488 |
| 0.0.22 | `b5e7d9c61e` | 1.270.976 | 39.744 |
| 0.0.27 | `8a461051b3` | 1.276.448 | 34.272 |
| 0.0.29 | `67163f4665` | 1.286.144 | 24.576 |
| 0.0.35 | `1509224d93` | 1.286.176 | 24.544 |
| 0.0.36 | `5d01b1b761` | 1.286.800 | 23.920 |
| 0.0.37 | `c0d9735b22` | 1.286.800 | 23.920 |
| 0.0.38 | `bea8f4172b` | 1.288.144 | **22.576** |

Fonte: `wroom/releases/0.0.XX/build-record.json`, `wroom/releases/0.0.XX/manifest.json` e fonte do commit correspondente. A série não cresceu monotonicamente (v18/20 chegaram a ser maiores que v29); não concluir que o problema nasce exclusivamente na fase recente.

O CI da .38: [build run 42003908000](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/42003908000). Suas métricas: `Sketch uses 1287988 bytes (98%)` de máximo 1310720 e `Global variables use 59856 bytes (18%)`, restam 267824 em momento de linkagem. Essas métricas **não** substituem a RAM mínima medida em operação.

**Atenção na fonte:** o `wroom/firmware/version.h` no commit de origem da .38 ainda declara `0.0.29`. O script `ci/build_wroom.py` copia as fontes em `wroom-build/firmware` e sobrescreve deliberadamente `version.h` com 0.0.38 ANTES da compilação. Portanto, o binário e o manifesto são .38 válidos, mas compilar manualmente a pasta original sem script poderá gerar versão incorreta e confundir diagnóstico. Fixar/validar metadados da versão na próxima linha.

## 2. Mapa de subsistemas no código 0.0.38
Fontes auditadas **no commit do build bea8f417...**, não na `main` desatualizada:
- `wroom/firmware/firmware.ino`: Wi-Fi, HTTP local, snapshots de status, OTA local e pela internet, HTTP(S) ThingsBoard, processamento RPC e serial. Principais funções: `beginUpload`, `finishUpload`, `checkInternetOTA`, `installInternetOTA`, `sendTelemetry`, `setup` e `loop`.
- `wroom/firmware/environment_catalog.h`: busca HTTPS do catálogo `wroom/catalog.json`, parsing cJSON, NVS cache, validação id/board/hash/URL e preparação dos ambientes.
- `wroom/firmware/module_bridge.h`: grava âncora NVS de retorno com endereço+SHA do MENU, detecta módulo no segundo slot, valida SHA antes de `startInstalledModule`.
- `wroom/firmware/face_engine.h`: rosto gerado por desenho geométrico, movimentos e expressões; sem assets de imagem.
- `wroom/firmware/oled_status.h`: display SSD1306, menus, diagnósticos, catálogo e task de animação dedicada, stack 4096, atualização ~62 ms.
- `wroom/firmware/navigation.h`: botões OK25/UP27/BACK32/DOWN33, retorno da face ao menu e catálogo por botões.
- `wroom/firmware/ping_rpc.h`: protocolo de comandos e respostas via ThingsBoard HTTPS. Inclui OTA, módulo, configuração PS4, catálogo (não é implementação inteira do controle PS4).
- `wroom/firmware/ps4_config.h`: guarda escolha de botão em NVS; o controle de DualShock/Bluepad32 aparece nos **firmwares de módulo**, não no MENU da .38.
- `wroom/firmware/remote_led.h`: GPIO26, auto-off em 30 segundos, código compacto; adição recente, não grande responsável pela ocupação.
- `wroom/firmware/provisioning.h`: configuração USB restrita, Wi-Fi e token ThingsBoard armazenados em NVS (não embutidos).
- `wroom/firmware/diagnostics.h`: amostragens de heap/latência e status, incluindo mínimo de RAM livre.
- `wroom/firmware/partitions.csv`: NVS, otadata, app0, app1, spiffs, coredump.
- `ci/build_wroom.py` e `.github/workflows/build-wroom.yml`: compilação ESP32 Arduino 3.3.12, bibliotecas OLED versionadas e validação de partição; o ELF existe no pacote temporário de Actions, mas não é publicado em `wroom/releases/0.0.38/`.

**Não está presente nesta base:** modelo ChatGPT rodando fisicamente na WROOM, áudio completo no microcontrolador, motor PS4 residente no MENU, bitmaps grandes de rosto, filesystem SPIFFS montado segundo as fontes examinadas. Não inventar o tamanho em bytes de bibliotecas individuais: sem análise do ELF/map ou compilação A/B é desconhecido.

## 3. Por que a 0.0.29/0.0.35 virou referência
- **0.0.29** consolidou o MENU da bancada com rosto, catálogo, ambientes instaláveis, navegação e retorno; o commit `67163f4665c3633bea6cb7c253e4183924d84d4e` é explicitamente preservado como `Menu 0.0.29` nos registros posteriores.
- **0.0.35** foi o ensaio de comando OTA via Live: o histórico documentado `validation/live-ota-0.0.35-procedure.md` registra instalação acionada por conversa/controle remoto e confirmação real da placa; a alteração funcional proposital foi um marcador `LIVE_TEST: TESTE LIVE 35`, **não um novo menu/PS4/voz na WROOM**. Tamanho +32 bytes. Isto explica por que serviu como âncora de testes sem substituir o design da 29.
- **0.0.36** teste de LED físico; **0.0.37** continuidade da interação remota; **0.0.38** expõe LED por HTTP POST local, acionado de fato via MCP A22 + ChatGPT, com desligamento automático.
- **0.0.39:** existe como LAB de voz **S3** `releases/0.0.39/manifest.json`, board `esp32s3-n16r8`, download e áudio browser PCM/Opus; não é release WROOM.

## 4. Histórico rápido da WROOM, das versões iniciais até .38
| Período | Direção predominante | Ganho real |
|---|---|---|
| 0.0.0–0.0.1 | Preparação/USB; 0.0.0 não localizada como release publicada | Primeira validação física de WROOM |
| 0.0.2–0.0.3 | Dual-slot OTA e painel móvel | Atualizar e confirmar boot sem desproteger origem |
| 0.0.4–0.0.7 | OLED, quatro botões, navegação, S3 PING/PONG | Interface e comunicação comprovadas |
| 0.0.8–0.0.12 | Menu, diagnósticos, confirmação/observação OTA, S3/WROOM status | Segurança e observabilidade |
| 0.0.13–0.0.20 | Ensaios de instalação de ambientes, janelas de teste, retorno; alternância automática/manual | Nasce modularidade, mas complexo e apertado |
| 0.0.21–0.0.26 | Menu estável e experimentos PS4/servos independentes | Protótipos de módulos |
| 0.0.27–0.0.29 | Rosto e catálogo do BOTIZIN; Menu 29 como base | Convergência para bancada modular |
| 0.0.30–0.0.34 | Módulos de PS4+servo, servo, LED, A22 Expresso em pacotes separados | Catálogo de ambientes concretos |
| 0.0.35–0.0.38 | Operação por Live, LED e MCP físico, no Menu 29 herdado | Controle físico pela conversa comprovado |

Para os períodos, referências: arquivos `validation/wroom-*.md`, versões em `wroom/releases`, commits de oferta/experimentos e `wroom/catalog.json`. Várias versões têm apenas binário/pacote e histórico de testes incompleto; NÃO interpretar tabela como validação física de todos os intermediários.

**Avaliação:** o projeto evoluiu bastante em capacidade de OTA/segurança/controle. Não decaiu tecnicamente; perdeu foco no produto final quando acumulou sistemas de manutenção e muitos ensaios, enquanto o ciclo completo menu→módulo→retorno na bancada ainda não foi demonstrado na configuração atual.

## 5. FLASH: dois problemas que NÃO podem ser confundidos
**A. Tamanho do código compilado**: 1.288.144 bytes, 98% da partição de app. Candidatos à simplificação (ordem):
1. **ThingsBoard telemetry + RPC** — substituir por status local e MCP/A22 onde os caminhos novos cubram os antigos. Não apagar até comprovar equivalência de acesso e segurança.
2. **Múltiplos caminhos de OTA e controle** — escolher mecanismo de catálogo e atualização unificado, com checagem de hash e retorno, preservando acesso local/recuperação; substituir interfaces redundantes antes de excluir.
3. Diagnósticos volumosos, snapshots/journal, menus legados e configuração PS4 histórica — preservar núcleo mínimo e migrar controles ao módulo.
4. Otimizar alocações no ciclo de TLS, cJSON e Strings, checar stack e pico de RAM; FreeRTOS/task face e 1 KiB framebuffer não explicam 1,28 MB de binário.
5. **Não retirar:** Wi-Fi e reconexão, menu/rosto/botões, autenticação e verificações, catálogo assinado/validado conforme design, integridade SHA OTA, política de volta, telemetria mínima de saúde.

**B. Layout da flash**: `partitions.csv` (WROOM clássica 4MB)
- `app0 0x10000/0x140000` e `app1 0x150000/0x140000`: ambos 1.310.720 B.
- `spiffs 0x290000/0x160000`: **1.441.792 B** reservados, sem montagem/uso detectado no firmware MENU .38 examinado.
- `coredump 0x3f0000/0x10000`: 65.536 B preservados.
- Ao remover/reorganizar SPIFFS, poderia haver dois slots de `0x1F0000 = 2.031.616 B`, app0 em 0x10000, app1 em 0x200000, coredump em 0x3f0000. Cada slot ganha +720.896 B (aproximadamente 55%). **Exemplo matemático, não uma tabela aprovada para gravação.**
- **Risco crítico:** módulo antigo `wroom/ps4-module/ps4-module.ino` verifica endereços FIXOS `0x10000` e `0x150000` em `armMenuReturn()`. Após mudança de partições, módulos antigos não retornariam como antes; recompilar e validar protocolo de retorno é obrigatório. A regravação da tabela de partições não é uma OTA normal, exige USB, backup integral de flash/NVS/credenciais, teste de recuperação e aprovação.
- Conclusão: a partição SPIFFS não pesa dentro do binário, mas seu espaço reservado causa o limite atual. Priorizar otimizar sem alterar layout; considerar reparticionamento apenas se benefícios excederem risco.

## 6. Retorno OTA: regras para não perder BOTIZIN
Na base .38, `saveReturnAnchor()` registra versão, endereço e hash da partição que executa o menu em NVS; `startInstalledModule()` verifica partição e hash antes de selecionar boot no módulo. Nos ambientes 0.0.30–.32, `armMenuReturn()` exige âncora e SHA corretos, seleciona menu como próxima partição de boot antes de ativar funções, e o botão BACK reinicia. Isto é mais robusto que um mero "reinicie e volta", mas **o ciclo completo não foi validado fisicamente com o firmware atual**, que registra `ENVIRONMENT_INSTALLED: NONE`.
- Dois slots executam **um aplicativo de cada vez**. Durante módulo independente, rosto/menu da base não executam. Voz no A22 pode permanecer independente se integrada.
- Preserve base no app0, alvo app1 com validação explícita; impedir atualização automática que sobrescreva base ao executar módulo; não confiar apenas em `esp_ota_get_next_update_partition` sem contexto.
- Restaurar o backup em USB deve continuar possível mesmo com menus ou protocolos incompatíveis.

## 7. Plano objetivo para a próxima versão
**Recomendação de numeração:** não recomeçar em 0.0.0; manter .38 intocada e trabalhar em branch isolada `wroom/botizin-base-040`, candidato `0.0.40`. Não publicar em `wroom/manifest.json` nem instalar até aprovação. O número 0.0.39 é mais conhecido como LAB S3, e não há necessidade de preencher lacuna WROOM. Uma `1.0.0` poderá indicar a base realmente estável concluída, não uma reescrita ainda não testada.

**Três etapas com evidência:**
1. **Profiling sem tocar placa**: exportar/recompilar ELF exato de .38, obter `xtensa-esp32-elf-nm --size-sort`, `objdump`, mapa de link, comparar builds experimentais A/B no MESMO core/toolchain para isolar peso de ThingsBoard, catálogos, TLS e OLED; registrar bytes, RAM, Flash e funcionalidades perdidas. Sem inventar economia.
2. **Candidato mínimo em branch**: rosto procedural + botões + menu + HTTP status + catálogo GitHub + OTA verificada + retorno robusto, retirando componentes somente quando alternativa estiver medida/testada. CI, build, testes de módulo, 0 atual protegido.
3. **Entrega física autorizada**: backup integral e recuperação, instalação de base candidata em slot controlado, prova BOTIZIN → módulo LED → BOTIZIN; então integrar voz por A22/S3 e módulos adicionais. Nenhuma expansão do servidor se não exigida pelo produto.

**Critério de aceite:** o usuário liga a WROOM, aparece BOTIZIN, consulta catálogo, seleciona módulo, usa o dispositivo e retorna sem notebook; estado da S3/MCP permanece íntegro e OTA/recuperação demonstradas. A mera redução de bytes não é sucesso do produto.

## Evidências e links
- [Fonte WROOM usada no build .38](https://github.com/Fernandodsrm/botizin-core-v2/tree/bea8f4172b01f5bcdc2896bf40c6e8f48ab42549/wroom/firmware).
- [Menu 0.0.29 — fonte de referência](https://github.com/Fernandodsrm/botizin-core-v2/tree/67163f4665c3633bea6cb7c253e4183924d84d4e/wroom/firmware).
- [Build da WROOM .38](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/42003908000).
- [Catálogo de módulos](https://github.com/Fernandodsrm/botizin-core-v2/blob/main/wroom/catalog.json).
- [Teste Live OTA .35](https://github.com/Fernandodsrm/botizin-core-v2/blob/main/validation/live-ota-0.0.35-procedure.md).
- [S3 LAB de voz .39](https://github.com/Fernandodsrm/botizin-core-v2/blob/main/releases/0.0.39/manifest.json).
- [Visão Mestre atualizada](BOTIZIN-VISAO-MESTRE-2026-11-11.md).
