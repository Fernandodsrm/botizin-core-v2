# BOTIZIN — checkpoint de VOZ + MCP + transcrição + plano dos olhos OLED
**Data:** 13/11/2026. **Natureza:** fatos medidos pelo proprietário, leitura de fontes e diagnósticos somente-leitura. **Nenhuma placa/serviço alterado por este registro.**

## O que foi comprovado fisicamente hoje
- **S3 ESP32-S3 N16R8:** LAB XiaoZhi **0.0.41** instalada pela **COM4**, gravada e lida por esptool a partir de `0x310000` (OTA1), SHA256 do .bin `6b2629a565ffdaa0604e985fd12a69e7902f92b401d86b8045dde75d5d65c741`, 2.047.232 bytes; `/lab/telemetry` retornou `lab=0.0.41` no hardware. CORE 0.0.25 (OTA0 `0x10000`) não foi sobrescrito.
- **Voz real:** A35 (navegador) conectado à S3, microfone PCM e saída de áudio funcionando com turnos automáticos pelo XiaoZhi; a interface mostrou estado `OUVINDO` / `FALANDO`. A35 acessa o painel na origem local `127.0.0.1:8088/botizin` pelo túnel configurado com A22.
- **Transcrição real:** página exibe `VOCÊ DISSE` (evento STT) e `BOTIZIN DISSE` (evento TTS); comprovação nas capturas. A interface atual só conserva as **últimas oito entradas** em memória do navegador; não mantém histórico persistente.
- **MCP voz→ação interna:** `self.botizin.test_action` foi solicitado por voz e `mcp_probe_count` aumentou **0→1** em `/lab/telemetry`. Isso comprova execução real no firmware S3, **NÃO** acionamento físico WROOM nem execução de comandos genéricos.
- **Comando por voz de encerrar:** XiaoZhi passou para `PARADO` após solicitação deliberada do usuário; **não classificar esse caso como travamento**. Depois do encerramento, wake-word/retomada sem clique segue não validado.
- **Handoff GPT/A22:** existe a ferramenta `self.chatgpt.open_voice` e a interface mostrou `Microfone liberado. Abrindo a voz do ChatGPT no A22...`, mas proprietário testou e relatou que o aplicativo NÃO abriu. Isso **não comprova** a execução final no Android; o firmware S3 ainda usa destino HTTP fixo `192.168.0.14:8080/gpt-live` em `phone_bridge.h` na fonte .41. Investigar `/lab/phone`, IP atual do A22, recepção HTTP e macro correspondente antes de afirmar sucesso. Não alterar MacroDroid sem teste de leitura e autorização.

## Verificações lidas ao vivo de integrações
- **WROOM 0.0.40** online, OLED `READY_128x64_SDA21_SCL22`, `OLED_PAGE: BOTIZIN`, `FACE_FRAMES: 167775`, `FACE_MAX_GAP_MS: 81`, `DIAG_FREE: 160016`, `OTA: READY`, app0/ota0, IP `192.168.0.23`. Face da versão .40 atualmente procedural/autônoma; **não espelha estados da S3**.
- **A22:** MCP porta 8765, túnel `/readyz` HTTP 200, SSH 8022 e MacroDroid 8080 consultados. Diagnóstico de identidade MacroDroid validado por leitura; processo VPN não verificado; supervisor acusa `s3_offline` enquanto o usuário confirma S3 via navegador e endpoint LAN. Isto pode refletir rota/compatibilidade do monitor legado — **não declarar S3 realmente desligada sem diagnóstico**.
- **S3 /status fotografado:** `state:5`, `web_audio_connected:true`, `mic_pcm_frames:6725`, `speaker_pcm_frames:7120`, `speaker_pcm_queued_samples:3643008`, `speaker_pcm_dropped_samples:809472`. Um diagnóstico da .41 mostrou `ws_open_count:5`, `ws_rx_errors:0`. Contadores de duração da sessão; comparar deltas entre leituras, e não apenas total acumulado.
- **AudioService/Opus:** na .39 houve falha de decodificação Opus seguida de assert TLSF; na .38 ocorreram múltiplas falhas Opus e brownouts, estabilização após trocar USB/porta. A .41 **não contém correção comprovada** para causas de Opus/heap/brownout. As amostras `speaker_pcm_dropped_samples` significam que parte do PCM decodificado foi rejeitada pelo FIFO de saída da S3 (capacidade 32.768 samples). **Não equivale automaticamente a perda de pacotes de rede.** Falta diagnóstico com deltas e teste com alimentação estável.
- **OTA:** `lab_confirmed:false` na API refere-se ao botão de confirmação explícita. No código `lab_return_025.cc`, `botizin_start_trial_timer()` marca o app VALID em startup se estava PENDING_VERIFY. Não afirmar rollback automático em prazo fixo apenas por esse flag; verificar estado OTA real se necessário.

## Como funcionam os olhos da página 0.0.41
- Estado `listening`: olhos verdes e piscam quando WS aberto, áudio ativo, captura enviando e estado XiaoZhi **5**.
- Estado `speaking`: animação de fala quando navegador tem `playback.size>0` (reprodução de áudio agendada); não é somente o estado **6**.
- Estado `connecting`: olhos amarelos enquanto conectando (ou estado XiaoZhi **4**). `idle=3` e desconexão tendem ao estado neutro.
- O OLED da WROOM deve acompanhar **evento real de voz**, não um rosto que adivinha o que a IA faz.

## Arquitetura experimental proposta, ainda NÃO implementada
1. **S3** continua XiaoZhi/Opus/VAD, gera estados reais e transcrição STT/TTS; NÃO transfere voz/IA pesada para WROOM.
2. **A22** agrega/supervisiona estados com cache pequeno, seq/timestamp/TTL, sem gravar áudio e sem guardar textos pessoais por padrão. Não duplicar polling excessivo da S3.
3. **WROOM OLED** passa a receber eventos de estado para mapear `idle/connecting/listening/speaking/disconnected` em expressões leves; nenhum pacote PCM, sem mudança do áudio no A35. Para sincronia fina entre lábios/olhos e áudio ouvido, avaliar evento de reprodução real do navegador; somente estado S3 pode antecipar/atrasar a animação por jitter.
4. **A35** continua sendo captura/reprodução e painel com transcrição; aumentar limite **8→20 ou 30 entradas** apenas na interface, com limite fixo/limpeza e sem persistência por padrão.
5. **Controle físico de saída**: após teste mínimo MCP, fazer ferramenta específica `self.wroom.led_on` com parâmetros mínimos e confirmação de estado real, segurança, limite de tempo e autenticidade. Não reutilizar ferramenta de ChatGPT automaticamente dentro da IA de voz; criar ponte explícita.

## Etapas curtas / critérios de aceite
- **P0 — áudio:** monitorar deltas por intervalos de 60–120 s em `/lab/telemetry`: `speaker_pcm_dropped_samples`, `speaker_pcm_queued_samples`, `speaker_fifo_now`, `speaker_popped_samples`, `ws_rx_errors`, `ws_tx_errors`, `mic_browser_rejected`, `mic_codec_reads_failed`, `backend_opus_rx_frames`, `backend_opus_rx_accepted`; usar diagnóstico do navegador para `speaker_frames_dropped` e `mic_frames_skipped`. Testar em alimentação estável. Se drops crescerem, corrigir FIFO/pacing/concorrência e testar Opus, sem desativar brownout.
- **P1 — sinal para WROOM:** especificar contrato curto `state,source,seq,age_ms`, TTL/fallback quando desconectado; simular estados em bancada, medir frame-gap e heap; preservar WROOM 0.0.40/OTA/menu.
- **P2 — legenda:** aumentar quantidade no navegador com limite; sem persistir falas em A22.
- **P3 — A22 handoff:** consultar `/lab/phone` da S3 e status do MacroDroid A22, diferenciar solicitação HTTP / recebimento da macro / app realmente abriu.
- **P4 — MCP físico:** após consulta e autorização, testar LED WROOM 30 s com comando verbal e verificar estado antes/depois.
- Preservar voz e AutoStop nativos XiaoZhi; preservar CORE 0.0.25 S3 e WROOM 0.0.40. Não reinstalar, reiniciar nem editar sistema em produção neste checkpoint.

## Fontes primárias de código
- `Fernandodsrm/botizin-core` branch `lab/s3-voice-transcript-mcp-041`: `research/xiaozhi-028aba2/upstream/main/application.cc`, `boards/botizin-web-s3/{web_audio_server.cc,web_audio_codec.h,web_panel.h,phone_bridge.h,lab_return_025.cc,botizin_web_board.cc}`.
- CI concluída com sucesso: https://github.com/Fernandodsrm/botizin-core/actions/runs/43373020129.
- Contexto anterior no projeto central: `docs/BOTIZIN-CHECKPOINT-XIAOZHI-S3-WROOM-2026-11-12.md`.

## Medição de áudio da LAB 0.0.41 — 13/11/2026

**Fonte:** console PowerShell enviada pelo proprietário após coleta de duas leituras de `/lab/telemetry`. Leitura inicial `uptime_ms=1529457`; leitura final `uptime_ms=1588187`; intervalo **58,73 segundos**, sem reinicialização observada.

| Métrica | Delta |
|---|---:|
| `speaker_pcm_queued_samples` | +495.360 |
| `speaker_pcm_dropped_samples` | **+0** |
| `speaker_popped_samples` | +499.200 |
| `mic_browser_rejected` | **+0** |
| `mic_codec_reads_failed` | +2.670 |
| `ws_rx_errors` | **+0** |
| `ws_tx_errors` | **+0** |
| `backend_opus_rx_frames` | +345 |
| `backend_opus_rx_accepted` | +344 |

Interpretação:
- A grande contagem acumulada `speaker_pcm_dropped_samples=809472` **não aumentou** durante a janela. Não há evidência de perda de amostras na FIFO de saída durante esta medição, apesar de perdas anteriores. Não afirmar que o defeito original foi permanentemente corrigido.
- WebSocket sem novos erros no intervalo. Houve **1 frame Opus não aceito entre 345** (aprox. 0,29%), o que não prova por si só erro do decodificador ou corrupção de pacote.
- `mic_codec_reads_failed` cresceu 2670; `Read()` no codec falha quando a FIFO do microfone não fornece quadro inteiro após tentativas limitadas. Averiguar se isso acontece nos momentos de escuta ou apenas enquanto a captura está pausada e o backend mudou de estado; não tratar o número cru como taxa de perda de rede.
- A leitura final mostrou `state=3`, `ws_fd=-1`, `cmd_stop` 3→4 e `ws_open_count` permaneceu 9. Pode ser término normal da sessão; sem relato do instante não classificar como travamento.
- O terminal apresentou erros adicionais porque o usuário aparentemente colou novamente texto que continha `PS C:\Users\ferna>` e prompts `>>`; esses são erros de digitação/colagem do PowerShell e **não evidenciam problema do firmware nem exigem regravação**.
- Próxima decisão: preservar LAB 0.0.41 e WROOM 0.0.40, fazer comparação em mais um teste concentrado no estado OUVINDO se a gagueira persistir, priorizar sincronização de expressão OLED após estabilidade, sem remapear áudio nem repetir testes desnecessários.
