import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
path = root / 'matrix.json'
print('# Dimensionamento por compilação — ' + root.name)
print('\nSondas de bibliotecas; não é o Menu final e não deve ser instalado.\n')
if not path.exists():
    print('Nenhuma medição concluída; consultar logs da compilação.')
    sys.exit(0)
rows = json.loads(path.read_text())
print('| Variante | BIN bytes | Slot % | Margem bytes | RAM estática | Incremento BIN sobre pai |')
print('|---|---:|---:|---:|---:|---:|')
for r in rows:
    print(f"| {r['variant']} | {r['bin_bytes']} | {r['slot_used_percent']} | {r['slot_margin_bytes']} | {r['static_ram_bytes']} | {r['delta_bin_vs_parent']} |")
print('\nIncrementos dependem da ordem e do código compartilhado; não são custos universais de cada biblioteca.')
print('RAM estática não inclui buffers alocados em execução, TLS, framebuffer OLED ou pilhas de tarefas.')
print('O Menu final ainda precisará de instalação validada, catálogo real, confirmação e recuperação.')
