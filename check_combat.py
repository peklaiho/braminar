#!/usr/bin/env python3
"""Compare the remake with saved, controlled DOS combat checkpoints."""

import argparse
import errno
import json
import os
from pathlib import Path
import pty
import re
import select
import shlex
import subprocess
import tempfile
import time


def run_check(executable, config):
    names = ('seed', 'hp', 'level', 'male', 'female', 'gold', 'food', 'swords',
             'bows', 'arrows', 'daggers', 'poison', 'ring', 'horse', 'cart',
             'choice', 'use_poison')
    arguments = [str(int(config[name])) if isinstance(config[name], bool)
                 else str(config[name]) for name in names]
    master, slave = pty.openpty()
    try:
        process = subprocess.Popen([str(executable), *arguments], stdin=slave,
                                   stdout=slave, stderr=slave)
    finally:
        os.close(slave)
    output = bytearray()
    try:
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            if select.select([master], [], [], 0.1)[0]:
                try:
                    data = os.read(master, 65536)
                except OSError as error:
                    if error.errno != errno.EIO:
                        raise
                    break
                if not data:
                    break
                output.extend(data)
            elif process.poll() is not None:
                break
        process.wait(timeout=2)
        if process.returncode != 0:
            raise RuntimeError(output.decode(errors='replace'))
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        os.close(master)
    return [json.loads(match) for match in re.findall(rb'CHECK (\{[^\r\n]*\})', output)]


def matches(actual, expected):
    return all(abs(actual[key] - value) < 0.005 if key == 'hp'
               else actual[key] == value for key, value in expected.items())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*', help='case names from doc/combat-checkpoints.json')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    cases = json.loads((root / 'doc/combat-checkpoints.json').read_text())
    unknown = set(args.cases) - {case['name'] for case in cases}
    if unknown:
        parser.error('Unknown cases: ' + ', '.join(sorted(unknown)))
    checked = 0
    with tempfile.TemporaryDirectory(prefix='braminar-check-combat-') as directory:
        executable = Path(directory) / 'check-combat'
        compiler = shlex.split(os.environ.get('CC', 'cc'))
        flags = ['-std=c99', '-O0', '-g']
        if args.sanitize:
            flags.append('-fsanitize=address,undefined')
        subprocess.run([*compiler, *flags, str(root / 'check_combat.c'),
                        '-o', str(executable)], check=True)
        for case in cases:
            if args.cases and case['name'] not in args.cases:
                continue
            name = case['name']
            expected = case['checkpoints']
            died = case['died']
            actual = run_check(executable, case['config'])
            if died:
                # DOS has regained DS at exit, so there is no original exit-state dump.
                if actual[-1]['hp'] > 0:
                    raise AssertionError(f'{name}: remake unexpectedly survived')
                actual = actual[:-1]
            cursor = 0
            for step, state in enumerate(actual):
                # Captures may also contain checkpoints during a delay, without input.
                while cursor < len(expected) and not matches(state, expected[cursor]):
                    cursor += 1
                if cursor == len(expected):
                    raise AssertionError(f'{name}, input checkpoint {step}: {state}')
                cursor += 1
            if not died and not matches(actual[-1], expected[-1]):
                raise AssertionError(f'{name}: final state differs')
            print(f'{name}: {len(actual)} remake input/exit checkpoints match')
            checked += 1
    if checked == 0:
        raise RuntimeError('No controlled combat captures found')
    print(f'{checked} combat scenarios passed')


if __name__ == '__main__':
    main()
