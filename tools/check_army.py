#!/usr/bin/env python3
"""Compare final army mechanics, RNG and terminal frames with original captures."""

import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


class Screen:
    def __init__(self):
        self.rows = [[' '] * 80 for _ in range(25)]
        self.row = self.col = 0

    def write(self, text):
        for token in re.findall(r'\x1b\[[0-9;]*[A-Za-z]|.', text, re.S):
            if token.startswith('\x1b['):
                command = token[-1]
                args = [int(n) if n else 1 for n in token[2:-1].split(';')]
                if command == 'H':
                    self.row, self.col = args[0] - 1, (args[1] if len(args) > 1 else 1) - 1
                elif command == 'J' and args == [2]:
                    self.rows = [[' '] * 80 for _ in range(25)]
            elif token == '\n':
                self.row += 1
                self.col = 0
            elif token == '\r':
                self.col = 0
            else:
                if 0 <= self.row < 25 and 0 <= self.col < 80:
                    self.rows[self.row][self.col] = token
                self.col += 1

    def text(self):
        return '\n'.join(''.join(row).rstrip() for row in self.rows).strip('\n')


def run(executable, seed, army, enemy, mode='battle'):
    result = subprocess.run([str(executable), str(seed), str(army), str(enemy),
                             mode], capture_output=True, text=True, timeout=10)
    screen = Screen()
    checks = []
    cursor = 0
    for match in re.finditer(r'\nCHECK (\{[^\n]*\})\n', result.stdout):
        screen.write(result.stdout[cursor:match.start()])
        check = json.loads(match[1])
        check['screen'] = screen.text()
        checks.append(check)
        cursor = match.end()
    return result, checks


def matches(actual, reference):
    return all(actual[key] == value for key, value in reference.items())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    events = json.loads(Path('doc/army-events-checkpoints.json').read_text())
    battles = json.loads(Path('doc/army-battles-checkpoints.json').read_text())
    flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined']
    with tempfile.TemporaryDirectory(prefix='braminar-check-army-') as directory:
        executable = Path(directory) / 'army'
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(Path(__file__).with_suffix('.c')), '-o', str(executable)], check=True)
        for case in events:
            result, checks = run(executable, case['seed'], case['army'], case['enemy'], 'event')
            assert result.returncode == 0, (case['name'], result)
            assert matches(checks[-1], case['applied']['state']), (case['name'], checks)
            applied = next(c for c in checks if c['delay'] == 3)
            lines = case['applied']['screen'].splitlines()[:2]
            assert applied['screen'].strip() == '\n'.join(lines).strip(), (case['name'], applied, lines)
            assert checks[-1]['screen'] == '', (case['name'], checks)
            assert checks[-1]['elapsed'] == 3.1, (case['name'], checks)
            assert {k: v for k, v in case['before'].items() if k != 'seed'} == {
                k: v for k, v in case['after'].items() if k != 'seed'}, case['name']
            print(case['name'] + ': effect, RNG, label/text, clearing, delay and player preservation match')
        for case in battles:
            result, checks = run(executable, case['seed'], case['army'], case['enemy'])
            if case.get('abort'):
                assert result.returncode == 1 and 'Division by zero.' in result.stderr, (case['name'], result)
                assert matches(checks[-1], case['casualties']['state']), (case['name'], checks)
                print(case['name'] + ': casualties and zero-denominator termination match')
                continue
            assert result.returncode == 0, (case['name'], result)
            assert matches(checks[-1], case['finished']['state']), (case['name'], checks)
            assert {k: v for k, v in case['before'].items() if k != 'seed'} == {
                k: v for k, v in case['after'].items() if k != 'seed'}, case['name']
            expected_delay = 3.1
            cursor = 0
            before = dict(seed=case['seed'], army=case['army'], enemy=case['enemy'])
            for index, round_ in enumerate(case['rounds']):
                _, casualties = run(executable, before['seed'], before['army'], before['enemy'], 'casualties')
                assert matches(casualties[-1], round_['casualties']['state']), (case['name'], index, casualties)
                references = ([round_['event']] if round_['event'] else []) + [round_['after']]
                for ref in references:
                    while cursor < len(checks) and not (matches(checks[cursor], ref['state']) and
                                                        checks[cursor]['screen'] == ref['screen']):
                        cursor += 1
                    assert cursor < len(checks), (case['name'], index, ref, checks)
                    cursor += 1
                if round_['event']:
                    expected_delay += 3.1
                if case['army'] < 10000:
                    expected_delay += (100 - case['army'] // 100) / 1000
                before = round_['after']['state']
            assert abs(checks[-1]['elapsed'] - expected_delay) < 0.00001, (case['name'], expected_delay, checks[-1])
            assert f"VICTORY {int(case['finished']['state']['army'] > 0)} UNCHANGED 1" in result.stdout, case['name']
            print(case['name'] + ': every casualty batch, event, terminal frame, final RNG and delays match')
    print(f'{len(events)} army event and {len(battles)} army battle scenarios passed')


if __name__ == '__main__':
    main()
