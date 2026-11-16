#!/usr/bin/env python3
"""Capture controlled Statue of Pan outcomes in the original DOS game."""

import json
from pathlib import Path
import struct

from combat_probe import real48, state
from dosbox_game import Game


def main():
    selected = {}
    for seed in range(1000000):
        value = seed
        rolls = []
        for limit in (16, 20, 3):
            value = (value * 129 + 0x361962E9) & 0xFFFFFFFF
            rolls.append((value >> 17) % limit)
        if rolls[0] == 14:
            selected.setdefault(rolls[1], (seed, rolls[2] + 1))
        if len(selected) == 20:
            break
    if len(selected) != 20:
        raise RuntimeError('Could not find seeds for all Pan questions')
    seed, answer = selected[0]
    cases = [
        ('success', seed, 'y', 'x0' + str(answer)),
        ('failure', seed, 'y', str(answer % 3 + 1)),
        ('decline', seed, 'n', ''),
        ('malformed', selected[2][0], 'y', str(selected[2][1])),
        ('last-question', selected[19][0], 'y', str(selected[19][1])),
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

        for text in ('key space', 'text PanProbe', 'key Return', 'key f', 'key space'):
            command(text)
        for name, seed, choice, answer in cases:
            config = dict(seed=seed, choice=choice, answer=answer, command='PRIOR')
            print('\nCASE ' + name + ' ' + json.dumps(config), flush=True)
            data = bytearray(game.read_memory('DS', 0x277, 0x49))
            struct.pack_into('<h', data, 0, 8)
            data[2:8] = real48(50)
            data[0x295 - 0x277:0x299 - 0x277] = bytes(4)
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
            screens.append(command('key ' + choice))
            if choice != 'n':
                for key in answer:
                    screens.append(command('key ' + key))
                screen = command('key space')
            else:
                screen = screens[-1]
            if not screen.startswith('*** PanProbe ***') or not screen.endswith('Any key.'):
                raise RuntimeError('Expected next status screen')
            result = state(game)
            result['medicine'] = int.from_bytes(game.read_memory('DS', 0x2AC, 2), 'little')
            result['command'] = game.read_memory('DS', 0x4FA, 5).decode('ascii')
            print('STATE final ' + json.dumps(result, sort_keys=True), flush=True)
            event_screen = next(s for s in reversed(screens) if 'You find a Statue of Pan.' in s)
            event_screen = event_screen[event_screen.rindex('You find a Statue of Pan.'):]
            fixtures.append(dict(name=name, config=config, final=result, screen=event_screen))
    Path('doc/pan-checkpoints.json').write_text(json.dumps(fixtures, indent=2) + '\n')


if __name__ == '__main__':
    main()
