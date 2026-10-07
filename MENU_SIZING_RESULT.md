# Menu: dependências cabem; implementação final ainda precisa de medição

Em 07/10/2026, o run [37680619211](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/37680619211) concluiu com sucesso 14 variantes na WROOM e 14 na S3. Fonte compilada: ca0541df7a3c04d166f8a9f7504f3461fced711b. Arduino CLI 1.5.1, core ESP32 3.3.12, OLED com versões fixadas no workflow. Chip e os dois slots OTA foram verificados nos arquivos gerados. Nenhum firmware foi instalado; manifestos atuais permanecem inalterados.

| Comparação | WROOM BIN bytes | WROOM slot | WROOM margem bytes | S3 BIN bytes | S3 slot |
|---|---:|---:|---:|---:|---:|
| Firmware atual publicado | 1.248.160 | 95,23% | 62.560 | 1.166.784 | 37,09% |
| Dependências do Menu + lista/botões simples | 1.139.664 | 86,95% | 171.056 | 1.125.072 | 35,77% |
| Mesmo Menu + servidor web de teste | 1.164.992 | 88,88% | 145.728 | 1.149.808 | 36,55% |
| Base + APIs de retorno local | 269.840 | 20,59% | 1.040.880 | 303.280 | 9,64% |

## O que mais pesa nesta comparação

Na WROOM, incrementar a sonda base com Wi-Fi acrescentou 607.408 bytes; incluir TLS sobre Wi-Fi acrescentou 176.576; HTTPS sobre essa combinação acrescentou 34.432. OTA/SHA sobre HTTPS acrescentou 4.896. OLED isolado acrescentou 37.008 sobre a base. Retorno local por seleção de partição e reinício acrescentou 832 sobre a base (800 na S3).

São incrementos de sondas específicas, não custos universais e independentes: bibliotecas compartilham código. Os pequenos incrementos de POST à nuvem e consulta HTTP do peer não representam o custo completo de telemetria, RPC, autenticação, retries, telas e supervisão existentes. Não usar esses números para estimar automaticamente a remoção de funções do firmware atual.

RAM estática da sonda Menu na WROOM: 50.704 bytes; com servidor: 51.128. Isso não mede buffers durante TLS, heap mínimo, framebuffer alocado, pilhas de tarefas, uso de CPU ou latência. Nenhuma inferência de desempenho do carrinho foi feita por compilação.

## Decisão proposta a partir da medição

As dependências cabem nos slots atuais. Não há justificativa nesta etapa para trocar bootloader ou tabela de partições. Manter download seguro e atualização sem cabo no Menu; permitir que cada módulo carregue somente o que precisa. O maior ganho potencial dos módulos locais está em dispensar as dependências de rede e suas operações, mantendo apenas retorno local.

Escolher servidor web leve para configuração pelo celular é possível na sonda, mas o painel completo não foi medido. O Menu real terá catálogo limitado, validação de metadados, confirmação, hash/chip/partições, instalação somente no slot do módulo, reconexão e recuperação. Ele será compilado e medido novamente antes de publicar firmware.

A sonda de retorno não valida identidade/hash do Menu e não representa um pacote de manutenção definitivo. Esses controles são obrigatórios no produto e podem acrescentar tamanho. Nenhuma sonda é distribuída como firmware instalável.

## Próxima entrega concreta

Preparar Menu funcional e primeiro módulo LED, sem instalá-los. Validar instalação pelo Menu no slot oposto, retorno ao Menu validado, configurações preservadas e ausência de gravação no slot protegido. Medir o build final e apresentar versões, tamanho, ações e procedimento de recuperação para confirmação antes de publicar uma oferta OTA. A ligação e GPIO do LED precisam estar confirmados antes de acioná-lo.

## Evidência

- [WROOM: todas as variantes](validation/menu-sizing-2026-10-07/wroom.md) e [JSON](validation/menu-sizing-2026-10-07/wroom.json).
- [S3: todas as variantes](validation/menu-sizing-2026-10-07/s3.md) e [JSON](validation/menu-sizing-2026-10-07/s3.json).
- Ambientes de compilação registrados junto dos JSONs; logs e matrizes nos artefatos do run. Somente relatórios e logs foram enviados como artefatos, sem BINs de sondas.
