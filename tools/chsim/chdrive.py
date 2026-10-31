#!/usr/bin/env python3
"""Void Bloom uses CHGame's shared simulator driver and button protocol."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from chgame_env import chgame_root
sys.path.insert(0, str(chgame_root() / 'tools/chsim'))
from chdrivelib import main
if __name__ == '__main__':
    raise SystemExit(main(ident='BLOOM'))
