# Dimensionamento por compilação — botizin-size-wroom

Sondas de bibliotecas; não é o Menu final e não deve ser instalado.

| Variante | BIN bytes | Slot % | Margem bytes | RAM estática | Incremento BIN sobre pai |
|---|---:|---:|---:|---:|---:|
| base | 269008 | 20.52 | 1041712 | 22116 | None |
| wifi | 876416 | 66.87 | 434304 | 45680 | 607408 |
| wifi_tls | 1052992 | 80.34 | 257728 | 47432 | 176576 |
| wifi_https | 1087424 | 82.96 | 223296 | 48792 | 34432 |
| wifi_https_ota | 1092320 | 83.34 | 218400 | 49112 | 4896 |
| wifi_https_ota_nvs | 1093472 | 83.43 | 217248 | 49112 | 1152 |
| wifi_https_ota_nvs_json | 1101872 | 84.07 | 208848 | 49120 | 8400 |
| oled | 306016 | 23.35 | 1004704 | 23676 | 37008 |
| module_return | 269840 | 20.59 | 1040880 | 22124 | 832 |
| menu_libraries | 1139664 | 86.95 | 171056 | 50704 | 37792 |
| menu_web | 1164992 | 88.88 | 145728 | 51128 | 25328 |
| menu_cloud | 1140032 | 86.98 | 170688 | 50704 | 368 |
| menu_peer | 1139872 | 86.97 | 170848 | 50704 | 208 |
| menu_all | 1165968 | 88.96 | 144752 | 51128 | 26304 |

Incrementos dependem da ordem e do código compartilhado; não são custos universais de cada biblioteca.
RAM estática não inclui buffers alocados em execução, TLS, framebuffer OLED ou pilhas de tarefas.
O Menu final ainda precisará de instalação validada, catálogo real, confirmação e recuperação.
