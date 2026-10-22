# BOTIZIN ESP32-S3 — Wi-Fi Manager 0.0.21

**Objetivo:** usar a S3 sem tela, botões, WROOM ou novos pinos, mantendo OTA GitHub existente e oferecendo cadastro de nova rede pelo celular quando o Wi-Fi falhar.

## Funcionamento
1. O firmware inicia STA usando credenciais antigas do ESP32 (compatível com 0.0.17); caso o usuário já tenha configurado outra rede via portal, lê SSID e senha do namespace NVS `botizin-net` e a prioriza.
2. Se a estação estiver conectada ao Wi-Fi, a placa mantém HTTP `botizin-s3.local/status`, MQTT e OTA; o ponto de acesso de configuração fica desligado.
3. Se não conectar após aproximadamente 35s de desconexão, inicia uma rede WPA2 **BOTIZIN-S3-XXXX** e um portal web em **http://192.168.4.1/**. `XXXX` é sufixo identificador da própria placa.
4. O telefone se conecta à rede temporária (pode informar "sem internet"; mantenha conexão) e abre o endereço. A interface oferece uma lista de redes 2,4 GHz escaneadas e também aceita SSID digitado manualmente, junto com a senha.
5. Envia o formulário via POST `/wifi/save`. A rota só aceita solicitações originadas do IP local da interface AP, quando a rede de configuração está ativa.
6. Salva SSID e senha na NVS da própria S3, responde ao telefone, reinicia e tenta a nova rede. Se não conseguir conectar, volta ao modo de configuração; não necessita USB para tentar novamente.
7. Ao conectar, desliga a rede temporária, sincroniza hora NTP e volta ao funcionamento normal com OTA GitHub.

## Senha da rede BOTIZIN-S3-XXXX
A S3 não tem display, então cria na primeira inicialização do 0.0.21 uma senha aleatória de **16 caracteres**, guardada em NVS (`botizin-net/ap_password`). A senha **não está no código-fonte nem no MQTT nem no /status**. Ela é exibida pela porta USB serial 115200 como `WIFI_SETUP_PASSWORD_USB_ONLY: ...` em cada boot; anote-a ou coloque etiqueta na base. Depois disso a configuração em outros lugares é só pelo celular.

**Importante:** não publicar captura/print da senha. Evitar redes Wi-Fi abertas com portais comerciais, pois a S3 não implementa aceite desses termos.

## Confirmação OTA e segurança
- Compatível com partições app0/app1 de 3 MiB; não muda bootloader, NVS antiga ou `otadata`.
- Herda política de confirmação tardia; no primeiro boot sem roteador conhecido, permite que um **SoftAP funcional** satisfaça o requisito de conectividade local, evitando rollback antes de abrir o portal.
- Continua verificando rede para OTA GitHub por HTTPS; sem Wi-Fi com Internet não faz OTA.
- A página de cadastro só está disponível na rede AP, protegida com senha individual. A conexão HTTP do portal não tem TLS, mas a rede Wi-Fi é protegida por WPA2. Não expor portas do roteador para acesso externo.
- O firmware S3 continua com outros serviços HTTP legados (incluindo upload OTA LAN) que precisam de auditoria de autenticação antes de uso público.
- Credenciais do roteador são armazenadas em NVS da placa, não no GitHub. NVS criptografada não foi implementada nesta versão.

## Testes efetuados
- Build CI **39744400091**: compilação bem sucedida para S3 com Arduino core 3.3.12, 1.224.864 bytes, SHA-256 `486ea4740e598b6917d7207a45f12beb1d7d645b4caec71cc8ddc727244c73c2`.
- Política OTA host 15/15 passou; verificação de símbolo forte `verifyRollbackLater` no ELF; invariantes Wi-Fi e versão examinadas.
- Estes testes **não comprovam ainda** conexão real de um celular ao portal, gravação de credenciais, nem adoção física da 0.0.21.

## Testes físicos de aceitação
1. Conferir `BOTIZIN CORE V0.0.21`, `WIFI: OK`, `OTA: READY`, `RUNNING_OTA_STATE: VALID` após atualização.
2. Com USB serial uma vez, guardar senha exclusiva `WIFI_SETUP_PASSWORD_USB_ONLY`.
3. Somente então desligar o roteador temporariamente (ou levar S3 a local sem Wi-Fi conhecido), após 35s procurar `BOTIZIN-S3-XXXX`.
4. Conectar pelo smartphone, abrir `http://192.168.4.1/`, selecionar rede 2,4 GHz e salvar; confirmar que a S3 reconecta e desliga AP.
5. Revalidar o OTA `UP_TO_DATE 0.0.21` e eventual correção de brownout.
6. Não testar automaticamente em laço novas imagens se não houver evidências de /status ou MQTT e recuperação pelo USB.
