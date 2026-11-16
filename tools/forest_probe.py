#!/usr/bin/env python3
"""Capture the six missing forest outcomes in the original DOS game."""

import argparse
import json
from pathlib import Path
import struct

from combat_probe import real48, state
from dosbox_game import Game


def rolls(seed):
    values = []
    for bound in (16, 100, 100):
        seed = (seed * 129 + 0x361962E9) & 0xFFFFFFFF
        values.append((seed >> 17) % bound + 1)
    return values


def cases():
    candidates = [n for n in range(200000) if rolls(n)[0] == 12]
    quiet = next(n for n in candidates if rolls(n)[1] >= 13)
    faerie = next(n for n in candidates if rolls(n)[1] >= 13 and rolls(n)[2] < 21)
    fatal = next(n for n in candidates if rolls(n)[1] < 13 and rolls(n)[2] >= 66)
    result = []
    for name, seed, hp, ring, plague, looks, enter in (
            ('decline', quiet, 50, 0, 0, True, False),
            ('ring-exclusion', quiet, 50, 20, 0, False, True),
            ('faerie-cure-and-looks', faerie, 50, 0, 50, False, True),
            ('faerie-healing', faerie, 50, 0, 0, True, True),
            ('faerie-full-health', faerie, 80, 0, 0, True, True),
            ('fatal-lure', fatal, 50, 0, 0, True, True)):
        result.append(dict(name=name, config=dict(seed=seed, hp=hp, ring=ring,
                           plague=plague, looks=looks, enter=enter, gender='F')))
    return result


def player_state(game):
    result = state(game)
    # The seed selects a reproducible outcome; RNG equality is not a check target.
    result.pop('seed')
    data = game.read_memory('DS', 0x294, 0x26)
    for name, address in (('looks', 0x294), ('plague', 0x296), ('staff', 0x297),
                          ('talisman', 0x298), ('horse', 0x2B8), ('cart', 0x2B9)):
        result[name] = data[address - 0x294]
    result['medicine'] = struct.unpack_from('<h', data, 0x2AC - 0x294)[0]
    return result


def reset_player(game, config):
    data = bytearray(game.read_memory('DS', 0x277, 0x49))
    struct.pack_into('<h', data, 0, 8)
    data[2:8] = real48(config['hp'])
    data[8:14] = b'\x05Black'
    for address, value in ((0x294, config['looks']), (0x295, config['ring']),
                           (0x296, config['plague']), (0x297, 0), (0x298, 0),
                           (0x299, 0), (0x2B8, 0), (0x2B9, 0)):
        data[address - 0x277] = value
    for address, value in ((0x29A, 0), (0x29C, 5), (0x29E, 5), (0x2AC, 3),
                           (0x2AE, 0), (0x2B0, 0), (0x2B2, 0), (0x2B4, 2), (0x2B6, 0)):
        struct.pack_into('<h', data, address - 0x277, value)
    for address, value in ((0x2A0, 8), (0x2A6, 100), (0x2BA, 0)):
        data[address - 0x277:address - 0x277 + 6] = real48(value)
    game.write_memory('DS', 0x277, data)
    game.write_memory('DS', 0x1FC, config['seed'].to_bytes(4, 'little'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/forest-checkpoints.json'))
    args = parser.parse_args()
    available = cases()
    if set(args.cases) - {case['name'] for case in available}:
        parser.error('Unknown case')
    selected = [case for case in available if not args.cases or case['name'] in args.cases]
    fixtures = json.loads(args.output.read_text()) if args.cases and args.output.exists() else []
    print('CONTROLLED PROBES: temporary player memory and RNG are modified; original files are untouched.', flush=True)
    with Game() as game:
        print(game.screen(), flush=True)

        def command(text):
            game.command(text)
            screen = game.screen()
            print(f'\n--- {text} ---\n{screen}', flush=True)
            return screen

        for text in ('key space', 'text ForestProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for case in selected:
            config = case['config']
            print('\nCASE ' + case['name'] + ' ' + json.dumps(config, sort_keys=True), flush=True)
            reset_player(game, config)
            case['initial'] = player_state(game)
            print('STATE initial ' + json.dumps(case['initial'], sort_keys=True), flush=True)
            screen = command('key space')
            marker = 'You come upon an enchanted forest.'
            if marker not in screen or not screen.endswith('(Y/N)'):
                raise RuntimeError('Expected forest entry question')
            case['entry_screen'] = screen[screen.index(marker):]
            screen = command('key ' + ('y' if config['enter'] else 'n'))
            if config['enter']:
                if not screen.endswith('Press any key...'):
                    raise RuntimeError('Expected forest exit pause')
                case['event_screen'] = screen[screen.index(marker):]
                case['event'] = player_state(game)
                print('STATE event ' + json.dumps(case['event'], sort_keys=True), flush=True)
                screen = command('key space')
            case['fatal'] = 'Game Over.' in screen
            if case['fatal']:
                case['exit_screen'] = screen
            else:
                if not screen.startswith('*** ForestProbe ***') or not screen.endswith('Any key.'):
                    raise RuntimeError('Expected next status screen')
                case['final'] = player_state(game)
                print('STATE final ' + json.dumps(case['final'], sort_keys=True), flush=True)
            fixtures[:] = [old for old in fixtures if old['name'] != case['name']]
            fixtures.append(case)
            args.output.write_text(json.dumps(fixtures, indent=2) + '\n')
            if case['fatal'] and case is not selected[-1]:
                raise RuntimeError('Fatal case must be last')


if __name__ == '__main__':
    main()
