#!/usr/bin/env python3
"""Controlled original final-assault army rounds, events and termination."""

import argparse
import json
from pathlib import Path
import struct

from dosbox_game import Game
from final_probe import CONVERTED, OVERLAY_BASE, cases as conversion_cases
from final_probe import resume, start_final_assault, wait_at
from keep_probe import paused_screen, state


def roll(seed, limit):
    if limit <= 0:
        return seed, 0
    seed = (seed * 129 + 0x361962E9) & 0xFFFFFFFF
    return seed, (seed >> 17) % limit + 1


def stop_at(game, cs, offset):
    game.debug_command('BPDEL *')
    game.debug_command(f'BP {cs:04X}:{OVERLAY_BASE + offset:04X}')
    resume(game)
    wait_at(game)


def snapshot(game):
    army, enemy = struct.unpack('<hh', game.dump('DS', 0x4FF, 4))
    return dict(army=army, enemy=enemy,
                seed=int.from_bytes(game.dump('DS', 0x1FC, 4), 'little'))


def reset(game, army, enemy, seed):
    game.write_memory('DS', 0x4FF, struct.pack('<hh', army, enemy), paused=True)
    game.write_memory('DS', 0x1FC, seed.to_bytes(4, 'little'), paused=True)


def capture(game, label):
    result = dict(state=snapshot(game), screen=paused_screen(game))
    print('\nCHECK ' + label + ' ' + json.dumps(result, sort_keys=True), flush=True)
    return result


def setup(game):
    config = conversion_cases()[0]['config']
    cs = start_final_assault(game, config)
    stop_at(game, cs, CONVERTED - OVERLAY_BASE)
    capture(game, 'conversion')
    return cs


def event_cases():
    cases = []
    for side in (1, 2):
        for kind in range(1, 11):
            for seed in range(1000000):
                s, trigger = roll(seed, 100)
                s, selected_side = roll(s, 2)
                s, selected_kind = roll(s, 10)
                if trigger < 5 and selected_side == side and selected_kind == kind:
                    cases.append(dict(name=f'event-{side}-{kind}', army=100, enemy=80,
                                      seed=seed, side=side, event=kind))
                    break
            else:
                raise RuntimeError('No event seed')
    for kind in (3, 4, 5, 6):
        case = next(c for c in cases if c['side'] == 1 and c['event'] == kind)
        cases.append(dict(case, name=f'small-event-{kind}', army=2, enemy=3))
    # Explicitly retain a surprise killing the opposing army past zero.
    for seed in range(1000000):
        s, trigger = roll(seed, 100)
        s, side = roll(s, 2)
        s, kind = roll(s, 10)
        s, amount = roll(s, 100)
        if trigger < 5 and side == 1 and kind == 7 and amount > 3:
            cases.append(dict(name='surprise-overkill', army=100, enemy=3,
                              seed=seed, side=side, event=kind))
            break
    return cases


def run_events(output):
    fixtures = []
    with Game() as game:
        cs = setup(game)
        reset(game, 100, 80, 1)
        stop_at(game, cs, 0x8DD)
        capture(game, 'first casualty batch before controlled event setups')
        for case in event_cases():
            print('\nCASE ' + json.dumps(case, sort_keys=True), flush=True)
            reset(game, case['army'], case['enemy'], case['seed'])
            case['before'] = state(game, paused=True)
            stop_at(game, cs, 0x6DA)
            case['applied'] = capture(game, 'event applied')
            stop_at(game, cs, 0x8FD)
            case['cleared'] = capture(game, 'event cleared')
            case['after'] = state(game, paused=True)
            fixtures.append(case)
            output.write_text(json.dumps(fixtures, indent=2) + '\n')
            # Stay inside this initialized army frame for the next independent probe.
            reset(game, 100, 80, 1)
            stop_at(game, cs, 0x8DD)
            capture(game, 'inter-case casualty batch (discarded controlled setup)')


def battle_cases():
    return [dict(name='baseline-loss', seed=3080093198, army=120, enemy=160),
            dict(name='equal-one', seed=1, army=1, enemy=1),
            dict(name='small-win', seed=1, army=8, enemy=2),
            dict(name='equal-small', seed=99, army=5, enemy=5),
            dict(name='zero-player', seed=1, army=0, enemy=0, abort=True),
            dict(name='zero-enemy', seed=1, army=5, enemy=0, abort=True)]


def run_battles(output, names):
    fixtures = json.loads(output.read_text()) if output.exists() else []
    for case in battle_cases():
        if names and case['name'] not in names:
            continue
        print('\nCASE ' + json.dumps(case, sort_keys=True), flush=True)
        with Game() as game:
            cs = setup(game)
            reset(game, case['army'], case['enemy'], case['seed'])
            case['before'] = state(game, paused=True)
            case['rounds'] = []
            if case.get('abort'):
                stop_at(game, cs, 0x8C4)
                case['casualties'] = capture(game, 'before zero-denominator bars')
                game.debug_command('BPDEL *')
                resume(game)
                game.settle(1)
                case['screen'] = game.screen()
                print(case['screen'], flush=True)
                if 'Run-time error 02' not in case['screen']:
                    raise RuntimeError('Expected zero-denominator runtime error')
            else:
                for index in range(1000):
                    stop_at(game, cs, 0x8DD)
                    casualties = capture(game, 'casualties')
                    s, trigger = roll(casualties['state']['seed'], 100)
                    event = None
                    if trigger < 5:
                        stop_at(game, cs, 0x6DA)
                        event = capture(game, 'event applied')
                    stop_at(game, cs, 0x8FD)
                    after = capture(game, 'round')
                    case['rounds'].append(dict(casualties=casualties, event=event, after=after))
                    if min(after['state']['army'], after['state']['enemy']) < 1:
                        break
                else:
                    raise RuntimeError('Army battle did not finish')
                stop_at(game, cs, 0x9D4)
                case['finished'] = capture(game, 'battle finished before caller/duel')
                case['after'] = state(game, paused=True)
                if case['finished']['state']['army'] == 0:
                    game.debug_command('BPDEL *')
                    resume(game)
                    game.settle(0.5)
                    case['screen'] = game.screen()
                    print(case['screen'], flush=True)
        fixtures[:] = [c for c in fixtures if c['name'] != case['name']] + [case]
        output.write_text(json.dumps(fixtures, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('events', 'battles'))
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    output = args.output or Path(f'doc/army-{args.mode}-checkpoints.json')
    print('CONTROLLED: temporary state/RNG reset after conversion; no original disk changes.', flush=True)
    if args.mode == 'events':
        if args.cases:
            parser.error('Event batch runs all sides/types in one initialized army frame')
        run_events(output)
    else:
        if set(args.cases) - {c['name'] for c in battle_cases()}:
            parser.error('Unknown battle case')
        run_battles(output, args.cases)


if __name__ == '__main__':
    main()
