# BOTIZIN 0.0.25-OTA3 — compatibilidade experimental

Base: commit histórico **f2ec674d2d354c41b591255ae491b275438615e3** (CORE 0.0.25). A release original `main/releases/0.0.25/firmware.bin` não é modificada.

## Objetivo
Preservar o comportamento histórico da CORE 0.0.25, mas adicionar uma cópia com o gerenciador OTA 3.0 para iniciar APP0/APP1 e instalar versões selecionadas do GitHub sem exigir USB para uma troca intencional entre versões que tenham painel OTA3.

## O que foi adaptado
- Identidade de firmware `0.0.25-ota3` para distinguir da versão original;
- `firmware/ota3_manager.h` compartilhado (catálogo sob demanda, download HTTPS em tarefa temporária, gravação na APP inativa sem alterar o boot, verificações SHA antes e após gravar);
- Controles do catálogo e das APPs no painel existente de Configurações, mantendo Wi-Fi antigo, ThingsBoard, MQTT, RPC e os demais módulos originais no código;
- Consulta OTA automática desativada; antigo `/update` e OTA via `/peer/ota` não fazem gravações neste build derivado.
- Não alterou o layout de partições da CORE original.

## **Limite importante: telemetria da 0.0.25**
O código de ThingsBoard da 0.0.25 depende de `BOTIZIN_TB_DEVICE_TOKEN`, que **não está no repositório público**. O teste CI `ota3-025-compat-ci.yml` compila com um valor fictício `VALIDATION_ONLY_NOT_A_DEVICE_TOKEN`, apenas para provar que o software compila. Esse BIN **não é funcional para telemetria ThingsBoard e NÃO deve ser instalado na S3**.

Para afirmar equivalência funcional com a CORE 0.0.25 real, será preciso executar build seguro com credencial legítima fora do código, medir os serviços em placa e registrar explicitamente o que foi mantido/alterado. Nunca inserir credenciais no catálogo, logs ou no repositório.

## Estado de validação
- Compilação em GitHub Actions: possível (somente software).
- Boot, Wi-Fi, HTTP, OTA, MQTT, ThingsBoard e RPC nesta edição: **não testados na placa**.
- Catalogação como versão aprovada: **bloqueada** até validação física e, no caso de ThingsBoard, configuração segura.

## Segurança / retorno
A 0.0.25 original permanece no GitHub com hash histórico; esta é uma edição distinta. A troca de partição depende da outra imagem possuir OTA3 para repetir o procedimento pelo painel. Bootloader e rollback nativo não garantem recuperação sem USB em todos os casos.

**Sem publicação automática, sem mudança no manifesto da main e sem atualização de placa.**
