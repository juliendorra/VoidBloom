# Build Void Bloom

The repository contains the game, its captures, and its test tools. CHGame
provides the board package, libraries, simulator, uploader, and cartridge
packer as a separate dependency.

## Host tests and simulator

Use Python 3.11 or newer and a C++ compiler (`clang++`, `g++`, or Zig).

```sh
git clone https://github.com/juliendorra/VoidBloom.git
cd VoidBloom
git clone https://github.com/bateske/CHGame.git .deps/CHGame
git -C .deps/CHGame checkout e1f8d05940ddb7bbef89c13326da75c003f02904
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
python chgame.py test
python chgame.py sim
```

For an existing CHGame checkout, set `CHGAME_ROOT` to its absolute path
instead of cloning it. The simulator uses the game's C++ rules and renderer.
Tests include 48 delayed-observation campaign pilots, a separate first-boss
dodge fixture, passive pressure measurements, and 80 framebuffer checks of
ship visibility at the movement boundaries. They run faster than wall
time while the game still advances in 1/60-second steps; they are not human
playtests. Results are written to `out/balance.json`.

## Console build and cartridge

Install [Arduino CLI](https://arduino.github.io/arduino-cli/) and the
[CHGame board package](https://github.com/bateske/CHGame#installing).
With the Python environment above active:

```sh
arduino-cli config add board_manager.additional_urls https://github.com/bateske/CHGame/releases/latest/download/package_chgame_index.json
arduino-cli core update-index
arduino-cli core install CHGame:ch32v@0.3.0
python chgame.py build
python chgame.py export --no-build
python chgame.py cart verify build/VoidBloom.chgame
```

`build/release/VoidBloom.ino.bin` is the Rev0 firmware;
`build/VoidBloom.chgame` contains the installable game and its metadata.
`python chgame.py upload` installs and starts the release on a connected
CHGame. Firmware targets `CHGame:ch32v:rev0` with size optimization and
upload-only USB. The build checks flash, static RAM, and space for both save
pages. The save identifier is `VBLM`.

## Captures

```sh
python tools/verify_sim.py --capture
python tools/capture_readme.py
python tools/verify_boundaries.py
python tools/capture_levels.py
```

`verify_sim.py --capture` updates the progression screenshots and README
preview. `capture_readme.py` can recreate just that preview: it starts at
the title screen, replays one ordinary campaign through wave 12, and picks
active excerpts with horizontal and vertical movement. It keeps normal
play speed, uses no stage jumps, and writes motion and timing details to
`docs/gameplay.json`. The pilot uses a vertical patrol preference for this
preview; benchmark pilots retain their usual preferences.

`verify_boundaries.py` checks the full sprite at the movement limits.
`capture_levels.py` records individual GIFs for waves 1–12 in
`docs/captures/`. These separate clips stage wave starts with the
simulator-only capture fixture. Normal firmware and the ordinary simulator
do not contain stage-selection commands.

## Offline web emulator

Download and extract `CHGame_Emulator-web.zip` from the
[CHGame emulator v1.2 release](https://github.com/joyrider3774/chgame_emulator/releases/tag/v1.2).
After building the release firmware, run:

```sh
python tools/make_artifact.py --emulator /path/to/CHGame_Emulator-web --output out/web/void-bloom.html
```

This produces the HTML fragment and a firmware hash manifest. Release ZIPs
also include a standalone `index.html`, ready to open in a modern browser.
The emulator executes the actual RISC-V firmware. Its MIT license is
embedded in the page. The hosted emulator is linked from the main README.
