"""Offer the authorized S3 trial briefly, then restore its baseline manifest."""
import base64
import hashlib
import json
import os
import time
import urllib.error
import urllib.request
from pathlib import Path

REPO = 'Fernandodsrm/botizin-core-v2'
BASE = 'https://api.github.com/repos/' + REPO + '/contents/'
BASE_SHA = 'db682f24db7af0624f4ed0603738d6536643ce7cb5e34eb08a5e99ff4fc8adab'


def request(path, payload=None):
    headers = {'Authorization': 'Bearer ' + os.environ['GITHUB_TOKEN'],
               'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28'}
    req = urllib.request.Request(BASE + path + ('?ref=main' if payload is None else ''),
          data=json.dumps(payload).encode() if payload is not None else None,
          headers=headers, method='PUT' if payload is not None else 'GET')
    with urllib.request.urlopen(req, timeout=30) as response:
        return json.load(response)


def decode(file):
    return base64.b64decode(file['content'])


def put(path, content, message, sha=None):
    body = {'message': message, 'branch': 'main',
            'content': base64.b64encode(content).decode()}
    if sha: body['sha'] = sha
    return request(path, body)


def main():
    folder = Path('trial-package')
    manifest = json.loads((folder / 'manifest.json').read_text())
    record = json.loads((folder / 'build-record.json').read_text())
    image = (folder / 'firmware.bin').read_bytes()
    assert record['source_commit'] == 'aa9e3340e50bb6f7a2ff2ba38e3418f1a89610b8'
    assert record['publishable'] and record['device_token_compiled']
    assert manifest['version'] == '0.0.14' and manifest['board'] == 'esp32s3-n16r8'
    assert manifest['size'] == len(image) <= 3145728
    assert manifest['sha256'] == hashlib.sha256(image).hexdigest() == '553591a8ceab9f12b01a873d496cc6d55d86993af867ba569fff58dd09fbf66a'
    original = request('manifest.json')
    original_bytes = decode(original)
    original_manifest = json.loads(original_bytes)
    assert original_manifest['version'] == '0.0.13' and original_manifest['sha256'] == BASE_SHA
    path = 'releases/0.0.14/firmware.bin'
    try:
        existing = request(path)
    except urllib.error.HTTPError as error:
        if error.code != 404: raise
        existing = None
    if existing:
        assert decode(existing) == image, 'Existing release differs; stop'
    else:
        put(path, image, 'Store verified S3 0.0.14 timed trial image')
    trial_bytes = (json.dumps(manifest, indent=2) + '\n').encode()
    # try/finally begins before PUT: a lost response must not skip withdrawal.
    try:
        put('manifest.json', trial_bytes, 'Offer authorized two-minute S3 trial', original['sha'])
        print('TRIAL_OFFERED: 0.0.14; withdrawal in 90 seconds', flush=True)
        time.sleep(90)
    finally:
        for attempt in range(5):
            try:
                current = request('manifest.json')
                content = decode(current)
                if content == original_bytes:
                    print('BASELINE_ALREADY_RESTORED', flush=True); break
                if content != trial_bytes:
                    raise RuntimeError('Manifest changed externally; refusing to overwrite')
                put('manifest.json', original_bytes, 'Withdraw timed trial; restore S3 0.0.13 offer', current['sha'])
                assert decode(request('manifest.json')) == original_bytes
                print('BASELINE_RESTORED: 0.0.13', flush=True); break
            except Exception:
                if attempt == 4: raise
                time.sleep(5)


if __name__ == '__main__':
    main()
