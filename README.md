# Void Bloom

An endless arcade shooter that starts with monochrome marching invaders and transforms into a neon bullet storm. Evolve your ship, thread the gaps, and bring down the Bloom Cores.

![Void Bloom gameplay](docs/gameplay.gif?preview=fdca42d66e2b)

Normal-speed excerpts from one simulator campaign, showing movement, gun switching, and weapon upgrades.

## Controls

| Button | Action |
|---|---|
| D-pad | Move; up/down unlock on wave 3 |
| A | Hold to fire; confirm; retry |
| B | Press for a panic bomb; title: controls; pause: end run |
| SELECT | Toggle wide/narrow fire from wave 5; title/pause: toggle sound |
| START | Pause/resume |

Wave 5 introduces spread fire with an animated **“Focus fire with Select!”** prompt. SELECT then switches between wide coverage and a tighter stream for concentrating shots on a target. Movement stays at full speed. **“Switched gun!”** confirms each change, and the circle around the ship stays visible in narrow mode. The selector is inactive during waves 1–4. Bombs appear as small rings with center dots beside **B** in the bottom HUD.

The existing web emulator uses **arrows / WASD**, **X / Space** for A, **Z** for B, **Right Shift / Backspace** for SELECT, and **Enter** for START. The playable preview also supplies touch controls.

## Rules

| Waves | Evolution | New abilities and pressure |
|---|---|---|
| 1 | Origin | Horizontal movement, one shot, monochrome marching formations |
| 2 | Awaken | First cyan accents; invaders charge, loop upward, and dive out of formation |
| 3–4 | Vector | Free flight, twin shot, moving attackers, first boss |
| 5–6 | Surge | Three-way spread, grazing rewards, radial volleys |
| 7–8 | Nova | Five-way fire, denser fans and spirals |
| 9 onward | Infinity | Double shot damage, tougher enemies, the largest barrages |

A boss appears every fourth wave, cycling radial rings, aimed fans, and rotating streams. Regular waves after bosses provide a recovery interval. Early fighters peel away before reaching the pilot; later ones commit farther down-screen. The small center of your ship is the collision core. From wave 5, press SELECT to tighten the spread around one target or switch back to wide coverage. Press B to clear danger with a bomb.

The first boss fires short bursts with a one-second breathing interval. Wave 4 allows at most 12 hostile bullets at once; later waves gradually allow denser patterns. Stay mobile while the aimed fans commit, then use the gap to reposition.

Wave 2 keeps the opening controls and weapon. One invader at a time flashes before leaving its slot, curls upward, then locks onto your current horizontal position for a diving attack. Sidestep the pink brackets: the diver does not track your dodge. Surviving invaders return to the formation from above.

Start with three lives and three bombs. Bombs clear hostile bullets, damage enemies, and briefly protect the ship. Five kills raise the score multiplier, up to ×8; a missed enemy, death, or three seconds without a kill resets it. Close bullet passes score once per bullet. Every 40 grazes award a bomb. Evolution and boss victories also refill one bomb; every 20,000 points awards a life and bomb, capped at five each.

## How to play

Load the cartridge, then press A. Clear the first formations to evolve; later waves continue after their attackers are destroyed or leave. The game continues indefinitely. Clearing wave 12 and reaching wave 13 is the first full progression milestone. High score and sound preference save independently of the other games.

Download the cartridge, firmware, or offline web emulator from [Releases](https://github.com/juliendorra/VoidBloom/releases). [Play in the hosted emulator](https://void-bloom-arcade.juliendorra.chatgpt.site).

From a source checkout, use `python3 chgame.py build`, then `python3 chgame.py export --no-build`. `python3 chgame.py upload` installs the release on a connected handheld. See [BUILDING.md](BUILDING.md) for dependencies and setup.

## Developer notes

- [Game.cpp](Game.cpp) contains the same fixed-step, allocation-free rules used by the firmware and native simulator.
- [View.cpp](View.cpp) draws the five visual stages directly into CHGfx's packed 128×128 framebuffer.
- [VoidBloom.ino](VoidBloom.ino) uses CHGame input, audio, asynchronous display transfers, and flash saves.
- [tools/tests/pilots.cpp](tools/tests/pilots.cpp) runs delayed-observation pilots using ordinary buttons; pressure fixtures are separate from no-cheat campaign runs.
- [NOTES.md](NOTES.md) describes validation, captures, packaging, and the offline web-emulator artifact.

Browse the [wave-by-wave GIF gallery](docs/captures/README.md) for key moments from waves 1–12, or recreate it with `python3 tools/capture_levels.py`.

## Credits

Original game rules, artwork, and pulse motif for a retro arcade shooter and bullet-heavy shoot 'em up. Uses Kevin Bates' CHGame / CHGfx libraries and the CHGame board package. Browser preview uses joyrider3774's CHGame emulator (MIT), executing the compiled RISC-V firmware.
