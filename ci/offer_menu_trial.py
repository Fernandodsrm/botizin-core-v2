"""Offer the authorized ten-minute WROOM Menu trial for five minutes, then restore its baseline manifest."""
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
BASE_SHA = '11a0a1c68a8b7bd7c9c1bd7782df070ece556e2ff1eec94f3899e01e4c6aa2f5'


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
    assert record['source_commit'] == os.environ['EXPECTED_SOURCE_COMMIT']
    assert record['partitions_verified'] and not record['credentials_compiled']
    assert record['module_id'] == 'menu_led_trial' and record['return_seconds'] == 600
    assert manifest['version'] == '0.0.14' and manifest['board'] == 'esp32-wroom-4mb'
    assert manifest['size'] == len(image) <= 1310720
    assert manifest['sha256'] == hashlib.sha256(image).hexdigest() == os.environ['EXPECTED_IMAGE_SHA256']
    original = request('wroom/manifest.json')
    original_bytes = decode(original)
    original_manifest = json.loads(original_bytes)
    assert original_manifest['version'] == '0.0.12' and original_manifest['sha256'] == BASE_SHA
    path = 'wroom/releases/0.0.14/firmware.bin'
    try:
        existing = request(path)
    except urllib.error.HTTPError as error:
        if error.code != 404: raise
        existing = None
    if existing:
        # GitHub omits inline content for files over 1 MiB. Verify Git blob identity.
        git_sha = hashlib.sha1(b'blob ' + str(len(image)).encode() + b'\0' + image).hexdigest()
        assert existing['size'] == len(image) and existing['sha'] == git_sha, 'Existing release differs; stop'
    else:
        put(path, image, 'Store verified WROOM Menu 0.0.14 timed trial image')
    trial_bytes = (json.dumps(manifest, indent=2) + '\n').encode()
    # try/finally begins before PUT: a lost response must not skip withdrawal.
    try:
        put('wroom/manifest.json', trial_bytes, 'Offer authorized ten-minute WROOM Menu trial', original['sha'])
        print('TRIAL_OFFERED: WROOM 0.0.14; withdrawal in 300 seconds', flush=True)
        time.sleep(300)
    finally:
        for attempt in range(5):
            try:
                current = request('wroom/manifest.json')
                content = decode(current)
                if content == original_bytes:
                    print('BASELINE_ALREADY_RESTORED', flush=True); break
                if content != trial_bytes:
                    raise RuntimeError('Manifest changed externally; refusing to overwrite')
                put('wroom/manifest.json', original_bytes, 'Withdraw timed trial; restore WROOM 0.0.12 offer', current['sha'])
                assert decode(request('wroom/manifest.json')) == original_bytes
                print('BASELINE_RESTORED: WROOM 0.0.12', flush=True); break
            except Exception:
                if attempt == 4: raise
                time.sleep(5)


if __name__ == '__main__':
    main()
