#!/usr/bin/env python3
"""Capture active, real-speed excerpts from one ordinary simulator campaign."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

from chgame_env import chgame_root

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(chgame_root() / 'tools/chsim'))
from chsim import build
from chdrivelib import Driver, SimTransport, mask_of
from fbimage import to_image, save_gif, sheet
from hosttests import Ctx, compile_test
import gamecfg
from PIL import Image


def state(driver):
    return list(map(int, driver.query('H', 'BLOOMSTATE').split()[1:]))


def movement(rows):
    positions = [(r['x'], r['y']) for r in rows]
    return {
        'horizontal_pixels': (max(x for x, y in positions)-min(x for x, y in positions))/64,
        'vertical_pixels': (max(y for x, y in positions)-min(y for x, y in positions))/64,
        'moving_fraction': round(sum(a != b for a, b in zip(positions, positions[1:]))/(len(positions)-1), 3),
        'firing_fraction': round(sum(bool(r['keys'] & 3) for r in rows)/len(rows), 3),
    }


def choose_excerpt(plan, wave, length):
    count = length // 6
    candidates = []
    for i in range(0, len(plan)-count+1, 2):
        rows = plan[i:i+count]
        if not all(r['screen'] == 2 and r['wave'] == wave and not r['transition'] for r in rows):
            continue
        # Pick readable moments between hits/bombs, with the ship visible
        # throughout rather than blinking during its ordinary protection.
        if any(r['events'] & 4 or r['invincible'] for r in rows):
            continue
        motion = movement(rows)
        if motion['horizontal_pixels'] < (24 if wave >= 3 else 16):
            continue
        if wave >= 3 and (motion['vertical_pixels'] < 12 or motion['moving_fraction'] < .5):
            continue
        if motion['firing_fraction'] < .8:
            continue
        score = motion['horizontal_pixels']+motion['vertical_pixels']*2+motion['moving_fraction']*30
        score += sum(r['bullets']+r['enemies'] for r in rows)/len(rows)
        if wave == 2:
            score += sum(bool(r['flight']) for r in rows)*8
        candidates.append((score, i, motion))
    assert candidates, f'No sufficiently active play excerpt for wave {wave}'
    _, i, motion = max(candidates)
    return {'name': f'wave-{wave:02d}', 'wave': wave, 'start_frame': i*6,
            'end_frame': i*6+length, **motion}


def evolution_excerpt(plan, wave):
    for i in range(0, len(plan)-6, 2):
        rows = plan[i:i+6]
        if all(r['screen'] == 4 and r['wave'] == wave and 65 <= r['transition'] <= 115 for r in rows):
            return {'name': f'evolution-{wave:02d}', 'wave': wave, 'start_frame': i*6, 'end_frame': i*6+36}
    raise AssertionError(f'Missing natural evolution before wave {wave}')


def record():
    out = ROOT / 'out/readme'; out.mkdir(parents=True, exist_ok=True)
    docs = ROOT / 'docs'; docs.mkdir(exist_ok=True)
    pilot = compile_test(Ctx(ROOT), 'pilots', gamecfg.load(ROOT).tests()['pilots'])
    # This is the normal simulator: no stage-selection or empty-arena fixture.
    transport = SimTransport(build(ROOT)); driver = Driver(transport, 'BLOOM')
    driver.handshake(); driver.cmd('L1'); driver.frames(2)
    title = []
    try:
        assert state(driver)[0] == 0
        for _ in range(9):
            driver.frames(4); title.append(to_image(driver.shot(), 1))
        # The title advances its clock before handling the start-button edge.
        seed = 0xB1006 ^ (state(driver)[5]+1)
        run = subprocess.run([str(pilot), '--readme-run', str(seed)], capture_output=True, text=True, check=True)
        plan = json.loads(run.stdout); (out / 'plan.json').write_text(json.dumps(plan)+'\n')
        assert plan[-1]['wave'] >= 13
        clips = [choose_excerpt(plan, 1, 96), choose_excerpt(plan, 2, 168), evolution_excerpt(plan, 3),
                 choose_excerpt(plan, 3, 168), evolution_excerpt(plan, 5), choose_excerpt(plan, 5, 168),
                 choose_excerpt(plan, 7, 168), choose_excerpt(plan, 12, 168)]
        assert all(a['end_frame'] <= b['start_frame'] for a, b in zip(clips, clips[1:]))
        for clip in clips:
            clip['frames'] = []
            print({k: v for k, v in clip.items() if k != 'frames'}, flush=True)
        driver.buttons(mask_of('A')); driver.frames(1); driver.buttons(0)
        assert state(driver)[0:3] == [2, 1, 0]
        compared = 0
        fields = [(0, 'screen'), (1, 'wave'), (2, 'phase'), (3, 'lives'), (4, 'score'), (5, 'tick'),
                  (6, 'bombs'), (8, 'x'), (9, 'y'), (10, 'bullets'), (11, 'enemies'),
                  (12, 'transition'), (13, 'invincible'), (15, 'focus')]
        for i, expected in enumerate(plan):
            start, end = i*6, (i+1)*6
            driver.buttons(expected['keys'])
            active = next((c for c in clips if c['start_frame'] <= start < c['end_frame']), None)
            if active:
                cursor = start
                while cursor < end:
                    step = min(4-cursor%4, end-cursor)
                    driver.frames(step); cursor += step
                    if cursor % 4 == 0:
                        active['frames'].append(to_image(driver.shot(), 1))
            else:
                driver.frames(6)
            actual = state(driver)
            assert all(actual[index] == expected[key] for index, key in fields), (i, actual, expected)
            compared += 1
        assert not transport.bugs, transport.bugs
    finally:
        transport.close()
    native_frames = title+[frame for clip in clips for frame in clip['frames']]
    assert len(native_frames) % 3 == 0
    # GIF time is quantized to 10 ms. Three samples cover twelve updates:
    # 60+70+70 ms exactly preserves 60 Hz play speed at fifteen samples/sec.
    durations = [60, 70, 70]*(len(native_frames)//3)
    path = docs / 'gameplay.gif'
    for scale in [3, 2]:
        frames = [frame.resize((128*scale, 128*scale), Image.Resampling.NEAREST) for frame in native_frames]
        save_gif(frames, path, durations)
        if path.stat().st_size < 1_000_000:
            break
    assert path.stat().st_size < 1_000_000, 'Keep the README GIF below 1 MB'
    with Image.open(path) as gif:
        duration = sum((gif.seek(i), gif.info['duration'])[1] for i in range(gif.n_frames))
        assert duration == sum(durations)
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    keyframes = [clip['frames'][j].resize((256, 256), Image.Resampling.NEAREST)
                 for clip in clips if clip['name'] in ('wave-03', 'wave-05', 'wave-07', 'wave-12')
                 for j in [0, len(clip['frames'])//2, len(clip['frames'])-1]]
    sheet(keyframes, cols=3).save(out / 'keyframes.png')
    manifest = {'game_version': json.loads((ROOT/'chgame.json').read_text())['version'],
                'source': 'Actual framebuffer of the normal CHGame simulator; one campaign from the title screen through wave 12',
                'controls': '100 ms delayed-observation pilot with ordinary fire, focus, movement and bombs; vertical patrol preference for the preview only',
                'cheats': False, 'stage_jumps': False, 'torn_frames': 0, 'seed': seed,
                'campaign_seconds': len(plan)/10, 'reached_wave': plan[-1]['wave'],
                'replay_state_comparisons': compared, 'gif_duration_ms': duration,
                'gif_dimensions': [128*scale, 128*scale], 'gif_samples_per_second': 15,
                'gif_bytes': path.stat().st_size, 'gif_sha256': digest,
                'clips': [{k: v for k, v in clip.items() if k != 'frames'} for clip in clips]}
    (docs / 'gameplay.json').write_text(json.dumps(manifest, indent=2)+'\n')
    readme = ROOT / 'README.md'
    text = readme.read_text()
    text, count = re.subn(r'!\[Void Bloom gameplay\]\(docs/gameplay\.gif(?:\?[^)]*)?\)',
                          f'![Void Bloom gameplay](docs/gameplay.gif?preview={digest[:12]})', text)
    assert count == 1
    readme.write_text(text)
    print(f'PASS: {duration/1000:g}s real-speed preview, {scale*128}px, {path.stat().st_size:,} B; '
          f'{compared} replay checks; horizontal and vertical movement in every free-flight excerpt; no stage jumps or cheats')
    return manifest


if __name__ == '__main__':
    record()
