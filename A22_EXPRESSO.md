# A22 Expresso — ambiente 0.0.33

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
