# Audio update 1.3.2

The higher-register pulse motif now takes precedence over routine gun clicks and survives a held fire button. Kills and important combat cues can still interrupt it. Pause and Help silence the real timer.

Validation uses the actual C++ audio backend and native Rev0 builds. Digital
levels and source checks do not establish measured acoustic loudness or
physical timer margin. These releases retain each public repository's
existing build and save setup; workspace console builds also included a
separate SD-save overlay that is not part of this audio change.

## Release build validation

Built against the clean upstream SDK at
`e1f8d05940ddb7bbef89c13326da75c003f02904` with GCC 16.2.

```text
VoidBloom: build
flash (sections): 27712 B
image: 28020 B of 50944 (save pages free: 2; two need <= 50432)
static RAM: 15296 B of 18416 (stack 2048 B separate)
```
