# BOTIZIN S3 — LAB 01: Wi-Fi + Recursos (somente consulta)

**Início:** 16/11/2026. **Base de fonte:** commit `f2ec674d2d354c41b591255ae491b275438615e3` (fonte histórica identificada como CORE 0.0.25; equivalência bit a bit com binário publicado não demonstrada). **Branch:** `lab/s3-wifi-recursos-readonly-01`.

## O que foi implementado nesta branch

- **Painel:** novas abas `Wi-Fi` e `Recursos`, sem tirar abas e controles existentes. Cards sob demanda, com hora, mensagens de carregamento e de falha próximas aos botões.
- **Wi-Fi:** rota `GET /read/wifi` limitada à interface STA conectada, retorna JSON somente com SSID (não a senha), IP, RSSI dBm, canal, gateway, DNS, estado mDNS e AP de recuperação. Sem scanning, testes ativos, NVS, temporizadores ou armazenamento adicional. A aba possui um atalho para a configuração autenticada **já existente**, não uma nova permissão de mudança.
- **Recursos:** reutiliza `GET /status` e os diagnósticos já calculados: heap interna livre/mínima, maior bloco, PSRAM livre, margem OTA, maior intervalo do loop e idade da amostra. Estima tamanho da imagem do firmware subtraindo a margem OTA do slot de 3 MiB; a estimativa **não representa armazenamento livre geral**.
- **Segurança:** nenhuma rota administrativa nova; login e permissões de `control_routes.h` permanecem. O endpoint de leitura só aceita requisição vinda da interface STA e não expõe segredos. HTTP não usa criptografia, por isso o painel só é apropriado para LAN confiável e **não deve ser exposto à internet**.

## O que AINDA NÃO foi implementado

- Seleção/varredura de novas redes na aba, teste ativo de gateway/DNS/internet, troca automática segura e rollback de configuração.
- Medição de tempo de conexão, histórico detalhado de desconexões e custo por módulo em tempo de execução.
- Política nova de OTA, rollback robusto, autenticação HTTPS ou integrações WROOM/XiaoZhi.
- **Nenhuma instalação física foi feita**. A S3 em produção permanece CORE 0.0.25, e a WROOM não foi alterada.

## Validação e orçamento de recursos

Workflow `.github/workflows/s3-wifi-recursos-lab-ci.yml`:
1. Validação estática de rotas, IDs do HTML, ausência de polling e edição de credenciais; `node --check` no JavaScript; testes de eventos e confirmação OTA.
2. Duas compilações reproduzíveis com `esp32:esp32@3.3.12`, `PubSubClient@2.8` e token de teste não funcional: baseline `f2ec674` e branch LAB.
3. Comparar bytes finais dos binários produzidos no mesmo ambiente, limite provisório máximo de +64 KiB; salvar `size-comparison.txt`. **Sem push, publicação, mudança do manifesto, nem artifact flashável intencional**.
4. Custo RAM/PSRAM/latência adicionais **pendentes de teste físico**. Não inferir custo de um card a partir do total da memória de uma leitura.

## Próximos critérios de aceite

- CI concluída sem falhas, tamanho da imagem dentro do slot.
- Revisar com o usuário o novo painel e confirmar que as ações perigosas continuam bloqueadas.
- Planejar teste físico somente depois de preparar retorno à base, preservar `app0`, conferir `app1` e backup/recuperação de NVS sem exposição de segredos.
- Não promover esta branch para `main` nem modificar `manifest.json` para anunciar uma versão LAB.

## Resultado da primeira CI (16/11/2026)

**Aprovada:** [GitHub Actions #43830699345](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/43830699345). Testes estáticos, sintaxe JavaScript, testes unitários existentes e as duas compilações Arduino 3.3.12 foram concluídos com sucesso, sem publicar release.

| Compilação, mesmo ambiente | Tamanho |
|---|---:|
| Base histórica compilada com token de teste | 1.264.224 bytes |
| LAB Wi-Fi + Recursos com token de teste | 1.271.568 bytes |
| **Crescimento da imagem** | **+7.344 bytes** |
| Espaço restante estimado no slot LAB de 3 MiB | 1.874.160 bytes |

A comparação em CI emprega o mesmo compilador, flags e token de teste nas duas imagens. **Não comparar diretamente os 1.264.224 bytes desta compilação-base com os 1.259.872 bytes do binário de produção e atribuir a diferença ao módulo.** O custo adicional de RAM, PSRAM e latência na S3 permanece desconhecido até medições físicas. Esta aprovação é de compilação/estática, não de funcionamento em placa.


## Auditoria de segurança e preparo de recuperação (16/11/2026)

**Revisão de código sem acesso à placa:**

- `GET /read/wifi` não contém ações que modifiquem NVS, SSID, interface ou rádio; aceita apenas pedidos na interface STA conectada; a resposta contém SSID e dados de rede, mas **nenhuma senha**. Como HTTP não é criptografado, SSID/IP podem ser lidos por clientes da rede local: comportamento deliberado de consulta sem login, não adequado para internet pública.
- Os novos cards não criam sessões, comandos de gravação, scanning ou timers. As rotas administrativas antigas permanecem protegidas por autenticação; **o firmware herdado ainda possui outras rotas que alteram OTA/NVS**, portanto não se deve chamar a imagem inteira de *firmware read-only*.
- **Marcador LAB** incluído em `/status`: `LAB_BUILD: WIFI_RECURSOS_01 (EXPERIMENTAL)`. O cabeçalho histórico `CORE V0.0.25` continua por compatibilidade com código existente; **não confundir esta imagem LAB com release publicado**.
- **Bloqueio antes de instalar:** o firmware herdado possui OTA automática habilitada e fluxos de atualização/recuperação ainda não ensaiados fisicamente nesta branch. Não gravar LAB em `app1` nem acionar OTA até que o modo LAB tenha proteção explícita contra substituir `app0` e exista plano de retorno verificado.
- As compilações CI usam token fictício de validação. **O binário CI não está preparado para retomar telemetria ThingsBoard de produção**; não afirmar que serviços externos funcionariam após instalá-lo.

### Backup USB pequeno, como alternativa ao backup integral que falhou

Ferramenta de **somente leitura**: `tools/s3/Backup-Critico-S3-64KB.ps1`. Usa esptool local 5.1.0 e Python 3.11, lê apenas `0x0..0x0FFFF` (64 KiB): área que compreende bootloader, tabela de partições, NVS e otadata no layout atual. **Não faz cópia do aplicativo app0, app1, armazenamento de dados ou dos 16 MiB inteiros**. Preserva dados sensíveis em `Downloads/BOTIZIN-RECUPERACAO-S3/BACKUP-PRIVADO`, verifica exit code e tamanho e reporta hash SHA-256; não confiar em `.partial` caso falhe.

O binário original app0 `releases/0.0.25/firmware.bin` já está publicado com SHA256 `bc68a1c45c43f4b0e63d40767521b50290a664725568cd0e6acfae24c37e049a` e foi verificado por esptool na recuperação. Para restaurar **todo** o estado, outras partições podem precisar de backup próprio e procedimento validado. **A leitura USB pode resetar a placa e interromper momentaneamente o Wi-Fi, mas não grava a flash**. Nunca compartilhar o backup, porque inclui possível senha Wi-Fi, token e estado OTA.

**Saída desta etapa:** apenas após `BACKUP_CRITICO_64KB_OK` e checklist técnico aprovado será planejado ensaio no slot `app1`. Não há gravação física agendada ou autorizada neste documento.

## Proteção OTA LAB e backup físico confirmado

**Backup físico confirmado em 16/11/2026:** ferramenta Windows v3 identificou ESP32-S3 na COM5 com MAC esperado, 16 MB flash; leitura `0x00000..0x0FFFF` terminou `BACKUP_CRITICO_64KB_OK`, **65.536 bytes**, hash SHA-256 gerado localmente. Arquivo permanece na pasta privada `Downloads/BOTIZIN-RECUPERACAO-S3/BACKUP-PRIVADO`; não carregar em repositórios/IA. Não substitui o backup completo de flash.

**Proteção incluída no código LAB (não na versão instalada):** `firmware/lab_policy.h` define `BOTIZIN_LAB_OTA_DISABLED=1`; bloqueia `queueOtaCheck`, `checkInternetOTA`, `installInternetOTA`, `beginUpload`, `handleUpload`, o upload HTTP em `/update` e o agendamento de atualização automática no loop. As rotas autenticadas de verificação/instalação OTA e ações OTA de peer retornam `LAB_OTA_DISABLED`. `/status`, telemetria e diagnóstico peer indicam que a OTA automática está desativada. O painel LAB sinaliza visualmente essa política e impede uso dos botões OTA. **O firmware estável 0.0.25, seu manifest principal e os arquivos da WROOM continuam sem alteração.**

**Limitações ainda existentes:** bloquear gravação OTA *dentro da LAB* não impede gravação USB externa nem prova que o `app0` ficará intocado durante o processo de seleção de `app1`. O bootloader Arduino pode confirmar a imagem antes de a rede ser validada. Falta pacote USB com validação de partição, procedimento de boot temporário, caminho de retorno documentado e teste físico em condições controladas. A LAB também é compilada com token ThingsBoard fictício, então não equivale a uma atualização de produção. **Não instalar só porque a CI passou.**
