#!/usr/bin/env python3
"""Compare the Overlord duel with controlled original rounds and complete fights."""

import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from army_probe import roll
from check_army import Screen


def screen_text(text):
    return text.replace('\x01', '☺').replace('\x02', '☻').replace('\x7f', '⌂')


def run(executable, config, mode='full', player_hp=0, overlord_hp=0, before_screen=None):
    names = ('seed','level','hp','talisman','ring','poison','swords','bows','arrows','daggers')
    arguments = [str(int(config[n])) if isinstance(config[n], bool) else str(config[n]) for n in names]
    result = subprocess.run([str(executable), *arguments, mode, str(player_hp), str(overlord_hp)],
                            capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result
    assert 'UNCHANGED 1' in result.stdout, result.stdout
    screen = Screen()
    if before_screen is not None:
        # Cleared duel frames start at the figures on row 14; DOS captures trim
        # leading blank rows. Retain prior HP digits between controlled rounds.
        screen.write('\n' * 13 + screen_text(before_screen))
    checks = []
    cursor = 0
    for match in re.finditer(r'\nCHECK (\{[^\n]*\})\n', result.stdout):
        screen.write(result.stdout[cursor:match.start()])
        check = json.loads(match[1])
        check['screen'] = screen.text()
        checks.append(check)
        cursor = match.end()
    return result.stdout, checks


def matches(check, ref):
    return all(k in check and abs(check[k] - v) < (0.0000001 if k.endswith('_hp') else 0.5)
               for k, v in ref.items())


def round_delay(seed):
    seed, a = roll(seed, 8)
    seed, b = roll(seed, 8)
    animations = (0,0,.9,.9,1.2,1.2,1,1)
    return 2 + animations[a-1] + animations[b-1]


def unchanged(case):
    assert {k: v for k, v in case['before'].items() if k != 'seed'} == {
        k: v for k, v in case['after'].items() if k != 'seed'}, case['name']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    rounds = json.loads(Path('doc/duel-rounds-checkpoints.json').read_text())
    full = json.loads(Path('doc/duel-full-checkpoints.json').read_text())
    flags = ['-std=c99','-O0','-g','-Wall','-Wextra','-Wpedantic']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined']
    with tempfile.TemporaryDirectory(prefix='braminar-check-duel-') as directory:
        executable = Path(directory) / 'duel'
        subprocess.run([*shlex.split(os.environ.get('CC','cc')), *flags,
                        str(Path(__file__).with_suffix('.c')), '-o', str(executable)], check=True)
        for case in rounds:
            config = dict(case['before'], hp=case['before']['hp'], seed=case['seed'])
            _, checks = run(executable, config, 'round', case['player_hp'], case['overlord_hp'], case['before_screen'])
            final = checks[-1]
            assert matches(final, case['applied']['state']), (case['name'], final, case)
            applied = next(c for c in reversed(checks) if c['kind'] == 'delay')
            assert applied['screen'] == screen_text(case['applied']['screen']), (case['name'], applied, case)
            assert final['screen'] == screen_text(case['cleared']['screen']), (case['name'], final, case)
            assert abs(final['elapsed'] - round_delay(case['seed'])) < .00001, (case['name'], final)
            unchanged(case)
            print(case['name'] + ': local HP, retaliation, RNG, messages, frames and animation delays match')
        for case in full:
            text, checks = run(executable, case['config'])
            assert matches(checks[0], case['initial']['state']), (case['name'], checks[0], case['initial'])
            cursor = 1
            refs = [r['applied'] for r in case['rounds']]
            if 'talisman' in case:
                # Talisman has no separate delay before the grave; its narrative
                # remains visible in the retained grave/finished frames.
                assert 'You use the Talisman to defeat the Overlord!' in text
            refs += [case['grave'], case['finished']]
            for ref in refs:
                while cursor < len(checks) and not (matches(checks[cursor], {'seed':ref['state']['seed']}) and
                                                    checks[cursor]['screen'] == screen_text(ref['screen'])):
                    cursor += 1
                assert cursor < len(checks), (case['name'], ref, checks)
                # Grave and finished have the same frame on victory; allow reuse.
            expected_delay = 2.5
            seed = case['initial']['state']['seed']
            for round_ in case['rounds']:
                expected_delay += round_delay(seed)
                seed = round_['applied']['state']['seed']
            assert abs(checks[-1]['elapsed'] - expected_delay) < .00001, (case['name'], checks[-1], expected_delay)
            assert checks[-1]['seed'] == case['finished']['state']['seed'], (case['name'], checks[-1])
            victory = case['finished']['state']['player_hp'] > 0
            assert f'VICTORY {int(victory)}' in text, (case['name'], text)
            assert ('The Overlord has slain you in hand - hand combat.' in text) == (not victory), case['name']
            unchanged(case)
            print(case['name'] + ': HP generation, full fight, frames, RNG, routing and player preservation match')
    print(f'{len(rounds)} duel round and {len(full)} complete duel scenarios passed')


if __name__ == '__main__':
    main()
