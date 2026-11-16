#!/usr/bin/env python3
"""Capture controlled slot-13 tree/hut cases with the shared DOSBox runner."""

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


def cases():
    candidates = [n for n in range(200000) if rolls(n, (16,))[0] == 13]
    def seed(limits, predicate):
        return next(n for n in candidates if predicate(rolls(n, (16, *limits))[1:]))
    result = []
    def add(name, value, **options):
        config = dict(seed=value, hp=50, gold=20, food=100, male=5, female=5,
                      medicine=3, poison=0, ring=0, plague=0, looks=False,
                      horse=True, cart=True, staff=False, talisman=False, charm=0,
                      protection=False, level=8, swords=2, bows=3, arrows=4, daggers=1)
        for key in list(options):
            if key in config:
                config[key] = options.pop(key)
        result.append(dict(name=name, config=config, options=options))
    tree = seed((100,), lambda r:r[0]==50)
    hut = seed((100,), lambda r:r[0]==51)
    add('tree-decline-50', tree, enter=False)
    add('hut-decline-51', hut, enter=False)
    add('tree-steal', tree, action='s')
    add('tree-trunk', tree, action='t', staff=True)
    add('tree-level-one', tree, action='s', level=1)
    add('tree-health-above-new-cap', tree, action='t', hp=75)
    add('tree-ring-no-exemption', tree, action='s', ring=100)
    for reward in range(1,6):
        value = seed((100,5), lambda r:r[0]<51 and r[1]==reward)
        add('tree-leave-reward-'+str(reward), value, action='l')
        if reward==5:
            add('tree-talisman-held', value, action='l', talisman=True)
    for occupant in (1,2):
        who = 'man' if occupant==1 else 'puppy'
        value = seed((100,2), lambda r:r[0]>=51 and r[1]==occupant)
        add(who+'-refuse', value, help=False)
        for reward in range(1,6):
            value = seed((100,2,5), lambda r:r[0]>=51 and r[1]==occupant and r[2]==reward)
            add(who+'-reward-'+str(reward), value, help=True)
            if reward==5:
                add(who+'-talisman-held', value, help=True, talisman=True)
        if occupant==1:
            add('man-last-medicine', value, help=True, medicine=1)
            add('man-no-medicine', value, help=True, medicine=0)
        else:
            add('puppy-last-food', value, help=True, food=1, male=0, female=0)
            add('puppy-no-food', value, help=True, food=0, male=0, female=0)
    add('tree-invalid-choice', tree, action='xs')
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
        if 'hollow tree with a door' in prompt or prompt == 'Will you enter?':
            answer = options.get('enter', True)
        elif 'Try to cure him?' in prompt or 'Share some of your food?' in prompt:
            answer = options.get('help', True)
        else:
            raise RuntimeError('Unexpected prompt ' + prompt)
        return 'y' if answer else 'n', prompt + '\n(Y/N)'
    if lines[-1] == 'Press any key...':
        return ' ', lines[-1]
    if lines[-1].endswith('(L)eave.'):
        index = options.get('_action_index', 0)
        answer = options.get('action', 'l')[index]
        options['_action_index'] = index + 1
        return answer, lines[-1]
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/watched-home-checkpoints.json'))
    args = parser.parse_args()
    available = cases()
    selected = ([c for name in args.cases for c in available if c['name'] == name]
                if args.cases else available)
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

        for text in ('key space', 'text WatchedProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for case in selected:
            config = case['config']
            options = case['options']
            print('\nCASE ' + case['name'] + ' ' + json.dumps(case, sort_keys=True), flush=True)
            data = bytearray(game.read_memory('DS', 0x277, 0x49))
            struct.pack_into('<h', data, 0, config['level'])
            data[2:8] = real48(config['hp'])
            data[8:14] = b'\x05Black'
            for address, value in ((0x294, config['looks']), (0x295, config['ring']),
                                   (0x296, config['plague']), (0x297, config['staff']), (0x298, config['talisman']),
                                   (0x299, config['protection']), (0x2B8, config['horse']), (0x2B9, config['cart'])):
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
            options.pop('_action_index', None)
            for step in range(120):
                if screen.startswith('*** WatchedProbe ***') and screen.endswith('Any key.'):
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
                event_text = screen
                marker = ('You come up to a hollow tree' if 'You come up to a hollow tree' in screen
                          else 'You come upon a grass hut.')
                if marker in screen:
                    event_text = screen[screen.index(marker):]
                checkpoint['screen'] = event_text
                case['steps'].append(checkpoint)
                print('STATE checkpoint ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                screen = command('text ' + answer if answer in '/=\\' else 'key ' + ('space' if answer == ' ' else answer))
                if (screen.rstrip().endswith('The old man seems better.')
                        or screen.rstrip().endswith('It is greatful.')
                        or screen.rstrip().endswith('thepuppy.')):
                    screen = command('wait 1')
            else:
                raise RuntimeError('Expected end of watched home visit')
            if 'final' in case:
                print('STATE final ' + json.dumps(case['final'], sort_keys=True), flush=True)
            fixtures[:] = [old for old in fixtures if old['name'] != case['name']]
            fixtures.append(case)
            options.pop('_action_index', None)
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
                for text in ('key space', 'text WatchedProbe', 'key Return', 'key f', 'key space'):
                    command(text)


if __name__ == '__main__':
    main()
