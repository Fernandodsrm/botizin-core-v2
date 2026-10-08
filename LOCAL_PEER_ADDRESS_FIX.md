# Endereços locais e correção da descoberta

Em 08/10/2026, o usuário confirmou pelas respostas HTTP locais: WROOM 0.0.12 em 192.168.0.8 e S3 0.0.13 em 192.168.0.9. A WROOM procurava 192.168.0.36 em peer_status.h, peer_ota_client.h e no texto de diagnóstico. Ambas responderam com Wi-Fi OK, telemetria HTTP_200 e OTA habilitado. Os painéis ThingsBoard examinados não possuem esses IPs gravados.

O teste físico Menu + LED 0.0.14 é independente e não compila comunicação com a S3. O retorno preservado à 0.0.12 ainda tem o endereço antigo; não declarar o problema corrigido só porque o teste do LED funcionou.

Próxima alteração autorizada: remover as URLs duplicadas e centralizar o endereço da S3; anunciar um nome local estável via mDNS, resolver com prazo limitado e guardar o IP enquanto funciona. Renovar após perda da conexão, com intervalo entre tentativas, sem busca contínua por toda a rede. Manter configuração manual de endereço como alternativa se a rede bloquear multicast. Publicar IP atual na telemetria existente e mostrar endereço/estado/idade no painel e diagnóstico. Validar acesso autenticado antes de comandos OTA da outra placa; descoberta de nome não substitui pareamento.

Preservar slots, configuração Wi-Fi/NVS e mecanismo OTA já validado. Compilar e medir custo adicional antes de oferecer. Verificação física necessária: desligar/religar S3 e verificar reencontro; confirmar mudança real de endereço quando possível. Nenhuma garantia de IP fixo por firmware ou dependência de acesso ao roteador.
