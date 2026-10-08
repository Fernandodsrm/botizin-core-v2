# Resultado do teste físico Menu + LED WROOM 0.0.14

08/10/2026. Usuário confirmou OLED, navegação e acionamento/apagamento do LED GPIO26 (fio laranja). Upload OTA local gravou 1.162.112 bytes, verificou SHA do recebimento e da flash, selecionou app0 e reiniciou sem cabo. A oferta de cinco minutos pelo GitHub foi retirada sem detecção pela placa; a causa de cache é hipótese. Download PowerShell inicial expirou antes de enviar; curl IPv4 baixou e verificou o arquivo.

## Dados observados

- Três amostras coerentes do mesmo boot 207ce2117ede5008: uptime 20, 80 e 140 segundos.
- RAM interna livre nas amostras: 167.068, 166.664 e 166.720 bytes. Mínimo observado: 95.644 bytes; maior bloco livre: 110.580 bytes.
- Intervalo máximo do loop da interface: 31.006, 51.008 e 90.008 microssegundos (90,008 ms). Não representa percentual de CPU nem latência de cada botão.
- Worker de rede pronto e retorno ARMED_600_SECONDS_RETURN_0.0.12 em todas as amostras. Zero falhas de telemetria registradas nas amostras. O último contador de tentativas é 2 porque o snapshot é montado antes de contabilizar o próprio envio.
- Flash do binário: 1.162.112 / 1.310.720 bytes (88,66% do slot); sobra 148.608 bytes. RAM estática do build: 51.024 bytes; não somar novamente ao heap livre.
- LED aparece apagado nas três amostras; liga/desliga foi confirmado fisicamente pelo usuário. A coleta de 60 segundos não captura cada acionamento curto.

## Retorno confirmado

RPC atual: WROOM 0.0.12, novo boot 838e998d70d07f72, reset SOFTWARE, execução e boot app1@0x150000, estado VALID, OTA automático ativo, manifesto UP_TO_DATE 0.0.12 e nenhum candidato. No teste, execução era app0 e próximo boot app1. Firmware anterior preservado; retorno manual confirmado. O vencimento de dez minutos não foi exercitado nesta sessão.

## Observação da interface e pendências

Usuário relatou ausência de animação/indicação ao apertar Direita e percebeu retorno depois de apertar Esquerda. Não há log individual de eventos; não atribuir causalidade ao botão Esquerda. O modelo de código usa Esquerda para cancelar/voltar e a segunda Direita para confirmar retorno. Acrescentar indicação explícita de retorno e validar fisicamente a sequência em próxima versão.

Endereços confirmados: WROOM 192.168.0.8, S3 192.168.0.9. O teste não contém link S3. A referência 192.168.0.36 da 0.0.12 continua pendente: centralizar endpoint, descoberta por nome com timeout/backoff, endereço manual alternativo e IP atual na telemetria. Preservar pareamento e OTA.

Dados completos: validation/wroom-menu-led-2026-10-08/proof.json. Este teste curto não prova estabilidade durante a noite, reconexão após falha nem retorno automático de dez minutos.
