# BOTIZIN S3 CORE 0.0.27 — Wi-Fi 2.1 / UX simplificada

## Problema comprovado na 0.0.26

O usuário criou hotspot 2,4 GHz no Galaxy A22 e solicitou um teste. A S3 registrou `CONFIRM_TIMEOUT` e `ROLLED_BACK`, comprovando associação à rede candidata e retorno à Thor. Porém, o painel deixava a senha limpa, a aba com dados antigos, botão de salvar bloqueado e status dependente de consultas manuais. O notebook permanecia na Thor enquanto a S3 atravessava outra sub-rede, de modo que a página não podia receber respostas. Após 10 minutos, o login retornava `AUTH_REQUIRED_OR_EXPIRED` e precisava autenticar outra vez na aba Wi-Fi. O painel não mostrava a informação que o `/status` já tinha.

## Mudanças nesta candidata

1. **Teste com retorno automático**: `QUEUED → CONNECTING (até 25 s) → OBSERVING (3,5 s) → RETURNING (até 30 s) → TEST_OK/TEST_FAILED`. Não grava a NVS durante o teste.
2. **Status como fonte do painel**: `/status` inclui estado, motivo da falha, alvo, IP obtido no teste, SSID da rede atualmente conectada e possibilidade de salvar. O HTML renderiza o estado com mensagens compreensíveis; nada depende de copiar logs.
3. **Consulta automática APENAS enquanto o teste do usuário está rodando**: tentativas limitadas por até 90 segundos. A ausência de HTTP enquanto a placa troca Wi-Fi é tratada como parte esperada do processo, não erro de senha; ao retornar à rede original, a tela apresenta o desfecho.
4. **Salvar é outra ação**: depois de `TEST_OK`, o botão `Salvar rede testada para próximo reinício` é habilitado temporariamente (janela de 3 min). Grava NVS somente ao clicar e confirmar. Não troca a conexão ativa nem reinicia nesse momento. A permanência da rede salva após reinício requer teste físico separado.
5. **Uma única senha na entrada**: a rota `/` serve uma tela de login quando a sessão não é válida. O sucesso cria cookie `HttpOnly; SameSite=Strict` associado à sessão em RAM e IP do navegador. O painel inteiro só é servido após login; controles continuam exigindo sessão. A sessão dura 10 minutos de inatividade e expira. Não há campos de senha duplicados nas abas.
6. **Compatibilidade**: mantenha `/status` e `/read/wifi` acessíveis para diagnósticos locais como antes (não estão atrás do login do painel), o MQTT observador e a OTA original. Não há ThingsBoard nem RPC legado.
7. **Sem publicação de produção**: `main/manifest.json` permanece em 0.0.25. A versão 0.0.27 é uma candidata de teste separada.

## Segurança e limites reais

- O painel usa **HTTP sem TLS**. A senha e o cookie não têm criptografia ponta a ponta; use somente na rede doméstica confiável, sem redirecionamento público de portas.
- O login protege a **interface HTML** e operações autenticadas, não constitui proteção universal a todos os endpoints legados. `/status` permanece leitura pública local para não quebrar diagnósticos existentes.
- O teste só comprova **associação Wi-Fi e um IP** na rede candidata, por pelo menos 3,5 s. Não mede alcance de internet, DNS externo ou 4G.
- Se a antiga rede não reconectar em até 30 segundos, `RECOVERY_FAILED` habilita a recuperação existente por AP. O navegador pode permanecer desconectado.
- A mudança do Wi-Fi da própria S3 inevitavelmente interrompe a acessibilidade da página na outra rede; o novo fluxo não exige que o notebook mude de rede para **testar**, apenas que aguarde o retorno.
- A edição de um firmware em `app1` durante execução de `app1` não deve ser feita via OTA comum, que escolheria `app0` (atualização destruiria a 0.0.25 original). O instalador de teste de 0.0.27 usa **USB explícito no endereço 0x310000** e não altera `app0`, nem `otadata`.
- Compilação e CI não substituem validação física de conexão, recuperação ou segurança.

## Como testar depois de instalar

Na rede Thor, entrar em `http://botizin-s3.local/` uma única vez e abrir Wi-Fi. Selecionar hotspot 2,4 GHz do A22, digitar senha e clicar **Testar e voltar automaticamente**. Sem trocar Wi-Fi do notebook, aguardar o resultado do próprio painel (até 90 s). Deve mostrar **TESTE APROVADO**, IP da rede do A22 e **volta à Thor**, ou falha com motivo. Sem novo login enquanto a sessão local permanecer válida.

Opcionalmente, clicar **Salvar rede testada para próximo reinício**. Só depois, em um segundo teste separado, reiniciar a S3 e confirmar persistência. Não é necessário salvar a rede A22 para validar o modo teste.
