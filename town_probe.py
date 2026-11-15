#!/usr/bin/env python3
"""Capture controlled City/Village cases with the shared DOSBox runner."""

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
    village = find_seed((16, 100), lambda r: r[0] == 1 and r[1] >= 8)
    city = find_seed((16, 100, 100), lambda r: r[0] == 2 and r[1] >= 18 and r[2] <= 88)

    def add(name, seed=village, **options):
        config = dict(seed=seed, hp=50, gold=500, food=100, male=5, female=5,
                      medicine=3, ring=0, plague=0, looks=True, horse=False, cart=False)
        for key in list(options):
            if key in config:
                config[key] = options.pop(key)
        result.append(dict(name=name, config=config, options=options))

    add('village-decline', enter=False)
    add('city-decline', city, enter=False)
    add('village-skip')
    add('city-skip', city)
    add('market-sell-clamped', market=True, sell=True, quantities=['9', '9'])
    add('market-buy-sequential', market=True, buy=True, gold=21, quantities=['1', '1'])
    add('market-buy-unaffordable', market=True, buy=True, gold=13, quantities=['2', '1'])
    add('store-all', store=True, shop=True, quantities=['10', '3'])
    price = rolls(village, (16, 100, 100))[2]
    add('store-exact-gold', store=True, sword=True, gold=price, quantities=['0', '0'])
    add('store-insufficient', store=True, shop=True, gold=0, quantities=['0', '0'])
    add('store-held-transport', store=True, horse=True, cart=True, quantities=['501', '2', '167', '1'])
    open_market = find_seed((16, 100, 100), lambda r: r[0] == 2 and r[1] < 18 and r[2] <= 88)
    add('marketplace-purchase', open_market, marketplace=True, quantities=['501', '2', '5', '1'])
    add('marketplace-decline', open_market)
    cleric = find_seed((16, 100, 100), lambda r: r[0] == 2 and r[1] >= 18 and r[2] > 88)
    add('cleric-decline', cleric)
    add('cleric-cure', cleric, cleric=True, plague=10, looks=False)
    warning_cleric = find_seed((16, 100, 100, 100), lambda r: r[0] == 2 and r[2] >= 18 and r[3] > 88)
    add('cleric-ring-overcap', warning_cleric, cleric=True, ring=31, hp=90, plague=10, looks=False)
    mugger = find_seed((16, 100, 100, 100), lambda r: r[0] == 2 and r[1] >= 18 and r[2] <= 88 and r[3] < 11)
    add('muggers-loss', mugger, male=0, female=0, gold=3, food=1)
    ring_mugger = find_seed((16, 100, 100, 100, 100), lambda r: r[0] == 2 and r[2] >= 18 and r[3] <= 88 and r[4] < 11)
    add('muggers-ring', ring_mugger, male=0, female=0, ring=31)
    for name, detected in (('pickpocket-noticed', True), ('pickpocket-theft', False)):
        seed = find_seed((16, 100, 100), lambda r: r[0] == 1 and r[1] < 8 and (r[2] < 33) == detected)
        add(name, seed)
    add('pickpocket-empty', result[-1]['config']['seed'], gold=0)
    add('village-ring-warning', ring=11)
    for room in (1, 2, 3):
        add('inn-room-' + str(room), inn=str(room), hp=85 if room == 3 else 50)
    add('inn-insufficient', inn='3', gold=19)
    rats = find_seed((16, 100, 5, 100), lambda r: r[0] == 1 and r[1] >= 8 and r[3] < 15)
    add('inn-rats', rats, inn='1', male=0, female=1, food=1, gold=5)
    for name in ('good', 'plain', 'saved', 'cured', 'fatal'):
        def wanted(r):
            if r[0] != 1 or r[1] < 8:
                return False
            if name == 'good':
                return r[3] < 30
            if r[3] < 30:
                return False
            if name == 'plain':
                return r[4] >= 15
            return r[4] < 15 and ((r[5] < 28) == (name == 'saved'))
        seed = find_seed((16, 100, 5, 100, 100, 100), wanted)
        add('drink-' + name, seed, tavern=True, drink=True,
            medicine=0 if name == 'fatal' else 3, hp=79 if name == 'good' else 50)
    for girl in (True, False):
        seed = find_seed((16, 100, 100, 5, 5, 100),
                         lambda r: r[0] == 1 and r[1] >= 8 and
                         (1 <= r[2] <= 10 if girl else 11 <= r[2] <= 20) and r[5] < 20)
        add('social-' + ('girl' if girl else 'guy'), seed, tavern=True, socialize=True, move=True)
        if girl:
            add('social-ugly', seed, tavern=True, socialize=True, move=True, looks=False)
            add('social-decline', seed, tavern=True, socialize=True)
    for name in ('win', 'tie', 'loss'):
        seed = find_seed((16, 100, 50, 50), lambda r: r[0] == 1 and r[1] >= 8 and
                         (r[2] > r[3] if name == 'win' else r[2] == r[3] if name == 'tie' else r[2] < r[3]))
        add('darts-' + name, seed, tavern=True, darts=True, gold=5)
        if name == 'loss':
            add('darts-ring', seed, tavern=True, darts=True, ring=61, gold=5)
    return [c for c in result if c['name'] not in ('drink-fatal', 'pickpocket-empty')] + [c for c in result if c['name'] in ('drink-fatal', 'pickpocket-empty')]


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


def response(screen, options, quantities):
    lines = [line.strip() for line in screen.splitlines() if line.strip()]
    last = lines[-1]
    if last == '(Y/N)':
        prompt = lines[-2]
        if 'Do you want to enter?' in prompt:
            answer = options.get('enter', True)
        else:
            mapping = {'Do you want to go to the slave Market?': 'market',
                       'Sell Slaves?': 'sell', 'Buy Slaves?': 'buy',
                       'Do you wish to enter the Tavern?': 'tavern',
                       'Do you wish to buy a drink? (2gp)': 'drink',
                       'Do you wish to socialize?': 'socialize', 'Will you Move in?': 'move',
                       'Ante is 5 GP': 'darts', 'Do you wish to go to the store?': 'store',
                       'It seems that the Market Place is open.  Enter?': 'marketplace',
                       'Do you want to recouperate at the Inn?': 'inn',
                       'donate all your gold to my church.  Donate?': 'cleric'}
            if prompt in ('Buy one?', 'Buy 12?'):
                item = next(line for line in reversed(lines) if ' Costs ' in line).split()[0].lower()
                answer = options.get('shop', False) or options.get(item, False)
            else:
                answer = options.get(mapping[prompt], False)
        return 'key', 'y' if answer else 'n', prompt + '\n(Y/N)'
    if last == 'Choose bedding:':
        return 'key', options['inn'], last
    if last in ('Males:', 'Females:', '?', 'How much to buy? (0 for none)'):
        if not quantities:
            raise RuntimeError('Missing quantity for ' + last)
        return 'line', quantities.pop(0), last
    if last.isdigit() and quantities:
        return 'line', quantities.pop(0), last
    if last == 'Press any key...':
        return 'key', ' ', last
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/town-checkpoints.json'))
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

        for text in ('key space', 'text TownProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for case in selected:
            config = case['config']
            options = case['options']
            quantities = list(options.get('quantities', []))
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
            for step in range(60):
                if screen.startswith('*** TownProbe ***') and screen.endswith('Any key.'):
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
                reply = response(screen, options, quantities)
                if reply is None:
                    screen = command('wait 1')
                    continue
                kind, answer, prompt = reply
                checkpoint = dict(kind=kind, answer=answer, prompt=prompt, state=player_state(game))
                case['steps'].append(checkpoint)
                print('STATE checkpoint ' + json.dumps(checkpoint, sort_keys=True), flush=True)
                if kind == 'line':
                    if answer:
                        command('text ' + answer)
                    screen = command('key Return')
                else:
                    previous = screen
                    screen = command('key ' + ('space' if answer == ' ' else answer))
                    if screen == previous or (prompt == 'Ante is 5 GP\n(Y/N)' and answer == 'y'):
                        screen = command('wait 1')
            else:
                raise RuntimeError('Expected end of town visit')
            if quantities:
                raise RuntimeError('Unused quantities')
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
                for text in ('key space', 'text TownProbe', 'key Return', 'key f', 'key space'):
                    command(text)


if __name__ == '__main__':
    main()
