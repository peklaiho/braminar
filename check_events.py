#!/usr/bin/env python3
"""Compare storms and illness with controlled original-game captures."""

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
    text = text.replace('slaves', 'servants').replace('Slaves', 'Servants')
    text = text.replace('Things seem strangely quite when out of no where jumps',
                        'Things seem strangely quiet when out of nowhere jumps')
    text = text.replace('win their horde.', 'win their hoard.')
    text = text.replace('You may of lost', 'You may have lost')
    text = text.replace('All servants & food is lost.', 'All servants & food are lost.')
    return re.sub(r'(Press any key\.\.\.)\s*', r'\1', ' '.join(text.split()))


def original_event(case):
    screen = case['screen']
    if case['name'].startswith('storm-'):
        start = screen.rindex('While you are walking along')
    elif case['config']['protection']:
        start = screen.rindex('Things seem strangely quite')
    elif case['config']['male'] + case['config']['female'] == 0:
        start = screen.rindex('There seems to be a bug')
    else:
        match = list(re.finditer(r'\d+ male and \d+ female slaves have come down with:', screen))[-1]
        start = match.start()
    text = screen[start:]
    # The DOS program counter and command prompt have no C/POSIX counterpart.
    text = re.sub(r', PC=[0-9A-F]+', '', text)
    text = text.split('C:\\>')[0]
    return normalize(text)


def run(executable, config):
    if config['inputs'] and (config['protection'] or config['inputs'][0] in ('y', 'n')):
        keys = ''.join(config['inputs']) + (' ' if not config['protection'] else '')
        lines = ''
    else:
        keys = ' ' if config['male'] + config['female'] else ''
        lines = ''.join(line + '\n' for line in config['inputs'])
    master, slave = pty.openpty()
    try:
        with tempfile.TemporaryFile() as output:
            process = subprocess.run([str(executable), str(config['seed']), str(config['male']),
                                      str(config['female']), str(config['medicine']), str(config['food']),
                                      str(int(config['protection'])), keys, lines], stdin=slave,
                                     stdout=output, stderr=output, timeout=10)
            output.seek(0)
            text = output.read().decode()
            if process.returncode:
                raise AssertionError(text)
    finally:
        os.close(slave)
        os.close(master)
    match = re.search(r'CHECK (\{[^\r\n]*\})', text)
    if not match:
        raise AssertionError(text)
    return json.loads(match[1]), text[:match.start()]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    cases = json.loads((root / 'doc/storm-illness-checkpoints.json').read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-events-') as directory:
        executable = Path(directory) / 'check-events'
        flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
        if args.sanitize:
            flags.append('-fsanitize=address,undefined')
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(root / 'check_events.c'), '-o', str(executable)], check=True)
        for case in cases:
            actual, text = run(executable, case['config'])
            reference = next((c for c in cases if c['name'] == case.get('compare_as')), case)
            for key, value in reference.get('final', {}).items():
                if abs(actual[key] - value) > (0.005 if key == 'hp' else 0):
                    raise AssertionError(f'{case["name"]}: {key}: {actual[key]} != {value}')
            aborted = case.get('aborted', False)
            assert actual['aborted'] == aborted, (case['name'], actual)
            expected = original_event(reference)
            if expected not in normalize(text):
                raise AssertionError(f'{case["name"]}: original text/flow missing:\n{expected}\nACTUAL:\n{text}')
            config = case['config']
            storm = case['name'].startswith('storm-')
            expected_keys = 2 if storm else (1 if config['male'] + config['female'] else 0)
            if aborted:
                expected_keys = 0
            expected_lines = 0 if storm or config['protection'] else len(config['inputs'])
            assert actual['keys'] == expected_keys, (case['name'], actual)
            assert actual['lines'] == expected_lines, (case['name'], actual)
            if storm:
                # Dots were printed once per second in the original wait loop.
                dots = re.search(r'(\.+)The storm is gone', case['screen'])
                expected_delay = len(dots[1]) - 1 if dots else 0
            else:
                expected_delay = 0 if aborted else 1
            assert actual['delay'] == expected_delay, (case['name'], actual, expected_delay)
            label = 'documented blank-input retry' if 'compare_as' in case else 'original'
            print(case['name'] + ': state, RNG, text, input counts and requested delays match ' + label)
    print(f'{len(cases)} storm/illness scenarios passed')


if __name__ == '__main__':
    main()
