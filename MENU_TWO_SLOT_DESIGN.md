# Menu fixo e módulos com dois slots

Proposta solicitada por Fernando em 2026-10-07. O teste S3 usa a 0.0.13 como referência preservada; ela ainda não é o Menu mínimo final.

## Papéis dos slots

Na S3 atual: app1 (0x310000, 3 MiB) preserva a referência/Menu; app0 (0x10000, 3 MiB) recebe o experimental/módulo. Não alterar a tabela para o teste.

- Apenas o Menu instala módulos, sempre no slot de módulos.
- Cada módulo mantém uma função de manutenção para selecionar o slot do Menu e reiniciar.
- O módulo não deve executar OTA genérico: a seleção automática de slot inativo enquanto o módulo está ativo poderia sobrescrever o Menu.
- O Menu confere identidade da placa, tamanho, hash, origem autenticada e destino antes de gravar.
- Configurações ficam em NVS separada, com chaves e formatos versionados por módulo.
- Trocar módulo normalmente basta: o processo de gravação OTA substitui a imagem necessária. Não é preciso apagar toda a placa ou NVS antes.

## Recursos

O slot inativo ocupa flash, mas não executa o aplicativo e não usa a RAM/CPU de execução do módulo. Um reboot reinicializa o runtime; não apaga preferências.

Um módulo PS4 pode manter o retorno local ao Menu sem Wi-Fi/HTTPS/download OTA. O custo exato precisa ser compilado; não assumir que toda biblioteca do sistema desaparece.

O OTA automático fica no Menu. Um módulo com rede desligada precisa voltar por botão, reset ou prazo local para disponibilizar OTA na nuvem novamente. Para receber um comando de retorno pela nuvem enquanto estiver ativo, precisa manter rede, ou ser assistido por outro dispositivo conectado. Não prometer comando remoto instantâneo com rádio desligado.

## Atualizar o próprio Menu

Este é um fluxo diferente de instalar módulos: exige um firmware de manutenção confiável no slot de módulos, que valida e grava o slot do Menu sem apagar a única imagem executável de recuperação. Esse fluxo não está implementado por este teste e precisa de projeto/validação próprios.

## Teste de manutenção 0.0.15

Verificar os bytes e SHA exatos da 0.0.13 na partição anterior, selecionar essa partição como próximo boot antes da rede, armar ESP timer de 600 segundos e bloquear gravações OTA enquanto o experimental roda. Confirmar por telemetria: versão, boot_id, partição em execução, próximo boot e status do temporizador.

O temporizador evita depender do loop principal ou da internet para o retorno normal. Falha anterior à sua armação e falhas graves do sistema ainda precisam de recuperação/rollback específicos; este ensaio não injeta essas falhas.
