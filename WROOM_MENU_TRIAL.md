# Teste remoto autorizado: WROOM 0.0.12 → Menu 0.0.13 → 0.0.12

Este primeiro Menu tem OLED, cinco áreas, botões existentes, configurações NVS somente leitura, Wi-Fi, telemetria e RPC de status. Não instala módulos, não grava configurações, não aciona LED/motor/servo e não comunica com S3. A versão normal 0.0.12 permanece no outro slot.

Antes de Wi-Fi, o Menu verifica layout e SHA256 exato do BIN 0.0.12 preservado, seleciona seu boot e arma esp_timer de 600 segundos. O callback reinicia a placa independentemente de esperas no loop. Botão de retorno exige confirmação. Uma reinicialização depois da seleção do boot também deve iniciar o baseline. Falha antes de selecionar o boot/armar não tem recuperação garantida; cabo pode ser necessário. Não houve alteração de bootloader/tabela.

Consulta externa confirma firmware, module_id, boot, slot, timer, memória e resposta a ping/menu_status. oled_initialized e buttons_configured indicam inicialização do software, não comprovam imagem física ou botão funcionando. Atualização automática fica desativada somente durante este firmware temporário; deve reaparecer após retorno ao 0.0.12.

Oferta temporária somente após build e testes passarem, com manifesto restaurado antes da execução ou por retirada programada. Não substituir o baseline pelo LED nesta rodada. O downloader do Menu final ainda será implementado em etapa separada.
