#!/usr/bin/env python3
"""Read-only firmware sizing and ThingsBoard history. Standard library only."""
import argparse
import hashlib
import json
import os
import time
import urllib.parse
import urllib.request
from pathlib import Path

DEVICES = {
    'S3': ('4a771080-c036-11f1-b143-c924012d9fae', '', 3145728),
    'WROOM': ('368826d0-c082-11f1-8cda-091689c2cb7f', 'wroom', 1310720),
}
KEYS = ('firmware_version', 'boot_id', 'uptime_seconds', 'reset_reason',
        'ram_internal_free_bytes', 'ram_internal_min_bytes',
        'ram_largest_block_bytes', 'loop_max_gap_us', 'diag_tb_failures',
        'running_partition', 'boot_partition')


def summarize(history, now_ms):
    """Every field retains its own timestamp; missing values stay unknown."""
    series = {k: sorted(history.get(k, []), key=lambda x: x['ts']) for k in KEYS}
    heartbeat = series['uptime_seconds']
    latest = {k: v[-1] for k, v in series.items() if v}
    gaps = [(b['ts'] - a['ts']) / 1000 for a, b in zip(heartbeat, heartbeat[1:])]
    boots = []
    for p in series['boot_id']:
        if not boots or p['value'] != boots[-1]:
            boots.append(p['value'])
    def numbers(key):
        result = []
        for p in series[key]:
            try:
                result.append(float(p['value']))
            except (ValueError, TypeError):
                pass
        return result
    heap = numbers('ram_internal_free_bytes')
    loop = numbers('loop_max_gap_us')
    return {
        'latest_fields': latest,
        'heartbeat_samples': len(heartbeat),
        'last_heartbeat_age_seconds': (now_ms - heartbeat[-1]['ts']) / 1000 if heartbeat else None,
        'largest_observed_report_gap_seconds': max(gaps) if gaps else None,
        'observed_boot_transitions': max(0, len(boots) - 1),
        'minimum_reported_free_heap_bytes': min(heap) if heap else None,
        'maximum_reported_loop_gap_us': max(loop) if loop else None,
        'reset_reasons_reported_in_window': sorted({str(p['value']) for p in series['reset_reason']}),
        'limitations': 'Gaps include transport delays; silence has no identified cause. Loop gap is not CPU load. Mixed firmware/boot history is not a single performance test.',
    }


def firmware_sizes(root, prefix, slot):
    base = root / prefix
    manifest = json.loads((base / 'manifest.json').read_text())
    rows = []
    for binary in sorted((base / 'releases').glob('*/firmware.bin')):
        data = binary.read_bytes()
        sha = hashlib.sha256(data).hexdigest()
        active = binary.parent.name == manifest['version']
        rows.append({'version': binary.parent.name, 'bytes': len(data),
                     'slot_bytes': slot, 'remaining_bytes': slot - len(data),
                     'slot_used_percent': round(100 * len(data) / slot, 2),
                     'sha256': sha, 'manifest_selected': active,
                     'manifest_matches': len(data) == manifest['size'] and sha == manifest['sha256'] if active else None})
    return rows


def get_history(device, key, now, hours):
    query = urllib.parse.urlencode({'keys': ','.join(KEYS), 'startTs': now - hours * 3600000,
                                   'endTs': now, 'agg': 'NONE', 'limit': 2000, 'orderBy': 'ASC'})
    req = urllib.request.Request('https://thingsboard.cloud/api/plugins/telemetry/DEVICE/' + device + '/values/timeseries?' + query,
                                 headers={'X-Authorization': 'ApiKey ' + key})
    with urllib.request.urlopen(req, timeout=30) as response:
        return json.load(response)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--out', type=Path, default=Path('observer-output'))
    parser.add_argument('--hours', type=int, choices=range(1, 25), default=6)
    args = parser.parse_args()
    now = int(time.time() * 1000)
    key = os.environ.get('TB_READ_API_KEY', '')
    report = {'generated_at_ms': now, 'window_hours': args.hours, 'devices': {}}
    for name, (device, prefix, slot) in DEVICES.items():
        entry = {'firmware_sizes': firmware_sizes(args.root, prefix, slot)}
        if key:
            try:
                history = get_history(device, key, now, args.hours)
                entry['telemetry'] = summarize(history, now)
                entry['history'] = history
                entry['history_limit_per_key'] = 2000
                entry['possibly_truncated'] = any(len(v) >= 2000 for v in history.values())
            except Exception as error:
                # Never output exception text: request details could contain credentials.
                entry['telemetry_unavailable'] = type(error).__name__
        else:
            entry['telemetry_unavailable'] = 'TB_READ_API_KEY not configured'
        report['devices'][name] = entry
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    lines = ['# Observação externa BOTIZIN', '', 'Somente leitura. Nenhum comando ou firmware enviado às placas.', '',
             '| Placa | Firmware selecionado | Ocupação do slot | Margem |', '|---|---|---:|---:|']
    for name, entry in report['devices'].items():
        for row in entry['firmware_sizes']:
            if row['manifest_selected']:
                lines.append(f"| {name} | {row['version']} | {row['slot_used_percent']}% | {row['remaining_bytes']} bytes |")
    for name, entry in report['devices'].items():
        lines.extend(['', f'## {name}', ''])
        if 'telemetry' in entry:
            t = entry['telemetry']
            lines.extend([f"- Relatórios na janela: {t['heartbeat_samples']}",
                          f"- Idade do último relatório: {t['last_heartbeat_age_seconds']} segundos",
                          f"- Maior intervalo observado entre relatórios: {t['largest_observed_report_gap_seconds']} segundos",
                          f"- Mudanças de boot observadas: {t['observed_boot_transitions']}",
                          f"- Motivos de reset reportados na janela: {', '.join(t['reset_reasons_reported_in_window']) or 'desconhecidos'}",
                          f"- Menor heap livre reportado: {t['minimum_reported_free_heap_bytes']} bytes",
                          f"- Maior intervalo de loop reportado: {t['maximum_reported_loop_gap_us']} microssegundos"])
        else:
            lines.append('Telemetria indisponível: ' + entry['telemetry_unavailable'])
    lines.extend(['', 'Ausência de dados não prova falha de energia. Intervalo do loop não mede percentual de CPU.',
                  'Campos no JSON mantêm timestamps próprios. Um valor antigo não descreve necessariamente o boot atual.',
                  'Tamanho do BIN não permite atribuir custo exato a cada biblioteca. Slots informados são os do projeto atual.',
                  'Esta execução é uma fotografia da janela consultada; não mantém monitoramento contínuo.'])
    (args.out / 'report.md').write_text('\n'.join(lines) + '\n')
    print('\n'.join(lines))


if __name__ == '__main__':
    main()
