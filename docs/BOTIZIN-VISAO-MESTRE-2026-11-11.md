# BOTIZIN — VISÃO MESTRE E DECISÕES DE PRODUTO
**Data:** 11/11/2026. **Fonte:** decisão do proprietário nesta conversa, complementada por inventário físico, repositórios e consultas reais às placas. **Estado:** decisões de projeto; nenhuma nova gravação, OTA ou alteração operacional aprovada por este texto.

> **Propósito:** transformar os testes acumulados em equipamentos que tenham utilização real, sejam concluídos, e possam ser usados sem notebook como dependência permanente. A bancada cria e valida módulos; os aparelhos finais os utilizam.

## Leitura em três níveis (para evitar mapa mental infinito)
1. **VISÃO DE 30 SEGUNDOS:** cinco placas, cinco papéis; S3 + A22 central; WROOM BOTIZIN de bancada; C3-A e C3-B par de comunicação; ESP8266 ferramenta portátil de IR/Wi-Fi; A35 cliente pessoal; 2WD é projeto mecânico ainda sem controlador decidido.
2. **VISÃO OPERACIONAL:** tabela de papéis, situações comprovadas e próximas entregas abaixo.
3. **VISÃO TÉCNICA:** inventário em `docs/inventario-hardware-botizin-lab-2026-11.md`; arquivos da WROOM, catálogo, firmware e releases; S3 e laboratório de voz; supervisor A22. Nunca misturar teste comprovado com ideia.

## Premissas consolidadas
- **Central:** S3 atual (16 MB flash, 8 MB PSRAM), A22 (Termux/PRoot, MCP/túnel/supervisor/serviços) e ChatGPT no A35 ou outros clientes. O modelo ChatGPT NÃO roda fisicamente na ESP32. A S3 coordena comandos e placas, enquanto IA/voz mais pesada depende de serviços remotos e integração.
- **BOTIZIN de bancada:** WROOM-32 atual é presença física, rosto OLED, menu, botões e lançador dos módulos. Objetivo: ligar e ver o BOTIZIN, escolher um módulo do catálogo GitHub, instalar na partição de experimento, usá-lo e voltar ao BOTIZIN-base.
- **ChatGPT na bancada:** desejo explícito de conversação inteligente. Implementação proposta: A22 capta/reproduz áudio e oferece serviço de IA, S3 coordena se necessário, WROOM mostra estado e controla hardware. Não confundir voz XiaoZhi testada no notebook com GPT nativo na WROOM ou voz no A22 já concluída. O Plus do ChatGPT não concede automaticamente uso da API.
- **Firmware alternativo e rosto não coexistem:** se o segundo slot OTA executar aplicativo independente, o programa-base que desenha o rosto na WROOM NÃO roda ao mesmo tempo. Durante um módulo, a conversa independente pelo A22 poderá continuar se o serviço de voz estiver ativo, mas rosto/menu da WROOM só retornam após reiniciar no firmware BOTIZIN. Alternativa futura: módulos pequenos internos no mesmo firmware ou controlador auxiliar para preservar rosto simultâneo.
- **Slots WROOM:** ESP32-WROOM 4 MB, slots app0 e app1 de cerca de 1.25 MiB cada. Um slot será a base BOTIZIN, outro temporário de módulo. Exige planejamento de boot, marcação de slot, protocolo de volta autenticado, verificação de hash/assinatura, compatibilidade de partições, proteção contra OTA automático e recuperação física se falhar. O 'voltar' deve funcionar no módulo, nunca ser presumido. Não prometer app0 imutável enquanto ainda não houver backup/restauração comprovada e mecanismo de recuperação.
- **Não recomeçar tudo, mas poder limpar:** manter as conquistas essenciais de rede, OTA, rollback, segurança e supervisor; auditoria e reconstrução enxuta da WROOM são permitidas. Nunca remover, reflashear ou redefinir um firmware estável antes de inspecionar a fonte da versão exata, testar build, garantir recuperação e obter autorização.
- **Modularidade física:** mini protoboards, cabos e sensores são peças conectáveis e removíveis. Firmwares de ambiente são outro tipo de módulo; não confundir 'conectar fisicamente' com 'trocar OTA'.

## Distribuição das cinco placas
| Equipamento | Situação | Missão principal | Definição de pronto |
|---|---|---|---|
| ESP32-S3 N16R8 atual | Presente e consultada: CORE 0.0.25, OTA/Wi-Fi | Central/ponte de comunicação e integração, não tela principal da bancada | Comando de A35/ChatGPT chega via A22 e alcança WROOM/outros dispositivos, com feedback e recuperação |
| ESP32-WROOM-32 atual | Presente, consultada: 0.0.38, OLED/rosto, menu, catálogo, LED GPIO26 validado | BOTIZIN de bancada/base e lançador de ambientes OTA | Inicia rosto, abre catálogo, instala módulo de teste autorizado e volta à base, sem notebook |
| ESP32-C3 com OLED nº A | Compra relatada, **ainda não recebida nem identificada fisicamente** | Primeiro dispositivo do PAR de comunicação, com tela | Troca dados reais com C3-B e mostra recebimento/estado |
| ESP32-C3 com OLED nº B | Compra relatada, **ainda não recebida nem identificada fisicamente** | Segundo dispositivo do PAR de comunicação, com tela | Recebe e responde sem depender da internet, com distância aferida |
| ESP8266 NodeMCU com OLED (terceira imagem do anúncio) | Compra relatada, **ainda não recebida nem identificada fisicamente** | Ferramenta portátil simples de IR e diagnóstico do próprio Wi-Fi | Opera por power bank; captura/envia IR permitido, mostra redes/qualidade do sinal, faz testes locais limitados |

**Contagem:** 2 placas já presentes + 3 em chegada = **5 placas**; a terceira não é outra S3. Identificar placas físicas, flash, pinagem, microcontrolador e controlador/display após recebimento. Imagens de anúncio não comprovam entrega ou especificação final. Se o pedido real divergir, corrigir inventário antes de decidir firmware.

### ESP8266: não é S3 e não é lixo
O ESP8266 é geração anterior (chip da família lançada em meados de 2014), sem Bluetooth, 2.4 GHz Wi-Fi, processador Tensilica tipicamente 80/160 MHz, RAM e GPIO mais limitados que ESP32 atuais. Serve bem como terminal pequeno de IR, leitura de redes e sinal, HTTP/MQTT simples, botões e OLED. Não serve para executar ChatGPT localmente, Bluetooth PS4 ou diagnósticos profundos de segurança; menos indicado para criptografia pesada, áudio e grande firmware. Pinagem/tela dessa placa específica são pendentes. Para ESP32-C3, Wi-Fi 2.4 GHz + BLE 5 (sem Bluetooth Classic), CPU RISC-V até 160 MHz; par C3 pode explorar ESP-NOW, BLE, Wi-Fi local e testes de alcance. Não prometer que o módulo específico ofereça todos os GPIO de datasheet.

## Projetos físicos e resultados
### P0 — Infraestrutura essencial: PRESERVAR, não expandir sem necessidade
S3 CORE 0.0.25; A22 com MCP, túnel, supervisor 2.1 e diagnósticos; WROOM 0.0.38 funcional e LED remoto; retorno de status; atualizações OTA com proteções. Validado: comandos MCP até LED físico e status das placas. Recuperação do túnel após todas as classes de falha ainda precisa validação; não forçar quedas sem motivo.
### P1 — BOTIZIN BASE DA BANCADA: PROJETO PRINCIPAL
1. Recuperar **fontes exatas das releases** Menu 0.0.29, WROOM 0.0.38 e módulos 0.0.30–0.0.34 (a árvore main de firmware não representa necessariamente a versão em execução).
2. Medir quais bibliotecas/funcionalidades realmente ocupam flash e RAM, tamanho do .bin, partições e arquivos. WROOM 0.0.38 = 1,288,144 bytes para slot de 1,310,720; apenas ~22,576 bytes livres. Não remover recursos "às cegas".
3. Criar base mínima BOTIZIN com OLED face, botões, Wi-Fi, autenticação/OTA, consulta de saúde, catálogo GitHub, transição/retorno seguro. O resto fica em ambientes/módulos.
4. Validar compilação e atualizações em bancada com backup físico conhecido, recuperação USB e possibilidade de voltar ao firmware íntegro. Só gravar após aprovação.
5. **Primeiro ciclo de módulo de verdade:** LED como piloto; entrar via catálogo, operar e voltar ao BOTIZIN sem notebook. Depois IR e servo.
6. Integração de conversa inteligente da bancada via A22/S3, sem supor que GPT estará no ESP32. Mostrar estado no OLED; iniciar/terminar voz no A22; explicar limites durante firmware alternativo.
**Pronto:** liga → aparece rosto → seleciona módulo → executa tarefa → volta; comunicação e recuperação preservadas.

### P2 — DUAS C3: PAR DE COMUNICAÇÃO COM TELAS
- Destino: dois dispositivos pequenos iguais para enviar mensagens, sinal de presença, alertas, troca de estados e experimentar alcance dentro de casa/quartos.
- Primeiro teste: mensagem "OLÁ" da A aparece na tela da B, B responde "RECEBIDO", sem internet; latência, RSSI/qualidade, alcance e perdas registrados. Começar com ESP-NOW (sem roteador) se suportado pela variante e biblioteca, depois comparar BLE e Wi-Fi via roteador.
- Produto útil candidato: campainha/aviso silencioso entre quartos ou nós de alerta/estado; escolher UMA aplicação após testes. Não são dispositivos ChatGPT locais.
- Limites: conexão à rede local e uso ESP-NOW/BLE podem exigir alternância de modo/canal; medição real de alcance e alimentação por power bank serão necessárias.

### P3 — ESP8266: FERRAMENTA DE BOLSO DE IR E WI-FI
- OLED e botões, power bank. Funções iniciais: listar SSIDs próprios e RSSI; aprender e retransmitir códigos IR de controle autorizado; verificar conexão HTTP/portas selecionadas **apenas de dispositivos pertencentes ao usuário** na LAN, com timeout/rate limit. Nada de invasão, credenciais coletadas ou scans ofensivos.
- Produto pronto quando três tarefas úteis funcionarem longe da bancada e a alimentação for estável.
- IR receptor e LED transmissor necessários com pinagem, corrente/resistores/transistor quando cabível, sem supor que todos estão testados.

### P4 — Carrinho 2WD: PROJETO EM FILA
- Já há chassi acrílico, 2 motores TT, roda boba, L298N, suporte 4 AA, encoders e controle PS4 com falha de carregamento.
- **Controlador ainda não reservado.** ESP32-C3 e S3 não possuem Bluetooth Classic para pareamento típico direto com DualShock 4; WROOM tradicional tem, mas fica na bancada. Alternativas a estudar: ponte pelo A22/controle USB/Classic compatível com comando por Wi-Fi para C3, ou revisão futura de recursos/placas. Não retirar WROOM da bancada sem decisão nova.
- Primeiro critério físico: acionamento seguro de motores + parada ao perder ligação; depois PS4 e autonomia. L298N possui perdas e precisa medição da fonte; não ligar motor diretamente na GPIO.

### P5 — Mini robô artesanal: IDEIA EM FILA, SEM PLACA RESERVADA
O par de C3 está reservado ao experimento de comunicação; uma delas pode evoluir para mini robô depois, se preservar o objetivo de troca entre duas unidades, ou chegar outro controlador. Estrutura metálica reaproveitada, motores pequenos, bateria/porta-bateria leve e forma precisam ser dimensionados. Não assumir que a compra de C3 inclui bateria/driver. Não tirar uma C3 do par antes de completá-lo.

## Qualidade e estados de implantação
Use sempre: **IDEIA → PREPARADO → COMPILADO → TESTADO NA BANCADA → TESTADO FISICAMENTE → PRONTO PARA USO**. Release compilada não prova ferramenta terminada. No projeto, manter uma entrega principal e uma pequena validação de laboratório; não abrir cinco frentes simultâneas.

## Ordem das próximas sessões
1. **Sessão 1 — Auditoria WROOM:** identificar fonte real da versão .38; comparar com Menu .29 e catálogo; mapa de flash/funcionalidades; desenho mínimo OTA0/OTA1; não gravar.
2. **Sessão 2 — Base BOTIZIN enxuta em branch:** build CI, testes de retorno, OTA e segurança; provar caminho de recuperação; preservar todas as releases antigas.
3. **Sessão 3 — Implantar somente após aceite:** testar ciclo BOTIZIN → módulo LED → retorno. Esta é a primeira entrega de produto.
4. **Depois:** voz inteligente A22↔WROOM (reaproveitando LAB .39 comprovada no notebook), módulo IR/servos; quando chegarem as C3, emparelhar e testar sem internet; ESP8266 vira ferramenta de bolso; carrinho e mini robô na fila.

## Lista de projetos e demandas de inventário que NÃO desaparecem
- Robô de bancada com OLED e botões; microfone/fone e áudio via A22/XiaoZhi; voz de notebook validada pelo usuário, operação A22 ainda pendente.
- Módulos: servo(s), LED, IR RX/TX, buzzer, display, botões, sensores, câmera, RF, relé (somente se módulo relé adequado estiver identificado), encoders e expansão GPIO.
- 2WD com PS4, ferramenta portátil, par C3, mini robô de chapinhas/metal; eventualmente comunicação offline da Serra do Japi (LoRa/mesh como projeto separado futuro, não presumir disponível com Wi-Fi/BLE).
- Fontes e energia: power bank sem carga medida, ATX sem saídas aferidas, conjunto 4 AA, bateria de ferramenta sem adequação validada. Priorizar multímetro e não arriscar ligação direta em GPIO.
- Peças avulsas, câmera IP, tela PSP, outras telas e reaproveitamentos permanecem no inventário/backlog, **não viram projetos ativos sem uso final**.

## Regras de preservação
- Não alterar S3 estável, A22 supervisor/MCP ou WROOM publicada sem prévia inspeção, backup, build e aceite.
- **Não atualizar automaticamente o slot que abriga o BOTIZIN-base com um módulo.** Controlar partição alvo e retorno seguro explicitamente.
- Um módulo não está pronto apenas por demonstrar um servo movendo: exige função, segurança, operação e desligamento/retorno.
- Projeto pode ser simplificado ou refeito em um firmware enxuto; não repetir retrabalho das conexões, OTA e supervisor que já foram conquistados.
- Documentação não implica ordem de instalar, nem autorização para ativar firmware.
