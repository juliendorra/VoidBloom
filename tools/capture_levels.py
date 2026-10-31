#!/usr/bin/env python3
"""Record key moments of waves 1–12 through the real CHGame simulator."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

from chgame_env import chgame_root

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(chgame_root() / 'tools/chsim'))
from chsim import build
from chdrivelib import Driver, SimTransport, mask_of
from fbimage import to_image, save_gif, sheet
from PIL import Image
from hosttests import Ctx, compile_test
import gamecfg

# Sample every six 60 Hz updates: a 100 ms GIF frame preserves play speed.
STEP, DURATION, SCALE = 6, 100, 3
CLIPS = [
    (1, 'origin', 60, 360, 'Marching formation and single-shot fire'),
    (2, 'awaken', 60, 360, 'Charge, upward loop, committed dive, and return'),
    (3, 'vector', 150, 360, 'Free flight and twin shot against moving attackers'),
    (4, 'first-boss', 90, 600, 'Rings, aimed fans, rotating streams, and recovery beats'),
    (5, 'surge', 150, 360, 'Three-way spread and the first radial volleys'),
    (6, 'focused-spread', 240, 360, 'Spread fire and dodging between turret volleys'),
    (7, 'nova', 180, 360, 'Five-way fire through weaving attackers'),
    (8, 'nova-boss', 90, 600, 'Denser boss rings, fans, and rotating streams'),
    (9, 'infinity', 180, 360, 'Full-power fire and stronger moving enemies'),
    (10, 'panic-bomb', 240, 360, 'A bomb clears a barrage before the attack resumes'),
    (11, 'endless-pressure', 240, 360, 'Full-power fire under sustained pressure'),
    (12, 'infinity-boss', 90, 600, 'The first full-progression boss and its largest patterns'),
]


def state(driver):
    return list(map(int, driver.query('H', 'BLOOMSTATE').split()[1:]))


def buttons(wave, frame):
    if wave == 2:
        # Keep the diver alive through its whole return. Side-step once it
        # commits, then use a short burst of single-shot fire at the end.
        return 'RIGHT' if 96 <= frame < 126 else 'LEFT' if 150 <= frame < 180 else 'A' if frame >= 300 else ''
    assert wave == 1
    direction = ['LEFT', 'RIGHT', 'RIGHT', 'LEFT'][(frame // 48) % 4]
    return f'A+{direction}'


def main():
    out = ROOT / 'docs/captures'
    out.mkdir(parents=True, exist_ok=True)
    exe = build(ROOT, ['BLOOM_CAPTURE'], out=ROOT / 'out/simulator/level-capture-sim.exe')
    pilot = compile_test(Ctx(ROOT), 'pilots', gamecfg.load(ROOT).tests()['pilots'])
    transport = SimTransport(exe)
    driver = Driver(transport, 'BLOOM')
    driver.handshake(); driver.cmd('L1'); driver.frames(2)
    entries, thumbnails = [], []
    try:
        for wave, slug, warmup, length, moment in CLIPS:
            driver.buttons(0)
            # Zero disables the legacy capture fixture's invulnerability.
            # Only stage selection is staged; damage and bullets stay normal.
            driver.cmd(f'J {wave} 0')
            driver.frames(warmup)
            plan = None
            if wave >= 3:
                run = subprocess.run([str(pilot), '--capture', str(wave), str(warmup), str(length)],
                                     text=True, capture_output=True, check=True)
                plan = json.loads(run.stdout)
            frames, peak, hits = [], 0, 0
            initial_lives = state(driver)[3]
            for frame in range(0, length, STEP):
                driver.buttons(plan[frame // STEP]['keys'] if plan else mask_of(buttons(wave, frame)))
                driver.frames(STEP)
                current = state(driver)
                assert current[0] == 2 and current[1] == wave, (wave, frame, current)
                if plan:
                    expected = plan[frame // STEP]
                    assert [current[1], current[3], current[8], current[9], current[10]] == [
                        expected[k] for k in ('wave', 'lives', 'x', 'y', 'bullets')], (wave, frame, current, expected)
                peak = max(peak, current[10])
                frames.append(to_image(driver.shot(), SCALE))
            hits = initial_lives - state(driver)[3]
            path = out / f'wave-{wave:02d}-{slug}.gif'
            save_gif(frames, path, DURATION)
            with Image.open(path) as gif:
                duration = sum((gif.seek(i), gif.info.get('duration', 0))[1] for i in range(gif.n_frames))
                assert gif.size == (384, 384) and duration == length * 1000 // 60
                assert gif.n_frames > 1 and path.stat().st_size < 1_000_000
                gif.seek(gif.n_frames // 2); thumbnails.append(gif.convert('RGB').resize((256, 256), Image.Resampling.NEAREST))
            entries.append({'wave': wave, 'file': path.name, 'moment': moment,
                            'duration_ms': duration, 'size_bytes': path.stat().st_size,
                            'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                            'peak_hostile_bullets': peak, 'lives_lost': hits})
            print(f'Wave {wave:02d}: {path.name}, {duration / 1000:g}s, {path.stat().st_size:,} B, peak {peak}, hits {hits}', flush=True)
        assert not transport.bugs, transport.bugs
    finally:
        transport.close()
    sheet(thumbnails, cols=4).save(out / 'contact-sheet.png')
    version = json.loads((ROOT / 'chgame.json').read_text())['version']
    manifest = {'game_version': version,
                'source': 'CHGame simulator; actual game rules and framebuffer',
                'capture': 'staged wave starts; scripted opening controls and delayed-observation pilot replay; normal damage, no invulnerability',
                'timing': '60 Hz simulation, one GIF frame per 6 updates, 100 ms per GIF frame; real play speed',
                'torn_frames': 0, 'clips': entries}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    rows = '\n'.join(f"| {e['wave']} | [{e['file']}]({e['file']}) | {e['duration_ms'] // 1000}s | {e['moment']} |" for e in entries)
    (out / 'README.md').write_text('# Wave captures\n\n'
        f'Key moments from the first 12 waves of Void Bloom {version}. The game continues endlessly after wave 12.\n\n'
        '![Wave contact sheet, in wave order](contact-sheet.png)\n\n'
        '| Wave | GIF | Duration | Key moment |\n|---|---|---|---|\n' + rows + '\n\n'
        'Recorded from the real CHGame simulator framebuffer at 384 × 384, with nearest-neighbor scaling. '
        'Each GIF preserves game speed: six 60 Hz updates per 100 ms frame. Each file is smaller than 1 MB.\n\n'
        'Wave starts are staged using a separate simulator-only capture build. Damage is enabled; '
        'controls use normal movement, fire, focus, and bombs; later waves replay the delayed-observation pilot. These are showcase clips, '
        'not a continuous playthrough or a claimed no-hit run. The capture command is absent from release firmware.\n\n'
        'Recreate with `python tools/capture_levels.py` from the game folder. '
        '[manifest.json](manifest.json) records hashes, timing, sizes, and observed bullet peaks.\n')
    print(f'PASS: {len(entries)} real-speed GIFs; zero torn frames; {out}')


if __name__ == '__main__':
    main()
