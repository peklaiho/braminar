#!/usr/bin/env python3
"""Capture G5's acquisition/removal chains without edits between encounters."""

import argparse
import json
from pathlib import Path
import struct

from dosbox_game import Game
from forest_probe import player_state, reset_player
from town_probe import find_seed


def cases():
    acquire = find_seed((16, 9, 10, 10, 10, 7, 100, 20, 20, 50, 16, 100),
                        lambda r: r[0] == 4 and r[6] < 8 and r[10] == 12 and r[11] >= 13)
    remove = find_seed((16, 16, 100, 100),
                       lambda r: r[0] == 8 and r[1] == 12 and r[2] >= 13 and r[3] >= 21)
    result = []
    for name, seed, ring, poison in (('acquire-then-forest', acquire, 0, 1),
                                    ('guru-then-forest', remove, 20, 0)):
        config = dict(seed=seed, hp=50, level=8, gold=8, food=100, male=5, female=5,
                      medicine=3, poison=poison, ring=ring, plague=0, looks=False,
                      horse=False, cart=False, staff=False, talisman=False, charm=0,
                      protection=False, swords=0, bows=0, arrows=2, daggers=0)
        result.append(dict(name=name, config=config))
    return result


def event_text(screen):
    for marker in ('Things seem strangely quite', 'You come upon a mountain trail.',
                   'You come upon an enchanted forest.'):
        if marker in screen:
            return screen[screen.index(marker):]
    return screen


def response(screen):
    lines = [line.strip() for line in screen.splitlines() if line.strip()]
    if lines[-1] == '(Y/N)':
        return 'y', lines[-2] + '\n(Y/N)'
    if lines[-1] == 'Do you wish to (F)ight (G)ive, or (R)un?':
        return 'f', lines[-1]
    if lines[-1] == 'Press any key...':
        return ' ', lines[-1]
    raise RuntimeError('Unexpected prompt ' + repr(lines[-1]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/ring-checkpoints.json'))
    args = parser.parse_args()
    available = cases()
    if set(args.cases) - {case['name'] for case in available}:
        parser.error('Unknown case')
    selected = [case for case in available if not args.cases or case['name'] in args.cases]
    fixtures = json.loads(args.output.read_text()) if args.cases and args.output.exists() else []
    print('CONTROLLED PROBES: initial temporary player state and RNG are edited once per chain.', flush=True)
    print('No state or RNG edits occur between encounters; original files are untouched.', flush=True)
    for case in selected:
        print('\nCASE ' + case['name'] + ' ' + json.dumps(case['config'], sort_keys=True), flush=True)
        with Game() as game:
            print(game.screen(), flush=True)

            def command(text):
                game.command(text)
                screen = game.screen()
                print(f'\n--- {text} ---\n{screen}', flush=True)
                return screen

            for text in ('key space', 'text RingProbe', 'key Return', 'key f', 'key space'):
                command(text)
            reset_player(game, case['config'])
            game.write_memory('DS', 0x2AE, struct.pack('<h', case['config']['poison']))
            case['initial'] = player_state(game)
            print('STATE initial ' + json.dumps(case['initial'], sort_keys=True), flush=True)
            screen = command('key space')
            case['steps'] = []
            completed = 0
            for step in range(20):
                if screen.startswith('*** RingProbe ***') and screen.endswith('Any key.'):
                    completed += 1
                    if completed == 2:
                        case['final'] = player_state(game)
                        break
                    checkpoint = dict(kind='turn', state=player_state(game))
                    case['steps'].append(checkpoint)
                    print('STATE turn ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                    screen = command('key space')
                    continue
                answer, prompt = response(screen)
                checkpoint = dict(kind='key', answer=answer, prompt=prompt,
                                  screen=event_text(screen), state=player_state(game))
                case['steps'].append(checkpoint)
                print('STATE checkpoint ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                screen = command('key ' + ('space' if answer == ' ' else answer))
            else:
                raise RuntimeError('Expected two completed encounters')
            print('STATE final ' + json.dumps(case['final'], sort_keys=True), flush=True)
        fixtures[:] = [old for old in fixtures if old['name'] != case['name']]
        fixtures.append(case)
        args.output.write_text(json.dumps(fixtures, indent=2) + '\n')


if __name__ == '__main__':
    main()
