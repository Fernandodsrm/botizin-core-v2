# Medir antes de instalar

Esta etapa compila sondas na nuvem com core ESP32 3.3.12 e bibliotecas OLED iguais ao build atual. Não publica manifestos, não inclui credenciais, não envia comandos e não instala firmware. Os artefatos entregues contêm apenas medições e logs; sondas BIN não são distribuídas.

Comparações: base; Wi-Fi; TLS com certificados; HTTPS; OTA e SHA256; NVS; JSON; OLED; retorno local por partição; dependências do Menu com lista e leitura de botões; servidor web; envio à nuvem; consulta HTTP de outra placa; conjunto completo. O acesso ao código das bibliotecas é mantido por um sinal volatile, sempre falso no sketch, para evitar eliminação pelo compilador. Nenhuma sonda deve ser gravada.

`menu_libraries` mede dependências e uma lista simples, não um produto pronto. O Menu real acrescentará catálogo limitado, validação de tamanho/hash/chip, confirmação explícita, gravação somente no slot do módulo, configuração Wi-Fi, recuperação e política de atualização. Esses custos serão medidos novamente no build final. Configuração pelo celular com servidor exige comparar `menu_web`.

Política proposta: somente o Menu baixa e grava módulos no outro slot. Módulo retorna ao slot de Menu identificado e validado; não usa um download para voltar e não grava o Menu preservado. Reboot não apaga NVS. Identidade module_id e versão são separadas; seleção de módulos não depende só de versão numérica maior. Atualização do próprio Menu exige fluxo de manutenção separado.

O automático de atualização da versão atual permanece intacto. A frequência e o comportamento no Menu novo serão definidos antes de instalar, preservando atualização sem cabo. Não usar uma sonda para substituir a referência atual.

Executar localmente: `python ci/size_lab/build_matrix.py --board wroom --out /tmp/botizin-size-wroom` com Arduino CLI e dependências instaladas. `--generate-only` apenas valida geração, sem compilar. Na branch menu-sizing-20261007, push dos arquivos da sonda dispara somente compilação e armazenamento de relatórios.
