# Pacote preparado: Menu + LED GPIO26, versão temporária 0.0.14

Autorização do usuário em 08/10/2026: testar navegação física e LED GPIO26, fio laranja. Um único firmware de teste ocupa o slot inativo; referência WROOM 0.0.12 preservada no outro. S3 permanece inalterada. LED ativo em HIGH, com resistor em série e GND comum. Nenhum acionamento remoto de LED foi implementado: usuário comanda pelos botões.

LED começa apagado. Abra LED (GPIO26) com direita; direita novamente alterna ligado/apagado. Esquerda sai e apaga. Cima/baixo percorrem áreas do Menu. Voltar ao anterior exige entrar e confirmar com direita. O timer retorna ao baseline após dez minutos independentemente da rede; callback apaga LED antes de reiniciar. Hash, tabela de partições e guardas do retorno são os da prova anterior.

HTTPS agora roda em tarefa separada com stack de 8192 bytes. Tela/GPIO ficam no loop; botões registram eventos por interrupção com debounce de 150 ms. Os campos compartilhados com a tarefa de rede usam atomics; status HTTP não compartilha Strings mutáveis com a tela. Telemetria mantém intervalo de 60 segundos. A mudança de latência precisa ser verificada na placa; sucesso da compilação não prova desempenho físico.

Build aprovado: [37730807051](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/37730807051); fonte 2c0d4e8e21cd844c85332a19fb2c20d8671e28b1; BIN 1.162.112 bytes; margem 148.608 bytes no slot; RAM estática 51.024 bytes. SHA256 ee3117cf69c100a8f420589b66a3866de44fbda79adefc1c806d929b8bd50baf. Testes nativos de navegação, LED e retorno passaram; core 3.3.12 e partições originais verificados.

Na preparação, WROOM ainda reportava 0.0.12, porém sua leitura tinha mais de 20 minutos e RPC não recebeu resposta. A oferta NÃO foi ativada. Usuário foi orientado a desligar/religar WROOM; é necessário confirmar contato atual antes da instalação.

O workflow offer-led-trial.yml somente dispara ao criar/alterar ci/activate_led_trial.json nesta branch. Esse arquivo ainda não existe. Após conexão confirmada, criar o ativador com {"device":"WROOM","version":"0.0.14"}. O workflow verifica o build fixado, baixa seu artefato, confere hash/tamanho, publica BIN em wroom/releases/0.0.14 e oferece o manifesto por cinco minutos com retirada no finally. Validar candidata com boot_id, versão, tamanho e SHA antes de confirmar. Depois acompanhar module_id=menu_led_trial, timer, OLED, GPIO e estado do LED; usuário informa visual e botões. Não declarar instalação ou teste físico concluído antes dessas evidências.

Após a prova, verificar retorno 0.0.12/automático true e retirar gatilho/ativador de oferta. Não substituir o baseline por outro módulo nesta rodada. Se falhar antes de selecionar/armar retorno, cabo pode ser necessário; não garantir recuperação de todas as falhas.
