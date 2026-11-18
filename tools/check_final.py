#!/usr/bin/env python3
"""Compare assault asset conversion against retained original-game checkpoints."""

import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--fixtures', type=Path, default=Path('doc/final-checkpoints.json'))
    args = parser.parse_args()
    cases = json.loads(args.fixtures.read_text())
    flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
    if args.sanitize:
        flags.append('-fsanitize=address,undefined')
    with tempfile.TemporaryDirectory(prefix='braminar-check-final-') as directory:
        executable = Path(directory) / 'check-final'
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(Path(__file__).with_suffix('.c')), '-o', str(executable)], check=True)
        for case in cases:
            config = case['config']
            names = ('seed', 'gold', 'male', 'female', 'medicine', 'poison', 'swords',
                     'bows', 'arrows', 'daggers', 'horse', 'cart', 'level')
            values = [str(int(config[n])) for n in names]
            values += [str(config.get('male_price', 8)), str(config.get('female_price', 13))]
            result = subprocess.run([str(executable), *values, '0'],
                                    capture_output=True, text=True, check=True)
            actual = json.loads(re.search(r'RESULT (.*)', result.stdout)[1])
            for key in ('net_worth', 'army', 'enemy'):
                assert actual[key] == case[key], (case['name'], key, actual, case)
            assert actual['seed'] == case['converted']['seed'], (case['name'], actual, case)
            assert actual['unchanged'], case['name']
            assert {k: v for k, v in case['entry'].items() if k != 'seed'} == {
                k: v for k, v in case['converted'].items() if k != 'seed'}, case['name']
            result = subprocess.run([str(executable), *values, '1'],
                                    capture_output=True, text=True, check=True)
            assert ''.join(case['screen'].split()) in ''.join(result.stdout.split()), (case['name'], result.stdout)
            assert 'DELAY 3.000' in result.stdout, (case['name'], result.stdout)
            print(case['name'] + ': conversion, RNG, preserved player, text and delays match')
    print(f'{len(cases)} final-assault conversion scenarios passed')


if __name__ == '__main__':
    main()
