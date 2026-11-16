# BOTIZIN 2.0 — Wi-Fi Manager, Fase 1

Estado: **código na branch LAB; build/ensaio físico pendentes no momento da escrita**. Não alterar o firmware estável da S3 nem a WROOM.

## Problema antigo

A rota `/control/wifi-save` persistia `ssid` e `pass` na NVS imediatamente e reiniciava. Um erro de senha/rede deixava o dispositivo dependente do portal de recuperação. Resultado de ação poderia ficar distante do botão no painel.

## Fluxo Wi-Fi 2.0 (implementado no código)

1. Na aba **Wi-Fi**, autenticar com a senha administrativa local que o dispositivo já possui.
2. **Procurar redes** usa apenas a ação manual autenticada `POST /control/wifi-scan` e limita a apresentação a 20 redes. Não existe scan periódico.
3. Selecionar SSID ou digitá-lo manualmente, informar a senha e clicar em **Testar conexão**.
4. `POST /control/wifi-test` executa a etapa de agendamento: captura credenciais da conexão anterior em RAM, guarda nova rede apenas em RAM e responde **202** antes de trocar STA. A resposta não contém senhas.
5. O loop inicia a conexão candidata aproximadamente 1,8 s depois; por até **25 s** tenta conectar. Durante esse intervalo, o gerenciador existente não abre o portal de recuperação por engano.
6. Se conectar, o estado se torna `CONFIRM_REQUIRED` com limite de **120 s**. O usuário pode conectar o navegador à nova rede, visitar `http://botizin-s3.local/`, autenticar e consultar o estado.
7. Ao clicar em **Confirmar e salvar**, `POST /control/wifi-confirm` grava na NVS apenas com a conexão candidata ativa e validada. O resultado fica junto aos botões.
8. Senha incorreta, queda da conexão ou ausência de confirmação até o prazo: a S3 tenta retornar à conexão anterior usando as credenciais em RAM. NVS anterior não é alterada.
9. Sem credenciais antigas recuperáveis, o teste é **recusado** (`OLD_NETWORK_PASSWORD_UNKNOWN`). Isso protege instalações legadas em que o Arduino não permita consultar a senha da conexão atual; não promete rollback inexistente.
10. Reinício manual enquanto o teste está ativo é bloqueado pelo painel. A recuperação AP antiga permanece para falhas persistentes de conexão.

## Endpoints LAB

| Endpoint | Método | Permissão | Efeito |
|---|---|---|---|
| `/read/wifi` | GET | STA local | Leitura de SSID, IP, RSSI, gateway, DNS, mDNS |
| `/control/wifi-scan` | POST | Administrador | Busca redes somente quando solicitado |
| `/control/wifi-test` | POST | Administrador | Inicia tentativa em RAM, sem alterar NVS |
| `/control/wifi-state` | POST | Administrador | Consulta `WIFI_TRIAL_STATE`, resultado e segundos restantes |
| `/control/wifi-confirm` | POST | Administrador | Grava credenciais somente após nova rede conectada |
| `/control/wifi-save` | POST | Administrador | **Bloqueada** na LAB: fluxo imediato legado desativado |

O portal físico de recuperação `/wifi/save` continua disponível **apenas no AP de setup** quando a S3 estiver desconectada. É diferente do fluxo autenticado de painel.

## Testes de aceitação na S3 (ainda não realizados)

- Senha correta, nova rede: estado `QUEUED` → `CONNECTING` → `CONFIRM_REQUIRED` → `COMMITTED`.
- Senha incorreta: volta à rede antiga em até aproximadamente 25 s de tentativa (mais reconexão).
- Sem confirmar: retorna à rede anterior após 120 s de espera.
- Reboot em período de teste não deve persistir credenciais novas; após reiniciar carrega a antiga do armazenamento.
- Rede desconhecida sem dados antigos suficientes: teste negado, sem desconectar.
- Verificar que scan manual não expõe segredos, não dispara continuamente e não vaza histórico de senhas.
- Depois de conexão/reversão: conferir mDNS, HTTP local, heap interno livre/mínimo, maior bloco e PSRAM; comparar custo de flash com base histórica.
- Verificar que as rotas OTA da LAB seguem bloqueadas e `main/manifest.json` não foi alterado.

**Importante:** Wi-Fi 2.0 ainda não provou funcionamento em hardware real. Em particular, a troca de redes muda IP e pode derrubar a aba atual; a confirmação exige novo acesso à S3 e nova autenticação. O browser não faz reconexão automática, propositalmente.

## Próximo objetivo

Após CI verde, preparar pacote de teste específico para a LAB e ensaiar rede nova/reversão. **Não repetir backup ou inventariar o outro slot sem uma necessidade surgida no teste.**

## Primeira CI verde — código Wi-Fi 2.0 (16/11/2026)

[Execução GitHub Actions #43959690703](https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/43959690703): **SUCCESS**. Asserções sobre o gerenciador por estados, leitura local, autenticação de rotas, ausência de polling e script JavaScript passaram. Compilações da fonte-base e da LAB com `esp32:esp32@3.3.12` e `PubSubClient@2.8` concluíram.

| Imagem compilada na mesma CI | Tamanho |
|---|---:|
| CORE 0.0.25 de referência (recompilada com token fictício) | 1.264.224 bytes |
| LAB com Wi-Fi 2.0, Recursos e bloqueio OTA | 1.262.688 bytes |
| Delta líquido | **−1.536 bytes** |
| Margem em slot 3 MiB | 1.883.040 bytes |

A economia líquida **não** é o custo isolado do Wi-Fi, pois também envolve alterações de política OTA na LAB. O RAM/PSRAM extra, queda de pacotes, retorno automático e confirmação ainda não foram medidos em hardware. **Nenhuma atualização foi publicada ou instalada**; os arquivos `main/manifest.json` e `releases/0.0.25/firmware.bin` estão fora desta branch.
