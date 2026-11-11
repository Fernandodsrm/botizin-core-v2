# BOTIZIN — Central do projeto (atualizada em 11/11/2026)

**A ideia em 30 segundos:** um ecossistema de cinco placas com resultados úteis, não mais uma coleção de testes. ESP32-S3 + A22 fazem a central; ESP32-WROOM é o **BOTIZIN de bancada**, rosto OLED, menu e lançador de módulos pelo GitHub; duas ESP32-C3 com telas serão um par de comunicação; uma ESP8266 com OLED será ferramenta portátil de IR/Wi-Fi. A35 é o cliente de conversa com ChatGPT. O carrinho 2WD e o mini robô ficam em fila, sem receber uma placa por impulso.

**[VISÃO MESTRE — decisões, prioridades, arquitetura, resultados e limites técnicos](docs/BOTIZIN-VISAO-MESTRE-2026-11-11.md)**

**[INVENTÁRIO — peças físicas, estado e chegada de placas](docs/inventario-hardware-botizin-lab-2026-11.md)**

**[AUDITORIA TÉCNICA DA WROOM 0.0.38 — tamanho, fontes exatas, SPIFFS e histórico 0–38](docs/AUDITORIA-WROOM-038-HISTORICO-E-OTIMIZACAO-2026-11-11.md)**

## Onde estamos realmente
- **ESP32-S3:** CORE 0.0.25, OTA/Wi-Fi, comunicação e diagnósticos confirmados. A voz XiaoZhi foi testada pelo proprietário no notebook com LAB; o CORE de produção atual não é a LAB de voz.
- **ESP32-WROOM:** 0.0.38, OLED com rosto BOTIZIN, menu/catálogo e LED GPIO26 controlado pelo ChatGPT; partições de app apertadas. Ainda não existe launcher enxuto OTA0↔OTA1 consolidado.
- **A22:** Termux, Debian PRoot, MCP privado, túnel e supervisor 2.1; diagnóstico via ChatGPT funcionando. Recuperação total após quedas de energia continua sob observação.
- **Duas C3 + ESP8266 OLED:** relatadas como encomendadas, ainda sem confirmação física ou testes.
- **Carrinho:** chassi 2WD montado, motor/driver/alimentação/PS4 ainda sem prova final.

## Próxima entrega, sem expandir infraestrutura
**Auditar o firmware exato da WROOM 0.0.38** versus Menu 0.0.29 e ambientes existentes; identificar ocupação de flash; criar em branch isolada a menor base possível BOTIZIN + OLED + menu + catálogo + OTA de módulo + retorno seguro. Preservar firmware atual até build, testes, backups e autorização para gravar.

Depois: **BOTIZIN → módulo LED → BOTIZIN** sem notebook. O próximo módulo só começa quando esse ciclo estiver fisicamente comprovado.

## Restrições que impedem confusão
- O modelo ChatGPT não roda dentro das ESP32; a conversa inteligente usa serviços conectados (A22/IA) e poderá acionar o hardware.
- Ao inicializar um firmware de módulo no segundo slot, o programa do rosto no outro slot **não executa simultaneamente**. Voz independente no A22 pode continuar, mas OLED/rosto na WROOM só volta quando ela reiniciar no BOTIZIN.
- A WROOM tem Bluetooth Clássico; C3, S3 e ESP8266 não têm. Não prometer PS4 direto na C3.
- Compilação, pacote arquivado, teste físico e uso real são estados diferentes. Nunca regravar slot estável nem alterar o túnel por interpretar um plano como autorização.
