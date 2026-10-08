# Descoberta local e retorno do Menu — resultado 08/10/2026

A WROOM 0.0.15 e a S3 0.0.16 foram compiladas, verificadas, publicadas e observadas nas placas por RPC real. Os dois estados OTA são VALID e a atualização automática está ativa. A WROOM encontrou a S3 por mDNS mesmo com a configuração de IP alternativo vazia.

## Implementação

- S3 anuncia botizin-s3.local; WROOM anuncia botizin-wroom.local. Ambos publicam wifi_ip, local_name e mdns_ready na telemetria existente e no RPC.
- A WROOM centraliza a origem das URLs de status e OTA da S3 em peer_address.h; não há mais referência a 192.168.0.36.
- Consulta por nome tem timeout de 250 ms e intervalo mínimo de 30 segundos entre tentativas. O endereço é guardado durante o funcionamento; falha ou mudança de rede invalida o endereço.
- Endereço IPv4 alternativo configurável por comando autenticado ThingsBoard peer_address (params ip + command_id + issued_at_ms), salvo em namespace NVS separado peer-address. String vazia desabilita a alternativa. IPv6, endereço da própria placa e endereço fora da sub-rede são rejeitados. Pareamento/HMAC continua sendo necessário para OTA da outra placa.
- A migração fornece 192.168.0.9 como alternativa inicial, mas ela foi desativada no teste. Configuração atual usa apenas descoberta por nome. Esse valor inicial não configura IP estático da placa.
- Nenhuma alteração nas rotinas de gravação OTA, hash, confirmação ou particionamento. A S3 também explicita WiFi.setAutoReconnect(true); não confundir isso com prova de reconexão sob toda falha.

## Provas observadas

S3: firmware 0.0.16, IP 192.168.0.9, mDNS pronto, app0@0x10000 / VALID, OTA automático ativo. WROOM: 0.0.15, IP 192.168.0.8, mDNS pronto, app0@0x10000 / VALID, OTA automático ativo. RPC confirmou s3_address_source=MDNS e s3_manual_ip vazio. Telemetria coerente confirmou S3_LINK=OK, versão S3 0.0.16 e relatório de cinco segundos de idade.

No arranque ocorreram tentativas falhas e reinícios da S3, incluindo BROWNOUT antes e depois da troca de versão, e depois POWERON. No último boot POWERON a S3 permaneceu respondendo por mais de sete minutos até a amostra registrada. Isso não diagnostica cabo/carregador e não prova que a alimentação está definitivamente resolvida.

Duas observações do mesmo boot WROOM mostram diag_peer_calls 11 → 23 e diag_peer_failures 8 → 8 entre uptime 146 → 506 segundos: doze operações posteriores sem nova falha contada. Esse contador inclui estados de espera; não é uma taxa de perda de pacotes. Na última amostra, operação peer levou 63.734 microssegundos. Não foi feita mudança real de IP pelo DHCP; foi comprovada descoberta sem usar o IP alternativo.

## Memória e tempo

| Placa | Binário | Slot | Sobra | RAM estática |
|---|---:|---:|---:|---:|
| S3 0.0.16 | 1.204.144 B | 3.145.728 B | 1.941.584 B | 55.252 B |
| WROOM 0.0.15 | 1.289.888 B | 1.310.720 B | 20.832 B | 68.200 B |
| Menu 0.0.17 | 1.162.544 B | 1.310.720 B | 148.176 B | 51.024 B |

A mudança acrescentou 37.360 bytes à S3 e 41.728 à base WROOM. A base WROOM ocupa 98,4% do slot: não aumentar essa imagem com funções de robô. Continuar com módulos separados, como no teste LED. Ocupação de flash não é percentual de CPU.

Última amostra: S3 RAM interna livre 251.372 B, mínimo 192.640 B; WROOM livre 151.948 B, mínimo 78.192 B. Maior gap do loop principal: S3 2.486.041 us, WROOM 3.486.059 us. O RPC/HTTPS em primeiro plano continua produzindo pausas da base completa; a descoberta de endereço não remove esse trabalho. Isso não mede CPU nem latência física de cada botão.

## Menu com indicação de retorno

Menu 0.0.17 compilado e armazenado em main/wroom/releases/0.0.17, sem alterar o manifesto ativo WROOM 0.0.15. Ao confirmar pela segunda Direita, apaga o LED, mostra Retorno confirmado / Voltando... / Anterior: 0.0.15 por 750 ms e reinicia. Esquerda continua cancelar/voltar.

O guard verifica tamanho e SHA da 0.0.15 preservada antes de armar retorno de 600 segundos; não instalar sobre outra base. A indicação ainda precisa de teste físico. Menu temporário não contém link S3 e não escreve outro OTA. Fonte e instruções: branch wroom-menu-feedback-20261008, MENU_RETURN_FEEDBACK.md. Binário: 1.162.544 B, SHA 75aac11d82cf1daabcdbdd1a94922f837c91bae8105b0e9d94462af9d412b733.

Dados completos em validation/local-name-discovery-2026-10-08/proof.json. Próximo teste físico: abrir a página S3 na OLED e conferir 0.0.16; exercitar desligamento/religamento e, quando possível, uma mudança real de endereço. Testar Voltando... no módulo 0.0.17 separado. mDNS requer uma rede local que permita multicast; se ela bloquear, a alternativa pode ser configurada pela nuvem sem cabo.
