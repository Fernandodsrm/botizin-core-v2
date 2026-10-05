# WROOM 0.0.6 — controle manual com automático ligado

A atualização automática continua habilitada, consultando o manifesto a cada 60 segundos. A S3 permanece no canal próprio 0.0.10.

Na OLED, UP/DOWN percorrem WROOM → S3 → OTA. Na página OTA, OK consulta o manifesto; quando existe candidata, outro OK instala exatamente a versão/SHA consultada. BACK cancela o pedido e retorna à WROOM. Nenhum botão muda Wi-Fi, token ou partições.

Uma consulta manual reserva cinco minutos antes de o automático retomar, inclusive se a consulta responder que a versão atual está em dia. Cancelar reserva cinco minutos para permitir a decisão do usuário; não equivale a desativar o automático permanentemente. Depois desse prazo uma versão publicada pode ser instalada automaticamente.

No painel: Consultar versão → aguardar → Ver estado → conferir candidata → Confirmar instalação. A placa valida candidate_id, versão, SHA, boot atual e validade. A instalação só é enfileirada depois de a resposta ao RPC ser aceita pelo servidor. Isso confirma o recebimento do pedido, não sucesso da atualização; o painel só anuncia sucesso com nova telemetria real, novo boot e OTA VALID. Comandos expiram e não ficam persistentes no servidor.

OLED mostra consulta, download em checkpoints de 64 KiB, SHA e reinício. O diário persistente continua registrando bytes/hashes/partição. OLED mostra progresso da WROOM; acompanhar uma instalação da S3 ainda exige uma etapa posterior no firmware da S3.

Não existe comando para desligar o automático nesta versão. PING continua disponível. A navegação e consulta local da S3 permanecem. LED e retransmissão S3→WROOM ficam para etapa seguinte.
