# Menu 0.0.17: retorno visível

Preparado após relato físico do usuário: ao confirmar a saída não houve indicação visível. Nesta versão, a confirmação pela segunda Direita apaga o LED, mostra Retorno confirmado / Voltando... / Anterior: 0.0.15 por pelo menos 750 ms, registra MENU_RETURN_CONFIRMED_RIGHT_TO_0.0.15 na serial e reinicia. Esquerda permanece cancelar/voltar. Não acrescenta gravações NVS, comandos de rede nem link com a S3.

Teste temporário: retorno automático de 600 segundos, guardado pelo tamanho e SHA da WROOM 0.0.15 verificada (1.289.888 bytes, 5428313389336d48831e9228fb98d18603f848d44fdd1557a3de337f1b45580c). Não instalar sobre outra versão. Firmware anterior preservado e OTA do teste bloqueado. Compilação não oferece instalação automaticamente. A indicação precisa de validação física pelo usuário.
