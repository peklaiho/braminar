#!/usr/bin/env python3
"""Capture controlled castle cases with the shared DOSBox runner."""

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
    candidates = [(n * 2654435761) & 0xFFFFFFFF for n in range(2000000)
                  if rolls((n * 2654435761) & 0xFFFFFFFF, (16,))[0] == 11]
    def seed(limits, predicate):
        return next(n for n in candidates if predicate(rolls(n, (16, *limits))[1:]))
    result = []
    def add(name, value, **options):
        config = dict(seed=value, hp=50, gold=20, food=100, male=5, female=5,
                      medicine=3, poison=0, ring=0, plague=10, looks=False,
                      horse=True, cart=True, staff=False, talisman=False, charm=0,
                      protection=False, level=8, swords=2, bows=3, arrows=4, daggers=1)
        for key in list(options):
            if key in config:
                config[key] = options.pop(key)
        result.append(dict(name=name, config=config, options=options))
    quiet = seed((100,), lambda r: r[0] == 26)
    add('decline', quiet, enter=False)
    add('throne-leave', quiet, room='t', theft=' ')
    add('throne-one-bag', quiet, room='t', theft='1/=\\h ')
    add('throne-two-bags', quiet, room='t', theft='1/=\\H1/=\\H ')
    add('king-poison', quiet, room='t', theft='x'*16, poison=1, use_poison=True)
    add('king-full-fight', quiet, room='t', theft='x'*16, hp=200)
    add('king-ring', quiet, room='t', theft='x'*16, hp=200, ring=1)
    add('king-bow', quiet, room='t', theft='x'*16, hp=200, swords=0, daggers=0)
    add('king-unarmed', quiet, room='t', theft='x'*16, hp=200, swords=0, daggers=0, bows=0)
    add('king-decline-poison', quiet, room='t', theft='x'*16, hp=200, poison=1)
    add('king-death', quiet, room='t', theft='x'*16, hp=0.5)
    add('throne-space-at-16', quiet, room='t', theft='x'*15+' ', poison=1, use_poison=True)
    add('throne-safe-15', quiet, room='t', theft='x'*14+' ')
    add('throne-error-progress', quiet, room='t', theft='1/1=\\H ', poison=1, use_poison=True)
    add('throne-warning', quiet, room='t', theft='x'*7+'1/=\\H ', poison=1, use_poison=True)
    add('throne-drops', quiet, room='t', theft='/////', hp=200, poison=1, use_poison=True)
    add('ghost-flee-25', seed((100,100), lambda r:r == [25,32]))
    add('ghost-resist-33', seed((100,100), lambda r:r == [25,33]), room='t', theft=' ')
    # Dungeon: appearance, gold, smell, Staff, males, [count], poison, door.
    limits = (100,10,100,100,100,100,100)
    normal = lambda r:r[0]>=26 and r[4]>=11
    add('dungeon', seed(limits, lambda r:normal(r) and r[2]>=19 and r[3]>=6 and r[5]>=4 and r[6]>=13))
    add('dungeon-smell-18', seed(limits, lambda r:normal(r) and r[2]==18))
    staff = seed(limits, lambda r:normal(r) and r[3]==5)
    add('dungeon-staff-5', staff)
    add('dungeon-staff-held', staff, staff=True)
    add('dungeon-staff-miss-6', seed(limits, lambda r:normal(r) and r[3]==6))
    add('dungeon-males-10', seed((100,10,100,100,100,5,100,100), lambda r:r[0]>=26 and r[4]==10))
    add('dungeon-poison-3', seed(limits, lambda r:normal(r) and r[5]==3))
    door = seed(limits, lambda r:normal(r) and r[6]==12)
    add('dungeon-door-servants', door, daggers=0)
    add('dungeon-door-female', door, male=0, daggers=0)
    add('dungeon-door-dagger', door, male=0, female=0)
    add('dungeon-starve', door, male=0, female=0, daggers=0)
    add('galley-no-vision', quiet, room='g')
    for vision in range(1,9):
        n = seed((100,100), lambda r:r[0]>=26 and r[1] == (vision*2 if vision<=7 else 15))
        add('galley-vision-'+str(vision), n, room='g', gaze=True)
    talisman = seed((100,100,200,100), lambda r:r[0]>=26 and r[1]==8)
    add('galley-talisman-8', talisman, room='g')
    add('galley-vision-held-items', seed((100,100), lambda r:r[0]>=26 and r[1]==8), room='g', gaze=True, staff=True, talisman=True)
    add('galley-talisman-held', talisman, room='g', talisman=True)
    add('galley-talisman-miss-9', seed((100,100,200,100), lambda r:r[0]>=26 and r[1]==9), room='g')
    for food in (0,199):
        add('galley-food-'+str(food), seed((100,100,200,100), lambda r:r[0]>=26 and r[2]==food+1), room='g')
    for pit in (35,36):
        add('galley-pits-'+str(pit), seed((100,100,200,100), lambda r:r[0]>=26 and r[3]==pit), room='g')
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
        if 'Dark Castle' in prompt:
            answer = options.get('enter', True)
        elif 'gaze into it?' in prompt:
            answer = options.get('gaze', False)
        elif 'Use your poison?' in prompt:
            answer = options.get('use_poison', False)
        else:
            raise RuntimeError('Unexpected prompt ' + prompt)
        return 'y' if answer else 'n', prompt + '\n(Y/N)'
    if lines[-1] == 'Press any key...':
        return ' ', lines[-1]
    if lines[-1].endswith('(G)alley'):
        return options.get('room', 'd'), lines[-1]
    if 'Space bar = leave the room.' in screen and 'The Mad king rushes' not in screen:
        index = options.get('_theft_index', 0)
        theft = options.get('theft', ' ')
        answer = theft[index] if index < len(theft) else ' '
        options['_theft_index'] = index + 1
        return answer, ''
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/castle-checkpoints.json'))
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

        for text in ('key space', 'text CastleProbe', 'key Return', 'key f', 'key space'):
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
            options.pop('_theft_index', None)
            for step in range(120):
                if screen.startswith('*** CastleProbe ***') and screen.endswith('Any key.'):
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
                marker = 'You come upon the Dark Castle'
                if marker in screen:
                    event_text = screen[screen.index(marker):]
                checkpoint['screen'] = event_text
                case['steps'].append(checkpoint)
                print('STATE checkpoint ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                screen = command('text ' + answer if answer in '/=\\' else 'key ' + ('space' if answer == ' ' else answer))
                if screen.rstrip().endswith('You drop some gold.'):
                    screen = command('wait 1')
            else:
                raise RuntimeError('Expected end of castle visit')
            if 'final' in case:
                print('STATE final ' + json.dumps(case['final'], sort_keys=True), flush=True)
            fixtures[:] = [old for old in fixtures if old['name'] != case['name']]
            fixtures.append(case)
            options.pop('_theft_index', None)
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
                for text in ('key space', 'text CastleProbe', 'key Return', 'key f', 'key space'):
                    command(text)


if __name__ == '__main__':
    main()
