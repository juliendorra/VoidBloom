#!/usr/bin/env python3
"""Check movement and actual framebuffer visibility at every arena edge."""
from pathlib import Path
import json
import sys

from chgame_env import chgame_root

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(chgame_root() / 'tools/chsim'))
from chsim import build
from chdrivelib import Driver, SimTransport, mask_of
from fbimage import to_image, sheet


def state(driver):
    return list(map(int, driver.query('H', 'BLOOMSTATE').split()[1:]))


def render(driver, wave, x, y, focus, hidden=False):
    driver.buttons(0)
    driver.cmd(f'E {wave} {x} {y} 1 {int(hidden)}')
    driver.buttons(mask_of('B') if focus else 0)
    driver.frames(1)
    return to_image(driver.shot(), 1)


def main():
    out = ROOT / 'out/boundaries'; out.mkdir(parents=True, exist_ok=True)
    exe = build(ROOT, ['BLOOM_CAPTURE'], out=out / 'sim.exe')
    transport = SimTransport(exe); driver = Driver(transport, 'BLOOM')
    driver.handshake(); driver.cmd('L1'); driver.frames(2)
    cases, corners = [], []
    try:
        for wave in [1, 3, 5, 7, 9]:
            for focus in [False, True]:
                reference = render(driver, wave, 64, 85, focus)
                background = render(driver, wave, 64, 85, focus, True)
                # Use the actual renderer, not a second copy of its sprite.
                pixels = {(x-64, y-85): reference.getpixel((x, y))
                          for y in range(77, 94) for x in range(55, 74)
                          if reference.getpixel((x, y)) != background.getpixel((x, y))}
                assert pixels and max(dy for dx, dy in pixels) == 7, 'reference must include the longest exhaust'
                if focus:
                    assert min(dx for dx, dy in pixels) == -7 and max(dx for dx, dy in pixels) == 7
                for direction in ['LEFT', 'RIGHT', 'UP', 'DOWN', 'UP+LEFT', 'UP+RIGHT', 'DOWN+LEFT', 'DOWN+RIGHT']:
                    driver.buttons(0); driver.cmd(f'E {wave} 64 111')
                    driver.buttons(mask_of(direction + ('+B' if focus else ''))); driver.frames(240)
                    current = state(driver); x, y = current[8] // 64, current[9] // 64
                    actual = render(driver, wave, x, y, focus)
                    # Keep a useful failing image if a later sprite change
                    # grows beyond the bounds or slips beneath either HUD.
                    name = f'w{wave}-{direction.lower().replace("+", "-")}-{"focus" if focus else "normal"}.png'
                    actual.resize((384, 384)).save(out / name)
                    for (dx, dy), color in pixels.items():
                        assert 0 <= x+dx <= 127 and 11 <= y+dy <= 119, (wave, direction, focus, 'outside field', x, y, dx, dy)
                        assert actual.getpixel((x+dx, y+dy)) == color, (wave, direction, focus, 'clipped ship', x, y, dx, dy)
                    expected_x = 7 if 'LEFT' in direction else 120 if 'RIGHT' in direction else 64
                    expected_y = 111 if wave == 1 else 58 if 'UP' in direction else 112 if 'DOWN' in direction else 111
                    assert (current[8], current[9]) == (expected_x*64, expected_y*64), (wave, direction, focus, x, y)
                    cases.append({'wave': wave, 'focus': focus, 'direction': direction, 'center': [x, y], 'full_ship_visible': True})
                    if wave == 9 and focus and '+' in direction:
                        corners.append(actual.resize((384, 384)))
        assert not transport.bugs, transport.bugs
    finally:
        transport.close()
    sheet(corners, cols=2).save(out / 'corners.png')
    (out / 'results.json').write_text(json.dumps({'cases': cases, 'passed': len(cases), 'torn_frames': 0,
        'method': 'hold normal/focus D-pad inputs for 240 frames, then compare the full actual ship against an unclipped renderer reference, including maximum exhaust and focus markers'}, indent=2)+'\n')
    print(f'PASS: {len(cases)} boundary cases across every weapon tier; full ship, exhaust and focus markers visible; zero torn frames')


if __name__ == '__main__':
    main()
