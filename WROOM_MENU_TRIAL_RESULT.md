# Teste remoto do Menu WROOM: ciclo completo comprovado

Em 07/10/2026, a placa real executou **WROOM 0.0.12 (app1) → Menu temporário 0.0.13 (app0) → WROOM 0.0.12 (app1)**, sem cabo e sem confirmação visual. A S3 não foi atualizada nesta rodada.

## Evidência

- Fonte: 4820db7e421026b27c63d26451edf46345c761b0; [build aprovado](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/37699024226), artifact 11516726886.
- BIN: 1.159.536 bytes; SHA256 804bb9990271f430450e81d198ee9701b8d7b12d954b083041e98c5964070e4c. Margem do slot: 151.184 bytes; RAM estática compilada: 50.528 bytes. Tabela original e chip verificados.
- Menu: boot 27ffaed37ce0eb26, app0 @0x10000. O app1 @0x150000, contendo hash exato da 0.0.12, foi selecionado para próximo boot antes da rede. Status ARMED_600_SECONDS_RETURN_0.0.12.
- RPC menu_status retornou MENU_STATUS com Wi-Fi conectado e timer armado. Uma tentativa ota_check retornou BLOCKED_TEMPORARY_TRIAL. Não existe downloader/upload neste Menu temporário.
- Dez relatórios distintos do Menu, uptime 20 a 560 segundos, mesmo boot, nenhuma falha de envio registrada nas amostras. Memória interna livre estabilizada em aproximadamente 176.700 bytes; mínimo interno observado 118.936; maior bloco 110.580.
- Retorno: boot 6b30b061d90ae1dc, app1, reset SOFTWARE. Primeira telemetria de retorno com uptime 26 segundos; automático true.
- Estimativa de duração por timestamp recebido menos uptime: 601,193 segundos, aproximadamente dez minutos. Inclui atraso de transporte e resolução do contador; não é medição instrumental de precisão do timer.
- RPC após retorno confirmou VALID, automatic_enabled true, manual READY, UP_TO_DATE 0.0.12 e nenhum candidato pendente.

## Oferta e retirada

[Publisher](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/37699423347) ofereceu 0.0.13 às 19:57:56 e restaurou 0.0.12 às 20:02:58 (Brasília). A retirada manual coincidiu com a automática; a atualização da referência por lease retornou erro, e a restauração automática foi confirmada posteriormente no manifesto e no log. A placa selecionou candidata com tamanho/hash/boot corretos, e INSTALL_ACCEPTED foi recebido. O arquivo público também foi baixado e seu SHA/tamanho conferidos.

Manifesto WROOM atual: 0.0.12. Manifesto S3 atual: 0.0.13. O gatilho de oferta desta branch foi removido após a rodada para evitar repetição acidental. Nenhum módulo LED foi instalado e o baseline permanece intacto.

## Limites e próximos ajustes

OLED inicializada e botões configurados foram confirmados pelo software, mas aparência e pressionamento físico ainda não foram verificados. O Menu é um protótipo de navegação e diagnóstico com retorno; catálogo real, instalação de módulos e configuração Wi-Fi editável ainda não estão implementados.

Maior intervalo entre passagens do loop: 2.476.027 microssegundos. Inclui espera de rede e não é percentual de CPU. Consultas HTTPS ainda bloqueiam a interface; o polling dos botões desta prova pode perder pressões curtas durante essas esperas. Antes do Menu definitivo, preservar eventos de botões e isolar operações de rede da interface. Para módulos de controle, carregar apenas recursos necessários e medir latência real.

Comprovamos uma rodada normal de execução e retorno por timer, com Wi-Fi salvo recuperado. Não injetamos queda de energia, perda de rede ou travamento antes da armação. O teste não garante recuperação desses casos nem corrige a suspensão após falhas existente na versão normal 0.0.12.

[JSON de evidência](validation/wroom-menu-trial-2026-10-07/proof.json) mantém apenas campos do mesmo timestamp em cada amostra. O ThingsBoard retém chaves antigas: module_id/timed_trial_* da rodada anterior não descrevem o firmware atual depois do retorno.
