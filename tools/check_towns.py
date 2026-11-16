#!/usr/bin/env python3
"""Compare City and Village with controlled original-game checkpoints."""

import argparse
import json
import os
from pathlib import Path
import pty
import re
import shlex
import subprocess
import tempfile


def arguments(config, keys, lines, finish=True):
    names = ('seed', 'hp', 'gold', 'food', 'male', 'female', 'medicine', 'ring',
             'plague', 'looks', 'horse', 'cart')
    return [str(int(config[n])) if isinstance(config[n], bool) else str(config[n])
            for n in names] + [keys, lines, str(int(finish))]


def run(executable, config, keys, lines, finish=True):
    master, slave = pty.openpty()
    try:
        with tempfile.TemporaryFile() as output:
            process = subprocess.run([str(executable), *arguments(config, keys, lines, finish)],
                                     stdin=slave, stdout=output, stderr=output, timeout=10)
            output.seek(0)
            text = output.read().decode()
    finally:
        os.close(slave)
        os.close(master)
    return process.returncode, text


def build(executable, sanitize=False):
    flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
    if sanitize:
        flags += ['-fsanitize=address,undefined']
    subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                    str(Path(__file__).with_suffix('.c')), '-o', str(executable)], check=True)


def normalize(text):
    return ' '.join(text.replace('slaves', 'servants').replace('Slaves', 'Servants')
                    .replace('slave Market', 'servant Market').split())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--fixtures', type=Path, default=Path(__file__).resolve().parent.parent / 'doc/town-checkpoints.json')
    args = parser.parse_args()
    cases = json.loads(args.fixtures.read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-towns-') as directory:
        executable = Path(directory) / 'check-towns'
        build(executable, args.sanitize)
        for case in cases:
            keys = ''.join(step['answer'] for step in case['steps'] if step['kind'] == 'key')
            lines = ''.join(step['answer'] + '\n' for step in case['steps'] if step['kind'] == 'line')
            code, text = run(executable, case['config'], keys, lines, not case.get('fatal', False))
            matches = list(re.finditer(r'CHECK (\{[^\r\n]*\})', text))
            actual = [json.loads(m[1]) for m in matches]
            expected = case['steps']
            if not case.get('aborted'):
                expected = expected + [dict(kind='final', state=case['final'])]
            assert (code != 0 if case.get('aborted') else code == 0), (case['name'], text)
            assert len(actual) == len(expected), (case['name'], text)
            for i, (check, reference) in enumerate(zip(actual, expected)):
                assert check['kind'] == reference['kind'], (case['name'], i, check, reference)
                for key, value in reference['state'].items():
                    if key == 'seed':
                        continue
                    result = check[key]
                    assert (result == value if isinstance(value, str) else
                            abs(result - value) <= (0.005 if key == 'hp' else 0)), (case['name'], i, key, result, value)
                if reference['kind'] != 'final':
                    prefix = text[matches[i - 1].end() if i else 0:matches[i].start()]
                    assert normalize(reference['prompt']) in normalize(prefix), (case['name'], i, reference['prompt'], prefix)
                    if 'screen' in reference:
                        screen = reference['screen']
                        marker = 'You come upon a City.'
                        if marker in screen:
                            screen = screen[screen.index(marker):]
                        narrative = re.sub(r'CHECK \{[^\r\n]*\}', '', text[:matches[i].start()])
                        assert normalize(screen) in normalize(narrative), (case['name'], i, screen, narrative)
            if case.get('aborted'):
                assert 'Division by zero.' in text, text
                print(case['name'] + ': original division-by-zero termination preserved')
                continue
            assert actual[-1]['keys'] == len(keys), case['name']
            assert actual[-1]['lines'] == len(case['steps']) - len(keys), case['name']
            delay = 0
            if case['name'].startswith('cleric-') or case['name'].startswith('darts-'):
                delay = 0.5
            elif case['name'].startswith('inn-') and case['name'] != 'inn-insufficient':
                delay = 2
            elif case['name'] == 'muggers-loss':
                delay = 1
            elif case['name'] == 'city-cleric-then-muggers':
                delay = 1.5
            assert actual[-1]['delay'] == delay, (case['name'], actual[-1])
            print(case['name'] + ': state, prompt order, input counts and requested delays match')
    print(f'{len(cases)} City/Village scenarios passed')


if __name__ == '__main__':
    main()
