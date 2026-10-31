#!/usr/bin/env python3
"""Exercise the simulator, then refresh screenshots and a normal-play preview."""
from pathlib import Path
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
from chgame_env import chgame_root
sys.path.insert(0, str(chgame_root() / 'tools/chsim'))
from chsim import build
from chdrivelib import Driver, SimTransport, mask_of
from fbimage import to_image, sheet

def connect(exe):
    transport = SimTransport(exe)
    d = Driver(transport, 'BLOOM')
    d.handshake(); d.cmd('L1'); d.frames(2)
    return transport, d

def state(d):
    return list(map(int, d.query('H', 'BLOOMSTATE').split()[1:]))

def tap(d, button):
    d.buttons(mask_of(button)); d.frames(1); d.buttons(0); d.frames(1)

def main():
    capture = '--capture' in sys.argv
    out = ROOT / 'out/simulator'; out.mkdir(parents=True, exist_ok=True)
    docs = ROOT / 'docs'; docs.mkdir(exist_ok=True)
    transport, d = connect(build(ROOT))
    captures = []
    try:
        assert state(d)[0] == 0
        if capture:
            title = to_image(d.shot(), 3)
            title.save(docs / 'title.png')
        tap(d, 'B'); assert state(d)[0] == 1
        tap(d, 'A'); assert state(d)[0] == 0
        tap(d, 'SELECT'); assert state(d)[16] == 1
        tap(d, 'SELECT'); assert state(d)[16] == 0
        tap(d, 'A'); d.frames(70); assert state(d)[0:3] == [2, 1, 0]
        y = state(d)[9]
        d.buttons(mask_of('A+UP')); d.frames(40); d.buttons(0)
        assert state(d)[9] == y, 'origin must lock vertical motion'
        tap(d, 'START'); assert state(d)[0] == 3
        frozen = state(d); shot = d.shot(); d.frames(180)
        assert frozen == state(d) and shot == d.shot(), 'pause must freeze state and pixels'
        tap(d, 'START'); assert state(d)[0] == 2
        d.buttons(mask_of('A')); d.frames(100); d.buttons(0)
        if capture:
            img = to_image(d.shot(), 3); img.save(docs / 'origin.png'); captures.append(img)
            d.buttons(mask_of('A'))
            d.frames(144)
            d.buttons(0)
        bombs = state(d)[6]; tap(d, 'SELECT'); assert state(d)[6] == bombs - 1
        tap(d, 'START'); tap(d, 'B'); assert state(d)[0] == 0
        assert not transport.bugs, transport.bugs
    finally:
        transport.close()
    if capture:
        # A separate simulator fixture jumps to authored late-wave states.
        # These captures are examples, not a claimed no-hit playthrough.
        transport, d = connect(build(ROOT, ['BLOOM_CAPTURE'], out=out / 'capture-sim.exe'))
        try:
            for name, wave, wait in [('awakening', 2, 144), ('vector', 3, 400), ('first-boss', 4, 480), ('surge', 5, 420),
                                     ('nova', 8, 310), ('infinity', 12, 530)]:
                d.cmd(f'J {wave}'); d.buttons(0)
                if wave == 2:
                    d.frames(126)
                    img = to_image(d.shot(), 3)
                    img.save(docs / 'awakening.png'); captures.append(img)
                    continue
                if wave == 4:
                    d.frames(wait)
                    peak = -1
                    for _ in range(18):
                        d.frames(10)
                        count = state(d)[10]
                        assert count <= 12, 'first boss exceeded its encounter budget'
                        if count > peak:
                            peak = count; to_image(d.shot(), 3).save(docs / 'first-boss.png')
                    continue
                d.frames(wait-24)
                d.buttons(mask_of('A')); d.frames(24); d.buttons(0)
                assert state(d)[1] == wave
                img = to_image(d.shot(), 3); img.save(docs / f'{name}.png'); captures.append(img)
            assert not transport.bugs, transport.bugs
        finally:
            transport.close()
        sheet(captures, cols=3).save(docs / 'evolution.png')
        # The README shows ordinary play from a single uninterrupted run.
        # Stage fixtures above are only for the separate static gallery.
        subprocess.run([sys.executable, str(ROOT/'tools/capture_readme.py')], check=True)
    (out / 'results.json').write_text(json.dumps({
        'normal_simulator': ['title', 'help', 'mute', 'start', 'vertical lock',
                             'pause state/pixels', 'resume', 'fire', 'bomb', 'quit'],
        'torn_frames': 0, 'staged_late_wave_captures': capture,
    }, indent=2) + '\n')
    print('PASS: real CHGame buttons, title/help/mute, opening movement, fire/bomb, frozen pause pixels, resume and quit; zero torn frames')

if __name__ == '__main__':
    main()
