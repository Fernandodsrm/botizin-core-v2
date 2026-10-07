#!/usr/bin/env python3
"""Compile sizing sketches only. Never upload, erase, publish, or connect to a port."""
import argparse
import csv
import hashlib
import json
import re
import shutil
import subprocess
import struct
from pathlib import Path

FEATURES = ['WIFI', 'TLS', 'HTTP', 'OTA', 'NVS', 'OLED', 'WEB', 'CLOUD', 'PEER', 'JSON', 'MENU', 'RETURN']
VARIANTS = [
    ('base', None, []),
    ('wifi', 'base', ['WIFI']),
    ('wifi_tls', 'wifi', ['WIFI', 'TLS']),
    ('wifi_https', 'wifi_tls', ['WIFI', 'TLS', 'HTTP']),
    ('wifi_https_ota', 'wifi_https', ['WIFI', 'TLS', 'HTTP', 'OTA']),
    ('wifi_https_ota_nvs', 'wifi_https_ota', ['WIFI', 'TLS', 'HTTP', 'OTA', 'NVS']),
    ('wifi_https_ota_nvs_json', 'wifi_https_ota_nvs', ['WIFI', 'TLS', 'HTTP', 'OTA', 'NVS', 'JSON']),
    ('oled', 'base', ['OLED']),
    ('module_return', 'base', ['RETURN']),
    ('menu_libraries', 'wifi_https_ota_nvs_json', ['WIFI', 'TLS', 'HTTP', 'OTA', 'NVS', 'JSON', 'OLED', 'MENU']),
    ('menu_web', 'menu_libraries', ['WIFI', 'TLS', 'HTTP', 'OTA', 'NVS', 'JSON', 'OLED', 'MENU', 'WEB']),
    ('menu_cloud', 'menu_libraries', ['WIFI', 'TLS', 'HTTP', 'OTA', 'NVS', 'JSON', 'OLED', 'MENU', 'CLOUD']),
    ('menu_peer', 'menu_libraries', ['WIFI', 'TLS', 'HTTP', 'OTA', 'NVS', 'JSON', 'OLED', 'MENU', 'PEER']),
    ('menu_all', 'menu_libraries', FEATURES),
]
BOARDS = {
    'wroom': ('esp32:esp32:esp32:FlashSize=4M,FlashMode=dio,CPUFreq=240,EraseFlash=none', 1310720),
    's3': ('esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,CPUFreq=240,USBMode=hwcdc,CDCOnBoot=cdc,EraseFlash=none', 3145728),
}


def parse_compile_summary(text):
    program = re.search(r'Sketch uses (\d+) bytes', text)
    ram = re.search(r'Global variables use (\d+) bytes', text)
    return int(program.group(1)) if program else None, int(ram.group(1)) if ram else None


def generate(folder, flags):
    folder.mkdir(parents=True, exist_ok=True)
    name = folder.name
    template = Path(__file__).parent / 'probe' / 'probe.ino'
    (folder / (name + '.ino')).write_bytes(template.read_bytes())
    (folder / 'features.h').write_text('#pragma once\n' + ''.join(
        f'#define LAB_{f} {int(f in flags)}\n' for f in FEATURES), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', default='arduino-cli', help='Caminho do Arduino CLI')
    parser.add_argument('--config-file')
    parser.add_argument('--board', choices=BOARDS, default='wroom')
    parser.add_argument('--out', default='matrix-results')
    parser.add_argument('--generate-only', action='store_true')
    parser.add_argument('--variants', nargs='+', choices=[v[0] for v in VARIANTS])
    args = parser.parse_args()
    fqbn, slot = BOARDS[args.board]
    out = Path(args.out).resolve()
    if out == Path(__file__).parent.resolve():
        parser.error('Escolha uma pasta de saída diferente da pasta da ferramenta.')
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    cli = [args.cli] + (['--config-file', args.config_file] if args.config_file else [])
    if not args.generate_only:
        version = subprocess.run(cli + ['version'], capture_output=True, text=True, check=True).stdout.strip()
        cores = subprocess.run(cli + ['core', 'list'], capture_output=True, text=True, check=True).stdout
        if not re.search(r'^esp32:esp32\s+3\.3\.12(?:\s|$)', cores, re.M):
            parser.error('O core esp32:esp32 3.3.12 precisa estar instalado. A ferramenta não instala dependências.')
        (out / 'environment.json').write_text(json.dumps({'cli': version, 'cores': cores, 'fqbn': fqbn}, indent=2))
    for name, parent, flags in VARIANTS:
        if args.variants and name not in args.variants:
            continue
        sketch = out / 'sources' / name
        generate(sketch, flags)
        if args.generate_only:
            print('GENERATED', name, flush=True)
            continue
        build = out / 'builds' / name
        build.mkdir(parents=True, exist_ok=True)
        # Fixed compile action, no upload/erase/port arguments or shell evaluation.
        cmd = cli + ['compile', '--fqbn', fqbn, '--build-path', str(build / 'work'),
                     '--output-dir', str(build / 'artifacts'), str(sketch)]
        print('COMPILING', name, flush=True)
        completed = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8', errors='replace')
        log = completed.stdout + '\n' + completed.stderr
        (build / 'compile.log').write_text(log, encoding='utf-8')
        if completed.returncode:
            raise SystemExit(f'Falha em {name}; veja {build / "compile.log"}. Nenhum firmware foi gravado.')
        binaries = list((build / 'artifacts').glob('*.ino.bin'))
        if len(binaries) != 1:
            raise SystemExit(f'Esperado exatamente um BIN de aplicativo em {name}.')
        data = binaries[0].read_bytes()
        chip = 0 if args.board == 'wroom' else 9
        if data[0] != 0xe9 or struct.unpack_from('<H', data, 12)[0] != chip:
            raise SystemExit('Chip inesperado; somente dimensionamento, nada publicado.')
        tables = list((build / 'artifacts').glob('*.partitions.bin'))
        if len(tables) != 1:
            raise SystemExit('Tabela de partições ausente.')
        table = tables[0].read_bytes()
        apps = []
        for offset in range(0, len(table) - 31, 32):
            magic, typ, sub, address, size, label, flags = struct.unpack_from('<HBBII16sI', table, offset)
            if magic == 0x50aa and typ == 0:
                apps.append((sub, address, size))
        expected = [(16, 0x10000, slot), (17, 0x150000 if args.board == 'wroom' else 0x310000, slot)]
        if apps != expected or len(data) > slot:
            raise SystemExit('Slot incompatível ou tamanho excedido; nada publicado.')
        sketch_bytes, ram = parse_compile_summary(log)
        row = {'variant': name, 'parent': parent or '', 'bin_bytes': len(data),
               'sketch_bytes': sketch_bytes, 'static_ram_bytes': ram, 'slot_bytes': slot,
               'slot_margin_bytes': slot - len(data), 'sha256': hashlib.sha256(data).hexdigest()}
        row['slot_used_percent'] = round(len(data) * 100 / slot, 2)
        row['partitions_verified'] = True
        prior = next((r for r in rows if r['variant'] == parent), None)
        row['delta_bin_vs_parent'] = len(data) - prior['bin_bytes'] if prior else None
        row['delta_ram_vs_parent'] = ram - prior['static_ram_bytes'] if prior and ram is not None and prior['static_ram_bytes'] is not None else None
        rows.append(row)
        (out / 'matrix.json').write_text(json.dumps(rows, indent=2), encoding='utf-8')
        print('MEASURED', name, row['bin_bytes'], 'bytes', flush=True)
    if rows:
        with (out / 'matrix.csv').open('w', newline='', encoding='utf-8-sig') as handle:
            writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
            writer.writeheader(); writer.writerows(rows)
    print('Somente arquivos locais gerados. Nenhuma placa ou serviço foi modificado.')


if __name__ == '__main__':
    main()
