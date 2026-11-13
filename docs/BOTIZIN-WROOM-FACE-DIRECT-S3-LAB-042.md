# WROOM LAB 0.0.42 — olhos de voz diretos da S3

**Preparação:** 13/11/2026. **Não instalada automaticamente.**

A WROOM 0.0.40 usa OTA0/app0 para o BOTIZIN-base e OLED. A LAB 0.0.42 é um firmware experimental para OTA1/app1, derivado da LAB 0.0.41 e que preserva layout de 4 MB e menu existente.

## A mudança
- WROOM consulta somente `http://192.168.0.24/status` da S3 a cada 650 ms, timeout HTTP de 200–220 ms. Não precisa do A22 nem do notebook para esses estados.
- Confere origem `XIAOZHI_REAL`, `web_audio_connected` e estado XiaoZhi: 3 repouso, 4 conexão, 5 ouvindo, 6 falando, 9 ativação (representado por conexão). Sem áudio nem transcrições transmitidas.
- Atualiza objeto atômico de estado, já usado pela tarefa independente do OLED. TTL de 4 s restaura rosto local em perda de comunicação; se mic/navegador desconectado, o rosto volta ao normal depois do TTL.
- API de diagnóstico WROOM passa a mostrar `VOICE_S3_POLL_OK`, `VOICE_S3_POLL_FAIL`, `VOICE_S3_AGE_MS`, `VOICE_SYNC_EVENTS`, `VOICE_SYNC_MODE`.
- Não modifica OTA0, não grava NVS por polling, não instala servicos no Windows, não mexe na S3 0.0.41.
- Como a interface web marca FALANDO com o áudio efetivamente enfileirado no navegador, enquanto a WROOM acompanha o estado de XiaoZhi da S3, **sincronia visual de estados não garante lip-sync exato do áudio no Chrome**. Ponto de aperfeiçoamento futuro.
- O estado `voiceLabNextBootBaseArmed` do código .41 é preservado: quando LAB roda em app1, tenta armar app0 como próximo boot. A proteção continua dependente de validação física.

## Instalação segura proposta
1. Verificar WROOM .40 rodando na OTA0 pelo MCP atual.
2. Baixar imagem compilada 0.0.42 com SHA-256 pinado.
3. Conectar **somente WROOM via USB**, identificar MAC e flash 4 MB, nunca gravar S3 COM4.
4. Backup completo 4 MB local no Windows, manter confidencial (contém Wi-Fi).
5. Conferir partições. Gravar só app1 0x150000, reler e comparar SHA.
6. Selecionar OTA1 sem tocar app0 e conferir WROOM LAB 0.0.42 + `VOICE_LAB_RETURN_BASE_ARMED: YES`.
7. Falar no Chrome e conferir contadores de poll e estado 5/6 da WROOM; ao reiniciar a placa, conferir retorno à base OTA0.
8. Se falhar, não insistir em reset/reinstalação sem ler os logs e verificar OTA0.
