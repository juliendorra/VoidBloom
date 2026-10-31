"""Bounded-pool shooter rules, sanitizer tests and native button checks."""
import json
import subprocess

TESTS = {
    "rules": {"sources": ["tools/tests/rules.cpp", "Game.cpp"], "args": "none"},
    "pilots": {"sources": ["tools/tests/pilots.cpp", "Game.cpp"], "run": False},
}
EXTRA_TEST_SCRIPTS = ["tools/verify_sim.py", "tools/verify_boundaries.py"]

def after_tests(ctx, exes):
    ctx.out.mkdir(exist_ok=True)
    runs = subprocess.run([str(exes['pilots']), '48'], capture_output=True, text=True, check=True)
    pressure = subprocess.run([str(exes['pilots']), '--pressure'], capture_output=True, text=True, check=True)
    dodge = subprocess.run([str(exes['pilots']), '--boss-dodge'], capture_output=True, text=True, check=True)
    report = json.loads(runs.stdout)
    report['pressure'] = json.loads(pressure.stdout)
    report['first_boss_dodge'] = json.loads(dodge.stdout)
    report['limitations'] = [
        'Heuristic agents with delayed observations and bounded inputs, not actual human playtests.',
        'Simulation is accelerated in wall time; every game update is exactly 1/60 second.',
        'Campaign pilots use no cheats. Passive pressure fixtures use invulnerability and are reported separately.',
        'The game is endless. Completion here means clearing wave 12 and reaching wave 13.',
        'Boss peaks are followed by recovery waves; the phase-level ramp is measured separately from these peaks.',
    ]
    (ctx.out / 'balance.json').write_text(json.dumps(report, indent=2) + '\n')
    field = [r['mean_bullets'] for r in report['pressure']['encounters'] if not r['boss']]
    bosses = [r['mean_bullets'] for r in report['pressure']['encounters'] if r['boss']]
    monotonic = lambda values: all(a < b for a, b in zip(values, values[1:]))
    caps = all(r['peak_bullets'] <= r['bullet_limit'] for r in report['pressure']['encounters'])
    dodge_ok = report['first_boss_dodge']['survived'] == report['first_boss_dodge']['pilots']
    ok = report['completed'] >= 30 and monotonic(field) and monotonic(bosses) and caps and dodge_ok
    print(f"{'PASS' if ok else 'FAIL'}: {report['completed']}/48 human-bounded pilots clear wave 12; regular-wave pressure {field}; boss pressure {bosses}")
    print(f"First boss, movement only for 60 seconds: {report['first_boss_dodge']}")
    return ok
