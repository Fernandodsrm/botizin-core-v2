# BOTIZIN OTA 3.0 — Refinamento final do painel (CORE 0.0.31)

**Estado:** implementação de interface candidata; não instalada na S3 e não considerada validada fisicamente até teste. Base: CORE 0.0.30 com OTA3 e Wi-Fi 2.2 já testados. Apenas branch de desenvolvimento, sem alterar `main/manifest.json`.

## Reclamações reais observadas durante os testes da 0.0.29 / 0.0.30

- O botão **Ler partições** apenas repetia o status. Não havia feedback visual útil ou novo estado.
- Instalar outro BIN no slot inativo não comunicava claramente qual APP estava sendo substituída; a atualização do texto das APPs não era óbvia após a gravação.
- UI apresentava uma sequência técnica difícil de ler, como `6671d0b | EXECUTANDO | próximo boot SIM`.
- O usuário queria **APP0/APP1 com versões claras, papéis explícitos, destino da próxima gravação, estados separados**.
- O resumo superior ("Firmware/slot", "Wi-Fi/HTTP", "OTA/rollback", "RAM") é geral e independente da aba; nomes poderiam ser mais intuitivos.

## O que foi implementado

- **Dois cartões** distintos: `APP 0 · OTA 0` e `APP 1 · OTA 1`, versão em destaque, etiqueta “EM EXECUÇÃO”/“INSTALADA”, estados separados de boot/VALID/suporte OTA3. A UI destaca o cartão da APP ativa.
- O botão renomeado **Atualizar informações das APPs** efetivamente lê `/control/ota3/status`, redesenha cartões e informa hora de atualização. Não cria polling periódico.
- Uma caixa **Instalar uma versão** com campo de catálogo e destino *calculado pela APP em execução*: `Destino: APP 0 (inativa) · substitui CORE...` e botão `Instalar na APP 0 (inativa)` ou `APP 1`. O servidor continua impondo o slot inativo, não apenas o navegador.
- Após clicar instalar, há **progresso visível** na caixa de resultado (fase e bytes) enquanto o usuário iniciou a operação. Ao receber `BUSY NO`, o painel consulta de novo `/control/ota3/status` e redesenha os slots **antes de qualquer boot**.
- Uma caixa separada **Iniciar uma versão já instalada** com comandos idênticos `Iniciar APP 0`/`APP 1` sem instalar. Exige confirmação antes de reiniciar, como antes.
- Detalhes técnicos permanecem disponíveis em um elemento `details`, sem dominar a visão normal. Dados de status não são jogados fora.
- Resumo superior global renomeado para **Versão em execução** e **Segurança de boot / OTA**. Mensagens OTA automáticas irrelevantes substituídas por `VALID · OTA manual`.
- Reconhecimento de histórico `0.0.28`, `0.0.29` e `0.0.30` no build da .31 via `app_elf_sha256` calculado no GitHub a partir de BINs SHA-256 verificados. Novas instalações guardam metadados de versão em NVS somente após flash+hash confirmados.

## Regras de segurança preservadas

1. **Nunca gravar APP em execução**; sempre instalar apenas no slot inativo.
2. **Não trocar boot automaticamente** após instalar; só após clique `Iniciar APP X`.
3. **Sem consulta periódica ao GitHub e sem instalação automática**.
4. Não instalar firmware de versão desconhecida sem catálogo e SHA. A tela de destino não substitui a verificação do firmware.
5. Sem proteger permanentemente 0.0.25 em slot específico; sua versão original continua guardada no GitHub.
6. Sem alterar a WROOM ou a arquitetura A22 nessa candidata.

## Aceite físico do layout

- [ ] Colocar CORE 0.0.30 em execução na APP1; preservar esta versão como retorno, se desejado.
- [ ] Instalar CORE 0.0.31 na APP0 usando o OTA3 da .30, mas **não iniciar imediatamente**.
- [ ] Confirmar na .30: destino APP0 e novo firmware instalado, sem troca de BOOT_PARTITION.
- [ ] Iniciar APP0/CORE .31 apenas depois da validação de flash; conferir HTTP, Wi-Fi, MQTT, versões APP0 .31 / APP1 .30, textos e botões.
- [ ] **Atualizar informações das APPs** deve indicar hora atual e redesenhar com os dois estados.
- [ ] Testar um próximo download OTA3 somente quando houver motivo, para evitar regravar desnecessariamente; quando o teste for aprovado, confirmar que o cartão do slot inativo atualiza antes do reboot.
- [ ] Testar ida .31 → .30 e volta .30 → .31, sem reinstalação; registrar status `VALID` nos dois slots.
- [ ] Registrar prints e resultado no Issue #2.

**Não publicar como release principal e não gravar a S3 automaticamente.**
