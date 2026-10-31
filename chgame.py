#!/usr/bin/env python3
"""Build, test, capture and package this standalone CHGame sketch."""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tools'))
from chgame_env import chgame_root

env = dict(os.environ)
workspace = ROOT.parent
# Reuse an existing isolated board installation when inside the development
# workspace. Standalone clones use the user's normal Arduino CLI settings.
if (workspace / 'arduino-cli.yaml').is_file():
    for key, rel in {
        'ARDUINO_CONFIG_FILE': 'arduino-cli.yaml',
        'ARDUINO_DIRECTORIES_DATA': '.arduino15',
        'ARDUINO_DIRECTORIES_DOWNLOADS': '.arduino15/staging',
        'ARDUINO_DIRECTORIES_USER': 'sketchbook',
        'ARDUINO_BUILD_CACHE_PATH': '.arduino15/cache',
        'TMPDIR': '.cache/tmp',
    }.items():
        env[key] = str(workspace / rel)
    (workspace / '.cache/tmp').mkdir(parents=True, exist_ok=True)
python = next((p for p in (ROOT / '.venv/bin/python', workspace / '.venv/bin/python',
                          workspace / '.cache/voidbloom-venv/bin/python') if p.is_file()), Path(sys.executable))
raise SystemExit(subprocess.call([
    str(python), str(chgame_root() / 'tools/chgame.py'),
    '--sketch', str(ROOT), *(sys.argv[1:] or ['build']),
], env=env, cwd=ROOT))
