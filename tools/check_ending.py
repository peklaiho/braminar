#!/usr/bin/env python3
"""Compare victory endings, certificate and rewards against original captures."""

import argparse
import json
import os
from pathlib import Path
import pty
import re
import shlex
import subprocess
import tempfile

from army_probe import roll


def normalize(text):
    return ''.join(text.replace('Slave', 'Servant').replace('Mecidine', 'Medicine').split())


def run(executable, config, gender='F', mode='ending', key=32):
    names = ('seed', 'hp', 'gold', 'food', 'male', 'female', 'medicine', 'poison',
             'staff', 'talisman', 'charm', 'swords', 'bows', 'arrows', 'daggers',
             'horse', 'cart', 'level', 'protection', 'ring', 'looks')
    arguments = [str(int(config[n])) if isinstance(config[n], bool) else str(config[n]) for n in names]
    master, slave = pty.openpty()
    try:
        result = subprocess.run([str(executable), *arguments, gender, mode, str(key)],
                                stdin=slave, capture_output=True, text=True, timeout=10, check=True)
    finally:
        os.close(slave)
        os.close(master)
    text, raw = result.stdout.split('\nRESULT ')
    check = json.loads(raw.splitlines()[0])
    assert check['unchanged'], result.stdout
    assert 'printer' not in text.lower(), text
    return text, check


def preserved(case):
    before = {k: v for k, v in case['before'].items() if k != 'seed'}
    after = {k: v for k, v in case['finished']['state'].items() if k != 'seed'}
    assert before == after, case['name']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    endings = json.loads(Path('doc/ending-endings-checkpoints.json').read_text())
    rewards = json.loads(Path('doc/ending-rewards-checkpoints.json').read_text())
    flags = ['-std=c99', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined']
    with tempfile.TemporaryDirectory(prefix='braminar-check-ending-') as directory:
        executable = Path(directory) / 'ending'
        subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), *flags,
                        str(Path(__file__).with_suffix('.c')), '-o', str(executable)], check=True)
        assert len(rewards) == 20
        for case in rewards:
            text, check = run(executable, case['before'], case['gender'], 'reward')
            assert text.strip() == case['text'], (case['name'], text, case['text'])
            assert check['seed'] == case['finished']['state']['seed'], (case['name'], check)
            assert check['keys'] == 0, check
            preserved(case)
            print(case['name'] + ': reward text, RNG and unchanged player match')
        assert len(endings) == 6
        for case in endings:
            preserved(case)
            text, check = run(executable, case['config'])
            if case['config']['ring']:
                assert normalize(case['finished']['screen']) == normalize(text.replace('\x1b[2J\x1b[H', '')), text
                assert check['seed'] == case['before']['seed'], check
                assert check['keys'] == 0 and 'Congratulations' not in text, text
            else:
                assert check['keys'] == 1, check
                assert 'Congratulations!!!' in text, text
                assert check['seed'] == roll(case['config']['seed'], 10)[0], check
                if case['answer'] == 'y':
                    # The complete certificate fits the captured screen; compare it
                    # independently of the deliberately omitted printer prompt.
                    original = case['certificate']['screen']
                    start = original.index('                            AssaultProbe')
                    assert normalize(original[start:]) in normalize(text), (case['name'], text, original)
                    assert check['seed'] == case['finished']['state']['seed'], check
                else:
                    assert case['finished']['state']['seed'] == case['before']['seed'], case
                status = case['status']['screen']
                assert normalize(status) in normalize(text), (case['name'], text, status)
                ugly = 'You now become the new King of Braminar.'
                assert (ugly in text) == (not case['config']['looks']), text
                if not case['config']['looks']:
                    original = case['prompt']['screen'].split('Do you have a printer')[0]
                    assert normalize(original) in normalize(text), (case['name'], text)
                eof_text, eof_check = run(executable, case['config'], key=-1)
                assert eof_check == check and eof_text == text, (case['name'], eof_check, check)
            print(case['name'] + ': ending priority, certificate/status, saved stats and input match output policy')
    print('20 rewards and 6 ending branches passed; final status EOF also terminates cleanly')


if __name__ == '__main__':
    main()
