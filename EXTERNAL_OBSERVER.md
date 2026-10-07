# Diagnóstico externo — primeira etapa

O coletor lê BINs do repositório e histórico já recebido pelo ThingsBoard. Não envia RPC, não consulta o servidor das placas, não publica manifestos e não instala firmware.

Execute `python ci/external_observer.py`. Sem credencial, entrega análise dos BINs e identifica telemetria indisponível. Para ler a nuvem, forneça `TB_READ_API_KEY` pelo ambiente, nunca no código. No GitHub Actions, use um secret com esse nome, preferencialmente de uma conta com permissão apenas de leitura. Não copiar a chave para arquivos, logs ou relatórios.

O workflow é manual e só fica disponível para acionamento normal quando incorporado à branch padrão. Esta branch de revisão não inicia execução periódica. Nenhuma credencial foi gravada no GitHub nesta entrega.

Relatórios preservam timestamps por campo, janela de seis horas por padrão (máximo 24), limite de 2000 leituras por chave com indicação de possível truncamento. Misturam versões quando a janela atravessa atualizações: consultar boot e versão antes de comparar desempenho. Silêncio pode representar rede, servidor, firmware ou alimentação; não determina causa elétrica.

Dentro dos módulos, aproveitar campos atuais: versão, boot, motivo de reset, memória interna livre/mínima, maior intervalo do loop e falhas de envio. Novos campos só após identificar uma lacuna concreta. Sem rede no módulo, observação externa não terá dados novos.

Próximas provas: observar S3 no carregador separado; compilar Menu mínimo; validar retorno na WROOM; LED com pino confirmado; celular A22 como painel; servo com alimentação verificada; carrinho com parada ao perder controle. Comunicação entre placas somente para uma tarefa que precise dela.
