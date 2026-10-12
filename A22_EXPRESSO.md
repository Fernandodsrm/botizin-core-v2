# A22 Expresso — ambiente 0.0.34 (histórico 0.0.33)

Primeiro ambiente de conexão direta entre celular e olhos da WROOM.
Menu/Botizin 0.0.29 permanece no seu slot; este ambiente ocupa o outro.
O catálogo mantém PS4, servo e LED disponíveis para reinstalar.

## Teste no A22

1. No menu físico, Ambientes > Atualizar lista > A22 Expresso.
2. Direita prepara o download; direita novamente confirma a instalação.
3. Espere o ambiente reconectar ao mesmo Wi-Fi já configurado na placa.
4. No Chrome, abra http://192.168.0.8/ (sem /status). Se o IP mudar, consulte o roteador ou tente http://botizin-wroom.local/.
5. Toque Feliz, Curioso e Atento. O painel distingue recebimento de aplicação na animação.
6. A expressão escolhida dura oito segundos, mantendo olhares e piscadas, e depois retorna à variedade automática.
7. Botão físico esquerdo ou Voltar ao menu Botizin no painel reinicia no menu preservado.
8. Reabra o ambiente instalado sem download. Sem internet, painel e olhos funcionam se celular e placa continuam no mesmo Wi-Fi local.

O ambiente reutiliza apenas SSID e senha já armazenados em NVS; não inclui credenciais no binário.
Não recebe comandos do aplicativo ChatGPT, não processa voz e não controla servo/motor nesta etapa.
Serviços de voz, memória e processamento no A22 ficam para a etapa seguinte.

## Funcionamento

- Olhos procedurais com oito expressões; renderização em tarefa própria, alvo 16 fps.
- Painel HTML embutido, sem serviços externos, sem aplicativo adicional.
- GET /api/status e /status: estado, confirmação aplicada, RAM e cadência da animação.
- POST /api/expression: expressão enum, chave temporária obtida na página local; sem CORS.
- POST /api/return: retorno ao menu ancorado por SHA e endereço de partição.
- IP/mDNS da placa mantidos; Wi-Fi reconecta sem bloquear a animação.
- Servo GPIO14 e LED GPIO26 ficam desligados; OLED21/22 e retorno32 preservados.
- Retorno confere o mesmo protocolo de âncora NVS dos ambientes já testados.

Teste físico informado pelo Fernando: abriu ambientes LED/servo e voltou ao menu; relatou melhora de estabilidade com WROOM independente.
Isso confirma o fluxo anterior; o teste físico de A22 Expresso ainda está pendente.

## Verificação automatizada

Testes de todas as expressões, duração de oito segundos, retorno automático e relógio atravessando rollover passaram.
O teste executa o JavaScript real da página e confere comando, confirmação aplicada, falha de rede e retorno ao menu.
Compilação: Arduino core 3.3.12, bibliotecas OLED fixadas e validação das partições existentes, sem apagamento de flash.

Compilação aprovada: run 38073862432, fonte 91c536adc74dd31e6607de16ff6f874a2126090a.
Binário: 1.021.696 bytes; margem no slot: 289.024 bytes.
SHA256: fb10267fcd5ae321bf41c236119a2b2d60adc434369eacf6930904c7f7d83419.

## Retorno 0.0.34

Fernando confirmou comandos e alterações físicas nos olhos na 0.0.33; o retorno iniciou Menu 0.0.29 via HTTP, mas o OLED ficou parado até reset EN.
Causa provável investigada: reinício simultâneo à transferência I2C do renderer. Não houve diagnóstico /status do período congelado para provar a causa.
A 0.0.34 pede parada ao renderer, aguarda sua transferência terminar e Wire.end concluir; somente depois reinicia. Botão físico e painel seguem a mesma sequência.
A 0.0.29 continua preservada. Testar múltiplas idas e voltas; se reaparecer, capturar OLED, FACE_FRAMES e FACE_MAX_GAP_MS de /status antes do EN.

Compilação 0.0.34 aprovada: run 38212557249; fonte 1d2c760a7c2af92a84e58b68f05fc56940ecf41f.
Binário: 1.021.888 bytes; SHA256 ccb52a6ce531775a4d0243d3f02dcb1894583d6dc907c39d124a62e0402fb78a.
