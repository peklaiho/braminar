#!/usr/bin/env python3
"""Compare the grass hut with controlled original-game checkpoints."""

import argparse
import json
import os
from pathlib import Path
import pty
import re
import shlex
import subprocess
import tempfile


def run(executable, config, keys, finish=True):
    master, slave = pty.openpty()
    try:
        with tempfile.TemporaryFile() as output:
            names = ('seed', 'hp', 'gold', 'food', 'male', 'female', 'medicine', 'poison',
                     'staff', 'talisman', 'charm', 'swords', 'bows', 'arrows', 'daggers', 'horse')
            arguments = [str(int(config[n])) if isinstance(config[n], bool) else str(config[n])
                         for n in names]
            process = subprocess.run([str(executable), *arguments, keys, str(int(finish))],
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
    return ' '.join(text.replace('slaves', 'servants').replace('slave', 'servant').split())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--fixtures', type=Path, default=Path(__file__).resolve().parent / 'doc/hut-checkpoints.json')
    args = parser.parse_args()
    cases = json.loads(args.fixtures.read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-hut-') as directory:
        executable = Path(directory) / 'check-hut'
        build(executable, args.sanitize)
        for case in cases:
            keys = ''.join(step['answer'] for step in case['steps'] if step['kind'] == 'key')
            code, text = run(executable, case['config'], keys, not case.get('fatal', False))
            matches = list(re.finditer(r'CHECK (\{[^\r\n]*\})', text))
            actual = [json.loads(m[1]) for m in matches]
            expected = case['steps']
            expected = expected + [dict(kind='final', state=case['final'])]
            assert code == 0, (case['name'], text)
            assert len(actual) == len(expected), (case['name'], text)
            for i, (check, reference) in enumerate(zip(actual, expected)):
                assert check['kind'] == reference['kind'], (case['name'], i, check, reference)
                for key, value in reference['state'].items():
                    result = check[key]
                    assert (result == value if isinstance(value, str) else
                            abs(result - value) <= (0.005 if key == 'hp' else 0)), (case['name'], i, key, result, value)
                if reference['kind'] != 'final':
                    prefix = text[matches[i - 1].end() if i else 0:matches[i].start()]
                    assert normalize(reference['prompt']) in normalize(prefix), (case['name'], i, reference['prompt'], prefix)
                    narrative = re.sub(r'CHECK \{[^\r\n]*\}', '', text[:matches[i].start()])
                    assert normalize(reference['screen']) in normalize(narrative), (case['name'], i, reference['screen'], narrative)
            assert actual[-1]['keys'] == len(keys), case['name']
            delay = 0
            if case['options'].get('help'):
                delay = 1
            if any('You are confronted by ' in step['screen'] for step in case['steps']):
                delay = 0.5
            assert actual[-1]['delay'] == delay, (case['name'], actual[-1])
            print(case['name'] + ': state, RNG, prompt order, input counts and requested delays match')
    print(f'{len(cases)} grass hut scenarios passed')


if __name__ == '__main__':
    main()
