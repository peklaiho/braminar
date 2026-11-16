#!/usr/bin/env python3
"""Capture G2's three occupied-gnome healing cases in the original game."""

import argparse
import json
from pathlib import Path
import re

from dosbox_game import Game
from forest_probe import player_state, reset_player


def rolls(seed):
    result = []
    for bound in (16, 100, 6, 5):
        seed = (seed * 129 + 0x361962E9) & 0xFFFFFFFF
        result.append((seed >> 17) % bound + 1)
    return result


def cases():
    seed = next(n for n in range(200000)
                if rolls(n)[0] == 6 and rolls(n)[1] < 56 and rolls(n)[3] >= 2)
    return [dict(name=name, config=dict(seed=seed, hp=hp, ring=0, plague=0,
                                       looks=True, gender='F'))
            for name, hp in (('injured', 50), ('healing-past-cap', 79), ('full-health', 80))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/gnome-checkpoints.json'))
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

        for text in ('key space', 'text GnomeProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for case in selected:
            print('\nCASE ' + case['name'] + ' ' + json.dumps(case['config'], sort_keys=True), flush=True)
            reset_player(game, case['config'])
            case['initial'] = player_state(game)
            print('STATE initial ' + json.dumps(case['initial'], sort_keys=True), flush=True)
            screen = command('key space')
            marker = 'You come up to a hollow tree with a door.'
            if marker not in screen or not screen.endswith('(Y/N)'):
                raise RuntimeError('Expected hollow-tree entry question')
            case['entry_screen'] = screen[screen.index(marker):]
            screen = command('key y')
            if 'Inside is a little Gnome smoking a pipe.' not in screen or not screen.endswith('Press any key...'):
                raise RuntimeError('Expected occupied gnome home and exit pause')
            case['event_screen'] = screen[screen.index(marker):]
            gift = re.search(r'The gnome greets you and gives you (\d+) food\.', screen)
            if gift is None:
                raise RuntimeError('Expected announced food gift')
            case['food_gift'] = int(gift[1])
            case['event'] = player_state(game)
            print('STATE event ' + json.dumps(case['event'], sort_keys=True), flush=True)
            screen = command('key space')
            if not screen.startswith('*** GnomeProbe ***') or not screen.endswith('Any key.'):
                raise RuntimeError('Expected next status screen')
            case['final'] = player_state(game)
            print('STATE final ' + json.dumps(case['final'], sort_keys=True), flush=True)
            fixtures[:] = [old for old in fixtures if old['name'] != case['name']]
            fixtures.append(case)
            args.output.write_text(json.dumps(fixtures, indent=2) + '\n')


if __name__ == '__main__':
    main()
