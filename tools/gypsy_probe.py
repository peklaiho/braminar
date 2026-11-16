#!/usr/bin/env python3
"""Capture controlled Gypsy camp cases with the shared DOSBox runner."""

import argparse
import json
from pathlib import Path
import struct

from combat_probe import real48, read_real48
from dosbox_game import Game


def rolls(seed, limits):
    values = []
    for limit in limits:
        seed = (seed * 129 + 0x361962E9) & 0xFFFFFFFF
        values.append((seed >> 17) % limit + 1)
    return values


def find_seed(limits, predicate):
    for seed in range(1000000):
        if predicate(rolls(seed, limits)):
            return seed
    raise RuntimeError('No seed found')


def cases():
    result = []
    camp = find_seed((16,), lambda r: r[0] == 3)

    def add(name, seed=camp, gold=20, hp=50, **options):
        config = dict(seed=seed, hp=hp, gold=gold, food=100, male=5, female=5,
                      medicine=3, ring=0, plague=0, looks=True, horse=False, cart=False)
        result.append(dict(name=name, config=config, options=options))

    add('decline', enter=False)
    add('skip')
    add('zero-play', gold=0, play=True)
    add('zero-decline', gold=0)
    for name, predicate in (
        ('loss', lambda r: r[1] < 16 and r[2] >= 16),
        ('both-hit', lambda r: r[1] < 16 and r[2] < 16),
        ('win', lambda r: r[1] >= 16 and r[2] < 16),
        ('second-round-win', lambda r: min(r[1:4]) >= 16 and r[4] < 16),
        ('boundary-15', lambda r: r[1] == 15 and r[2] == 16),
        ('boundary-16', lambda r: r[1] == 16 and r[2] == 15),
    ):
        seed = find_seed((16, 100, 100, 100, 100), lambda r: r[0] == 3 and predicate(r))
        add(name, seed, play=True)
    safe = find_seed((16, 100, 100), lambda r: r[0] == 3 and min(r[1:]) >= 16)
    add('last-coin-exhausted', safe, gold=1, play=True, advice=True)
    win = next(c['config']['seed'] for c in result if c['name'] == 'win')
    add('last-coin-win', win, gold=1, play=True)
    add('advice-insufficient', gold=9, advice=True)
    add('loss-advice-unaffordable', next(c['config']['seed'] for c in result if c['name'] == 'loss'),
        gold=10, play=True, advice=True)
    add('win-and-advice', win, play=True, advice=True)
    for rumor in range(1, 36):
        seed = find_seed((16, 35), lambda r: r == [3, rumor])
        add('advice-' + str(rumor), seed, gold=10 if rumor == 1 else 11, advice=True)
    loss = next(c['config']['seed'] for c in result if c['name'] == 'loss')
    add('fatal-injury-advice', loss, hp=0.2, play=True, advice=True)
    return result


def player_state(game):
    base = 0x1FC
    data = game.read_memory('DS', base, 0x1F6)
    result = {'seed': int.from_bytes(data[:4], 'little')}
    for name, address in (('level', 0x277), ('male', 0x29C), ('female', 0x29E),
                          ('medicine', 0x2AC), ('poison', 0x2AE), ('swords', 0x2B0),
                          ('bows', 0x2B2), ('arrows', 0x2B4), ('daggers', 0x2B6)):
        result[name] = struct.unpack_from('<h', data, address - base)[0]
    for name, address in (('hp', 0x279), ('gold', 0x2A0), ('food', 0x2A6), ('experience', 0x2BA)):
        result[name] = round(read_real48(data[address - base:address - base + 6]), 8)
    for name, address in (('looks', 0x294), ('ring', 0x295), ('plague', 0x296),
                          ('protection', 0x299), ('horse', 0x2B8), ('cart', 0x2B9)):
        result[name] = data[address - base]
    length = data[0x27F - base]
    result['hair'] = data[0x280 - base:0x280 - base + length].decode('ascii')
    return result


def response(screen, options):
    lines = [line.strip() for line in screen.splitlines() if line.strip()]
    if lines[-1] == '(Y/N)':
        prompt = lines[-2]
        mapping = {'You come upon a Wandering Gypsy Camp.  Enter?': ('enter', True),
                   'Play the game?': ('play', False),
                   'will you buy some advice?': ('advice', False)}
        option, default = mapping[prompt]
        return 'y' if options.get(option, default) else 'n', prompt + '\n(Y/N)'
    if lines[-1] == 'Press any key...':
        return ' ', lines[-1]
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/gypsy-checkpoints.json'))
    args = parser.parse_args()
    selected = [c for c in cases() if not args.cases or c['name'] in args.cases]
    if not selected or set(args.cases) - {c['name'] for c in selected}:
        parser.error('Unknown case')
    output = args.output
    fixtures = json.loads(output.read_text()) if args.cases and output.exists() else []
    print('CONTROLLED PROBES: temporary player memory, fixed servant prices 8/13 and RNG are modified.', flush=True)
    with Game() as game:
        print(game.screen(), flush=True)

        def command(text):
            game.command(text)
            screen = game.screen()
            print(f'\n--- {text} ---\n{screen}', flush=True)
            return screen

        for text in ('key space', 'text GypsyProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for case in selected:
            config = case['config']
            options = case['options']
            print('\nCASE ' + case['name'] + ' ' + json.dumps(case, sort_keys=True), flush=True)
            data = bytearray(game.read_memory('DS', 0x277, 0x49))
            struct.pack_into('<h', data, 0, 8)
            data[2:8] = real48(config['hp'])
            data[8:14] = b'\x05Black'
            for address, value in ((0x294, config['looks']), (0x295, config['ring']),
                                   (0x296, config['plague']), (0x297, 0), (0x298, 0),
                                   (0x299, 1), (0x2B8, config['horse']), (0x2B9, config['cart'])):
                data[address - 0x277] = value
            for address, value in ((0x29C, config['male']), (0x29E, config['female']),
                                   (0x2AC, config['medicine']), (0x2AE, 0), (0x2B0, 0),
                                   (0x2B2, 0), (0x2B4, 0), (0x2B6, 0)):
                struct.pack_into('<h', data, address - 0x277, value)
            for address, value in ((0x2A0, config['gold']), (0x2A6, config['food']), (0x2BA, 0)):
                data[address - 0x277:address - 0x277 + 6] = real48(value)
            game.write_memory('DS', 0x277, data)
            game.write_memory('DS', 0x3EE, struct.pack('<hh', 8, 13))
            game.write_memory('DS', 0x1FC, config['seed'].to_bytes(4, 'little'))
            print('STATE initial ' + json.dumps(player_state(game), sort_keys=True), flush=True)
            screen = command('key space')
            case['steps'] = []
            for step in range(120):
                if screen.startswith('*** GypsyProbe ***') and screen.endswith('Any key.'):
                    case['final'] = player_state(game)
                    break
                if 'Program aborted' in screen:
                    case['aborted'] = True
                    case['screen'] = screen
                    break
                if 'Game Over.' in screen:
                    case['fatal'] = True
                    case['final'] = case['steps'][-1]['state']
                    break
                reply = response(screen, options)
                if reply is None:
                    screen = command('wait 1')
                    continue
                answer, prompt = reply
                kind = 'key'
                checkpoint = dict(kind=kind, answer=answer, prompt=prompt, state=player_state(game))
                marker = 'You come upon a Wandering Gypsy Camp.'
                event_text = screen[screen.index(marker):] if marker in screen else screen
                checkpoint['screen'] = event_text
                case['steps'].append(checkpoint)
                print('STATE checkpoint ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                screen = command('key ' + ('space' if answer == ' ' else answer))
                # Gambling runs automatically between Play and the result pause.
                if prompt == 'Play the game?\n(Y/N)' and answer == 'y' and config['gold'] > 0:
                    while not screen.endswith('Press any key...'):
                        screen = command('wait 0.5')
            else:
                raise RuntimeError('Expected end of camp visit')
            if 'final' in case:
                print('STATE final ' + json.dumps(case['final'], sort_keys=True), flush=True)
            fixtures[:] = [old for old in fixtures if old['name'] != case['name']]
            fixtures.append(case)
            output.write_text(json.dumps(fixtures, indent=2) + '\n')
            if case.get('fatal') or case.get('aborted'):
                if case is selected[-1]:
                    break
                command('text BRAMINAR.COM')
                command('key Return')
                for step in range(10):
                    if command('wait 1').lstrip().startswith('Welcome to:'):
                        break
                else:
                    raise RuntimeError('No welcome screen after restart')
                for text in ('key space', 'text GypsyProbe', 'key Return', 'key f', 'key space'):
                    command(text)


if __name__ == '__main__':
    main()
