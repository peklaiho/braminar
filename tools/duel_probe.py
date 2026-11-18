#!/usr/bin/env python3
"""Controlled original Overlord duel, entered through the real assault and army."""

import argparse
import json
from pathlib import Path

from army_probe import reset, roll, stop_at
from combat_probe import read_real48, real48
from dosbox_game import Game
from final_probe import CONVERTED, OVERLAY_BASE, cases as conversion_cases
from final_probe import resume, start_final_assault
from keep_probe import paused_screen, prepare, state


def frame(game):
    path = game.path / 'MEMDUMP.BIN'
    path.unlink(missing_ok=True)
    game.debug_command('MEMDUMPBIN SS:EBP-10 10')
    data = path.read_bytes()
    if len(data) != 16:
        raise RuntimeError('Unexpected duel frame dump')
    return data


def local_state(game):
    data = frame(game)
    return dict(player_hp=read_real48(data[2:8]), overlord_hp=read_real48(data[8:14]),
                seed=int.from_bytes(game.dump('DS', 0x1FC, 4), 'little'))


def set_locals(game, player_hp, overlord_hp, seed):
    for expr, value in (('EBP-E', player_hp), ('EBP-8', overlord_hp)):
        data = real48(value)
        game.debug_command('SM SS:' + expr + ' ' + ' '.join(f'{n:02X}' for n in data))
    data = frame(game)
    if data[2:14] != real48(player_hp) + real48(overlord_hp):
        raise RuntimeError('Duel-local write failed readback')
    game.write_memory('DS', 0x1FC, seed.to_bytes(4, 'little'), paused=True)


def capture(game, label):
    result = dict(state=local_state(game), screen=paused_screen(game))
    print('\nCHECK ' + label + ' ' + json.dumps(result, sort_keys=True), flush=True)
    return result


def setup(game, config):
    cs = start_final_assault(game, conversion_cases()[0]['config'])
    stop_at(game, cs, CONVERTED - OVERLAY_BASE)
    print('CONVERSION\n' + paused_screen(game), flush=True)
    reset(game, 8, 2, 1)
    stop_at(game, cs, 0x2059)
    print('ARMY WIN, DUEL ENTRY\n' + paused_screen(game), flush=True)
    prepare(game, config, paused=True)
    game.write_memory('DS', 0x4F8, b'\x01', paused=True)
    # Delay uses only this runtime loop calibration; speed controlled captures.
    game.write_memory('DS', 0x12, b'\x01\x00', paused=True)
    print('DELAY CALIBRATION: DS:0012 set to 1; requested game delays unchanged.', flush=True)
    print('STATE controlled duel setup ' + json.dumps(state(game, True), sort_keys=True), flush=True)
    stop_at(game, cs, 0x221F)
    return cs


def seed_for(player, enemy):
    for seed in range(10000):
        s, a = roll(seed, 8)
        s, b = roll(s, 8)
        if (a, b) == (player, enemy):
            return seed
    raise RuntimeError('No attack-pair seed')


def round_cases():
    result = [dict(name=f'pair-{i}', seed=seed_for(i, i), player_hp=100, overlord_hp=100)
              for i in range(1, 9)]
    for name, hp, enemy, a, b in (('killing-blow-retaliation',20,.1,8,8),
                                 ('simultaneous-death',.1,.1,8,8),
                                 ('exact-enemy-zero',100,12.7,8,1),
                                 ('exact-player-zero',11.2,100,1,8)):
        result.append(dict(name=name, seed=seed_for(a,b), player_hp=hp, overlord_hp=enemy))
    return result


def full_cases():
    base = conversion_cases()[0]['config']
    result = []
    for name, changes in (
            ('talisman', dict(seed=1, hp=.1, talisman=True)),
            ('normal-victory', dict(seed=0, level=1, hp=1)),
            ('endgame-duel', dict(seed=0, level=20, hp=1)),
            ('normal-defeat', dict(seed=5, level=1, hp=500)),
            ('simultaneous-defeat', dict(seed=1, level=1, hp=500)),
            ('ignored-inventory', dict(seed=0, level=1, hp=999, ring=100, poison=99,
                                       swords=99, daggers=99, bows=99, arrows=99))):
        result.append(dict(name=name, config=dict(base, **changes)))
    return result


def run_rounds(output):
    result = []
    with Game() as game:
        config = conversion_cases()[0]['config']
        cs = setup(game, config)
        capture(game, 'initialized duel')
        stop_at(game, cs, 0x2299)
        for case in round_cases():
            print('\nCASE ' + json.dumps(case), flush=True)
            set_locals(game, case['player_hp'], case['overlord_hp'], case['seed'])
            case['before'] = state(game, True)
            case['before_screen'] = paused_screen(game)
            stop_at(game, cs, 0x27F0)
            case['applied'] = capture(game, 'round applied')
            stop_at(game, cs, 0x2882)
            case['cleared'] = capture(game, 'round cleared')
            case['after'] = state(game, True)
            result.append(case)
            output.write_text(json.dumps(result, indent=2) + '\n')
            # Prevent a controlled lethal round from ending this reusable frame.
            set_locals(game, 100, 100, 1)
            stop_at(game, cs, 0x2299)


def run_full(output, names):
    result = json.loads(output.read_text()) if output.exists() else []
    for case in full_cases():
        if names and case['name'] not in names:
            continue
        print('\nCASE ' + json.dumps(case, sort_keys=True), flush=True)
        with Game() as game:
            cs = setup(game, case['config'])
            case['initial'] = capture(game, 'initialized duel')
            case['before'] = state(game, True)
            case['rounds'] = []
            if case['config']['talisman']:
                stop_at(game, cs, 0x28AD)
                case['talisman'] = capture(game, 'talisman applied')
            else:
                for index in range(1000):
                    stop_at(game, cs, 0x27F0)
                    applied = capture(game, 'round applied')
                    stop_at(game, cs, 0x2882)
                    cleared = capture(game, 'round cleared')
                    case['rounds'].append(dict(applied=applied, cleared=cleared))
                    if min(applied['state']['player_hp'], applied['state']['overlord_hp']) <= 0:
                        break
                else:
                    raise RuntimeError('Duel did not finish')
            stop_at(game, cs, 0x291F)
            case['grave'] = capture(game, 'grave after final delay')
            if case['grave']['state']['player_hp'] <= 0:
                stop_at(game, cs, 0x297E)
                case['finished'] = capture(game, 'duel death before return')
                case['after'] = state(game, True)
                game.debug_command('BPDEL *')
                resume(game)
                game.settle(.5)
                print('EXIT\n' + game.screen(), flush=True)
            else:
                # Stop before ending/reward processing changes the global player.
                stop_at(game, cs, 0x2975)
                case['finished'] = capture(game, 'duel victory before ending')
            if case['grave']['state']['player_hp'] > 0:
                case['after'] = state(game, True)
        result[:] = [c for c in result if c['name'] != case['name']] + [case]
        output.write_text(json.dumps(result, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('rounds','full'))
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    output = args.output or Path(f'doc/duel-{args.mode}-checkpoints.json')
    print('CONTROLLED: real loader and small army win; temporary player, RNG and duel locals edited.', flush=True)
    if args.mode == 'rounds':
        if args.cases:
            parser.error('Round batch runs all selected attack pairs in one initialized frame')
        run_rounds(output)
    else:
        if set(args.cases) - {c['name'] for c in full_cases()}:
            parser.error('Unknown duel case')
        run_full(output, args.cases)


if __name__ == '__main__':
    main()
