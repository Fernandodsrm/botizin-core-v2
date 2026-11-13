# BOTIZIN — protótipo de sincronização de olhos S3 → A22 → WROOM
**Data:** 13/11/2026. **Estado:** código experimental em branch. NÃO instalar nem alterar as placas/serviços por este documento.

## Ponto de partida confirmado
- **S3 LAB 0.0.41:** XiaoZhi original, estados 3=parado,4=conectando,5=ouvindo,6=falando,9=ativação. Firmware em S3 OTA1. CORE 0.0.25 preservado em OTA0.
- **WROOM 0.0.40:** base física no app0, OLED 128×64, menu, catálogo, API LED e OTA. Manter intocada até demonstrar imagem, recuperação e preservação do módulo.
- **A22:** ponte de rede e supervisor. IP local observado 192.168.0.14; WROOM 192.168.0.23; S3 192.168.0.24. Endereços são de laboratório, não descoberta automática.
- **A35:** navegação HTTPS local via túnel A22, microfone + saída de voz. O áudio NÃO é encaminhado pela WROOM.

## Implementação no branch
- `wroom/firmware/voice_sync_state.h`: máquina mínima em RAM, aceita apenas 3/4/5/6, último estado/idade/contagem; TTL 4 s e fallback para rosto procedural sem reiniciar.
- `wroom/firmware/face_engine.h`: mood/olhos especiais associados a estados. Parado=neutro, conectando=expectativa, ouvindo=atento, falando=animado. Sem audio I2S/UART/Bluetooth.
- `wroom/firmware/oled_status.h`: `faceTask` consulta apenas memória atômica. Sem chamadas de rede na task OLED.
- `wroom/firmware/firmware.ino`: `POST /face/voice?state=5` (3/4/5/6), só aceita conexão da origem A22 **192.168.0.14**, não acessa NVS nem OTA, responde `VOICE_SYNC_OK`. Telemetria /status acrescida de `VOICE_SYNC_MODE`, `VOICE_SYNC_AGE_MS`, `VOICE_SYNC_EVENTS`. IP source-only é restrição local de laboratório, **não autenticação forte**; não publicar porta na internet.
- `tools/a22/voice_face_relay.py`: lê `GET S3 /status` em modo leve (~1,25 requisições/s); identifica origem `XIAOZHI_REAL`; se estado mudou ou vence heartbeat ~1,8s, envia um pequeno POST HTTP à WROOM. Em falha de comunicação, não reenvia estado velho; TTL expira. Oferece `--once`, `--watch` e `--dry-run`.
- `tools/a22/wroom_module_agent.py`: compatibilidade para aceitar identidade da base .40 ou .41, porém **essa alteração está apenas na fonte**; não implica atualização do agente em execução no A22.
- `wroom/tests/voice_sync_state_test.cpp` e `tools/a22/test_voice_face_relay.py`: validação de estados, TTL, rollover, identidade, rejeição de valores ruins.

## Etapas de validação
1. CI compila WROOM 0.0.41 com ESP32 Arduino 3.3.12, garante binário <= 1.310.720 bytes e tabela de partições **sem mudanças**; testa OLED e conversor A22. Reprovar se qualquer regressão.
2. Antes de instalar, preservar backup atual da 0.0.40 e confirmar método de inicialização/retorno: **WROOM base está em app0 e módulo em app1**. Não substituir app0 só para fazer a demonstração.
3. Validar A22 `python3 voice_face_relay.py --once --dry-run`: consulta S3 real sem acionar WROOM. Conferir retorno, IP e identificação.
4. Só instalar a WROOM se houver decisão explícita, caminho de rollback testado e transição do agente de módulos A22 validada.
5. Teste físico WROOM: conversação 'ouvindo → falando → ouvindo', captação e reprodução continuam no A35; monitorar FACE_MAX_GAP_MS, DIAG_FREE/MIN e VOICE_SYNC_EVENTS. Tirar a ponte de operação e confirmar fallback em <=4s.
6. Testar sem regressões menu, OLED, LED GPIO26 com auto-off, módulo OTA manual, boot, WROOM status; monitorar S3 opus/PCM em paralelo.
7. Depois refinar sincronismo falando pelo áudio *reproduzido no A35* (não somente estado S3) e melhorar animações.

## Limites atuais
- Implementação apenas de **estado**, sem transcrição da fala no OLED; a transcrição fica na página A35.
- Sem API genérica para IA comandar GPIO da WROOM. MCP físico exige ferramenta separada com autorização.
- IP fixo precisa ser atualizado se os aparelhos mudarem de rede; futuramente descobrir via mapa do A22 com validação.
- O WebServer WROOM e o backend S3 são sensíveis a polling excessivo; limitar frequência, não fazer chamadas de HTTP a partir da task OLED.
- Uma expressão 'speaking' baseada só em S3 pode anteceder ou atrasar o áudio do A35; é uma aproximação operacional, não lip-sync de amostra.
- O teste .41 S3 de áudio mostrou +0 amostras PCM descartadas, +0 erros WS e 344/345 pacotes Opus aceitos em 58,7s; ainda não encerra estudo de Opus, brownouts ou 2670 falhas de leitura de mic.

## Segurança/recuperação
- Os arquivos de firmware e a política OTA da WROOM .40 não são alterados automaticamente nesta branch.
- CI nunca deve publicar release para `main` ou fazer OTA/flash.
- WROOM app0 .40 e S3 OTA0 CORE .25 permanecem preservados durante o estudo.
