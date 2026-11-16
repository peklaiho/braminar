#!/usr/bin/env python3
"""Compare G5 ring persistence using the existing two-turn C harness."""

import argparse
import json
from pathlib import Path
import re
import tempfile

from check_thicket import build, run
from check_forest import compare_state


def normalize(text):
    text = text.replace('slaves', 'servants').replace('Slaves', 'Servants')
    text = text.replace('Things seem strangely quite when out of no where jumps',
                        'Things seem strangely quiet when out of nowhere jumps')
    text = text.replace('win their horde.', 'win their hoard.')
    text = text.replace('opponents finger', "opponent's finger")
    text = text.replace('He lookes up', 'He looks up')
    return ' '.join(text.split())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    cases = json.loads((root / 'doc/ring-checkpoints.json').read_text())
    with tempfile.TemporaryDirectory(prefix='braminar-check-ring-') as directory:
        executable = Path(directory) / 'check-ring'
        build(executable, args.sanitize)
        for case in cases:
            keys = ''.join(step['answer'] for step in case['steps'] if step['kind'] == 'key')
            code, text = run(executable, case['config'], keys, turns=2)
            assert code == 0, (case['name'], text)
            matches = list(re.finditer(r'CHECK (\{[^\r\n]*\})', text))
            expected = case['steps'] + [dict(kind='final', state=case['final'])]
            assert len(matches) == len(expected), (case['name'], text)
            for i, (match, reference) in enumerate(zip(matches, expected)):
                actual = json.loads(match[1])
                assert actual['kind'] == reference['kind'], (case['name'], i, actual)
                compare_state(actual, reference['state'], case['name'] + ' ' + str(i))
                if reference['kind'] == 'key':
                    prefix = text[matches[i - 1].end() if i else 0:match.start()]
                    assert normalize(reference['prompt']) in normalize(prefix), (case['name'], i, prefix)
                    narrative = re.sub(r'CHECK \{[^\r\n]*\}', '', text[:match.start()])
                    assert normalize(reference['screen']) in normalize(narrative), (case['name'], i, narrative)
            assert actual['keys'] == len(keys) and actual['delay'] == 0, (case['name'], actual)
            assert 'Game Over.' not in text, text
            print(case['name'] + ': ring persistence, state, narrative, prompt order and pauses match')
    print(f'{len(cases)} two-event ring chains passed; exact RNG equality is not checked')


if __name__ == '__main__':
    main()
