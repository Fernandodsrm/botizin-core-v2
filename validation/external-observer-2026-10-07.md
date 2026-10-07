# Primeira observação externa — 07/10/2026

Consulta de seis horas, somente leitura. Os dados selecionados e timestamps estão no JSON adjacente. Horários abaixo em Brasília (UTC−3).

- As duas placas tinham relatório recente no momento da consulta: S3 aproximadamente 31 segundos, WROOM 44 segundos.
- S3: BROWNOUT reportado às 13:07; último relatório antes do silêncio às 13:11, retorno dos relatórios às 15:06 (aproximadamente 1h55). Não prova que ficou desligada durante todo o intervalo. O motivo BROWNOUT se refere ao boot identificado nessa leitura, não a um novo reset a cada envio.
- S3: boots experimentais às 15:19 e 15:29 correspondem ao teste autorizado de instalação e retorno, portanto não contar como reinícios inesperados.
- WROOM: BROWNOUT reportado às 15:50; outra mudança de boot às 16:39. O motivo deve ser lido junto do boot e seu timestamp. Não atribuir o último evento a uma fonte de alimentação sem evidência física.
- Maior intervalo de loop reportado na janela: S3 2,562 s; WROOM 3,225 s. Não mede percentual de CPU. São máximos reportados, podem representar picos anteriores dentro do boot.
- S3 BIN selecionado: 37,09% do slot, margem 1.978.944 bytes. WROOM: 95,23%, margem 62.560 bytes.

A janela mistura operação normal, mudanças de alimentação feitas pelo usuário e o teste experimental. Não permite comparação controlada entre carregador e powerbank. Próxima observação deve registrar horário da troca de alimentação e comparar períodos sem alterações de firmware. Brownout indica queda de tensão detectada, mas não identifica sozinho powerbank, cabo, conector ou regulador como causa.
