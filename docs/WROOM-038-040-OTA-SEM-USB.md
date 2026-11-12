# BOTIZIN WROOM 0.0.38 -> 0.0.40 sem USB (laboratorio)

Nao e uma atualizacao automatica. A ferramenta exige confirmacao explicita em cada gravacao/reinicio e NAO modifica main/wroom/manifest.json.

## Objetivo
Estado inicial: WROOM 0.0.38 no app0, OTA READY e OTA automatico OFF.
Estado final: BOTIZIN 0.0.40 no app0 e modulo LED 0.0.32 no app1.

### Sequencia
1. Baixar e verificar a imagem oficial 0.0.38, 1.288.144 B, SHA256 27739579d6e8dbeb1608b9f0e1200919fd90d1c563aeb4ab1848fc27d629fd13.
2. A partir da 0.0.38 em app0, enviar a imagem 0.0.38 para app1, verificar e reiniciar de maneira separada.
3. Confirmar que a 0.0.38 esta rodando em app1 e que app0 e o proximo destino.
4. Baixar a candidata de laboratorio 0.0.40, 1.069.744 B, SHA256 1a52d58619bdfa8fb7af505c40fc668dc35f233b184ced58f50be41bbe28ef51, sem modificar o manifesto em main.
5. A partir da 0.0.38 em app1, enviar a 0.0.40 para app0, verificar e reiniciar em etapa separada.
6. Confirmar 0.0.40 no app0, antes de habilitar agente de modulos no A22.
7. Somente depois, instalar o LED 0.0.32 no app1 com autorizacao fisica e verificar BACK -> app0.

Nao usar o modulo LED como ponte; a 0.0.38 tem servidor de atualizacao OTA funcional em ambos os slots.

## A22: comandos dentro do Debian PRoot
Arquivo: tools/a22/ota_bridge_038_040.py

python3 /root/botizin-mcp/ota_bridge_038_040.py inspect
python3 /root/botizin-mcp/ota_bridge_038_040.py prepare38

Apos checagens especificas e supervisao, executar uma etapa de cada vez:

python3 /root/botizin-mcp/ota_bridge_038_040.py upload38 --confirm GRAVAR_038_OTA1
python3 /root/botizin-mcp/ota_bridge_038_040.py reboot38 --confirm REINICIAR_038_OTA1
python3 /root/botizin-mcp/ota_bridge_038_040.py prepare40
python3 /root/botizin-mcp/ota_bridge_038_040.py upload40 --confirm GRAVAR_040_OTA0
python3 /root/botizin-mcp/ota_bridge_038_040.py reboot40 --confirm REINICIAR_040_OTA0

O script valida identidade, OTA READY, particao ativa e destino, tamanho, hash, journal da atualizacao e boot apos reinicio. Usar os comandos de gravacao somente mediante verificacao atual e autorizacao, e nao encadear as etapas em um unico comando.

## Seguranca e limites de recuperacao
- O envio usa HTTP local, sem autenticacao criptografica propria: apenas LAN domestica confiavel.
- O A35 via Tailscale/SSH administra o A22; isso nao transforma o HTTP da WROOM em uma interface segura para internet.
- Falha de energia, boot ou Wi-Fi depois da gravacao pode exigir USB. Nao afirmar rollback garantido: o Arduino pode confirmar a imagem ao iniciar antes de validar a conectividade.
- Sem backup integral de flash pelo A22: o firmware 0.0.38 nao oferece esse endpoint, por isso manter uma rota fisica de recuperacao.
- Nao tocar S3, NVS de provisionamento, particoes, serviços MCP ou tunel.
- Candidato de teste do GitHub sera uma pre-release distinta do manifesto main; nunca flash sem SHA correto.

Estado: preparacao remota e testes offline nao alteram hardware. A gravacao real depende de comandos especificos de usuario no A22.
