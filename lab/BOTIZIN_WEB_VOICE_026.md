# BOTIZIN LAB — protótipo web 0.0.26 (isolado)
**Estado: código de pesquisa, NÃO é XiaoZhi executando IA e NÃO foi gravado na S3.**

## Objetivo
Manter o CORE 0.0.25 já validado em ota_0 e estudar a experiência de rosto, texto e voz do navegador em /botizin enquanto um firmware de laboratório eventualmente roda em ota_1. A aplicação Android/Chrome precisa abrir a página diretamente por HTTPS confiável para acessar o microfone; Remote Desktop exibe a tela do notebook, mas não transforma o microfone do celular no microfone do site remoto.

## Gates de proteção
- Nunca alterar main/manifest.json de produção ou publicar 0.0.26 automaticamente. GitHub separado é só fonte.
- OTA de gravação desativado no LAB; retornar ao CORE antes de outra atualização para impedir sobrescrita de ota_0.
- Endpoint POST /control/return-core deve exigir autenticação ativa, validar SHA-256 do binário oficial 0.0.25 nos bytes físicos da partição ota_0 e verificar a imagem antes de mudar o boot. Botão exige confirmação humana.
- Para a primeira gravação, bancada presencial, alimentação estabilizada, backup flash completo e procedimento USB de recuperação. Nem o GitHub nem rollback por boot substituem isso.
- Sem instalar XiaoZhi upstream até compilar board custom ESP-IDF e comprovar <=3MB + compatibilidade da tabela de partições e retorno.
- O prototype /botizin é apresentação: CSS, Web Speech API e respostas locais predeterminadas. NÃO há LLM, backend XiaoZhi, Opus ou áudio físico do ESP32 neste estágio.
- Página HTTP em LAN não acessa microfone via getUserMedia normalmente. HTTPS com acesso autenticado é pré-requisito para experiência pelo celular.
- Nenhum arquivo daqui deve incluir credenciais, tokens ou senha individual.

## Sequência real
1. CI GitHub compila a imagem 0.0.26 sem publicar.
2. Inspecionar ELF, <=3 MiB, partições Arduino originais, retenção do OTA native startup.
3. Em casa, backup 16 MB da S3, hash, teste de recuperação, verificar versão/slot originais.
4. Instalar somente após aprovação, testar /status, /botizin, resposta de texto/TTS e retorno controlado a CORE.
5. Configurar HTTPS privado no A22, abrir no A35 e testar microfone real.
6. Implementar backend real XiaoZhi (serviço autenticado e codec de áudio), preservando proteção do CORE.
