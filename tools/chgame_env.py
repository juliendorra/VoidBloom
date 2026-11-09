"""Locate the shared CHGame tools in a workspace or standalone checkout."""
from pathlib import Path
import os

ROOT = Path(__file__).resolve().parents[1]
CHGAME_REVISION = 'edf822c8803a4939f4d0808f779d9522ee41c2ba'


def chgame_root():
    candidates = ([Path(os.environ['CHGAME_ROOT'])] if os.environ.get('CHGAME_ROOT') else
                  [ROOT / 'CHGame', ROOT.parent / 'CHGame', ROOT / '.deps/CHGame'])
    for path in candidates:
        if (path / 'tools/chgame.py').is_file():
            return path.resolve()
    raise SystemExit('CHGame tools not found. Set CHGAME_ROOT or clone '
                     'https://github.com/bateske/CHGame.git into .deps/CHGame '
                     f'and check out {CHGAME_REVISION}.')
