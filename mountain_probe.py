#!/usr/bin/env python3
"""Capture controlled mountain trail outcomes in the original DOS game."""

import json
from pathlib import Path
import struct

from combat_probe import real48, state
from dosbox_game import Game


def main():
    seed = next(seed for seed in range(1000000)
                if (((seed * 129 + 0x361962E9) & 0xFFFFFFFF) >> 17) % 16 == 7)
    cases = [
        ('decline', 'n', True, 5),
        ('refuse', 'yn', True, 5),
        ('accept-staff', 'yy', True, 5),
        ('accept-no-staff', 'yy', False, 1),
        ('accept-strong-ring', 'xYzY', True, 100),
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

        for text in ('key space', 'text TrailProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for name, keys, staff, ring in cases:
            config = dict(seed=seed, keys=keys, staff=staff, ring=ring)
            print('\nCASE ' + name + ' ' + json.dumps(config), flush=True)
            data = bytearray(game.read_memory('DS', 0x277, 0x49))
            struct.pack_into('<h', data, 0, 8)
            data[2:8] = real48(50)
            data[0x295 - 0x277:0x299 - 0x277] = bytes([ring, 0, staff, 1])
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
            game.write_memory('DS', 0x4F9, b'\x05PRIOR')
            print('STATE initial ' + json.dumps(state(game), sort_keys=True), flush=True)
            screens = [command('key space')]
            for key in keys:
                screens.append(command('key ' + key))
            screen = command('key space')
            if not screen.startswith('*** TrailProbe ***') or not screen.endswith('Any key.'):
                raise RuntimeError('Expected next status screen')
            result = state(game)
            result['medicine'] = int.from_bytes(game.read_memory('DS', 0x2AC, 2), 'little')
            result['staff'] = game.read_memory('DS', 0x297, 1)[0]
            result['talisman'] = game.read_memory('DS', 0x298, 1)[0]
            print('STATE final ' + json.dumps(result, sort_keys=True), flush=True)
            event_screen = next(s for s in reversed(screens) if 'You come upon a mountain trail.' in s)
            event_screen = event_screen[event_screen.rindex('You come upon a mountain trail.'):]
            fixtures.append(dict(name=name, config=config, final=result, screen=event_screen))
    Path('doc/mountain-checkpoints.json').write_text(json.dumps(fixtures, indent=2) + '\n')


if __name__ == '__main__':
    main()
