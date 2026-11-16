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
    arguments = [str(config['seed']), str(config['male']), str(config['female']),
                 str(config['medicine']), str(config['food']),
                 str(int(config['protection'])), keys, lines]
    if 'integer' in config:
        arguments = ['--integer', config['integer']]
    master, slave = pty.openpty()
    try:
        with tempfile.TemporaryFile() as output:
            process = subprocess.run([str(executable), *arguments], stdin=slave,
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
    root = Path(__file__).resolve().parent.parent
    cases = json.loads((root / 'doc/storm-illness-checkpoints.json').read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-events-') as directory:
        executable = Path(directory) / 'check-events'
        flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
        if args.sanitize:
            flags.append('-fsanitize=address,undefined')
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(root / 'tools/check_events.c'), '-o', str(executable)], check=True)
        integer_cases = [
            ('0\n', 0), ('42\n', 42), ('00012\n', 12), (' 7 \n', 7),
            ('-1\n', 0), ('abc\n', 0), ('1.5\n', 0), ('12x\n', 0),
            ('12 extra\n', 0), ('$10\n', 0), ('0x10\n', 0), ('+2\n', 0),
            ('2147483647\n', 2147483647), ('2147483648\n', 0),
            ('1' + ' ' * 159 + 'x\n', 0), ('\n \n7\n', 7),
        ]
        config = dict(cases[0]['config'])
        for line, expected in integer_cases:
            config['integer'] = line
            actual, text = run(executable, config)
            assert actual == dict(read=1, value=expected), (line, actual)
            assert 'error' not in text.lower() and 'abort' not in text.lower(), text
        for line in ('', '\n'):
            config['integer'] = line
            actual, text = run(executable, config)
            assert actual['read'] == 0, (line, actual)
        print(f'{len(integer_cases) + 2} generic integer input checks passed')
        input_changes = {
            'illness-negative': (['-1', '2'], dict(male=9, female=10, medicine=1, food=90)),
            'illness-hex': (['$0002', '1'], dict(male=9, female=9, medicine=2, food=91)),
            'illness-whitespace': ([' 2 extra', '1'], dict(male=9, female=9, medicine=2, food=91)),
            'illness-invalid': (['abc', '0'], dict(male=9, female=8, medicine=3, food=91)),
            'illness-overflow': (['32768', '0'], dict(male=12, female=8, medicine=0, food=90)),
        }
        for case in cases:
            config = dict(case['config'])
            changed = case['name'] in input_changes
            if changed:
                config['inputs'], state_changes = input_changes[case['name']]
            actual, text = run(executable, config)
            reference = next((c for c in cases if c['name'] == case.get('compare_as')), case)
            expected_state = reference.get('final', {})
            if changed:
                expected_state = dict(next(c for c in cases if c['name'] == 'illness-partial')['final'])
                expected_state.update(state_changes)
            for key, value in expected_state.items():
                if abs(actual[key] - value) > (0.005 if key == 'hp' else 0):
                    raise AssertionError(f'{case["name"]}: {key}: {actual[key]} != {value}')
            aborted = case.get('aborted', False) and not changed
            assert actual['aborted'] == aborted, (case['name'], actual)
            expected = original_event(reference)
            if changed:
                expected = expected.split('How many males to cure?')[0]
                expected += ('How many males to cure? ' + config['inputs'][0] +
                             ' How many females to cure? ' + config['inputs'][1] +
                             ' Any not cured pass away. Press any key...')
                expected = normalize(expected)
            if expected not in normalize(text):
                raise AssertionError(f'{case["name"]}: original text/flow missing:\n{expected}\nACTUAL:\n{text}')
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
            label = 'documented integer-input rules' if changed else (
                'documented blank-input retry' if 'compare_as' in case else 'original')
            print(case['name'] + ': state, RNG, text, input counts and requested delays match ' + label)
    print(f'{len(cases)} storm/illness scenarios passed')


if __name__ == '__main__':
    main()
