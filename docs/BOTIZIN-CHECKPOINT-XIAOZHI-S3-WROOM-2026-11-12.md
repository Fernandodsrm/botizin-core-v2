# BOTIZIN 1.0 — checkpoint da arquitetura XiaoZhi, S3, WROOM e A22
**Data:** 2026-11-12
**Estado:** decisao em estudo; nenhuma gravacao, atualizacao, modificacao de MCP ou reinicio de servicos autorizado por este registro.

## Motivo do checkpoint
O proprietario interrompeu a expansao de modulos para esclarecer o objetivo original. O BOTIZIN pretendido deve reaproveitar o **XiaoZhi real**, com maquina de estados de conversa (espera, conectando, ouvindo, falando) e expressoes vinculadas a esses eventos, em vez de somente um rosto autonomo de demonstracao.

**Nao continuar desenvolvendo modulos ou face generica antes de validar esta distribuicao.**

## Evidencia de estado atual, consultada ao vivo em 2026-11-12
- ESP32-S3 N16R8: CORE 0.0.25, executando app0, Wi-Fi/HTTP/OTA ativos, *nao executa a LAB XiaoZhi neste instante*.
- ESP32-WROOM-32 4 MB, sem PSRAM: BOTIZIN 0.0.40 no app0, OLED SSD1306 128x64, face procedural e menu; firmware alternativo 0.0.38 em app1. OTA/Wi-Fi ativos.
- Samsung A22: supervisor e MCP ativos. O A35 chegou ao A22 por Tailscale+SSH/ConnectBot via dados moveis e o A22 alcancou a WROOM pela rede local; nenhum desses testes prova caminho de audio XiaoZhi final.
- A versao 0.0.40 foi instalada remotamente pelo fluxo OTA 0.0.38(app0) -> 0.0.38(app1) -> 0.0.40(app0), com checks de hash, boot e status. **Nao transferir S3 nem modificar seus slots como consequencia automatica.**

## XiaoZhi ja estudado / diferenciar evidencias
- Projeto original estudado no commit 028aba25e78ab58f5db681a5deffb9dd11be6fa0, no repositorio privado botizin-core branch research/xiaozhi-028aba2, documento research/xiaozhi-028aba2/ESTUDO-01.md.
- XiaoZhi define estados em main/device_state.h e os aplica em main/device_state_machine.cc. Interface OLED implementa OledDisplay::SetEmotion, de acordo com ESTUDO-01.
- S3 e candidata indicada para XiaoZhi: 16 MB flash e 8 MB PSRAM. WROOM atual possui 4 MB sem PSRAM e slots app0/app1 de 1.310.720 bytes; a execucao do XiaoZhi completo na WROOM NAO foi comprovada.
- Foram criadas LABs de voz S3, incluindo release 0.0.39 (manifest de laboratorio registrado com physical_test_pending), e o proprietario relatou teste de voz pelo notebook/navegador. Isso NAO significa que a S3 CORE 0.0.25 atual execute o XiaoZhi de voz.
- XiaoZhi nao roda modelo ChatGPT inteiro localmente; conversa depende de integracao com servidor de IA e transporte de audio.

## Arquitetura candidata (nao implementada)
1. **S3:** rodar XiaoZhi adaptado, cliente/estados da conversa e canal de eventos para outras placas.
2. **WROOM:** renderizar expressoes no OLED de acordo com os eventos verdadeiros enviados pela S3 (ou retransmitidos pelo A22); manter botoes e funcoes fisicas. O rosto atual da 0.0.40 e procedural e independente, sem comando de expressao vindo da S3.
3. **A22:** MCP, tunel, gerenciamento remoto, opcional ponte de audio no celular/navegador, a validar.
4. **A35:** controle por ChatGPT e acesso remoto via Tailscale/SSH, ja testados para comandos/diagnostico, NAO como audio bidirecional final.
5. **Audio real:** decidir origem do microfone e destino da reproducao. Opcao inicial, testar navegador/A22; plano anterior visava fone Bluetooth HFP por WROOM + UART para S3, MAS essa ponte HFP ainda nao foi validada. A S3 nao possui no conjunto atual microfone/amplificador ja testados diretamente para esse fluxo.

## Sincronizacao da face: contrato a pesquisar
Exemplo de eventos: idle, connecting, listening, speaking, disconnected/error.
- S3 emite somente estados reais observados de XiaoZhi, com sequencia/timestamp e timeout de desconexao.
- WROOM mapeia estado a expressao, sem depender de codificar audio no OLED.
- Transicoes devem ser testadas fisicamente no OLED e nao apenas por resposta de HTTP.
- 'Pensando' NAO e um enum formal da maquina de estados upstream do snapshot 028aba2; usar evento real de espera por resposta, sem apresentar como estado oficial.
- O endpoint de controle de expressoes entre S3 e WROOM ainda NAO existe na WROOM 0.0.40. Qualquer versao futura requer testes e procedimento seguro de OTA.

## Conflito com OTA0/OTA1 da WROOM
A arquitetura atual alterna firmware BOTIZIN-base (app0) e modulo independente (app1). Quando app1 roda, o rosto desenhado pelo app0 NAO roda simultaneamente.
- Para expressoes continuas, preferir funcoes leves dentro da base ou controle por outro microcontrolador; tratar app1 como laboratorios/ambientes de firmware inteiro.
- Teste do modulo LED 0.0.32 + BACK -> base 0.0.40 permanece pendente e nao e necessario para decidir fluxo XiaoZhi.
- Nao presumir que menu, modulo PS4, Bluetooth HFP e XiaoZhi funcionam ao mesmo tempo na unica WROOM sem medir memoria/stack/recursos.

## Ordem sugerida ao retomar
1. Confirmar com o proprietario a arquitetura logica: XiaoZhi na S3 + expressoes na WROOM.
2. Auditar o caminho real da LAB de voz S3 0.0.39 e o snapshot upstream, separando codigo, compilado, teste com navegador e teste fisico.
3. Fazer prototipo somente de eventos de estado (simulador S3/A22 -> receptor WROOM em laboratorio) e medir custo, sem gravar as placas.
4. Definir claramente o caminho de microfone e alto-falante/fone (HFP WROOM ou A22/navegador) para uma conversa real, antes de ampliar face/modulos.
5. Testar firmware e mecanismo de retorno sem perder CORE 0.0.25 S3 nem BOTIZIN 0.0.40 WROOM, com autorizacao separada.
6. So apos aprovacao e teste, implementar a integracao de voz e expressao.

**Ponto-chave:** nao confundir (a) XiaoZhi original com estados de IA, (b) apresentacao web de laboratorio e (c) face procedural WROOM .40. Sao sistemas distintos que podem ser integrados, mas ainda nao estao conectados.
