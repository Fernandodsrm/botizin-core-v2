# BOTIZIN — Central do projeto (atualizada em 11/11/2026)

## Checkpoint de recuperação S3 — 15/11/2026

**Situação observada fisicamente (somente S3):** a placa ESP32-S3 N16R8 voltou a executar o aplicativo original **CORE 0.0.25**, publicado em `releases/0.0.25/firmware.bin` (1.259.872 bytes; SHA-256 `bc68a1c45c43f4b0e63d40767521b50290a664725568cd0e6acfae24c37e049a`). A gravação USB em `app0` (`0x10000`) e a verificação por leitura foram concluídas; `otadata` foi apagada para selecionar `app0`. O boot reportou `RUNNING_PARTITION: app0`, estado `VALID`, 16 MB flash, 8 MB PSRAM e `OTA: READY`. **O bootloader e a tabela de partições não foram reinstalados: permanecem os do pacote CORE.1**; compatibilidade observada nesta inicialização, sem declarar equivalência completa.

**Teste novo de recuperação de Wi-Fi:** após não encontrar a rede conhecida, a S3 ativou `BOTIZIN-S3-...` e apresentou o portal `http://192.168.4.1/`. O usuário selecionou a rede doméstica 2,4 GHz, salvou a senha localmente e verificou reconexão. Depois `http://botizin-s3.local/status` respondeu, com `WIFI: OK`, `MDNS: READY`, `HTTP_SERVER_STARTED: YES`, `HTTP_CONFIRMED_BY_REQUEST: YES`, `INTERNET_OTA: UP_TO_DATE 0.0.25`, `TELEMETRY: HTTP_200` e MQTT publicado. A janela de espera inicial do firmware inclui tentativa STA de até 30 s e recuperação AP por tempo de desconexão de ~35 s; **os tempos reais ponta a ponta ainda precisam ser medidos**, não deduzidos das constantes.

**Limites e cuidados:** `PEER_PAIRED: NO`; comunicação S3↔WROOM e troca/retorno físico entre versões OTA **não foram validadas nesta recuperação**. O backup integral de 16 MB tentou ler a flash, mas falhou com erro USB de transferência em ~25%; **não existe backup integral válido deste procedimento**. A NVS não foi apagada na restauração. `OTA_AUTOMATIC_ENABLED: YES`: o manifesto publicado segue na 0.0.25, mas **mudar o manifesto de produção pode provocar OTA automática**; proteger esse caminho antes de ensaios. Não modificar `manifest.json`, `app0`, slots nem WROOM para fins de auditoria.

**Direção aprovada:** manter o hardware na CORE 0.0.25 e auditar os subsistemas um a um (boot/partições; Wi-Fi/portal/mDNS; HTTP/painel/auth; eventos/diagnósticos; OTA GitHub/local e rollback; telemetria ThingsBoard; MQTT; RPC/pareamento; comunicação entre placas). Classificar **confirmado / parcialmente testado / não testado / candidato a substituir**, sempre medindo uso de RAM/flash e latência antes de remover. O painel já tem abas e recursos de controle: reaproveitar a implementação existente. Futuro BOTIZIN 2.0: S3 como central local, CORE estável preservado, A22 auxiliar e XiaoZhi integrado por etapas, sem tornar a comunicação local dependente de voz, internet ou serviços externos. Nenhuma regravação de firmware foi feita para registrar este checkpoint.


**A ideia em 30 segundos:** um ecossistema de cinco placas com resultados úteis, não mais uma coleção de testes. ESP32-S3 + A22 fazem a central; ESP32-WROOM é o **BOTIZIN de bancada**, rosto OLED, menu e lançador de módulos pelo GitHub; duas ESP32-C3 com telas serão um par de comunicação; uma ESP8266 com OLED será ferramenta portátil de IR/Wi-Fi. A35 é o cliente de conversa com ChatGPT. O carrinho 2WD e o mini robô ficam em fila, sem receber uma placa por impulso.

**[VISÃO MESTRE — decisões, prioridades, arquitetura, resultados e limites técnicos](docs/BOTIZIN-VISAO-MESTRE-2026-11-11.md)**

**[INVENTÁRIO — peças físicas, estado e chegada de placas](docs/inventario-hardware-botizin-lab-2026-11.md)**

**[AUDITORIA TÉCNICA DA WROOM 0.0.38 — tamanho, fontes exatas, SPIFFS e histórico 0–38](docs/AUDITORIA-WROOM-038-HISTORICO-E-OTIMIZACAO-2026-11-11.md)**

**Checkpoint validado em 13/11/2026:** [LAB S3 0.0.41 — voz, transcrição, MCP, áudio e plano dos olhos OLED WROOM](docs/BOTIZIN-CHECKPOINT-VOZ-MCP-OLED-2026-11-13.md). S3 LAB 0.0.41 testada pelo proprietário; WROOM 0.0.40 preservada; próximas verificações incluem perdas PCM e handoff A22.

## Plano aprovado — Setores Wi-Fi, Recursos e Limites, Segurança

**Decisão de 16/11/2026 (foco exclusivo na S3):** preservar a CORE 0.0.25 restaurada, já testada em boot, Wi-Fi, AP de recuperação, HTTP, /status, memória, OTA em modo de consulta, eventos, MQTT e telemetria. **Não tocar na WROOM** neste ciclo. Desenvolver alterações primeiro em branch/laboratório e medir custo antes de qualquer instalação. A baseline estável só muda mediante teste, comparação e autorização específica.

### Setor Wi-Fi: quatro blocos no painel

1. **Conexão atual (consulta):** rede/SSID (não senha), IP, STA/AP, mDNS, intensidade RSSI (futuro), gateway, DNS, tempo conectado, último erro e horário da leitura. Distinguir acesso local de internet.
2. **Trocar rede (ação):** procurar redes 2,4 GHz sob demanda; selecionar/digitar SSID, inserir senha e confirmar; proteção administrativa simples (não liberar alteração a qualquer visitante da LAN). Manter credenciais na NVS, nunca em GitHub/log/status. Preservar rede anterior até validar a nova ou ter retorno seguro ao AP.
3. **Testar rede (consulta):** testes iniciados pelo usuário, com resultado e latência separados por Wi-Fi, roteador/gateway, DNS, HTTP local e internet; limites curtos de timeout, sem varredura ou polling contínuo.
4. **Recuperação/histórico:** tempo de boot e de reconexão medido por estado, tentativas/falhas, motivo de troca de modo e AP de recuperação. Mostrar etapas claras e resultado imediatamente próximo do botão.

**UX padrão:** cards reutilizáveis por setor, com estado/data da última coleta; ações têm indicador "executando", confirmação/erro no próprio bloco, e nunca parecem inativas porque a resposta está no rodapé. Relatórios de leitura não exigem login; modificar Wi-Fi/OTA/reiniciar requer autenticação apropriada.

### Setor transversal: Recursos e Limites

**Objetivo:** saber o custo incremental de cada funcionalidade em **flash**, RAM interna, PSRAM e tempo de execução sem transformar o monitor em fonte de carga.

- **Flash por build:** guardar tamanho de firmware `.bin`, limites/slots, margem OTA e delta com a baseline; para atribuição por componente, usar `.elf`/mapa de link e builds de comparação. **Não inferir custo individual de RAM/flash de um único /status.**
- **RAM em execução, sob demanda:** heap interna livre, mínimo histórico, maior bloco contínuo, PSRAM livre, RAM de pico nos testes. Medir em repouso e durante cada operação; atribuição de custo por diferença controlada, considerando ruído e dependências compartilhadas.
- **Tempo/saúde:** duração de Wi-Fi, HTTP, TLS/OTA e maior intervalo de loop; eventos somente em transições, anel limitado de 64 eventos, sem log infinito/NVS em cada ciclo. Preferir leituras ao clicar ou amostragem limitada de laboratório; desativar automaticamente medições experimentais fora do teste.
- **Orçamento:** estabelecer avisos e limites de aceite depois de medir baseline/variação com carga real; nenhuma função nova entra sem indicar impacto de tamanho, pico de RAM, latência, dependências e como desativar/recuperar. Não reservar limiar rígido arbitrário sem medição.
- **Amostra de referência já observada na CORE 0.0.25 (uma leitura, não orçamento garantido):** firmware publicado 1.259.872 B em slot de 3.145.728 B; margem estimada de 1.885.856 B; heap interna livre 242.584 B, mínimo observado 185.740 B, maior bloco 151.540 B, PSRAM livre 8.374.052 B. É necessário medir em cenários de carga para definir limites.

### Setor transversal: Segurança

- Auditoria das rotas e permissões; informação de leitura acessível pela LAN confiável, mas mudança de rede, atualização OTA, troca de slot, reinicialização e configuração sensível exigem autenticação/autorização.
- O painel HTTP atual **não é HTTPS**; não fazer port-forward, não expor no túnel público, não imprimir/salvar credenciais no README. Verificar sessões de 10 min, limites de tentativas e política de recuperação antes de alterar UI.
- **Risco já identificado:** CORE 0.0.25 possui OTA automática; enquanto o manifesto `main/manifest.json` indicar 0.0.25 ela reporta `UP_TO_DATE`, mas **não publicar versão mais nova nesse manifesto** durante experimentos. Catálogo de versões/teste deve ficar isolado até ter política explícita de atualização manual e rollback validado.
- Fazer mudanças por setor com evidências e indicadores: **mantido / modificado / não testado / removido com justificativa**. Não retirar serviços (MQTT, ThingsBoard, RPC) antes de mapear consumidores.

**Primeira entrega de desenvolvimento planejada:** especificar e implementar *apenas a leitura* dos cards de rede e recursos (sem mudar o fluxo Wi-Fi existente) em branch experimental; comparar compilação e memória com baseline, simular falhas, preservar OTA0. Só então considerar botão de troca de rede e testes ativos. Nenhum trabalho descrito nesta seção corresponde a firmware já gravado.


## Onde estamos realmente
- **ESP32-S3:** CORE 0.0.25, OTA/Wi-Fi, comunicação e diagnósticos confirmados. A voz XiaoZhi foi testada pelo proprietário no notebook com LAB; o CORE de produção atual não é a LAB de voz.
- **ESP32-WROOM:** 0.0.38, OLED com rosto BOTIZIN, menu/catálogo e LED GPIO26 controlado pelo ChatGPT; partições de app apertadas. Ainda não existe launcher enxuto OTA0↔OTA1 consolidado.
- **A22:** Termux, Debian PRoot, MCP privado, túnel e supervisor 2.1; diagnóstico via ChatGPT funcionando. Recuperação total após quedas de energia continua sob observação.
- **Duas C3 + ESP8266 OLED:** relatadas como encomendadas, ainda sem confirmação física ou testes.
- **Carrinho:** chassi 2WD montado, motor/driver/alimentação/PS4 ainda sem prova final.

## Próxima entrega, sem expandir infraestrutura
**Foco atual: S3 CORE 0.0.25** — auditoria por setores, cards de Wi-Fi, Recursos e Limites, Segurança, planejamento da central local e gerenciador OTA; implementação apenas em ambiente isolado e com métricas. Preservar a 0.0.25 instalada.

**WROOM em espera:** o plano anterior de auditar a 0.0.38, criar um lançador de módulos com retorno seguro e testar BOTIZIN → módulo LED → BOTIZIN continua registrado, mas não será iniciado nesta etapa; sem gravação na WROOM.

## Restrições que impedem confusão
- O modelo ChatGPT não roda dentro das ESP32; a conversa inteligente usa serviços conectados (A22/IA) e poderá acionar o hardware.
- Ao inicializar um firmware de módulo no segundo slot, o programa do rosto no outro slot **não executa simultaneamente**. Voz independente no A22 pode continuar, mas OLED/rosto na WROOM só volta quando ela reiniciar no BOTIZIN.
- A WROOM tem Bluetooth Clássico; C3, S3 e ESP8266 não têm. Não prometer PS4 direto na C3.
- Compilação, pacote arquivado, teste físico e uso real são estados diferentes. Nunca regravar slot estável nem alterar o túnel por interpretar um plano como autorização.

### Checkpoint: backup crítico S3 de 64 KB validado (16/11/2026)

- A leitura USB de **0x00000 a 0x0FFFF (65.536 bytes)** concluiu com `BACKUP_CRITICO_64KB_OK` na S3 reconhecida como **ESP32-S3 N16R8**, 16 MB flash, na **COM5** (identificação física por MAC já conferida). Python 3.11.9; esptool 5.1.0. Nenhuma gravação realizada na S3.
- Backup mantido **somente no notebook**, em `Downloads\\BOTIZIN-RECUPERACAO-S3\\BACKUP-PRIVADO`. Contém NVS e pode conter senhas/tokens; **não publicar nem enviar para o GitHub**. SHA-256 foi obtido localmente. Não tratar os 64 KB como imagem integral de recuperação: aplicativo app0, app1 e armazenamento restante não estão nesse arquivo.
- A referência do aplicativo 0.0.25 continua publicada em `releases/0.0.25/firmware.bin`, recuperada e testada fisicamente com o bootloader/tabela de partições remanescentes do CORE.1. **S3 em produção permanece intocada; WROOM fora do escopo.**
- O próximo passo é **blindar a branch LAB** contra OTA automática, gravação por HTTP/peer e publicação no manifesto de produção, revisar método de ativação e retorno de app1 e só então considerar ensaio físico.
