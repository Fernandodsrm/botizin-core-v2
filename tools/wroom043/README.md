# BOTIZIN WROOM — Pacote de instalacao candidata 0.0.43

**Status em 13/11/2026:** compilacao CI aprovada, firmware arquivado com SHA, testes de PowerShell 5.1 e seletor OTA aprovados. **A 0.0.43 ainda nao foi gravada no hardware do proprietario**; ultima evidencia fisica: WROOM .42 na OTA1, OTA0 .40 preparada para retorno.

## Conteudo deste pacote
- `lab-builds/wroom-voice-stable-043/firmware.bin` — imagem 1.104.992 bytes SHA256 `d462bec420b8ca8c30e9d86be78ad29e8472257c6c62d9c0947037f59650f705`.
- `tools/wroom043/install_stable.py` — valida MAC `30:76:f5:90:67:e0`, flash 4 MB, base BOTIZIN .40 app0, mapa de particoes e hash; salva 4MB backup privado; grava e le somente OTA1/app1 (0x150000); verifica seletor boot, tenta retornar base se teste falhar.
- `tools/wroom043/instalar-botizin-043.ps1` — PowerShell 5.1 um comando: valida S3, WROOM e pacote, USB COM6, usa reset de leitura para retornar .42->.40 se necessario, chama instalador e aguarda confirmacao de boot estavel.
- `tools/wroom043/test_*.py` e workflows GitHub — testes automatizados.

## Como a persistencia funciona
No primeiro boot app1, firmware **arma app0 .40 como proximo boot**, preservando a imagem. Depois de no minimo 60s com Wi-Fi, S3 `/status` de origem certa, OLED responsivo, RAM >90KB e >=25 polls, tenta confirmar app1 como proximo boot (maximo 3 tentativas espacadas 5s). `/status` deve mostrar `VOICE_STABLE_BOOT: CONFIRMED_APP1` e `BOOT_PARTITION: app1`. Se nao confirmar, mantenha WROOM ligada, verifique status e **nao regrave cegamente**; app0 permanece o destino de recuperacao se a armacao inicial deu certo.

**Limites:** ainda nao passou por teste fisico longo ou teste de falhas/brownout com a .43. Nao e prova de '100% estavel'. Manter backup offline. Este metodo nao garante recuperacao de todo problema de bootloader/flash.

## Antes de executar
1. WROOM em USB **COM6** (confira se mudou), S3 0.0.41 ligada e ambas no mesmo Wi-Fi.
2. Confirme .42/app1 + boot app0 **ou** .40/app0+boot app0. Se outro estado, script bloqueia.
3. Notebook Windows com PowerShell 5.1 e Python 3.11 (ja usados no piloto).
4. Nao desligar WROOM durante leitura/escrita. Backup inclui Wi-Fi/NVS e nunca deve ser publicado.

### Instalacao (execucao explicita do proprietario)

Use o commit SHA do branch `wroom/voice-face-stable-043` que contem o pacote verificado:

```powershell
$rev = '12a9e9e747d7e590182688877327b85a6a279d54'
$p = "$env:USERPROFILE\BOTIZIN_WROOM_STABLE_043\instalar-botizin-043.ps1"
New-Item -ItemType Directory -Path (Split-Path $p) -Force | Out-Null
Invoke-WebRequest "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/$rev/tools/wroom043/instalar-botizin-043.ps1" -UseBasicParsing -OutFile $p
powershell.exe -NoProfile -ExecutionPolicy Bypass -File $p -SourceRevision $rev -Port COM6 -Confirm INSTALAR_WROOM_043_OTA1
```

Substituir `12a9e9e747d7e590182688877327b85a6a279d54` pelo commit escolhido no momento do uso; nao usar `main` como origem do binario experimental.

## Diferenca entre fisico e remoto
A WROOM .43 consulta a S3 **diretamente**; nao precisa de A22/notebook para olhos, desde que S3 e Wi-Fi estejam ligados. Audio/transcricao XiaoZhi ainda precisam de microfone e speaker no navegador (hoje notebook). ChatGPT pelo MCP depende do computador rodando `tunnel-client` + `windows_mcp.py`; o codigo Windows no branch `lab/windows-hub-a22-offline-20261113` foi atualizado para reconhecer .43, mas o processo **ja rodando no notebook** ainda requer reinicio controlado para carregar essa alteracao. Nao reiniciar Windows automaticamente como parte do flash.

## Decisao arquitetural pendente
Este pacote mantem app0 .40 de seguranca e app1 .43 de rosto, portanto **reserva a OTA1** e impede carregar um modulo diferente em paralelo nessa mesma WROOM. A consolidacao final como firmware-base OTA0 exige projeto e autorizacao separados.
