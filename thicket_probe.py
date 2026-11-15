#!/usr/bin/env python3
"""Capture controlled thicket cases with the shared DOSBox runner."""

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
    candidates = [seed for seed in range(1000000)
                  if (((seed * 129 + 0x361962E9) & 0xFFFFFFFF) >> 17) % 16 == 4]

    def seed(limits, predicate):
        for value in candidates:
            if predicate(rolls(value, (16, *limits))[1:]):
                return value
        raise RuntimeError('No seed found for ' + str(limits))

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

    # No-wizard draws: roast, appearance, males, females, gold, Staff, potion chance.
    loot_limits = (8, 100, 60, 60, 250, 100, 100)
    quiet = seed(loot_limits, lambda r: r[1] == 51 and r[6] >= 26)
    add('decline', quiet, enter=False)
    add('explore-wizard-absent-51', quiet)
    staff = seed(loot_limits, lambda r: r[1] >= 51 and r[5] == 4 and r[6] >= 26)
    add('staff-found-4', staff)
    add('staff-held', staff, staff=True)
    add('staff-missed-5', seed(loot_limits, lambda r: r[1] >= 51 and r[5] == 5 and r[6] >= 26))
    add('potions-decline-25', seed(loot_limits, lambda r: r[1] >= 51 and r[6] == 25))
    add('potions-absent-26', seed(loot_limits, lambda r: r[1] >= 51 and r[6] == 26))
    for potion in range(1, 12):
        value = seed((*loot_limits, 5, 11), lambda r: r[1] >= 51 and r[6] <= 25 and r[7:] == [1, potion])
        add('potion-' + str(potion), value, drink=True, hp=85 if potion == 6 else 50)
        if potion == 8:
            add('potion-charm-increments', value, drink=True, charm=2)
    for hair in range(1, 9):
        value = seed((*loot_limits, 5, 11, 8), lambda r: r[1] >= 51 and r[6] <= 25 and r[7:9] == [1, 3] and r[9] == hair)
        add('potion-hair-' + str(hair), value, drink=True)
    add('five-potions', seed((*loot_limits, 5), lambda r: r[1] >= 51 and r[6] <= 25 and r[7] == 5), drink=True)
    pentacle = seed((*loot_limits, 100), lambda r: r[1] >= 51 and r[6] >= 26 and r[7] == 10)
    add('pentacle-decline-10', pentacle)
    add('pentacle-absent-11', seed((*loot_limits, 100), lambda r: r[1] >= 51 and r[6] >= 26 and r[7] == 11))
    for outcome in (1, 2, 3, 5):
        value = seed((*loot_limits, 100, 5), lambda r: r[1] >= 51 and r[6] >= 26 and r[7] <= 10 and r[8] == outcome)
        add('pentacle-' + str(outcome), value, step_in=True)
        if outcome == 1:
            add('pentacle-ring-replaced', value, step_in=True, ring=100)
        if outcome == 5:
            add('pentacle-high-level', value, step_in=True, level=21, staff=True)
        if outcome == 2:
            wind = value
    figure = seed((*loot_limits, 100, 5), lambda r: r[1] >= 51 and r[6] >= 26 and r[7] <= 10 and r[8] == 4)
    add('pentacle-creature-poison', figure, step_in=True, poison=1, use_poison=True, hp=100)
    appearance = seed((8, 100), lambda r: r[1] == 50)
    add('wizard-ring-50', appearance, ring=1, poison=1)
    add('wizard-poison-50', appearance, poison=1, use_poison=True)

    # Choose one-round fights where a 1-HP wizard dies on the player's first hit.
    # A dynamic HP die is modelled separately, because the level draw changes its bound.
    fights = []
    for value in candidates:
        prefix = rolls(value, (16, 8, 100, 10))
        if prefix[2] > 50:
            continue
        level = prefix[3] + 2
        r = rolls(value, (16, 8, 100, 10, level, 10, 9, 9, 100))
        if r[4:7] == [1, 1, 9]:
            fights.append((value, r[7], r[8]))
    def fight(counter, predicate=lambda roll: True):
        return next(value for value, action, roll in fights if action == counter and predicate(roll))
    for counter in range(1, 10):
        add('wizard-action-' + str(counter), fight(counter))
    add('wizard-ugly-saved', fight(2, lambda r: r > 71), looks=True)
    add('wizard-ugly-failed', fight(2, lambda r: r <= 71), looks=True)
    add('wizard-drain-saved', fight(4, lambda r: r > 40))
    add('wizard-drain-failed', fight(4, lambda r: r <= 40))
    add('wizard-plague-saved', fight(6, lambda r: r > 72))
    add('wizard-plague-failed', fight(6, lambda r: r <= 72))
    add('wizard-female-fried', fight(3), male=0)
    add('wizard-no-servants', fight(3), male=0, female=0)
    # A longer fight exercises attack arithmetic, extra draws, and repeated saves.
    full = seed((8, 100, 10), lambda r: r[1] <= 50 and r[2] >= 7)
    add('wizard-full-fight', full, hp=100)
    add('wizard-decline-poison', full, hp=100, poison=1)
    add('potion-charm-to-hut', 1178019, drink=True, follow_hut=True)
    add('wizard-death', fight(9), hp=0.5)
    add('pentacle-wind-death', wind, step_in=True, hp=10)
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
        if 'thicket in the woods' in prompt:
            answer = options.get('enter', True)
        elif prompt == 'Will you enter?' or 'double your posessions' in prompt:
            answer = True
        elif 'potions.  Try them?' in prompt:
            answer = options.get('drink', False)
        elif 'Step in it?' in prompt:
            answer = options.get('step_in', False)
        elif prompt in ('Do you want to use your poison?', 'Do you wish to use your poison?'):
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
    parser.add_argument('--output', type=Path, default=Path('doc/thicket-checkpoints.json'))
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

        for text in ('key space', 'text ThicketProbe', 'key Return', 'key f', 'key space'):
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
            for step in range(120):
                if screen.startswith('*** ThicketProbe ***') and screen.endswith('Any key.'):
                    if options.get('follow_hut') and not case.get('turns'):
                        case['turns'] = 2
                        checkpoint = dict(kind='turn', state=player_state(game))
                        case['steps'].append(checkpoint)
                        print('STATE turn ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                        screen = command('key space')
                        continue
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
                for marker in ('You come up to a thicket in the woods.', 'You come upon a grass hut.'):
                    if marker in screen:
                        event_text = screen[screen.index(marker):]
                        break
                checkpoint['screen'] = event_text
                case['steps'].append(checkpoint)
                print('STATE checkpoint ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                screen = command('key ' + ('space' if answer == ' ' else answer))
            else:
                raise RuntimeError('Expected end of thicket visit')
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
                for text in ('key space', 'text ThicketProbe', 'key Return', 'key f', 'key space'):
                    command(text)


if __name__ == '__main__':
    main()
