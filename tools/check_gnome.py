#!/usr/bin/env python3
"""Compare occupied-gnome outcomes, allowing the intentional food-gift fix."""

import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from check_forest import compare_state, run


def normalize(text):
    return ' '.join(text.split())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    cases = json.loads((root / 'doc/gnome-checkpoints.json').read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-gnome-') as directory:
        executable = Path(directory) / 'check-gnome'
        flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
        if args.sanitize:
            flags.append('-fsanitize=address,undefined')
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(root / 'tools/check_forest.c'), '-o', str(executable)], check=True)
        for case in cases:
            result = run(executable, case['config'], 'y ')
            assert result.returncode == 0, (case['name'], result)
            matches = list(re.finditer(r'CHECK (\{[^\r\n]*\})', result.stdout))
            assert len(matches) == 2, result.stdout
            actual = [json.loads(match[1]) for match in matches]
            for check, kind in zip(actual, ('event', 'final')):
                assert check['kind'] == kind, result.stdout
                expected = dict(case[kind])
                expected['food'] += case['food_gift']
                compare_state(check, expected, case['name'] + ' ' + kind)
                assert check['inputs'] == 2, (case['name'], check)
            narrative = result.stdout[:matches[0].start()]
            assert normalize(case['event_screen']) == normalize(narrative), (case['name'], narrative)
            assert 'SURVIVED 1' in result.stdout and 'Game Over.' not in result.stdout, result.stdout
            print(case['name'] + ': healing, inventory, text and pauses match with intentional food gift')
    print(f'{len(cases)} occupied-gnome outcomes passed; exact RNG equality is not checked')


if __name__ == '__main__':
    main()
