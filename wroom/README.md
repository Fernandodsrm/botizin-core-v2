# BOTIZIN WROOM independente

ESP32 clássico com 4 MiB de flash. Nenhuma consulta, descoberta ou dependência da S3.

Menu 0.0.22: Wi-Fi, painel, HTTPS OTA, OLED e botões; telemetria de recursos e tempos.
Módulo PS4 0.0.23: Bluetooth dedicado, LED GPIO26 e retorno pelo botão GPIO32, sem iniciar Wi-Fi.
OLED: SDA21/SCL22. Botões: direita25, cima27, esquerda32, baixo33.

A tabela de partições permanece com dois slots de 1310720 bytes. O menu instala no outro slot e registra sua âncora de retorno; o PS4 verifica a âncora, hash e seleção de boot antes de iniciar o Bluetooth.

As versões 20/21 com integração antiga estão preservadas nas branches arquivo-wroom-s3-codigo-20261009 e arquivo-wroom-s3-releases-20261009. O PS4 21 não é compatível com a âncora do Menu 22.

Credenciais Wi-Fi e token da própria WROOM ficam na configuração NVS; não são compilados nas imagens. Configuração inicial usa provision_usb.py. Configuração móvel de Wi-Fi por página local ainda não foi implementada.

Leia WROOM_INDEPENDENTE.md na raiz para builds, tamanhos, limites da validação e sequência de implantação. Builds desta branch não ativam o manifesto público nem atualizam placas automaticamente.
