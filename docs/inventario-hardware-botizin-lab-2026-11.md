# Inventário físico — BOTIZIN / Fernando Lab
Atualização: 10/11/2026. Base: fotografias e descrição do proprietário. **Documento de catálogo: não comprova teste elétrico.**
Esta lista evita duplicar compras e distingue equipamentos demonstrados, montagens pendentes e possibilidades de reaproveitamento. Nenhum firmware, credencial, endereço IP ou configuração de rede está registrado aqui.

## 1. Computação, controle e conectividade
| Item | Quantidade | Estado e observações |
| --- | ---: | --- |
| Samsung Galaxy A22 | 1 | Servidor BOTIZIN: Termux, Linux PRoot, SSH, MCP e túnel; recuperação automática v2 ainda precisa validação no dispositivo |
| Samsung A35 | 1 | Cliente móvel de acesso remoto; fora da bancada na maioria das sessões |
| Notebook Samsung branco | 1 aparente | Visível nas fotos, modelo e estado não verificados |
| ESP32-S3 N16R8 | 1 | CORE BOTIZIN; hardware 16 MB flash / 8 MB PSRAM; físico visto/relatado em sessões anteriores |
| ESP32-WROOM-32 | 1 | Controlador de LEDs/OLED; v0.0.38 e LED GPIO26 testados anteriormente |
| Controle PlayStation DualShock 4 | 1 | Falha de carregamento relatada; priorizar diagnóstico/reparo, não desmontar antes |
| Controle remoto infravermelho | 1 | Ainda não testado com ESP32 |
| Câmera IP com antena, LEDs IR e aplicativo móvel | 1 | Visualização remota relatada como funcional; identificar app/modelo exato (provavelmente Yoosee), protocolo RTSP/ONVIF ainda não comprovado |
| Câmera antiga branca de quatro fios | 1 | Possível câmera analógica; alimentação e pinagem desconhecidas, não ligar GPIO diretamente |
| Relógio digital inteligente quadrado | 1 | Liga e recarrega; integração com ESP32 e protocolo de controle pendentes |
| Fone TWS + estojo recarregável | 1 conjunto | Relatado como funcional; estojo usado para carregar relógio |
| Notebook e periféricos/cabos USB | diversos | Uso e pinagens individualmente pendentes |

## 2. Alimentação e medição
| Item | Quantidade | Estado e observações |
| --- | ---: | --- |
| Multímetro DT9205A + 2 pontas | 1 conjunto | Possui faixas para V, A, resistência, capacitância e teste de continuidade; calibração/fusíveis não verificados |
| Fonte ATX e placa de distribuição 24 pinos | 1 conjunto | Borneiros marcados -12 V, GND, +12 V, +5 V, +3,3 V; **saídas ainda não medidas** |
| Power banks | 3 | Preto na bancada, branco pessoal, antigo com falha (lanterna funciona); capacidades e saídas não medidas |
| Suporte de 4 pilhas AA no carrinho | 1 | Instalado sem ligação completa aos motores |
| Pilhas AA/AAA avulsas | diversas | Mistura de tipos/condições; separar antes de utilizar |
| Bateria removível de ferramenta, etiqueta 21 V / 1,5 Ah | 1 | Foto de pack preto/amarelo; tensão real/química/carregador/compatibilidade com motor não confirmados |
| Máquina Kemei KM-6558 | 1 | Funcional, manter intacta; contém 2 células Ni-Cd AA 1,2 V 500 mAh, presumivelmente em série (2,4 V nominais) |
| Máquinas/aparadores antigos adicionais | até 2 relatados | Pelo menos um com defeito; triagem de motor, chave e circuito de carga pendente |

## 3. Robótica, atuadores e montagem
| Item | Quantidade | Estado e observações |
| --- | ---: | --- |
| Chassi acrílico 2WD | 1 | Mecânica montada; ainda não andou |
| Motores TT e rodas motrizes | 2 + 2 | Instalados no carrinho; fios e soldas pendentes |
| Roda giratória livre | 1 | Instalada |
| Discos encoder | 2 | No carrinho; leitura ainda não demonstrada |
| Ponte H L298N | 1 | Fixada no carrinho; ligação/ensaios pendentes; avaliar queda de tensão com 4 AA |
| Motor com redução/mandril de furadeira/parafusadeira aberta | 1 conjunto | Motor com caixa redutora e gatilho; **não pressupor 21 V**; há conjunto com plugue de dois pinos na fotografia, investigar circuito e tensão antes de energizar |
| Microservos e acessórios | 4 relatados em sessões anteriores | Modulação/PWM, alimentação externa e testes futuros |
| Estruturas e chapas metálicas recuperáveis | diversas | Alternativas DIY à impressão 3D |

## 4. Prototipagem e materiais
| Item | Quantidade | Estado e observações |
| --- | ---: | --- |
| Protoboard grande | 1 | Montada na bancada |
| Mini protoboards | 2 visíveis | Presentes na bandeja |
| LEDs vermelhos, amarelos, verdes, azuis, branco/transparente | pelo menos 5 cores | Na bancada; **resistores limitadores ainda não confirmados** |
| Buzzers | 2 | Na protoboard; teste pendente |
| Módulos IR/sensores pequenos | diversos | Modelos e pinagens ainda a identificar |
| Placa de expansão/controle e circuitos integrados avulsos | diversos | Montados e não montados; identificação exata pendente |
| Fios de cobre com isolamento | vários | Vermelho, preto, azul e outros; diferentes bitolas |
| Fio de aço inox Daiso 0,9 mm x 10 m | 1 rolo | Estruturas leves e suportes; soldabilidade com estanho não presumida |
| Fio revestido de vinil Daiso 1,6 mm x 16 m | 1 rolo | Fixação mecânica, não utilizar como fio de alimentação sem caracterizar |
| Material de soldagem, pinças, alicates (3), chaves, estiletes, escova, trena | diversos | Na caixa de ferramentas |
| Fitas: isolante, crepe, adesiva, silver tape, dupla face | diversos | Fixações e proteção; fita comum não substitui isolamento adequado de rede elétrica |
| Tapete azul de silicone e base/caixa de vinho de madeira | 1 cada | Bancada modular física |

## 5. Estado atual da bancada
- **Demonstrado:** LED WROOM liga por HTTP e desliga após 30 s; câmera Wi-Fi visualizável por aplicativo; Kemei operante.
- **Montado, não validado:** base com ATX e borneiros, breadboard com cinco cores de LEDs, buzzers, expansoras, sensor IR, LEDs extras, servos, carrinho 2WD.
- **Para reparar:** controle PS4; power bank antigo; máquina/aparador antigo defeituoso.
- **Preservar:** barbeador Kemei funcional; power bank pessoal; servidores e firmwares existentes.

## 6. Próximas entregas possíveis (sem novas compras)
1. **Segurança elétrica da bancada:** medir e rotular saídas ATX com multímetro; instalar resistores individuais de limitação de corrente para LEDs, se ausentes.
2. **Teste IR + luz + som:** receptor IR + controle remoto, cinco LEDs e dois buzzers; primeiro com baixa tensão e resistor.
3. **Carrinho 2WD:** validar alimentação da ponte H e motor antes de comandar pela ESP32.
4. **Diagnóstico do motor potente:** obter foto de etiqueta do motor, placa de gatilho, tipo de cabo e bateria; não assumir compatibilidade com bateria 21 V.
5. **Câmeras/reaproveitamento:** identificar protocolo da câmera IP, placa da câmera quatro fios, e circuito das máquinas antigas.

## 7. Alertas
- Furadeira aberta com fios e conector aparentando plugue de tomada: **não conectar à rede fora da carcaça ou com terminais acessíveis**.
- Bateria de ferramenta e bateria Ni-Cd são sistemas diferentes. Não ligar carregadores/placas aleatórias em baterias sem identificação.
- Placa ATX pode fornecer correntes altas mesmo em 5 V e 12 V. Conferir polaridade e tensão antes de conectar ESP32 e periféricos.
- Driver L298N para motores TT, não para motor potente de furadeira sem dimensionamento específico.
- Este documento é inventário descritivo, não esquema elétrico nem registro de testes concluídos.
