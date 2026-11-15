#!/usr/bin/env python3
"""Capture controlled grass hut cases with the shared DOSBox runner."""

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

    def seed(limits, predicate):
        return find_seed((16, *limits), lambda r: r[0] == 7 and predicate(r[1:]))

    quiet = seed((100,), lambda r: r[0] == 50)
    noisy = seed((100,), lambda r: r[0] == 51)

    def add(name, value, **options):
        config = dict(seed=value, hp=50, gold=20, food=100, male=5, female=5,
                      medicine=3, poison=0, ring=0, plague=0, looks=True,
                      horse=True, cart=True, staff=False, talisman=False, charm=0,
                      swords=2, bows=3, arrows=4, daggers=1)
        for key in list(options):
            if key in config:
                config[key] = options.pop(key)
        result.append(dict(name=name, config=config, options=options))

    add('decline-quiet-50', quiet, enter=False)
    add('decline-noisy-51', noisy, enter=False)
    old = seed((100, 3), lambda r: r[0] > 50 and r[1] == 1)
    add('old-man-medicine', old, help=True)
    add('old-man-comfort', old, help=True, medicine=0)
    add('old-man-decline', old)
    puppy = seed((100, 3), lambda r: r[0] > 50 and r[1] == 2)
    add('puppy-feed', puppy, help=True)
    add('puppy-pet', puppy, help=True, food=0)
    add('puppy-last-food', puppy, help=True, food=1)
    add('puppy-decline', puppy)
    for find in (1, 2, 4, 5, 6, 7, 9):
        value = seed((100, 9), lambda r: r[0] <= 50 and r[1] == find)
        add('find-' + str(find), value)
    staff = seed((100, 9, 100), lambda r: r[0] <= 50 and r[1] == 3 and r[2] == 4)
    add('staff-found-4', staff)
    add('staff-held', staff, staff=True)
    add('staff-missed-5', seed((100, 9, 100), lambda r: r[0] <= 50 and r[1] == 3 and r[2] == 5))
    add('gate-closes-13', seed((100, 9, 100), lambda r: r[0] <= 50 and r[1] == 8 and r[2] == 13))
    hermit = seed((100, 3), lambda r: r[0] > 50 and r[1] == 3)
    add('hermit-leave', hermit)
    charm_win = seed((100, 3, 3, 100), lambda r: r[0] > 50 and r[1] == 3 and r[3] == 21)
    add('hermit-charm-double-21', charm_win, duel=True, charm=1, poison=2)
    charm_theft = seed((100, 3, 3, 100, 10), lambda r: r[0] > 50 and r[1] == 3 and r[3] == 20 and r[4] > 5)
    add('hermit-charm-theft-20', charm_theft, duel=True, charm=1)
    add('hermit-theft-no-females', charm_theft, duel=True, charm=1, female=0)
    add('hermit-kill-charmed', hermit, kill=True, charm=1)
    poisoned_win = seed((100, 3, 14, 50, 100), lambda r: r[0] > 50 and r[1] == 3 and r[4] >= 21)
    add('hermit-poison-double', poisoned_win, duel=True, use_poison=True, poison=1)
    add('hermit-kill-poison', hermit, kill=True, use_poison=True, poison=1)
    # Select a short unpoisoned dragon encounter by its minimum HP and large first hit.
    full_dragon = seed((100, 3, 14, 50, 7), lambda r: r[0] > 50 and r[1] == 3 and r[3] == 1 and r[4] == 7)
    add('dragon-fight', full_dragon, duel=True, hp=100)
    add('dragon-decline-poison', full_dragon, duel=True, hp=100, poison=1)
    for reward in range(1, 7):
        monster = min(reward, 5)
        value = seed((100, 9, 100, 5, 14, 100, 6),
                     lambda r: r[0] <= 50 and r[1] == 8 and r[2] <= 12 and
                     r[3] == monster and r[5] > (16, 16, 18, 20, 22)[monster - 1] and r[6] == reward)
        add('creature-reward-' + str(reward), value, use_poison=True, poison=1,
            horse=False, cart=False, hp=90 if reward == 5 else 50)
        if reward == 1:
            add('creature-held-staff-food', value, use_poison=True, poison=1, staff=True)
        if reward == 2:
            add('creature-held-talisman-food', value, use_poison=True, poison=1, talisman=True)
    gate_fight = seed((100, 9, 100, 5, 5), lambda r: r[0] <= 50 and r[1] == 8 and r[2] == 12 and r[3:] == [1, 1])
    add('creature-fight', gate_fight, hp=100)
    saved = seed((100, 9, 100, 5, 5, 100), lambda r: r[0] <= 50 and r[1] == 8 and r[2] <= 12 and r[3:5] == [1, 1] and r[5] <= 16)
    add('creature-saves-poison', saved, hp=100, poison=1, use_poison=True)
    add('dragon-death', full_dragon, duel=True, hp=0.05)
    add('creature-death', gate_fight, hp=0.5)
    return result


def player_state(game):
    base = 0x1FC
    data = game.read_memory('DS', base, 0x1F6)
    result = {'seed': int.from_bytes(data[:4], 'little')}
    for name, address in (('level', 0x277), ('charm', 0x29A), ('male', 0x29C), ('female', 0x29E),
                          ('medicine', 0x2AC), ('poison', 0x2AE), ('swords', 0x2B0),
                          ('bows', 0x2B2), ('arrows', 0x2B4), ('daggers', 0x2B6)):
        result[name] = struct.unpack_from('<h', data, address - base)[0]
    for name, address in (('hp', 0x279), ('gold', 0x2A0), ('food', 0x2A6), ('experience', 0x2BA)):
        result[name] = round(read_real48(data[address - base:address - base + 6]), 8)
    for name, address in (('staff', 0x297), ('talisman', 0x298), ('looks', 0x294), ('ring', 0x295), ('plague', 0x296),
                          ('protection', 0x299), ('horse', 0x2B8), ('cart', 0x2B9)):
        result[name] = data[address - base]
    length = data[0x27F - base]
    result['hair'] = data[0x280 - base:0x280 - base + length].decode('ascii')
    return result


def response(screen, options):
    lines = [line.strip() for line in screen.splitlines() if line.strip()]
    if lines[-1] == '(Y/N)':
        prompt = lines[-2]
        if prompt == 'Will you enter?':
            answer = options.get('enter', True)
        elif 'Try to cure him?' in prompt or 'Share some of your food?' in prompt:
            answer = options.get('help', False)
        elif 'double your posessions' in prompt:
            answer = options.get('duel', False)
        elif prompt == 'Kill the hermit?':
            answer = options.get('kill', False)
        elif prompt == 'Do you wish to use your poison?':
            answer = options.get('use_poison', False)
        else:
            raise RuntimeError('Unexpected prompt ' + prompt)
        return 'y' if answer else 'n', prompt + '\n(Y/N)'
    if lines[-1] == 'Press any key...':
        return ' ', lines[-1]
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/hut-checkpoints.json'))
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

        for text in ('key space', 'text HutProbe', 'key Return', 'key f', 'key space'):
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
                                   (0x296, config['plague']), (0x297, config['staff']), (0x298, config['talisman']),
                                   (0x299, 1), (0x2B8, config['horse']), (0x2B9, config['cart'])):
                data[address - 0x277] = value
            for address, value in ((0x29A, config['charm']), (0x29C, config['male']), (0x29E, config['female']),
                                   (0x2AC, config['medicine']), (0x2AE, config['poison']), (0x2B0, config['swords']),
                                   (0x2B2, config['bows']), (0x2B4, config['arrows']), (0x2B6, config['daggers'])):
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
                if screen.startswith('*** HutProbe ***') and screen.endswith('Any key.'):
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
                marker = 'You come upon a grass hut.'
                event_text = screen[screen.index(marker):] if marker in screen else screen
                checkpoint['screen'] = event_text
                case['steps'].append(checkpoint)
                print('STATE checkpoint ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                screen = command('key ' + ('space' if answer == ' ' else answer))
            else:
                raise RuntimeError('Expected end of hut visit')
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
                for text in ('key space', 'text HutProbe', 'key Return', 'key f', 'key space'):
                    command(text)


if __name__ == '__main__':
    main()
