#!/usr/bin/env python3
"""Compare player-visible forest outcomes with retained DOS captures."""

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
    return ' '.join(text.replace('The nymph it luring', 'The nymph is luring').split())


def run(executable, config, keys):
    master, slave = pty.openpty()
    try:
        arguments = [str(int(config[name])) if isinstance(config[name], bool) else str(config[name])
                     for name in ('seed', 'hp', 'ring', 'plague', 'looks', 'gender')]
        result = subprocess.run([str(executable), *arguments, keys], stdin=slave,
                                capture_output=True, text=True, timeout=10)
    finally:
        os.close(slave)
        os.close(master)
    return result


def compare_state(actual, expected, name):
    for key, value in expected.items():
        assert abs(actual[key] - value) <= (0.005 if key == 'hp' else 0), (name, key, actual[key], value)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    cases = json.loads((root / 'doc/forest-checkpoints.json').read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-forest-') as directory:
        executable = Path(directory) / 'check-forest'
        flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
        if args.sanitize:
            flags.append('-fsanitize=address,undefined')
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(root / 'check_forest.c'), '-o', str(executable)], check=True)
        for case in cases:
            config = case['config']
            keys = 'y ' if config['enter'] else 'n'
            result = run(executable, config, keys)
            assert result.returncode == 0, (case['name'], result)
            matches = list(re.finditer(r'CHECK (\{[^\r\n]*\})', result.stdout))
            assert len(matches) == 2, result.stdout
            event, final = [json.loads(match[1]) for match in matches]
            assert event['kind'] == 'event' and final['kind'] == 'final', result.stdout
            compare_state(event, case.get('event', case['initial']), case['name'])
            if not case['fatal']:
                compare_state(final, case['final'], case['name'])
            narrative = normalize(result.stdout[:matches[0].start()])
            expected = case.get('event_screen', case['entry_screen'])
            if not config['enter']:
                # DOS clears this departure line before the next captured status.
                expected += '\nYou move on.'
            assert normalize(expected) == narrative, (case['name'], expected, narrative)
            assert event['inputs'] == len(keys), (case['name'], event)
            assert f'SURVIVED {int(not case["fatal"])}' in result.stdout, result.stdout
            assert ('Game Over.' in result.stdout) == case['fatal'], result.stdout
            print(case['name'] + ': health, supplies, appearance, plague, ring, text and pauses match')
    print(f'{len(cases)} forest outcomes passed; exact RNG equality is not checked')


if __name__ == '__main__':
    main()
