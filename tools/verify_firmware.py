#!/usr/bin/env python3
"""Run actual release firmware through the workspace's existing CHGame emulator."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--emulator',type=Path,default=ROOT.parent/'.cache/chgame-emulator-review/chg_headless_fixed')
args=parser.parse_args()
out=ROOT/'out/emulator';out.mkdir(parents=True,exist_ok=True)
binary=ROOT/'build/release/VoidBloom.ino.bin'
frames=[]
for seconds in [8,12]:
    run=subprocess.run([str(args.emulator.resolve()),str(binary),str(seconds),str(out/f'{seconds}.ppm'),
                        '--press',f'a@1.0:{seconds-1.1}'],capture_output=True,text=True,check=True)
    log=run.stdout+run.stderr
    (out/f'{seconds}.log').write_text(log)
    assert 'FAULT' not in log and 'display on' in log,log
    frames.append(int(re.search(r'frames written (\d+)',log)[1]))
fps=(frames[1]-frames[0])/4
assert 59<=fps<=60.5,fps
report={'firmware_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),
        'emulator':str(args.emulator),'sample_seconds':[8,12],'frame_counts':frames,
        'steady_opening_fps':fps,'cpu_faults':0,
        'limits':['Actual release RISC-V firmware in the existing native emulator.',
                  'Opening gameplay window only; not a physical-device or dense-boss performance measurement.']}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: real release firmware, {fps:.2f} fps in the measured opening window, no CPU faults')
