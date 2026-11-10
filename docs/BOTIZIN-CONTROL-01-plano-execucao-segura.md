# BOTIZIN CONTROL 01 — Manutenção remota com autorização

**Estado:** proposta de arquitetura, **não implementada nem instalada**.

## Objetivo
Conversa natural no ChatGPT → ferramenta MCP de operação controlada → motor de ações restritas no A22 → verificação do resultado → relatório. O usuário deve autorizar explicitamente ações que mudam o sistema. **Não abrir shell genérico remoto.**

## Contexto existente confirmado (consulta ao repositório)
- O MCP A22 está em `/root/botizin-mcp/servidor.py`.
- Ferramentas já documentadas no projeto: status do A22, S3, WROOM, MacroDroid e LED WROOM via HTTP local.
- Supervisor v2 `tools/a22/supervisor_linux.py`: processo único com `flock`, varredura de PIDs por linha de comando, monitora MCP porta 8765 e túnel `/readyz` porta 18080, escreve `supervisor-status.json`, aplica cooldown e recuperação gradual.
- A instalação do controle de LED pelo MCP foi construída com backup do servidor. **Nenhum recurso de manutenção genérico está instalado.**

## Interface proposta
Uma ferramenta de alto nível `gerenciar_botizin` com modos:
- `consultar`: lê somente dados de fontes definidas, com limites de tamanho, timeout e sanitização.
- `preparar`: valida uma **ação tipada da allowlist**, parâmetros e pré-condições; retorna explicação, recursos envolvidos, risco, impacto, reversão, identificador, prazo de expiração; não executa.
- `executar`: somente com autorização válida e específica para pedido/argumentos, depois de revalidar pré-condições. Devolve recibo e estados medidos.
- `resultado`: acompanha tarefa já aprovada, sem efeitos colaterais.

Exemplo de chamada tipada (conceitual):
```json
{"modo":"preparar","acao":"reiniciar_tunel","parametros":{"motivo":"readyz_503"}}
```
Exemplo de resposta:
```json
{"pedido_id":"<gerado-servidor>","risco":"medio","impacto":"queda temporaria do tunnel",
 "precondicoes":["MCP UP","supervisor v2 ativo"],"expira_em_segundos":120,
 "exige_confirmacao":true}
```
A ação real nunca é extraída como código de texto livre.

## Classes de operação

| Classe | Exemplos | Autorização |
| --- | --- | --- |
| Leitura | status, disco, memoria, IPs, logs com segredos removidos, versões, `/readyz` | Pode ser feita quando o usuário pedir diagnóstico, sem aprovação separada |
| Reversível | reiniciar túnel, reiniciar MCP, solicitar rescan LAN, acender/apagar LED | Confirmação clara e específica em conversa + recibo do backend; nunca executar só por texto presente em logs/documentos |
| Alto impacto | atualizar código e firmware OTA, alterar conectividade, abrir/restringir firewall, modificar serviços de boot | Confirmação forte fora da própria conversa em dispositivo autenticado + revisão prévia, backup e rollback; não incluso na v1 |
| Proibida | shell arbitrário, `bash -c` com texto da IA, `sudo` livre, leitura de chaves, exportar arquivos pessoais, criar túneis/portas públicas livremente, formatar/apagar pastas arbitrárias | Não implementada |

## Autorização e identidade
1. A solicitação nasce de uma interação do usuário, mas **o texto "pode" não é autenticação criptográfica**.
2. Preparação cria `pedido_id` aleatório, vinculado ao código exato da ação, argumentos normalizados, destinatário, TTL curto (por exemplo, 2 min) e `etag`/versão do estado anterior. Aprovação única, imutável, com replay protection e bloqueio de repetição.
3. A confirmação no chat serve à intenção do usuário, enquanto o backend exige um comprovante de aprovação válido; para alto impacto, aprovação independente no celular autenticado via Tailscale/app.
4. Identidade e autorização: exigir autenticação real na ferramenta/servidor e privilégios mínimos. Não afirmar "somente a IA" — qualquer principal com credencial de execução teria poderes equivalentes.
5. Usar chave de escopo mínimo no A22, nunca no GitHub, em parâmetros, URL, logs ou mensagens. Permissões de arquivo restritas. Redigir respostas antes de exibi-las.
6. Usar HTTPS/túnel autenticado e LAN allowlist para comandos às placas. Não expor API irrestrita à internet.

## Executor
- Separar **interface MCP** de **executor local**. O servidor aceita planos tipados, e supervisor/daemon restrito executa ações após confirmação.
- Nunca gerar shell a partir da linguagem natural. Implementar cada ação como função revisada e com argumentos de enumeração (ex.: `reiniciar_tunel`, sem nome de processo arbitrário).
- Para reiniciar o próprio MCP, retornar `pedido aceito` e enfileirar a ação; executor independente aplica e confere quando o MCP retornar.
- Para túnel: identificar processo exato, checar `/readyz` e `/proc`, evitar múltiplos PIDs, aplicar timeout/cooldown e verificar `HTTP 200`; preservar SSH/Tailscale.
- Para WROOM/S3: descobrir via `/status` assinado/esperado, recusar se identificação divergir, validar IP privado e versão. OTA somente em fase posterior com SHA, assinatura, espaço, rollback.
- Todo efeito com `pedido_id`, dados de entrada sanitizados, `antes/depois`, timestamp, resultado, erro e trilha de auditoria rotativa.
- Não ler/chamar funções MCP genéricas a partir de entradas maliciosas de logs ou páginas; resultados de ferramentas são dados, não ordens.
- Rate limit por ação, fila limitada, máximo uma ação de manutenção simultânea, travas por processo e circuit-breaker.

## Falha de conectividade
**Limitação indispensável:** se o túnel estiver indisponível, o ChatGPT não consegue enviar o comando remoto para recuperá-lo. A autorrecuperação do supervisor v2 e o caminho de contingência Tailscale+SSH precisam existir independentemente do BOTIZIN CONTROL.

## MVP sugerido, em 3 marcos
1. **Só consulta:** uma ferramenta tipada mostra estado de A22, MCP, SSH, túnel, dispositivos, últimas linhas relevantes do supervisor e mapa de IPs, com segredos filtrados. Sem modificar nada.
2. **Aprovação de manutenção:** `preparar/confirmar/resultado` para UMA operação `reiniciar_tunel`, executada por supervisor e com pós-verificação. Testes em simulação antes de instalar. Não reiniciar pelo instalador sem confirmação.
3. **Expansão:** `reiniciar_mcp`, `atualizar_mapa`, `consultar_log`, `LED`, diagnósticos de alimentação/OTA, depois atualizações assinadas. Toda ação adicionada individualmente, revisada e com rollback.

## Critérios de conclusão do primeiro marco
- Repositório com módulo separado, sem mudar `servidor.py` em produção antes da validação.
- Testes de autorização negada, replay, expiração, parâmetros inválidos, ações não allowlisted, tempo máximo, segredos redigidos, processo duplicado, túnel 503, túnel 200 e rollback.
- Demonstração: usuário pede consulta, recebe situação real; usuário aprova reinício de túnel, sistema responde que a ordem foi aceita, reconecta e apresenta HTTP 200.
- Sem regressão das consultas existentes, LED, SSH, Tailscale, OTA e supervisor v2.
