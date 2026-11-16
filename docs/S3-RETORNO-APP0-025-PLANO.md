# BOTIZIN S3 — plano de retorno da LAB para a CORE 0.0.25

Estado: **procedimento de contingência preparado, não executado fisicamente**. Não autoriza instalar LAB.

## Referências comprovadas

- ESP32-S3 N16R8, 16 MB flash, 8 MB PSRAM; firmware físico CORE 0.0.25 executa em `app0 / ota_0` e possui estado `VALID`.
- Manifesto de produção: versão `0.0.25`, binário de **1.259.872 bytes**, SHA-256 `bc68a1c45c43f4b0e63d40767521b50290a664725568cd0e6acfae24c37e049a` (a imagem instalada foi verificada durante a recuperação).
- Tabela de partições reconhecida no firmware e no pacote CORE.1: `otadata` em `0xE000` / `0x2000`; `app0` em `0x10000` / `0x300000`; `app1` em `0x310000` / `0x300000`. Não há partição `factory` nesse layout.
- Backup crítico dos primeiros **65.536 bytes** concluído com hash no notebook privado, sem gravação. Pode conter NVS com credenciais: **NUNCA enviar/publicar**.
- Fonte do CORE 0.0.25 e binário publicado **não foram demonstrados idênticos bit a bit**: referenciar o binário publicado para verificação/restauração física, não assumir equivalência da reconstrução de laboratório.
- A LAB experimental tem `LAB_BUILD: WIFI_RECURSOS_01 (EXPERIMENTAL)` no `/status` e bloqueios de OTA (GitHub, HTTP e peer); ainda não passou teste físico.

## Fase A — pré-verificação, SOMENTE LEITURA (ação atual)

Arquivo local privado: `Verificar-Retorno-S3-025.ps1`. Executar em PowerShell 5.1 com Python 3.11 + esptool 5.1.0 pré-instalados, monitores seriais fechados. Etapas:

1. Localizar e conferir tamanho e SHA-256 **do backup anterior de 64 KiB** na pasta privada.
2. Analisar tabela de partições desse backup e exigir offsets/tamanhos exatos de `otadata`, `app0`, `app1`.
3. Encontrar a porta USB e reconhecer a **mesma S3 física** pelo modelo, flash de 16 MB e MAC conhecido (o identificador não é versionado no repositório).
4. Ler **somente os 1.259.872 bytes iniciais do app0** em `0x10000` e calcular SHA-256. Se não coincidir com o binário oficial, **parar**.
5. Preservar cópia dessa imagem na pasta privada. Em caso de sucesso, imprimir `APP0_ORIGINAL_025_OK` e `RETORNO_APP0_PREPARADO_PARA_PLANEJAMENTO`.

O script **não contém comandos de escrita, erase-region ou mudança de boot**. Uma leitura via USB pode reiniciar/desconectar momentaneamente a placa. Não enviar as cópias, pois o binário pode conter tokens compilados.

## Fase B — contingência futura se um teste LAB falhar (NÃO EXECUTAR AGORA)

Conforme documentação oficial Espressif ESP-IDF, apagar `otadata` faz o bootloader procurar a imagem factory; **sem factory**, tenta o primeiro slot OTA disponível, normalmente `ota_0`. **Isso não é garantia universal:** só considerar após Fase A comprovada e após verificar também a tabela de partições presente na placa no dia do ensaio.

1. Com acesso USB, autenticar **novamente** placa, mapa de partições e integridade `app0`.
2. Antes de qualquer modificação, preservar uma cópia de `otadata` atual para registro.
3. **Apenas sob aprovação explícita e somente na situação de emergência**: limpar **somente `otadata` em `0xE000` com 8.192 bytes** usando método oficial de erase-region, sem `erase-flash`, sem alterar `app0`, `app1`, bootloader, tabela de partições ou NVS. A limpeza muda a seleção de boot e será uma gravação física; não está automatizada nem autorizada neste documento.
4. Reiniciar e confirmar `BOTIZIN CORE V0.0.25`, `RUNNING_PARTITION: app0`, `RUNNING_OTA_STATE: VALID`, Wi-Fi/mDNS/HTTP, painel e memória.
5. Caso a imagem `app0` não seja idêntica à original, **não aplicar o método otadata**: recuperar pelo binário original verificado mediante procedimento separado.

## Antes de qualquer teste físico em app1

- Planejar a **instalação explícita da LAB em `app1`** e seleção de boot sem usar caminhos de OTA inseguros; escrever uma análise de rotas e autenticação, já que o HTTP legado inclui um upload dedicado.
- Não modificar `main/manifest.json` nem o release 0.0.25, e não enviar segredos para workflows CI.
- Conferir tamanho binário LAB, SHA-256, proteção contra OTA e comportamento do bootloader.
- Definir critérios para desligar o ensaio e reverter: ausência de rede/painel, reboot repetido, heap mínima crítica, falha de controle local.
- A CORE.1 deixou bootloader/tabela de partições na flash durante a recuperação; **manter o binário original e o backup de 64 KiB disponíveis offline**.

Referências oficiais:
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html
- https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html

**Proibição atual:** não escrever/alternar qualquer partição da S3 até a pré-verificação ser concluída e o usuário aprovar um teste específico.

## Resultado físico da Fase A — 16/11/2026

**VALIDADO — SOMENTE LEITURA.** Saída do PowerShell fornecida pelo usuário:

- `BACKUP_PRIVADO_64KB: HASH_CONFERE`
- `PARTICOES_OK: app0=0x10000/3MiB; app1=0x310000/3MiB; otadata=0xE000/8KiB`
- `PLACA_CONFIRMADA: COM5 | ESP32-S3 | 16MB | MAC_CONFERE`
- `APP0_ORIGINAL_025_OK: SHA256_CONFERE` (imagem de **1.259.872 bytes** idêntica ao SHA-256 registrado no release oficial)
- `COPIA_APP0_PRIVADA_OK: 1259872 bytes`
- `RETORNO_APP0_PREPARADO_PARA_PLANEJAMENTO`
- `NENHUMA_GRAVACAO: SIM`

**Conseguimos confirmar que `app0` ainda possui o aplicativo original da CORE 0.0.25**, e que os primeiros 64 KB (incluindo NVS, boot e otadata conforme o layout) possuem backup privado validado. O próprio procedimento declara que **não executou nem testou retorno ou rollback**. Nenhum dos binários privados ou credenciais foi transferido para GitHub.

**Próximo critério antes de qualquer gravação:** revisar a forma de selecionar `app1` sem escrever no `app0`, especificar com precisão como recuperar a seleção de boot em caso de falha e realizar revisão do plano de contingência. A aprovação da CI e desta leitura NÃO autoriza instalar LAB. `main/manifest.json` e a imagem 0.0.25 seguem intocados.


## Kit de retorno com trava dupla — 16/11/2026

**Preparado e validado estaticamente, SEM execução na S3**: arquivo privado `Kit-Retorno-S3-025-v1.ps1`. O arquivo de recuperação é entregue ao operador e NÃO é publicado no repositório porque contém o identificador físico da placa. O backup e a imagem original continuam privados.

### Modo padrão (único recomendado agora)

Rodar em PowerShell 5.1, sem `-AplicarRetorno`. Verifica:

- Python 3.11/esptool 5.1.0; existência e SHA-256 do backup 64 KiB e de cópia privada app0;
- identidade da ESP32-S3 pelo MAC físico, 16 MB e porta serial; modelo e flash;
- tabela de partições **lida diretamente da placa**, igual à cópia privada, sem partição `factory`, com `app0`, `app1` e `otadata` nos offsets e tamanhos aprovados;
- imagem real em `app0` lida na hora, tamanho 1.259.872 bytes e SHA-256 igual ao release oficial;
- leitura de `otadata` (0xE000, 8192 bytes), preservando snapshot com hash em pasta privada; sem envio do binário nem tokens;
- termina indicando `PRECHECK_RETORNO_OK: SOMENTE_LEITURA`, `NENHUMA_GRAVACAO_NA_S3: SIM`, `RETORNO_NAO_EXECUTADO: SIM`. Pode ocasionar reboot USB temporário.

### Modo de EMERGÊNCIA futura: bloqueado operacionalmente

O mesmo arquivo contém opção `-AplicarRetorno`, que **NÃO deve ser executada agora**. Ela exige repetir as verificações anteriores e responder **duas confirmações interativas exatas** antes de chamar exclusivamente `esptool erase-region 0xE000 0x2000`. Não contém `write-flash` ou `erase-flash`. Se o erase for iniciado e houver erro, não afirmar que nada foi alterado: o estado de `otadata` pode ser incerto, exigindo inspeção por USB.

A limpeza de `otadata` é uma mudança física, não rollback comprovado no hardware. Em layout sem `factory`, a documentação Espressif prevê boot no primeiro slot OTA disponível (geralmente `ota_0`), desde que a imagem seja válida. O usuário deve verificar posteriormente pelo `/status`: versão 0.0.25, `app0`, estado `VALID`, Wi-Fi e HTTP.

Referências: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html e https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html

### Validações realizadas sem hardware

- O Python embutido passou `compile()`/AST; checagem estática confirmou que o caminho de apagamento fica atrás de `-AplicarRetorno` e das duas confirmações;
- o parser de partições foi testado com dados sintéticos: layout esperado aceito, `factory` e endereço `otadata` divergente rejeitados;
- sintaxe específica do PowerShell 5.1, comportamento do esptool no Windows e recuperação real **ainda dependem de teste físico**. Não considerar esse kit prova de recuperação.

**Próximo passo autorizado na rotina de trabalho:** apenas executar o modo padrão em leitura. A S3 CORE 0.0.25 instalada, a WROOM e os manifestos OTA continuam intactos.

## Checkpoint do kit de retorno — pré-verificação física aprovada

**Log de execução física fornecido em 16/11/2026 — SOMENTE LEITURA:**

- `FERRAMENTAS_OK 5.1.0`
- `BACKUP_64KB_PRIVADO_OK: SHA_CONFERE`
- `COPIA_APP0_PRIVADA_OK: SHA_CONFERE`
- `PORTAS_CANDIDATAS: COM5, COM6`
- **COM6:** esptool reconheceu um dispositivo `ESP32` (não `ESP32-S3`); foi corretamente ignorado. Não usar COM6 para os procedimentos da S3.
- `PLACA_CONFIRMADA: COM5 | ESP32-S3 | 16MB | MAC_CONFERE`
- `PARTICOES_REAL_OK: app0=0x10000 app1=0x310000 otadata=0xE000`
- `APP0_REAL_025_OK: SHA256_CONFERE` (1.259.872 bytes do firmware original)
- `COPIA_OTADATA_ATUAL_OK: 8192 bytes | HASH_SHA256_VERIFICADO`
- `PRECHECK_RETORNO_OK: SOMENTE_LEITURA`
- `COMANDO_ERASE_SIMULADO: erase-region 0xE000 0x2000` **(apenas texto; não foi executado)**
- `NENHUMA_GRAVACAO_NA_S3: SIM`
- `RETORNO_NAO_EXECUTADO: SIM`

**Conclusão:** a integridade do slot estável `app0`, a identificação da placa e a disponibilidade de cópias privadas estão verificadas. **Não foi realizado teste de alternância entre slots, escrita em app1, erase de otadata, reinicialização de emergência ou rollback físico.** O snapshot de `otadata` é privado, não deve ser publicado.

### Porta de aprovação para testar o app1 (ainda não liberada)

1. **Inventariar app1 existente antes de sobrescrever:** confirmar se contém firmware útil, coletar identidade, estado e cópia privada recuperável; nenhum `write-flash` enquanto o conteúdo anterior estiver desconhecido.
2. Validar de forma estática o pacote binário LAB exato (nome de build/versão, SHA-256, core, esquema de partições, credenciais de teste), sem confundi-lo com a CORE 0.0.25 publicada.
3. Desenhar instalação em `app1` que nunca escreve nos endereços de `app0`, bootloader, tabela de partições ou NVS, com duas barreiras independentes antes da seleção de boot.
4. Planejar seleção `otadata` conforme algoritmo do bootloader realmente presente; **não improvisar bytes de seleção OTA** nem usar apenas a ordem dos slots como prova de fallback.
5. Não mudar manifesto de produção, não executar a opção `-AplicarRetorno` e não acionar a OTA antiga pela LAN.
6. Autorizar qualquer teste de escrita somente após inventário e plano reversível, com verificação de RAM/PSRAM, tempo de inicialização e serviços após boot.

**Estado nesta data:** PRÉ-CHECK APROVADO; INSTALAÇÃO LAB NÃO AUTORIZADA, NENHUMA GRAVAÇÃO REALIZADA.
