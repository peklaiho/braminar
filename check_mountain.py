#!/usr/bin/env python3
"""Compare the mountain trail with retained DOS checkpoints."""

import argparse
import json
import os
from pathlib import Path
import pty
import re
import shlex
import subprocess
import tempfile


def normalize(text):
    return ' '.join(text.replace('He lookes up', 'He looks up').split())


def probe(executable, seed, keys, ring, staff):
    master, slave = pty.openpty()
    try:
        with tempfile.TemporaryFile() as output:
            process = subprocess.run([str(executable), str(seed), keys, str(ring), str(int(staff))],
                                     stdin=slave, stdout=output, stderr=output, timeout=10)
            output.seek(0)
            text = output.read().decode()
            assert process.returncode == 0, text
    finally:
        os.close(slave)
        os.close(master)
    match = re.search(r'CHECK (\{[^\r\n]*\})', text)
    assert match, text
    return json.loads(match[1]), text[:match.start()]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    cases = json.loads((root / 'doc/mountain-checkpoints.json').read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-mountain-') as directory:
        executable = Path(directory) / 'check-mountain'
        flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
        if args.sanitize:
            flags.append('-fsanitize=address,undefined')
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(root / 'check_mountain.c'), '-o', str(executable)], check=True)
        for case in cases:
            config = case['config']
            keys = config['keys'] + ' '
            actual, text = probe(executable, config['seed'], keys, config['ring'], config['staff'])
            for key, expected in case['final'].items():
                value = actual[key]
                assert (value == expected if isinstance(expected, str) else
                        abs(value - expected) <= (0.005 if key == 'hp' else 0)), (case['name'], key, actual)
            assert actual['inputs'] == len(keys), (case['name'], actual)
            assert normalize(case['screen']) in normalize(text), (case['name'], text)
            print(case['name'] + ': state, RNG, text and input count match')
        for keys in ('', 'y'):
            # EOF at either question must stop the event before bookkeeping.
            master, slave = pty.openpty()
            try:
                result = subprocess.run([str(executable), str(cases[0]['config']['seed']), keys, '5', '1'],
                                        stdin=slave, capture_output=True, timeout=10)
                assert result.returncode == 3, result
            finally:
                os.close(slave)
                os.close(master)
        print('EOF at either question stops the event')


if __name__ == '__main__':
    main()
