"""Native 128x128 Void Bloom menu cover with editable pixel lettering."""
from pathlib import Path
import hashlib
import json
import math
import sys

from PIL import Image, ImageDraw, ImageFilter
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from chgame_env import chgame_root

sys.path.insert(0, str(chgame_root() / 'tools'))
from pixkit import FONT35, glyph35

ROLES = ['void', 'nebula', 'steel', 'cobalt', 'cyan', 'ice', 'rose', 'blush', 'gold', 'cream', 'black']
COLORS = ['000011', '111133', '334466', '3344AA', '33DDEE', 'BBFFFF', 'FF4488', 'FFAADD', 'FFEE88', 'FFF4D6', '000000']
# Each glyph is authored at its final 13x17 pixel size, without scaling.
LETTERS = {
    'V': ['###.......###']*7+['.###.....###.']*2+['..###...###..']*2+
         ['...###.###...']*2+['....#####....']*2+['.....###.....']*2,
    'I': ['..#########..']*3+['.....###.....']*11+['..#########..']*3,
    'O': ['..#########..', '.###########.', '#############']+
         ['###.......###']*11+['#############', '.###########.', '..#########..'],
    'D': ['##########...', '###########..', '############.', '###......####']+
         ['###.......###']*9+['###......####', '############.', '###########..', '##########...'],
    'B': ['##########...', '############.', '#############']+['###.......###']*4+
         ['############.', '###########..', '############.']+['###.......###']*4+
         ['#############', '############.', '##########...'],
    'L': ['###..........']*14+['#############']*3,
    'M': ['####.....####']*3+['#####...#####']*2+['######.######']*2+
         ['###.#####.###']*2+['###..###..###']*2+['###...#...###']+['###.......###']*5,
}


def title(image, word, y, face, light):
    width = len(word)*13+(len(word)-1)*6
    x = (128-width)//2
    mask = Image.new('1', (128, 128))
    for i, letter in enumerate(word):
        rows = LETTERS[letter]
        assert len(rows) == 17 and all(len(row) == 13 for row in rows)
        for dy, row in enumerate(rows):
            for dx, ink in enumerate(row):
                if ink == '#': mask.putpixel((x+i*19+dx, y+dy), 1)
    outline = mask.filter(ImageFilter.MaxFilter(3))
    for yy in range(128):
        for xx in range(128):
            if outline.getpixel((xx, yy)):
                if xx+2 < 128 and yy+3 < 128: image.putpixel((xx+2, yy+3), 1)
                image.putpixel((xx, yy), 10)
    for yy in range(y, y+17):
        for xx in range(128):
            if not mask.getpixel((xx, yy)): continue
            bevel = not mask.getpixel((xx, yy-1)) or not mask.getpixel((max(0, xx-1), yy))
            image.putpixel((xx, yy), 9 if bevel and yy < y+5 else light if bevel else face)


def draw():
    im = Image.new('P', (128, 128), 0)
    palette = [int(color[i:i+2], 16) for color in COLORS for i in (0, 2, 4)]
    im.putpalette(palette+[0]*(768-len(palette)))
    d = ImageDraw.Draw(im)
    # Curved, receding flight paths remain behind the title and hero craft.
    d.arc((38, 47, 127, 122), 185, 348, fill=1)
    d.arc((49, 51, 119, 111), 30, 180, fill=3)
    d.arc((61, 57, 109, 97), 190, 340, fill=2)
    for x, y in [(8, 53), (68, 49), (115, 57), (11, 77), (71, 115), (119, 103), (87, 117)]:
        d.point((x, y), fill=2)
        d.point((x+1, y), fill=1)
    # Small original marching-formation silhouettes introduce the retro tier.
    alien = ['..#...#..', '...#.#...', '..#####..', '.##.#.##.', '#########', '#.#.#.#.#', '.#.....#.']
    for cx, cy, ink in [(18, 57, 2), (34, 57, 5), (50, 57, 4)]:
        for y, row in enumerate(alien):
            for x, c in enumerate(row):
                if c == '#': d.point((cx+x-4, cy+y-3), fill=ink)
    # Mechanical bloom core and eight native-resolution orbiting petals.
    bx, by = 91, 74
    d.ellipse((bx-22, by-20, bx+22, by+20), outline=1)
    for i in range(8):
        angle = i*math.pi/4
        px, py = round(bx+math.cos(angle)*16), round(by+math.sin(angle)*12)
        d.line((bx, by, px, py), fill=6)
        d.rectangle((px-3, py-3, px+3, py+3), fill=1, outline=6)
        d.line((px-2, py-2, px+1, py-2), fill=7)
        d.point((px-2, py-1), fill=9)
    d.polygon([(bx, by-7), (bx+7, by), (bx, by+7), (bx-7, by)], fill=10, outline=6)
    d.rectangle((bx-3, by-3, bx+3, by+3), fill=6)
    d.line((bx-2, by-2, bx+1, by-2), fill=7)
    d.point((bx-2, by-1), fill=9)
    # Downward fans leave a readable gap around the foreground ship.
    for x, y in [(67, 80), (62, 90), (61, 103), (82, 99), (86, 111), (106, 100), (113, 112)]:
        d.line((x, y-2, x, y+2), fill=8)
        d.point((x, y-2), fill=9)
    # Forward shots and a banked, faceted interceptor, drawn at native size.
    d.line((54, 68, 58, 61), fill=4)
    d.line((58, 73, 62, 66), fill=5)
    d.polygon([(31, 102), (25, 119), (34, 114), (39, 105)], fill=6)
    d.polygon([(33, 103), (29, 114), (35, 109), (37, 104)], fill=8)
    hull = [(52, 73), (49, 93), (60, 103), (43, 100), (32, 109), (30, 99), (17, 97), (37, 87)]
    d.polygon(hull, fill=10)
    d.polygon([(50, 78), (46, 93), (55, 100), (41, 97), (33, 105), (33, 96), (23, 96), (39, 89)], fill=2)
    d.polygon([(50, 78), (43, 94), (39, 95), (39, 89)], fill=5)
    d.polygon([(39, 90), (37, 96), (23, 96)], fill=4)
    d.polygon([(43, 95), (54, 100), (41, 98), (33, 105)], fill=4)
    d.line([(49, 80), (42, 93), (38, 96)], fill=9)
    d.line((26, 95, 37, 91), fill=5)
    d.line((43, 98, 52, 100), fill=5)
    d.polygon([(46, 84), (45, 91), (41, 94), (42, 89)], fill=3)
    d.point((44, 87), fill=4)
    # Calm title band: large, hand-authored glyphs and shaped bevels.
    title(im, 'VOID', 5, 4, 5)
    title(im, 'BLOOM', 27, 6, 7)
    text = 'EVOLVING SHOOTER'
    x = (128-(len(text)*4-1))//2
    for ch in text:
        glyph = glyph35(ch)
        if glyph >= 0:
            bits = FONT35[glyph]
            for y in range(5):
                for xx in range(3):
                    if bits[xx] & (1 << y): d.point((x+xx, 121+y), fill=5)
        x += 4
    # The menu's installed border stays distinct from the composition.
    d.rectangle((0, 0, 127, 127), outline=0)
    return im


if __name__ == '__main__':
    from boxart import save
    from chcart import model, runtime, picture
    art = ROOT/'tools/art/cover'; art.mkdir(parents=True, exist_ok=True)
    native = draw(); native.save(art/'cover.png', optimize=True)
    path = save(native, ROOT/'docs/cart.png')
    data = path.read_bytes(); assert model.picture_ok(data)
    pic = runtime.menu_picture(data); assert len(pic) == 8704
    (art/'VOIDBLOO.PIC').write_bytes(pic)
    picture.preview(data, scale=3, mark=True).save(ROOT/'docs/cart-preview.png')
    picture.preview(data, scale=3, mark=True, bar=.5).save(art/'install-preview.png')
    (art/'palette.json').write_text(json.dumps({'size': [128, 128], 'roles': dict(zip(ROLES, COLORS)),
        'used_indices': sorted(i for n, i in native.getcolors()),
        'indexed_pixels_sha256': hashlib.sha256(native.tobytes()).hexdigest(),
        'picture_sha256': hashlib.sha256(pic).hexdigest(), 'picture_bytes': len(pic)}, indent=2)+'\n')
    print(path)
    print('PASS: native indexed cover, legal palette, RGB565 menu picture, installed border and bar previews')
