#!/usr/bin/env python3
"""Capture controlled slot-6 empty-home outcomes in the original DOS game."""

import json
from pathlib import Path
import struct

from combat_probe import real48, state
from dosbox_game import Game


def seeds():
    found = {}
    for seed in range(100000):
        value = seed
        rolls = []
        for limit in (16, 100, 4):
            value = (value * 129 + 0x361962E9) & 0xFFFFFFFF
            rolls.append((value >> 17) % limit + 1)
        if rolls[0] == 6 and rolls[1] >= 56:
            found.setdefault(rolls[2], seed)
        if len(found) == 4:
            return found
    raise RuntimeError('Could not find empty-home seeds')


def main():
    selected = seeds()
    cases = [
        ('arrows', selected[1], 't', False),
        ('medicine', selected[2], 't', False),
        ('staff', selected[3], 't', False),
        ('staff-held', selected[3], 't', True),
        ('guardian', selected[4], 't', False),
        ('food', selected[1], 's', False),
        ('leave', selected[1], 'l', False),
        ('decline', selected[1], 'n', False),
    ]
    fixtures = []
    print('CONTROLLED PROBES: temporary emulator state and RNG are modified.', flush=True)
    with Game() as game:
        print(game.screen(), flush=True)

        def command(text):
            game.command(text)
            screen = game.screen()
            print(f'\n--- {text} ---\n{screen}', flush=True)
            return screen

        for text in ('key space', 'text TreeProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for name, seed, choice, held in cases:
            config = dict(seed=seed, choice=choice, staff=held)
            print('\nCASE ' + name + ' ' + json.dumps(config), flush=True)
            data = bytearray(game.read_memory('DS', 0x277, 0x49))
            struct.pack_into('<h', data, 0, 8)
            data[2:8] = real48(50)
            data[0x295 - 0x277:0x299 - 0x277] = bytes((0, 0, held, 0))
            for address, value in ((0x29C, 5), (0x29E, 5), (0x2AC, 3),
                                   (0x2AE, 0), (0x2B0, 0), (0x2B2, 0),
                                   (0x2B4, 2), (0x2B6, 0)):
                struct.pack_into('<h', data, address - 0x277, value)
            for address, value in ((0x2A0, 8), (0x2A6, 100), (0x2BA, 0)):
                start = address - 0x277
                data[start:start + 6] = real48(value)
            data[0x2B8 - 0x277:0x2BA - 0x277] = bytes(2)
            game.write_memory('DS', 0x277, data)
            game.write_memory('DS', 0x1FC, seed.to_bytes(4, 'little'))
            command('key space')
            if choice == 'n':
                command('key n')
            else:
                command('key y')
                command('key ' + choice)
            screen = command('key space')
            if not screen.startswith('*** TreeProbe ***') or not screen.endswith('Any key.'):
                raise RuntimeError('Expected next status screen')
            result = state(game)
            result['medicine'] = int.from_bytes(game.read_memory('DS', 0x2AC, 2), 'little')
            result['staff'] = bool(game.read_memory('DS', 0x297, 1)[0])
            print('STATE final ' + json.dumps(result, sort_keys=True), flush=True)
            fixtures.append(dict(name=name, config=config, final=result))
    Path('doc/tree-checkpoints.json').write_text(json.dumps(fixtures, indent=2) + '\n')


if __name__ == '__main__':
    main()
