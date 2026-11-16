#!/usr/bin/env python3
"""Compare slot-6 empty-home branches with saved original-game checkpoints."""

import argparse
import json
import os
from pathlib import Path
import pty
import re
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    cases = json.loads((root / 'doc/tree-checkpoints.json').read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-tree-') as directory:
        executable = Path(directory) / 'check-tree'
        flags = ['-std=c99', '-O0', '-g']
        if args.sanitize:
            flags.append('-fsanitize=address,undefined')
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(root / 'tools/check_tree.c'), '-o', str(executable)], check=True)
        for case in cases:
            config = case['config']
            master, slave = pty.openpty()
            try:
                # Output is small; a temporary file avoids blocking on the PTY buffer.
                with tempfile.TemporaryFile() as output:
                    subprocess.run([str(executable), str(config['seed']), config['choice'],
                                    str(int(config['staff']))], stdin=slave, stdout=output,
                                   stderr=output, check=True, timeout=10)
                    output.seek(0)
                    text = output.read().decode()
            finally:
                os.close(slave)
                os.close(master)
            match = re.search(r'CHECK (\{[^\r\n]*\})', text)
            if match is None:
                raise AssertionError(text)
            actual = json.loads(match[1])
            expected = {key: case['final'][key] for key in actual if key != 'inputs'}
            expected['inputs'] = 2 if config['choice'] == 'n' else 3
            for key, value in expected.items():
                if abs(actual[key] - value) > (0.005 if key == 'hp' else 0):
                    raise AssertionError(f'{case["name"]}: {key}: {actual[key]} != {value}')
            print(case['name'] + ': state, RNG and input count match')
    print(f'{len(cases)} empty-home scenarios passed')


if __name__ == '__main__':
    main()
