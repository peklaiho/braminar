#!/usr/bin/env python3
"""Capture a controlled original-game band encounter using a chosen state and seed."""

import argparse
import json
import math
import struct

from dosbox_game import Game


def real48(value):
    if value == 0:
        return bytes(6)
    fraction, exponent = math.frexp(abs(value))
    mantissa = round((fraction * 2 - 1) * (1 << 39))
    if mantissa == 1 << 39:
        mantissa = 0
        exponent += 1
    if not 0 < exponent + 128 < 256:
        raise ValueError('Value is outside the normal Real48 range')
    mantissa |= (value < 0) << 39
    return bytes([exponent + 128]) + mantissa.to_bytes(5, 'little')


def read_real48(data):
    if data[0] == 0:
        return 0.0
    mantissa = int.from_bytes(data[1:], 'little')
    value = (1 + (mantissa & ((1 << 39) - 1)) / (1 << 39)) * 2.0 ** (data[0] - 129)
    return -value if mantissa >> 39 else value


def state(game):
    data = game.read_memory('DS', 0x277, 0x49)
    result = {}
    for name, address in (('level', 0x277), ('male', 0x29C), ('female', 0x29E),
                          ('poison', 0x2AE), ('swords', 0x2B0), ('bows', 0x2B2),
                          ('arrows', 0x2B4), ('daggers', 0x2B6)):
        result[name] = struct.unpack_from('<h', data, address - 0x277)[0]
    for name, address in (('hp', 0x279), ('gold', 0x2A0), ('food', 0x2A6), ('experience', 0x2BA)):
        start = address - 0x277
        result[name] = round(read_real48(data[start:start + 6]), 8)
    result['ring'] = data[0x295 - 0x277]
    result['seed'] = int.from_bytes(game.read_memory('DS', 0x1FC, 4), 'little')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=lambda text: int(text, 0), required=True)
    parser.add_argument('--choice', choices=('f', 'g', 'r'), default='f')
    parser.add_argument('--hp', type=float, default=50)
    parser.add_argument('--level', type=int, default=8)
    for name, default in (('male', 12), ('female', 12), ('gold', 100), ('food', 100),
                          ('swords', 0), ('bows', 0), ('arrows', 0), ('daggers', 0),
                          ('poison', 0), ('ring', 0)):
        parser.add_argument('--' + name, type=int, default=default)
    parser.add_argument('--horse', action='store_true')
    parser.add_argument('--cart', action='store_true')
    parser.add_argument('--use-poison', action='store_true')
    args = parser.parse_args()
    if not 0 <= args.seed <= 0xFFFFFFFF:
        parser.error('seed must fit in 32 bits')
    next_seed = (args.seed * 129 + 0x361962E9) & 0xFFFFFFFF
    if (next_seed >> 17) % 16 != 3:
        parser.error('seed must select main-loop slot 4 (a band encounter)')
    if not math.isfinite(args.hp) or args.hp <= 0 or not 1 <= args.level <= 100:
        parser.error('use positive finite HP and a level between 1 and 100')
    for name in ('male', 'female', 'gold', 'food', 'swords', 'bows', 'arrows', 'daggers', 'poison'):
        if not 0 <= getattr(args, name) <= 10000:
            parser.error(name + ' must be between 0 and 10000')
    if not 0 <= args.ring <= 255:
        parser.error('ring must fit in one byte')

    print('CONTROLLED PROBE: emulator state and RNG seed are modified; originals are untouched.', flush=True)
    print('CONFIG ' + json.dumps(vars(args), sort_keys=True), flush=True)
    with Game() as game:
        print(game.screen(), flush=True)

        def command(text):
            game.command(text)
            screen = game.screen()
            print(f'\n--- {text} ---\n{screen}', flush=True)
            return screen

        for text in ('key space', 'text CombatProbe', 'key Return', 'key m', 'key space'):
            command(text)
        # Patch only the temporary emulated player block; preserve unrelated fields.
        data = bytearray(game.read_memory('DS', 0x277, 0x49))
        for name, address in (('level', 0x277), ('male', 0x29C), ('female', 0x29E),
                              ('poison', 0x2AE), ('swords', 0x2B0), ('bows', 0x2B2),
                              ('arrows', 0x2B4), ('daggers', 0x2B6)):
            struct.pack_into('<h', data, address - 0x277, getattr(args, name))
        for name, address in (('hp', 0x279), ('gold', 0x2A0), ('food', 0x2A6)):
            start = address - 0x277
            data[start:start + 6] = real48(getattr(args, name))
        data[0x2BA - 0x277:0x2C0 - 0x277] = bytes(6)
        data[0x295 - 0x277] = args.ring
        data[0x296 - 0x277] = 0
        data[0x2B8 - 0x277] = args.horse
        data[0x2B9 - 0x277] = args.cart
        game.write_memory('DS', 0x277, data)
        game.write_memory('DS', 0x1FC, args.seed.to_bytes(4, 'little'))
        print('STATE initial ' + json.dumps(state(game), sort_keys=True), flush=True)
        screen = command('key space')
        for step in range(250):
            if 'Game Over.' in screen:
                return
            print('STATE checkpoint ' + json.dumps(state(game), sort_keys=True), flush=True)
            if 'Any key.' in screen and screen.startswith('*** CombatProbe ***'):
                return
            if screen.rstrip().endswith('Do you wish to (F)ight (G)ive, or (R)un?'):
                screen = command('key ' + args.choice)
            elif screen.rstrip().endswith('(Y/N)'):
                screen = command('key ' + ('y' if args.use_poison else 'n'))
            elif screen.rstrip().endswith('Press any key...'):
                screen = command('key space')
            else:
                screen = command('wait 2')
        raise RuntimeError('Probe exceeded 250 steps; inspect its captured screens')


if __name__ == '__main__':
    main()
