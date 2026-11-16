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

