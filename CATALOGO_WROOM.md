# Catálogo WROOM — Menu 0.0.29

O Botizin e o menu ficam juntos. O segundo slot contém um ambiente por vez.
A lista pode ser atualizada pelo GitHub e fica em NVS para consulta offline.
Novos ambientes podem ser publicados no catálogo sem recompilar o menu (limite atual: oito).

Ambientes separados: PS4 + servo 0.0.30, Servo pelos botões 0.0.31, LED pelos botões 0.0.32.
PS4 mantém L1 como habilitação do servo e desliga PWM com pacote ausente.
Servo: cima/baixo alteram 15 graus entre 15 e 165; direita para o PWM.
LED: direita alterna GPIO26. Esquerda retorna ao menu em todos os ambientes.
OLED21/22, servo14, LED26, direita25, cima27, esquerda32, baixo33 preservados.

Na placa: Menu > Ambientes > Atualizar lista. Selecione o ambiente e confirme.
Já instalado: abrir sem rede, mediante verificação SHA e endereço do slot.
Outro ambiente: preparar e confirmar download; substitui exclusivamente o outro slot.
Retorno: ancora NVS do menu é verificada por SHA do firmware, sem lista fixa de versões.
A lista não armazena os binários: offline pode iniciar o ambiente instalado, não baixar outro.

No ThingsBoard: importar a nova versão do widget existente; Atualizar lista > Ver ambientes.
Selecione o ambiente > Preparar download > Instalar. Abrir instalado inicia o ambiente reportado.
O painel é online e os ambientes são offline; durante sua execução, use a OLED e os botões físicos.
O retorno ao Botizin restabelece Wi-Fi, telemetria e comandos do painel.

Verificações: navegação, vinculação de confirmação a boot/versão/SHA/prazo, diagnóstico e geometria do rosto;
teste do controlador do painel incluindo catálogo, candidata de ambiente e confirmação exata.
Compilação com partições existentes e SHA do pacote; teste físico de ida e volta ainda necessário.

No detalhe de um ambiente instalado, baixo prepara download/atualização; direita confirma.
Direita, sem preparação de download, abre a versão já instalada offline.

## Pacotes publicados e verificados

Menu 0.0.29: 1.286.144 bytes, margem de 24.576 bytes no slot.
SHA256: 97150c7ed17b93ce4af148ee2d88fdff77cc29f4e71882fa3b42f1d76478238b
Fonte: 67163f4665c3633bea6cb7c253e4183924d84d4e; build 38038677846.

PS4 0.0.30: 761.360 bytes; SHA256 225fcc4a132bc8918adb3374b7392e4706c8a6e210b51ee0f1f8a75ff9688c25.
Servo 0.0.31: 758.992 bytes; SHA256 2f3c152373c097b93941304cec74f2782efdfb30cee50cf8478102fb1a7d20ff.
LED 0.0.32: 753.328 bytes; SHA256 a45d4963571d144b0a316e787b7977e3594cd8c0f9907dceefc207e897d7564b.
Fonte dos ambientes: 8f2c81ffd85e17b0c72c1d52a397981017aa6ed0; build 38038303027.
Publicação de pacotes e catálogo: build 38038894448.

Teste físico proposto: instalar menu29, atualizar lista, instalar LED32, alternar LED, voltar ao Botizin, abrir instalado sem download e repetir sem rede.
Depois testar Servo31 e PS4/servo30.
