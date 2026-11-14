#!/usr/bin/env python3
"""Capture controlled original-game storm and servant-illness branches."""

import argparse
import json
from pathlib import Path
import re
import struct

from combat_probe import real48, state
from dosbox_game import Game


def rolls(seed, limits):
    values = []
    for limit in limits:
        seed = (seed * 129 + 0x361962E9) & 0xFFFFFFFF
        values.append((seed >> 17) % limit + 1)
    return values


def find_seed(slot, outcome=None, shelter=False):
    for seed in range(1000000):
        values = rolls(seed, (16, 8, 5))
        if values[0] == slot and (outcome is None or values[1] == outcome):
            if not shelter or values[2] == 1:
                return seed
    raise RuntimeError('No seed found')


def cases():
    result = []

    def add(name, seed, inputs, **overrides):
        config = dict(seed=seed, male=12, female=12, medicine=3, food=100,
                      protection=False, inputs=inputs)
        config.update(overrides)
        result.append(dict(name=name, config=config))

    for number, name in enumerate(('rain', 'hail', 'snow', 'wind', 'tornado',
                                   'hurricane', 'monsoon', 'tidal'), 1):
        add('storm-' + name, find_seed(10, number), ['n'])
    for number in (1, 8):
        add('storm-shelter-' + str(number), find_seed(10, number, True), ['y'])
    storm = find_seed(10, 5)
    add('storm-small-party', storm, ['n'], male=1, female=2)
    add('storm-no-males', storm, ['n'], male=0, female=12)
    add('storm-empty-party', storm, ['n'], male=0, female=0)
    add('storm-starvation', storm, ['n'], food=0)
    illness = find_seed(9)
    add('illness-partial', illness, ['2', '1'])
    add('illness-no-medicine', illness, ['10', '10'], medicine=0)
    add('illness-full-cure', illness, ['1', '2'], male=1, female=2, medicine=3)
    add('illness-excess', illness, ['10', '10'], male=1, female=2, medicine=20)
    add('illness-no-males', illness, ['0', '3'], male=0)
    add('illness-no-females', illness, ['3', '0'], female=0)
    add('illness-empty-party', illness, [], male=0, female=0)
    add('illness-negative', illness, ['-1', '2', '-2', '1'])
    add('illness-protected', illness, ['g'], protection=True)
    add('illness-hex', illness, ['$0002', '1'])
    add('illness-blank', illness, ['', '2', '1'])
    add('illness-whitespace', illness, [' 2 extra', '1'])
    add('illness-invalid', illness, ['abc'])
    add('illness-overflow', illness, ['32768'])
    return result


def player_state(game):
    result = state(game)
    result['medicine'] = int.from_bytes(game.read_memory('DS', 0x2AC, 2), 'little')
    result['plague'] = game.read_memory('DS', 0x296, 1)[0]
    result['protection'] = bool(game.read_memory('DS', 0x299, 1)[0])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    args = parser.parse_args()
    selected = [case for case in cases() if not args.cases or case['name'] in args.cases]
    if not selected or set(args.cases) - {case['name'] for case in selected}:
        parser.error('Unknown case')
    output = Path('doc/storm-illness-checkpoints.json')
    fixtures = json.loads(output.read_text()) if args.cases and output.exists() else []

    def save(case):
        if case['name'] == 'illness-blank':
            case['compare_as'] = 'illness-partial'
        pattern = r'While you are walking along|\d+ male and \d+ female slaves have come down with:|There seems to be a bug|Things seem strangely quite'
        start = list(re.finditer(pattern, case['screen']))[-1].start()
        case['screen'] = case['screen'][start:]
        fixtures[:] = [old for old in fixtures if old['name'] != case['name']]
        fixtures.append(case)
        output.write_text(json.dumps(fixtures, indent=2) + '\n')
    print('CONTROLLED PROBES: temporary player memory and RNG are modified.', flush=True)
    with Game() as game:
        print(game.screen(), flush=True)

        def command(text):
            game.command(text)
            screen = game.screen()
            print(f'\n--- {text} ---\n{screen}', flush=True)
            return screen

        for text in ('key space', 'text EventProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for case in selected:
            config = case['config']
            print('\nCASE ' + case['name'] + ' ' + json.dumps(config), flush=True)
            data = bytearray(game.read_memory('DS', 0x277, 0x49))
            struct.pack_into('<h', data, 0, 8)
            data[2:8] = real48(50)
            data[0x295 - 0x277:0x29A - 0x277] = bytes((0, 0, 0, 0, config['protection']))
            for address, value in ((0x29C, config['male']), (0x29E, config['female']),
                                   (0x2AC, config['medicine']), (0x2AE, 0),
                                   (0x2B0, 0), (0x2B2, 0), (0x2B4, 0), (0x2B6, 0)):
                struct.pack_into('<h', data, address - 0x277, value)
            for address, value in ((0x2A0, 100), (0x2A6, config['food']), (0x2BA, 0)):
                start = address - 0x277
                data[start:start + 6] = real48(value)
            data[0x2B8 - 0x277:0x2BA - 0x277] = bytes(2)
            game.write_memory('DS', 0x277, data)
            game.write_memory('DS', 0x1FC, config['seed'].to_bytes(4, 'little'))
            print('STATE initial ' + json.dumps(player_state(game), sort_keys=True), flush=True)
            screens = [command('key space')]
            for answer in config['inputs']:
                if case['name'].startswith('storm-') or config['protection']:
                    screens.append(command('key ' + answer))
                else:
                    if answer:
                        screens.append(command('text ' + answer))
                    screens.append(command('key Return'))
            # Poll delayed paths without sending keys into later prompts.
            for step in range(15):
                screen = screens[-1]
                if screen.startswith('*** EventProbe ***') and screen.endswith('Any key.'):
                    break
                if 'Program aborted' in screen:
                    break
                if screen.endswith('Press any key...'):
                    screens.append(command('key space'))
                    if not screens[-1].endswith('Any key.'):
                        screens.append(command('wait 1'))
                else:
                    screens.append(command('wait 1'))
            else:
                raise RuntimeError('No next status or runtime error: ' + case['name'])
            if not screens[-1].endswith('Any key.'):
                case['aborted'] = True
                case['screen'] = screens[-1]
                save(case)
                if case is selected[-1]:
                    break
                # Restart after an invalid numeric read returns control to DOS.
                command('text BRAMINAR.COM')
                command('key Return')
                for step in range(10):
                    if command('wait 1').lstrip().startswith('Welcome to:'):
                        break
                else:
                    raise RuntimeError('No welcome screen after restart')
                for text in ('key space', 'text EventProbe', 'key Return', 'key f', 'key space'):
                    command(text)
                continue
            case['final'] = player_state(game)
            print('STATE final ' + json.dumps(case['final'], sort_keys=True), flush=True)
            # Retain the last event screen before the next status for text comparisons.
            case['screen'] = screens[-2]
            save(case)


if __name__ == '__main__':
    main()
