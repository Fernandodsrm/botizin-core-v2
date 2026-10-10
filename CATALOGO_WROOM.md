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
