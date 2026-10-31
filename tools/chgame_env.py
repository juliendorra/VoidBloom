"""Locate the shared CHGame tools in a workspace or standalone checkout."""
from pathlib import Path
import os

ROOT = Path(__file__).resolve().parents[1]
CHGAME_REVISION = 'e1f8d05940ddb7bbef89c13326da75c003f02904'


def chgame_root():
    candidates = ([Path(os.environ['CHGAME_ROOT'])] if os.environ.get('CHGAME_ROOT') else
                  [ROOT.parent / 'CHGame', ROOT / '.deps/CHGame'])
    for path in candidates:
        if (path / 'tools/chgame.py').is_file():
            return path.resolve()
    raise SystemExit('CHGame tools not found. Set CHGAME_ROOT or clone '
                     'https://github.com/bateske/CHGame.git into .deps/CHGame '
                     f'and check out {CHGAME_REVISION}.')
