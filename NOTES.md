# Void Bloom implementation and validation

`Game::update()` advances exactly one 1/60-second step. No wall clock or heap allocation is involved in the rules. Fixed pools hold 192 enemy bullets, 40 player shots, 24 enemies, and 32 sparks. Per-wave gameplay budgets further limit hostile bullets. Saturation defers new whole volleys without replacing an existing bullet. Fixed-point positions use 1/64-pixel units; directions use an integer sine table. Saved data has its own `VBLM` identifier.

The flash/RAM budget is checked by the existing `chgame build` tool, including space for both save pages and the separate 2 KB stack. Frame scheduling targets 60 Hz; the native simulator's virtual clock is not a hardware performance measurement. On 31 October 2026, the verified 24,400-byte release was uploaded to a physical CHGame Rev0 (CX35), bootloader v3. The bootloader accepted flash CRC32 `0x1906ADCD`, RUN was sent, and the application reappeared on USB. This confirms upload and launch, not a physical gameplay or performance measurement.

## Wave 2: Awaken (1.1.0)

This bridge keeps phase-zero movement and the single shot. Pale cyan eyes, shots, and HUD accents introduce color while the rest of the formation stays monochrome. Starting 1.5 seconds into play, one invader charges for 20 frames, curls upward through a 48-frame three-quarter circle, then commits to a 72-frame dive. Its aim is locked at the start of the dive and marked briefly near the pilot; it cannot home onto a dodge. After leaving the bottom it returns through the top over 64 frames and rejoins its moving slot. At most one sortie is active. Killing or bombing it removes it normally.

The 1.1.0 release is 25,720 bytes with 15,272 bytes of static RAM and both save pages preserved. On 31 October 2026 it was uploaded to the same physical Rev0; the bootloader accepted CRC32 `0xBD8FB88C`, RUN was sent, and the application reappeared on USB. Opening firmware emulation measured 60.00 fps. These checks do not measure dense-wave performance on the physical device.

Integer interpolation of the existing sine samples keeps the loop smooth. The empty logical slot remains in the formation bounds, including when the diver is the last survivor. A unit check covers charge/loop/dive/return, path continuity, upward travel, fixed aim, exact rejoining, and kills during flight. Passive wave-two bullet pressure must sit between waves one and three; the existing 48 delayed-observation pilots still run the whole campaign without cheats.

## First-boss breathing room (1.1.1)

Human play exposed crowding on wave 4 despite the earlier heuristic campaign passing. `Game::bulletLimit()` now centralizes adjustable encounter budgets: waves 1–12 allow 4, 6, 10, 12, 24, 28, 40, 56, 72, 80, 88, and 104 hostile bullets, with a bounded 128 thereafter. Every emission path obeys the budget. Rings and fans only fire when the entire volley fits; bullets already in flight never disappear or change course at the limit.

Wave 4 uses eight-spoke rings and slower firing intervals, with two seconds of attack followed by one second without new shots before each next pattern. Later bosses retain their faster attack schedules. Cap checks and full-volley admission run once per volley, without a per-frame counter or larger state.

The extended pressure fixture observes bosses for 60 seconds, covering repeated full pattern cycles. A separate staged wave-4 test uses 24 delayed-observation pilots for 60 seconds with movement alone, no fire, focus, bombs, invulnerability, or health changes. All 24 survived without a hit, and the measured peak stayed at 12. The 48 normal campaign pilots also reached wave 13. These heuristic results support tuning; actual human reports remain the basis for assessing readability.

The 1.1.1 release is 25,948 bytes with 15,272 bytes static RAM and both save pages preserved. On 31 October 2026 the connected Rev0 accepted CRC32 `0x734C1E2E` and confirmed application launch after RUN. Actual release firmware emulation measured 60.00 fps in the opening window with no CPU faults; dense-boss hardware performance is not measured by that check.

## Full ship bounds (1.1.2)

The old center limits let the ship reach y=119, where the playfield ends, so its lower hull and exhaust were clipped by the bottom HUD. Focus markers could also pass the left and right edges. Movement now reserves the complete footprint: center x=7–120 and y=58–112. This keeps the focus markers (seven pixels to either side), focus ring, side pods, and the longest exhaust plume inside the field. The upper movement band, hitbox, weapons, bullet budgets, and enemy patterns are unchanged.

Shared playfield and ship extents connect movement to the renderer's clipping region. `tools/verify_boundaries.py` checks 80 normal/focus edge and diagonal cases across all five weapon tiers. It uses the real rendered ship at an interior position as its reference, then compares every visible ship pixel at the boundary, including the longest exhaust frame. It reproduced the old failure before the fix and is now part of the standard test command. The empty-arena command is simulator-only.

The same 48 seeds and 100/150/200 ms pilot delays completed the wave-13 milestone both before and after the fix. Total deaths across those campaigns rose from 24 to 37; bomb uses rose from 435 to 459; median completion time changed from 190.41 to 191.94 seconds. The added deaths are concentrated in the later waves, especially wave 12. The first-boss movement-only test still completed 24/24 without a hit. The fix removes some previously available edge space and makes later dodging harder, while preserving completion for this cohort. These are heuristic simulations, not human completion-rate measurements. [docs/bounds-comparison.json](docs/bounds-comparison.json) contains the paired results.

[Bottom-edge comparison](docs/bounds-before-after.png) shows 1.1.1 on the left and 1.1.2 on the right. [Focused corner captures](docs/boundary-corners.png) show the complete ship at all four corners of the movement band.

On 1 November 2026, the connected Rev0 accepted the 25,948-byte 1.1.2 release with CRC32 `0x090290A8` and confirmed application launch after RUN. Static RAM remains 15,272 bytes and both save pages are preserved. The standalone checkout produces the same firmware bytes. Release firmware emulation measured 60.00 fps in the opening window with zero CPU faults; this does not measure dense-wave performance on the physical console.

## Reproduce

See [BUILDING.md](BUILDING.md) for the standalone setup and pinned CHGame dependency. Run these commands from the game folder. The launcher also recognizes the existing isolated board installation when used inside the development workspace.

```sh
python3 chgame.py test
python3 chgame.py build
python3 chgame.py export --no-build
python3 chgame.py cart verify build/VoidBloom.chgame
python3 tools/verify_sim.py --capture
python3 tools/capture_levels.py
```

Tests compile through CHGame's host-test tool with undefined-behavior sanitization. They cover movement locks/bounds, weapon tiers, focus, pause, grazing, bomb priority and button edges, pool saturation, death/restart, all evolution transitions, and an endless run. That mechanical endurance run uses invulnerability and is explicitly separate from the campaign pilots.

`tools/game.py` also builds and runs **48 seeded campaign pilots**. Their observations are delayed by 100, 150, or 200 ms, and controls can change only at that cadence. They extrapolate visible linear bullet trajectories and recent enemy motion, choose among ordinary D-pad directions, fire, focus, and bomb. Their own position is tracked from their movement. They have no knowledge of future enemy decisions and cannot alter health, invulnerability, bullets, waves, or damage. Small seeded aim lapses vary the runs. The six-minute simulated-time limit targets clearing the first Infinity boss (wave 12), because there is no final level in an endless game.

Tests run faster than real time on the host; the simulated rules still advance at 60 Hz. These are heuristic agents, not human subjects, and cannot prove human enjoyment, accessibility, or completion rates. They do provide reproducible evidence about survivability and the early-to-late ramp. The checked record is [docs/balance.json](docs/balance.json); reruns write `out/balance.json`.

Pressure measurement uses 20 seeds per encounter, no firing, and a stationary target. Only this separate fixture uses invulnerability so it can observe the whole pattern. Mean active hostile bullets must increase through regular encounter tiers and through boss tiers independently. Boss peaks are intentionally followed by recovery waves; difficulty is not claimed to increase monotonically on every individual wave. The seeded campaign must achieve at least 30 completions out of 48 for the balance check to pass.

## Captures and playable artifact

The menu's dedicated cover is authored at 128 × 128 in `tools/cart.py`, with native-size pixel lettering, original mechanical bloom and ship artwork, and semantic palette roles. `tools/art/cover/cover.png` is the indexed source output; its palette and pixel hashes are recorded alongside it. The recipe validates the menu-picture rules and writes an 8,704-byte `VOIDBLOO.PIC`. `docs/cart-preview.png` shows the decoded menu colors and installed border; the separate install-bar preview checks that the title and primary ship remain clear of that overlay. The cartridge embeds this cover through `chgame.json`'s existing `cartImage` entry. The menu picture changes packaging, not game firmware.

`docs/captures/` contains twelve separate real-speed GIFs, one for each wave through the first full progression. Each is 384 × 384 and smaller than 1 MB. Boss clips last ten seconds to include all three attack families; other clips last six seconds. `tools/capture_levels.py` records the simulator framebuffer every six 60 Hz updates with a 100 ms GIF duration. It stages wave starts, enables ordinary damage, and replays the existing delayed-observation pilot for waves 3–12. Replay positions, lives, and bullet counts must match the rules model after every sample. The gallery index, contact sheet, and hash manifest are generated alongside the GIFs. These are showcase clips, not a continuous playthrough.

`tools/verify_sim.py --capture` uses CHGame's existing simulator, debug protocol, and framebuffer decoder. It verifies the ordinary title, help, mute, fire, bombs, pause/resume, and quit controls with no torn frames. Progression screenshots use a separately compiled `BLOOM_CAPTURE` fixture to stage later waves; these screenshots are examples, not a claimed no-hit playthrough. The `J` staging command is absent from the normal simulator and cartridge.

The README's `docs/gameplay.gif` is generated by `tools/capture_readme.py`, also invoked by `verify_sim.py --capture`. It captures the normal simulator from the title screen through one complete wave-12 campaign, then selects chronological excerpts with continuous firing and visible movement. The preview pilot has a vertical patrol preference; the ordinary benchmark policy is unchanged. Every free-flight excerpt must span at least 24 horizontal pixels and 12 vertical pixels, with the ship moving in at least half of the control samples. Excerpts avoid the normal post-hit/bomb blink period so the ship remains visible. There are no stage jumps, forced clears, or modified health in this recording. The replay checks position, score, wave, lives, bullets, and other state against the rules model after every six frames. Natural evolution cards are included between play excerpts.

The GIF samples every four 60 Hz updates. A repeating 60/70/70 ms frame-duration pattern preserves normal play speed at 15 samples per second. It stays below 1 MB using nearest-neighbor scaling; `docs/gameplay.json` records its timing, source run, movement measurements, and SHA-256. The README image URL includes that hash to refresh cached previews.

`tools/make_artifact.py` embeds the existing **joyrider3774/chgame_emulator v1.2** JavaScript and WebAssembly release plus the actual release firmware into an offline HTML fragment, with captures and touch controls. It does not reimplement gameplay. The upstream runtime is unchanged except for escaping a closing script tag for embedding; its machine and bootloader execute the real RISC-V instructions. The emulator's MIT license is included in the preview. The fragment is less than 1 MB, requires no API or ROM download, and uses the existing visualization wrapper for local QA.

Download `CHGame_Emulator-web.zip` from the emulator's v1.2 release, extract it, then run:

```sh
python3 tools/make_artifact.py --emulator /path/to/CHGame_Emulator-web --output /absolute/path/void-bloom.html
```

The resulting `manifest.json` records SHA-256 hashes of firmware and emulator. Native firmware sanity checks use the existing `chg_headless_fixed` binary in `.cache/chgame-emulator-review/`; the emulator and native simulator have different purposes and both are exercised.

## Gun controls (1.2.0)

B now uses a panic bomb. SELECT toggles wide and narrow fire from wave 5 onward; waves 1–4 have no spread to narrow, so SELECT has no gameplay action there. The first spread-weapon transition pauses combat for an animated “Focus fire with Select!” introduction. Every switch shows “Switched gun!” and a mode label. A cyan circle remains around the ship in narrow mode. Movement stays at full speed in either mode, and only A fires the weapon. The selection persists across waves. Bomb-counter icons are cyan rings with white center dots.

The same 48 campaign seeds all cleared wave 12 with 100–200 ms delayed observations and input cadence. This control-aware pilot chooses full-speed directions and presses SELECT for narrow fire on wave 6 and bosses, using ordinary button edges. It no longer chooses slow movement. The new runs recorded 53 deaths and 471 bombs, with a median clear time of 183.26 seconds; 1.1.2 recorded 37 deaths, 459 bombs and 191.94 seconds. Both controls and the pilot policy changed, so this comparison does not isolate a single causal effect. These are heuristic simulations, not actual human playtests. Enemy patterns and bullet caps are unchanged; passive pressure remains progressive and all 24 movement-only first-boss pilots survived without hits.

Rules checks cover early-wave lockout, one toggle per press, no automatic firing from SELECT, equal movement speed, shot velocities, mode persistence, the first unlock, and paused animation timers. The actual simulator verifies physical B bombs and SELECT switching, plus 80 complete-sprite boundary cases. The README replay reaches wave 13 with 1,843 matching state samples and shows horizontal and vertical flight. `docs/focus-unlock.gif` shows the introduction, and the wave-6 capture shows animated switching and the persistent circle.

## Free-flight introduction (1.2.1)

The first frame of wave 3's existing evolution transition now introduces vertical movement with a large animated “Move up and down!” message. Two centered lines slide in and bounce vertically above a D-pad reminder. The introduction uses the existing transition clock, so combat timing, movement, weapons, and difficulty are unchanged. The normal campaign replay records it in `docs/movement-unlock.gif` and the README sequence.

## Standalone tutorial text (1.2.2)

The wave-3 movement and wave-5 gun introductions now draw only large outlined words over the frozen start of the level. The text shrinks gently by about 11% during the first 90 frames, then holds. Panel borders, auxiliary captions, sliding and bouncing are removed from these two introductions. Fractional nearest-neighbor scaling uses the existing CHGfx 5×7 glyphs. The starfield and ship exhaust use the fixed entry clock; outgoing-wave sparks are hidden. Gameplay rules and transition durations are unchanged. The normal campaign capture checks that background pixels outside the text stay identical while the text bounds shrink.

## Rumble pack and mode labels (1.3.0)

The unchanged Pinball `src/rumble` driver provides 1 kHz PWM on GPIO1 / PC0 / H1.8 through TIM2 CH4, independently of TIM1 sound. Its interrupt expires every pulse even while rendering or USB work is blocked; the driver caps pulses at 500 ms. `Haptics.cpp` routes actual game events: ordinary kills 90/255 for 22 ms (at most one per 90 ms), gun switches 105 for 35 ms, bonus rewards 155 for 70 ms, evolution 180 for 110 ms, bombs three fading ripples over all 30 blast frames (500 ms at 60 Hz), bosses 240 for 240 ms, and damage 255 for 180 ms. Stronger effects take priority. Pause, help and title stop output immediately; the final damage pulse can finish on the result screen. Sound mute is independent. A dedicated BossDown event distinguishes a boss defeat from a bonus-life reward.

SELECT now shows “Narrow fire!” or “Spread fire!” in the lower alert position. The redundant small mode caption is removed; the circle continues to identify narrow mode. The glyphs and shadow stay above the complete ship at the upper movement boundary.

The connected Rev0 passed boot-off, immediate pause stop, all three ripple crests and their gaps, and 35 ms refresh expiry without advancing game frames. Samples were 235, 0, 210, 0, 185 and 0/255 across the 30-frame blast. The ordinary release was restored and accepted CRC `45F981D5`. These checks confirm the controller and timer on the device; subjective motor feel remains a human check. Debug status formatting now avoids variadic printf, whose output was garbled on the compact handheld C runtime. The native simulator and physical board use the same numeric state protocol.
