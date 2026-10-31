#!/usr/bin/env python3
"""Embed the existing CHGame web emulator and real release firmware, offline.

The emulator zip comes from joyrider3774/chgame_emulator v1.2. Its runtime
and machine are unchanged. Only the surrounding UI and embedded inputs differ.
"""
from pathlib import Path
import argparse
import base64
import gzip
import html
import json
import hashlib

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--emulator',type=Path,default=ROOT.parent/'.cache/voidbloom-web-emulator/CHGame_Emulator-web')
parser.add_argument('--output',type=Path,default=ROOT/'out/artifact/void-bloom.html')
args=parser.parse_args()
def b64(data):return base64.b64encode(data).decode('ascii')
source=(ROOT/'tools/artifact.html').read_text()
source=source.replace('@@VERSION@@',html.escape(json.loads((ROOT/'chgame.json').read_text())['version']))
firmware=(ROOT/'build/release/VoidBloom.ino.bin').read_bytes()
runtime=(args.emulator/'CHGame_Emulator.js').read_text()
wasm=(args.emulator/'CHGame_Emulator.wasm').read_bytes()
# Preserve the upstream runtime within a function receiving its Module.
runtime=runtime.replace('</script','<\\/script')
source=source.replace('@@RUNTIME@@',runtime).replace('@@FIRMWARE@@',b64(firmware))
source=source.replace('@@WASM@@',b64(gzip.compress(wasm,mtime=0)))
source=source.replace('@@LICENSE@@',html.escape((args.emulator/'LICENSE').read_text()))
for name in ['origin','awakening','vector','surge','nova','infinity','title']:
    source=source.replace('@@'+name.upper()+'@@','data:image/png;base64,'+b64((ROOT/f'docs/{name}.png').read_bytes()))
assert '@@' not in source
assert len(source.encode())<1_000_000
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(source)
(args.output.parent/'manifest.json').write_text(json.dumps({
    'firmware_sha256':hashlib.sha256(firmware).hexdigest(),
    'emulator_wasm_sha256':hashlib.sha256(wasm).hexdigest(),
    'emulator':'joyrider3774/chgame_emulator v1.2',
    'artifact_bytes':len(source.encode()),'network_required':False,
},indent=2)+'\n')
print(args.output)
