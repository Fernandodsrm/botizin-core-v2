# BOTIZIN CORE 0.0.28 — fechamento do Wi-Fi (candidata)

## Decisões aprovadas
- S3 independente do A22, notebook ou servidor para gerenciar rede.
- Até 5 redes conhecidas gravadas apenas na S3. Última rede conectada é preferida; outras só são tentadas se a conexão atual cair. Nunca troca por sinal mais forte.
- Teste de rede: conectar, observar e voltar automaticamente; só salva por ação explícita.
- O painel mostra IP do teste e botões Copiar / Abrir; oferece redes lembradas sem mostrar senhas, opção Esquecer e detalhes técnicos opcionais.
- O AP de recuperação, após autenticar pela senha individual da placa, tenta a nova rede sem reiniciar, mostra o IP obtido e fica disponível por até dois minutos após sucesso para permitir copiar o endereço.
- Uma senha de login para o painel; ações protegidas. MQTT, OTA, status, mDNS e app0 estável preservados.

## Manual rápido
1. Normal: use http://botizin-s3.local/ na mesma rede; se não resolver, tente o IP observado no teste (pode mudar).
2. Se a placa não conseguir rede conhecida: após cerca de 35 segundos, conecte-se à rede protegida BOTIZIN-S3-XXXX e abra http://192.168.4.1/.
3. No portal, selecione rede 2,4 GHz e digite senha. Após conectar, copie o IP recebido; antes de abrir, mude o notebook para a mesma nova rede.
4. Se o nome .local não resolver e o IP anterior mudar, mas a S3 ainda estiver conectada: consulte lista de clientes do roteador ou use a recuperação via USB. O AP não liga apenas por falha de resolução mDNS.
5. Senha do AP é única e gerada na S3, mostrada pela USB. Guarde em local seguro; não existe senha universal.

## Segurança e limites
- Senhas Wi-Fi ficam na NVS local, não são mostradas no painel, /status ou logs; a flash não está criptografada neste firmware.
- HTTP local não tem TLS: não abrir porta do roteador para a internet, usar apenas em redes confiáveis.
- O serviço mDNS iniciado não garante que hotspots ou Windows resolvam o nome.
- O IP é recebido por DHCP e pode mudar sem reinicialização do roteador.
- O AP e STA compartilham rádio; mudar de canal para uma rede nova pode causar breve interrupção da página de recuperação.

## Preservação
- 0.0.25 fica em app0; candidata 0.0.28 deve ser gravada apenas em app1 via USB na COM4.
- Manifesto de produção continua em 0.0.25; não liberar 0.0.28 como atualização automática.
- Todos os cinco testes da 0.0.27 já foram aprovados em placa; mudanças da 0.0.28 ainda precisam ser testadas fisicamente.

## Checklist físico antes de encerrar
1. Iniciar CORE 0.0.28 em app1, conferir Wi-Fi, HTTP, memória e OTA/MQTT.
2. Testar hotspot 2,4 GHz pelo painel: conectar, registrar IP, retornar, copiar link; não salvar automaticamente.
3. Salvar hotspot, reiniciar, confirmar que o hotspot e a Thor estão lembrados.
4. Desligar hotspot e verificar fallback automático para Thor sem senha, sem reboot.
5. Desligar redes conhecidas e verificar o AP. Selecionar rede nova e confirmar IP em página de recuperação.
6. A S3 não deve trocar para rede mais forte enquanto Thor estiver funcionando.
7. Verificar sessão única e bloqueio de operações sem senha; sem credenciais em /status.

**Compilação CI não substitui testes físicos na ESP32-S3.**