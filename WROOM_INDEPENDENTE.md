# WROOM independente — decisão de 09/10/2026

## Referências preservadas antes da limpeza
- Código com S3 interligada: branch arquivo-wroom-s3-codigo-20261009, commit a117740ae17b5e25e4c7a78f4dae53a942973345.
- Publicação e arquivos OTA: branch arquivo-wroom-s3-releases-20261009, commit 8b7ad83a8313040d688915302f35bdb3f24c3011.
- Menu WROOM 0.0.20: 1293232 bytes, SHA256 f955a7a2f96cd12b27168650f2bc3a4ddabb3ca609b8dac38f8e6e3f7696a39d.
- Módulo PS4 0.0.21: 755024 bytes, SHA256 eb929d8cc58ccbbf90813df3837b267fbacc5bda29c04902c118fec397bd50e3.
- Partição de aplicativo WROOM: 1310720 bytes. Não confundir ocupação de flash com RAM ou CPU.

## Validação física relatada pelo Fernando
Em 09/10/2026 Fernando relatou dois ciclos completos: consulta ao GitHub, instalação do PS4, pareamento, acionamento rápido do LED inclusive ao segurar o botão, retorno pelo botão esquerdo ao menu OTA e repetição bem-sucedida.
Esta evidência é um relato físico do usuário; não representa novas medições de telemetria nesta sessão.
Servo e motores ainda não foram testados com este firmware. A versão 0.0.21 não contém acionamento de servo ou motores.

## Escopo autorizado
A WROOM será independente e móvel. A S3 deixa de integrar todo o projeto da WROOM e será usada em outro projeto independente, somente online e por painel. Nenhum módulo da WROOM depende da S3.
A base deve preservar atualização sem cabo, verificação de imagem e retorno entre menu e módulo.
O Botizin visual deverá ser a tela de descanso/entrada do menu, se couber após medição.

## Limpeza a implementar e medir
1. Remover do firmware básico consultas, descoberta, emparelhamento, comandos remotos, páginas OLED e campos de telemetria exclusivos da S3.
2. Preservar o acesso local à própria WROOM, Wi-Fi, painel essencial, atualização manual e retorno de módulo.
3. Auditar páginas, fontes e dependências realmente incorporadas ao binário; remover duplicações demonstradas.
4. Compilar na mesma configuração da base e comparar bytes do firmware e margem de slot. Medir RAM e latência no hardware quando disponível.
5. Validar consulta e instalação OTA, integridade do retorno ao menu e operação PS4 sem S3 ligada.
6. Atualizar o painel para retirar dados da S3 e apresentar apenas estados reais da WROOM.

Não prometer economia de bytes antes da compilação. Não modificar partições ou o mecanismo de retorno sem análise e autorização específica.
A limpeza foi implementada e compilada no commit b5e7d9c61ee8910d5abc24ffe1b65eb6d98cb1aa. Ainda não foi instalada na placa. O painel revisado foi preparado; aplicação no ThingsBoard depende de autenticação.
Não alterar o manifesto público nem instalar uma nova versão automaticamente nesta etapa.

## Sequência após a base independente validada
1. Base OTA preservada e validada.
2. Botizin visual mínimo no menu.
3. Servo: ligação e posições simples, depois controle PS4.
4. Carrinho: motores e parada por perda do controle.
5. Celular: painel/texto, depois voz e IA externa.

## Compilação validada — 09/10/2026

| Imagem | Bytes do arquivo OTA | Margem no slot |
|---|---:|---:|
| Menu 20 preservado | 1293232 | 17488 |
| Menu 22 independente | 1270976 | 39744 |
| PS4 23 para Menu 22 | 755024 | 555696 |

Redução do Menu: 22256 bytes. Esta é a diferença entre arquivos OTA compilados, não uma medida de RAM ou carga de CPU. RAM em execução, recuperação Wi-Fi e latência do hardware ainda precisam de validação física.

Menu 22 SHA256: f33bad604443e37a878099920f4d65020e8c920685dd6c6c98fbd3f31f0686e9.
PS4 23 SHA256: d0b88834186518403a176894a820ab2b7a0bc54cf9de49fa3ae1271906b12113.
Builds aprovados: 37908127080 (Menu), 37908127144 (PS4).
Os testes de navegação local, cancelamento/expiração/confirmação OTA e amostragem diagnóstica passaram. O controlador do painel também passou nos testes locais.
A tabela de partições foi mantida e conferida por ambas as compilações.

## Compatibilidade e implantação posterior

O PS4 21 exige Menu 20. O novo par é Menu 22 + PS4 23, que exige âncora íntegra do Menu 22. Não iniciar PS4 21 esperando retorno ao Menu 22.
Primeiro instalar e comprovar Menu 22; depois oferecer PS4 23 e verificar retorno físico ao Menu 22. Preservar os arquivos 20/21 e não alterar manifesto ou política automática sem revisão da sequência.
O manifesto público e as placas não foram alterados nesta etapa.

## Telemetria para os próximos passos

- Menu: amostra local a cada 5 s e envio de telemetria a cada 60 s; RAM livre/mínima/maior bloco, margem OTA, intervalo máximo do ciclo; duração máxima/última/média e falhas de Git, nuvem, RPC, OLED e coleta.
- Botizin: comparar custo de desenho e intervalo do ciclo com a base, sem consultas adicionais de rede para animações.
- Servo e carrinho: exigir ligação elétrica conferida, testes com carga e parada segura por perda do controle. Telemetria não substitui medição elétrica com multímetro.
- PS4: Wi-Fi não é iniciado; não existe telemetria remota simultânea. Validar OLED, serial ou retorno ao menu. Não adicionar consultas de nuvem ao controle de tempo crítico.
- Celular: validar troca de rede e reconexão antes de adotar voz/IA externa.
