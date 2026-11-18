# BOTIZIN OTA 3.0 — manutenção 0.0.30 (candidata de laboratório)

Base: CORE 0.0.29 já instalada e validada na S3, em `app0`. A 0.0.28 está gravada em `app1`, sem OTA3.

## Objetivo limitado desta revisão

1. **Versão correta nos slots**: a versão do `esp_app_desc_t.version` veio como `6671d0b` nos dois BINs; era o identificador do build, não a versão funcional BOTIZIN. Agora:
   - slot em execução usa `BOTIZIN_VERSION`;
   - imagens históricas 0.0.28 / 0.0.29 recebem identificação por `app_elf_sha256`, gerada no GitHub a partir de BINs guardados e SHA-256 conhecidos;
   - futuras instalações pelo OTA3 gravam ID, versão e assinatura interna em NVS, **somente após SHA e boot inalterado verificados**; metadata só é aceita quando a assinatura ainda coincide com a imagem;
   - se a imagem não corresponder a nenhuma identificação, o painel mostra `NAO_IDENTIFICADA`, nunca inventa uma versão.
2. **Estado UNDEFINED explicado**: `esp_ota_get_state_partition` pode retornar `ESP_ERR_NOT_FOUND` quando ainda não há registro do slot no `otadata`. O painel informa `SEM_REGISTRO_OTADATA` e a leitura do cabeçalho separadamente (`APPx_IMAGE_DESC`). Isso **não afirma que a imagem foi inicializada e validada**. Na seleção de boot, o ESP-IDF valida a partição.
3. **Finalização com comprovante**: o progresso mantém o último `BYTES: concluído/total`, sem zerar para `0/0`. Apenas tentativas de instalar firmware (e não atualização do catálogo) gravam `OTA3_LAST_RESULT`, `OTA3_LAST_BYTES` e `OTA3_LAST_SHA256` em NVS. A verificação de SHA é feita também após gravação na flash.

### Preservações

- Consulta ao GitHub somente a pedido do operador; sem instalar automaticamente.
- Slot ativo nunca sobrescrito. `esp_ota_begin/write/end` não seleciona próximo boot; seleção ocorre apenas por botão de boot autenticado.
- Wi-Fi 2.2, MQTT, HTTP e demais módulos da 0.0.29 permanecem no código.
- CORE 0.0.29 app0 não foi modificada por esta implementação; `main/manifest.json` ainda intocado.
- 0.0.25 histórica e 0.0.25-ota3 experimental continuam separadas. Não precisamos instalar a edição de teste com credencial ThingsBoard fictícia para testar a alternância: a 0.0.29 e a 0.0.30 já incluem OTA3.

## Publicação / teste

- `candidate/ota3-0.0.30` contém o código; CI compila usando Arduino core 3.3.12 e disponibiliza BIN somente na branch candidata.
- Somente após CI aprovado + SHA-256 registrado, inserir a candidata no **catálogo manual da CORE 0.0.29**, com rótulo explícito `CANDIDATA / TESTE FÍSICO PENDENTE`.
- Teste físico planejado: a 0.0.29 **em app0** deve instalar a 0.0.30 em **app1**, mantendo `BOOT_PARTITION app0`. Apenas após confirmação explícita do usuário, iniciar APP1, conferir HTTP/Wi-Fi/MQTT, versões APP0/APP1 e posterior retorno a APP0.
- Não alterar flash, otadata nem WROOM remotamente sem que o usuário dê o comando no próprio painel.
- O primeiro boot continua usando as confirmações nativas do Arduino/ESP-IDF e não fornece garantia absoluta de recuperação sem USB.

**Status: candidato; não declarar aprovado sem teste físico.**
