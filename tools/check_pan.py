#!/usr/bin/env python3
"""Compare Pan with retained DOS checkpoints and the original question data."""

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
    return ' '.join(text.replace('Do Lake:=1 to 5', 'Do Lake: 1 to 5').split())


def probe(executable, seed, keys, command='PRIOR'):
    master, slave = pty.openpty()
    try:
        with tempfile.TemporaryFile() as output:
            process = subprocess.run([str(executable), str(seed), keys, command],
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
    root = Path(__file__).resolve().parent.parent
    cases = json.loads((root / 'doc/pan-checkpoints.json').read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-pan-') as directory:
        executable = Path(directory) / 'check-pan'
        flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
        if args.sanitize:
            flags.append('-fsanitize=address,undefined')
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(root / 'tools/check_pan.c'), '-o', str(executable)], check=True)
        for case in cases:
            config = case['config']
            keys = config['choice'] + config['answer']
            if config['choice'] == 'y':
                keys += ' '
            actual, text = probe(executable, config['seed'], keys, config['command'])
            for key, expected in case['final'].items():
                value = actual[key]
                assert (value == expected if isinstance(expected, str) else
                        abs(value - expected) <= (0.005 if key == 'hp' else 0)), (case['name'], key, actual)
            assert actual['inputs'] == len(keys), (case['name'], actual)
            assert normalize(case['screen']) in normalize(text), (case['name'], text)
            if config['choice'] == 'n':
                assert 'You move on.' in text and 'Press any key' not in text, text
            print(case['name'] + ': state, command, RNG, text and input count match')
        # Check every embedded question against the declared Pascal strings.
        data = (root / 'dos/BRAM.DAT').read_bytes()
        seen = set()
        for seed in range(1000000):
            value = (seed * 129 + 0x361962E9) & 0xFFFFFFFF
            if (value >> 17) % 16 != 14:
                continue
            value = (value * 129 + 0x361962E9) & 0xFFFFFFFF
            record = (value >> 17) % 20
            if record in seen:
                continue
            _, text = probe(executable, seed, 'y1 ')
            for offset in (0, 81, 102, 123):
                start = record * 144 + offset
                field = data[start + 1:start + 1 + data[start]].decode('ascii')
                field = field.replace('Do Lake:=1 to 5', 'Do Lake: 1 to 5')
                assert field in text, (record + 1, field, text)
            seen.add(record)
            if len(seen) == 20:
                break
        assert len(seen) == 20
        print('All 20 question records match original data with the approved text correction')


if __name__ == '__main__':
    main()
