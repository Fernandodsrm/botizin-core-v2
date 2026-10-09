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

## Arquivos OTA publicados e conferidos — continuação de 09/10/2026

Workflow de armazenamento 37952187313 concluiu com sucesso. Os arquivos foram gravados em main/wroom/releases/0.0.22 e main/wroom/releases/0.0.23, sem alterar main/wroom/manifest.json.
Ambos os binários públicos foram baixados novamente e seus SHA256 conferem com os registros acima.
O campo published:false do build-record do Menu é o registro original da etapa de compilação; a publicação posterior está registrada aqui.

A oferta pública continua na versão 0.0.21. Não substituir diretamente por 23: o PS4 23 exige retorno íntegro ao Menu 22.
Sequência de implantação: colocar a WROOM no menu atual, conferir estado VALID, instalar Menu 22, observar novo boot/telemetria; só então oferecer PS4 23 e comprovar retorno ao Menu 22.
A sessão do ThingsBoard ainda solicita login; painel preparado ainda não foi aplicado online. Nenhuma placa foi atualizada nesta etapa.

## Primeiro boot físico do Menu 22 — 09/10/2026

Fernando forneceu /status às 13:41 (America/Sao_Paulo): versão 0.0.22, boot 5dd84950e4640e33, uptime 46 s, reset SOFTWARE, app0/ota_0 rodando e selecionada para boot, estado VALID. Wi-Fi OK, mDNS READY, OTA READY, OLED MENU e TELEMETRY HTTP_200. Nenhum campo S3 ou contador de consultas ao peer permanece no status.
A gravação local a partir do Menu 20 escreveu 1270976 bytes, com SHA recebido e lido da flash f33bad604443e37a878099920f4d65020e8c920685dd6c6c98fbd3f31f0686e9 e Update.end TRUE, sem erro.
Margem OTA confirmada: 39744 bytes (antes 17488; ganho 22256). RAM livre 160556, mínima 95912, maior bloco 110580. Pico do ciclo 2422069 us, RPC 2390416 us, nuvem 1328918 us. Estes são valores de um boot curto; não demonstram melhora sustentada de RAM ou latência frente ao boot anterior de 464 s.
Atualizações automáticas permanecem desligadas. Após esta confirmação, main/wroom/manifest.json passou a oferecer PS4 23 no commit 8c953835926a71856c643e0e16a90a9eb5ea6b3f. A instalação depende de consulta e confirmação manual pelo Fernando; não foi enviado comando de instalação remota.
Próximo teste pendente: instalar 23 pela consulta ao GitHub no menu, parear PS4, testar LED, retornar fisicamente ao Menu 22 e reiniciar módulo já instalado pelo menu de slots. O módulo substituirá o antigo Menu 20 em app1; Menu 22 permanecerá em app0. Painel ThingsBoard revisado ainda não aplicado por falta de autenticação.

## Validação relatada e teste de servo — 09/10/2026, tarde

Fernando confirmou o ciclo Menu 22 → PS4 23 → retorno ao menu funcionando perfeitamente. Importou manualmente o widget individual e a atualização do dashboard no ThingsBoard; a captura posterior mostra painel independente sem S3, telemetria do Menu 22 e início de PS4 aceito pelo painel.
Pendência do módulo PS4: Fernando informou que o controle não reconectou sem novo pareamento. Investigar persistência de bonding/reconexão quando trabalhar no PS4; não considerar esse item validado.
Pendência do painel: distinguir módulo ativo com nuvem pausada de telemetria recente do menu. Catálogo de módulos ainda não implementado. Menu 22 reconhece somente pacote 23 para reiniciar módulo instalado, e o painel tem a mesma restrição. Para o teste 24, instalar pela consulta GitHub e confirmação na OLED; depois de voltar, não prometer que o botão de iniciar módulo 23 inicia 24.

Servo SG90: marrom→azul→GND dedicado; vermelho→verde→5V dedicado; laranja→amarelo→S de D14. Fernando confirmou leitura estável aproximadamente 5 V com pontas firmes, sem carga; capacidade da alimentação em movimento ainda não validada. Barramento V geral continua em 3,3 V.
Teste 0.0.24 mantém Menu 22 intacto e retorno validado por digest/NVS. Servo inicia com sinal desativado; OK físico (GPIO25) ativa centro nominal 90°, cima27/baixo33 ajustam 5° por toque entre 60° e 120°, OK desativa pulsos; esquerda32 desativa pulsos antes do retorno. Desativar pulsos não corta alimentação elétrica. Não depende de controle PS4 conectado; PS4 continua acionando LED26.
Teste de lógica host passou: bloqueio por retorno inválido, ausência de ativação automática, debounce, limites, desligamento e falha de driver. Compilação contra Bluepad32 4.1.0 corrigida para construtor C++11 explícito. Teste físico ainda pendente.

### Pacote 24 publicado para teste manual

Build 37968551099 (fonte 13de077384711772bc8d2174ff2ab33a2411ba8d) concluiu; armazenamento 37968766954 concluiu. Binário público baixado novamente: 760960 bytes, SHA256 29b71784dc7e142e58060faa899902aa1e4583e18f7134445330cc6ec2872ca2, margem do slot 549760 bytes. Manifesto principal oferece 24 no commit 226e0720e7cc09b7ef7a0cfad2634f2ffbd1c3b4. Menu 22 não foi alterado. Instalação física/servo ainda pendentes; nenhuma ordem RPC de instalação foi enviada.
