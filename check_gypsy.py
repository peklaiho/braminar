#!/usr/bin/env python3
"""Compare the Gypsy camp with controlled original-game checkpoints."""

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
            process = subprocess.run([str(executable), str(config['seed']), str(config['hp']),
                                      str(config['gold']), keys, str(int(finish))],
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
    return ' '.join(text.replace('slaves', 'servants').split())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--fixtures', type=Path, default=Path(__file__).resolve().parent / 'doc/gypsy-checkpoints.json')
    args = parser.parse_args()
    cases = json.loads(args.fixtures.read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-gypsy-') as directory:
        executable = Path(directory) / 'check-gypsy'
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
            # The binary requests three half-second delays per gambling round.
            delay = 0
            if case['options'].get('play') and case['config']['gold'] > 0:
                pause = next(step for step in case['steps'] if step['answer'] == ' ')
                seed = case['config']['seed']
                draws = 0
                while seed != pause['state']['seed']:
                    seed = (seed * 129 + 0x361962E9) & 0xFFFFFFFF
                    draws += 1
                    assert draws < 1000
                assert draws % 2 == 1
                delay = (draws - 1) / 2 * 1.5
            assert actual[-1]['delay'] == delay, (case['name'], actual[-1])
            print(case['name'] + ': state, RNG, prompt order, input counts and requested delays match')
    print(f'{len(cases)} Gypsy camp scenarios passed')


if __name__ == '__main__':
    main()
