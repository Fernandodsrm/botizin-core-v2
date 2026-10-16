# Procedimento padrão: OTA solicitado no Live

Validado em 16/10/2026, com Fernando, na ESP32-WROOM 4 MB.

## Resultado observado

Pedido por voz no Work gerou a versão 0.0.35 a partir do código da base Menu 0.0.29 (commit 67163f4665c3633bea6cb7c253e4183924d84d4e). Mudança funcional mínima: linha `LIVE_TEST: TESTE LIVE 35` no relatório HTTP de status. Não foram acrescentados comandos de servo ou novas expressões.

Fonte do teste: branch `live-ota-0.0.35`, commit 1509224d93e47269ee5aa2ae5c109926028bf8cc. Compilação GitHub Actions 38872826554 concluída com sucesso; quatro testes existentes aprovados. Firmware: 1286176 bytes, SHA256 233e66b6dda7c3712b379601f5a42c6b221e003e7653a535a7464e50ccbd6aa1. Marcador e SHA conferidos no binário baixado. Pacote e manifesto publicados no repositório.

Login seguro no ThingsBoard com Google foi concluído pelo usuário. O painel recebia telemetria da 29, mas a lista de compatibilidade do widget Independente terminava na 22. Foram acrescentadas somente 0.0.29 e 0.0.35 à lista principal; verificações de origem, frescor, correlação, candidata, boot e SHA foram preservadas. Código implantado preservado em `wroom/panel/independent-live35-widget.js`.

Pelo painel autenticado, o assistente enviou consulta, obteve candidata 0.0.35, enviou confirmação e recebeu aceite. Telemetria posterior mostrou versão 0.0.35, partição app1 e confirmação automática do widget condicionada a novo boot e OTA VALID. Fernando também relatou ver o download e a versão 35. Não precisou confirmar a instalação pelos botões físicos. Busca automática permaneceu desligada.

## Sequência para repetir

1. Identificar placa e versão atual por telemetria recente. Conferir o código-fonte correspondente, evitando confundir o conteúdo antigo da main com o fonte usado em cada release.
2. Definir alteração pequena e verificar a autorização do usuário para publicação e instalação. Não tratar exemplos de hardware como pedidos de acionamento.
3. Preparar fonte em branch própria, compilar e conferir testes, alvo ESP32 clássico, tamanho, partições preservadas, manifesto e SHA do binário. Não publicar um pacote que falhou.
4. Publicar pacote verificado e atualizar o manifesto OTA no caminho utilizado pela placa. Preservar releases anteriores. Publicação não equivale a instalação.
5. Abrir ThingsBoard por canal disponível e autenticar pelo fluxo seguro se necessário. Não presumir que a sessão persiste nem pedir senhas no chat.
6. Conferir identidade e frescor da WROOM. Se o painel rejeitar a versão, verificar compatibilidade no código antes de corrigir uma lista; não forçar controles desabilitados ou remover verificações.
7. Enviar Consultar e recuperar Ver resultado. Conferir versão e candidata retornadas pela placa. A candidata expira; obter outra consulta se necessário.
8. Enviar Instalar usando candidata, boot e SHA vinculados. Manter alimentação durante a gravação. Aceite do comando não prova instalação.
9. Aguardar versão nova, novo boot e OTA VALID em telemetria posterior. Se faltar evidência, informar pendência e investigar antes de repetir instalação.

## Limites e próximos testes

Este teste comprova execução ponta a ponta nesta sessão do Live e neste dispositivo; não garante acesso permanente, toda ferramenta em qualquer modo de voz ou ausência de falhas futuras. O IP local da casa não foi acessível diretamente pelo ambiente do assistente; os comandos foram pelo ThingsBoard.

A 35 é referência do teste concluído, ainda sem certificação de estabilidade prolongada. Próximos testes propostos, não executados: permanência por 30 minutos sem reset inesperado; recuperação após desligar e religar; recuperação de Wi-Fi; verificação do retorno à 29 pelo procedimento apropriado; comando remoto simples com efeito físico somente após conferir hardware e alimentação. Não é necessário gerar outra versão apenas para testar voz novamente. A integração automática de estados ouvindo/pensando/falando continua não implementada.
