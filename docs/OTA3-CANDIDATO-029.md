# BOTIZIN OTA 3.0 — candidato CORE 0.0.29

Estado: **candidato não instalado e não publicado**. Base: checkpoint Wi-Fi 2.2 CORE 0.0.28. Autorização de implementação: 17/11/2026. 

## Requisitos do usuário
1. GitHub guarda versões, compila, testa, gera SHA-256 e catálogo.
2. S3 tem APP0 e APP1 livres. A próxima instalação usa **apenas a APP que não está executando**.
3. Instalar não deve mudar o próximo boot. A troca de boot depende de clique separado.
4. Consultas GitHub somente quando usuário pedir. Não instalar automaticamente.
5. Botões de reinício da APP0 ou APP1, e reinício da atual.
6. Sem temporizador de cinco minutos ou processo de monitoramento em repouso.
7. Original 0.0.25 preservada no GitHub e futura 0.0.25-OTA3 gerada a partir da BASE HISTÓRICA, não fingindo que 0.0.29 é equivalente à 0.0.25.

## Implementação deste ramo
- `ota3/catalog.json`: lista manual de versões historicamente testadas, inicialmente duas. Padrão do catálogo com campos `schema,board,entries[id,version,label,board,size,sha256,url,ota3,approved]`.
- `firmware/ota3_manager.h`: download HTTPS sob demanda em tarefa temporária, valida esquema, allowlist de URL, ID, chip/partição, tamanho máximo, SHA256 dos dados e da partição gravada. Grava por `esp_ota_begin/write/end` **sem acionar `esp_ota_set_boot_partition` na instalação**.
- `/control/ota3/catalog-refresh`: requisita atualização do catálogo, autenticação existente.
- `/control/ota3/catalog`: retorna o catálogo validado ao painel.
- `/control/ota3/install`: exige ID do catálogo e estágio da imagem no slot inativo.
- `/control/ota3/status`: indica estado, progresso e APP0/APP1.
- `/control/ota3/boot`: seleciona APP0/APP1 explicitamente via `esp_ota_set_boot_partition`; reinicia depois de responder.
- Aba OTA 3.0 no painel: consulta, escolha, instalação, inicialização e estados. Ações exigem sessão do painel.
- Consulta automática original no `loop()` inibida; `main/manifest.json` não é modificado.
- URI antiga `POST /update` é desativada nesta candidata. Fluxo legado de comandos OTA por paridade `/peer/ota` oferece apenas status, e não atualizações automáticas.
- Sem alocar firmware completo na RAM. Worker usa buffer de 4096 bytes, pilha temporária e cache de catálogo estático de cerca de 4 KiB.

## Limites / riscos conhecidos
- O OTA3 não foi testado fisicamente; CI comprova **compilação, não funcionamento real na S3**.
- Versões que não incluem OTA3, como a 0.0.25 original e a 0.0.28 histórica, não terão botões de alternância ao serem inicializadas. Para alternância bidirecional, instalar versões compatíveis nas duas APPs.
- NATIVE ESP-IDF: uma imagem gravada sem mudança de boot pode aparecer em estado OTA `UNDEFINED` até ser selecionada. `esp_ota_set_boot_partition` valida a imagem antes de programar o boot.
- Nenhum mecanismo de reversão via cinco minutos foi adicionado; o bootloader pode rejeitar uma imagem inválida conforme a política nativa, mas não há garantia absoluta de recuperação sem USB.
- Worker HTTPS em tarefa separada não bloqueia diretamente o `loop()`, porém download e flash consomem recursos do dispositivo enquanto a operação está em curso.
- `/status` consulta em tempo real as informações de partição e lê o último resultado de NVS; evitar polling excessivo.
- Firmware candidato novo é derivado da 0.0.28 aprovada, não sobrescreve a 0.0.25 original.

## Aceite antes de qualquer instalação
- [ ] CI de `candidate/ota3-0.0.29` em verde com BIN menor que 3MiB.
- [ ] Listagem de catálogo, hash e tamanho validados sem revelar segredos.
- [ ] Simulações automatizadas para estado, instalação no slot oposto, sem troca automática de boot.
- [ ] Teste físico: APP1 executando + APP0 desocupada: instalar BIN compatível em APP0, confirmar `BOOT_PARTITION` ainda APP1.
- [ ] Reiniciar APP1 e confirmar que não mudou.
- [ ] Selecionar APP0, confirmar novo boot, acesso HTTP, Wi-Fi e MQTT.
- [ ] Da APP0 compatível, instalar APP1 e voltar pelo painel.
- [ ] Registrar estado/memória e só então considerar liberação como versão aprovada.

## Política de publicação
**NÃO** mudar `main/manifest.json`, criar atualização automática, nem gravar placa até aprovação posterior específica. BIN CI fica como artefato temporário; sua publicação no catálogo permanente requer aprovação e hash imutável.
