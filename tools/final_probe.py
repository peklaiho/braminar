#!/usr/bin/env python3
"""Start FINASSLT.CHN through the real loader, with a controlled player and RNG."""

import argparse
import json
import os
from pathlib import Path
import struct

from combat_probe import read_real48
from dosbox_game import Game
from keep_probe import cases as keep_cases, paused_screen, prepare, state


OVERLAY_BASE = 0x2D7C
ENTRY = OVERLAY_BASE + 0x2985
CONVERTED = OVERLAY_BASE + 0x2C49


def cases():
    baseline = next(c['config'] for c in keep_cases() if c['name'] == 'correct')
    baseline.update(seed=1)
    result = []
    def add(name, **changes):
        result.append(dict(name=name, config=dict(baseline, **changes)))
    add('baseline')
    add('baseline-repeat')
    add('level-21', level=21)
    add('weapons-9', swords=9, bows=9, arrows=9, daggers=9)
    add('weapons-10', swords=10, bows=10, arrows=10, daggers=10)
    add('weapons-19', swords=19, bows=19, arrows=19, daggers=19)
    add('weapons-20', swords=20, bows=20, arrows=20, daggers=20)
    add('servant-prices', male_price=11, female_price=17)
    add('valuation', seed=77, level=19, poison=2, medicine=7, male=3, female=8,
        gold=13, horse=False, cart=False)
    empty = dict(male=0, female=0, medicine=0, swords=0, bows=0, arrows=0,
                 daggers=0, horse=False, cart=False)
    add('below-one-soldier', **empty, gold=3)
    add('one-soldier', **empty, gold=4)
    add('seven-gold', **empty, gold=7)
    add('excluded-assets', food=30000, hp=199, staff=False, talisman=True,
        charm=99, protection=True, looks=True, ring=80, plague=80, experience=4)
    return result


def resume(game):
    game.output.clear()
    os.write(game.master, b'\x1b[15~')


def wait_at(game):
    # Curses may redraw only the changed digits of an address. The prompt
    # reappears when the sole installed breakpoint stops execution.
    game.await_output(b'->')


def start_final_assault(game, config):
    """Initialize and load the assault; return its code segment, debugger paused.

    Call on a fresh Game at its welcome screen. State is restored at the loaded
    module's entry, so Keep bookkeeping cannot change the requested experiment.
    """
    print(game.screen(), flush=True)
    def command(text):
        game.command(text)
        screen = game.screen()
        print('\n--- ' + text + ' ---\n' + screen, flush=True)
        return screen
    for text in ('key space', 'text AssaultProbe', 'key Return', 'key f', 'key space'):
        command(text)
    gate = next(c['config'] for c in keep_cases() if c['name'] == 'correct')
    prepare(game, gate)
    code_segment = int.from_bytes(game.read_memory('DS', 0x18C, 2), 'little') - 0xFA0
    if game.read_memory(code_segment, 0xF450, 3) != bytes.fromhex('bf7902'):
        raise RuntimeError('Unexpected main executable layout')
    print('STATE synthetic Keep setup ' + json.dumps(state(game), sort_keys=True), flush=True)
    for text in ('key space', 'key y', 'text PRIOR', 'key Return'):
        command(text)
    with game.debugger():
        game.debug_command(f'BP {code_segment:04X}:{ENTRY:04X}')
    game.command('key space')
    wait_at(game)
    overlay = Path('dos/FINASSLT.CHN').read_bytes()
    if game.dump(code_segment, ENTRY, 6) != overlay[0x2985:0x298B]:
        raise RuntimeError('Assault overlay was not loaded at the expected address')
    print('\n--- loaded assault entry ---\n' + paused_screen(game), flush=True)
    prepare(game, config, paused=True)
    game.write_memory('DS', 0x4F8, b'\x01', paused=True)
    game.write_memory('DS', 0x3EE, struct.pack('<hh', config.get('male_price', 8),
                                           config.get('female_price', 13)), paused=True)
    game.debug_command('BPDEL *')
    print('STATE assault entry ' + json.dumps(state(game, paused=True), sort_keys=True), flush=True)
    return code_segment


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--output', type=Path, default=Path('doc/final-checkpoints.json'))
    parser.add_argument('--interactive', action='store_true', help='continue playing after conversion')
    parser.add_argument('-c', '--command', action='append', default=[], help='command after conversion')
    args = parser.parse_args()
    available = cases()
    names = args.cases or [c['name'] for c in available]
    if set(names) - {c['name'] for c in available}:
        parser.error('Unknown case')
    if args.interactive and len(names) != 1:
        parser.error('Interactive mode requires one case name')
    fixtures = json.loads(args.output.read_text()) if args.output.exists() else []
    print('CONTROLLED EXPERIMENTS: temporary memory/RNG edits; original dos/ files unchanged.', flush=True)
    for name in names:
        case = next(c.copy() for c in available if c['name'] == name)
        print('\nCASE ' + json.dumps(case, sort_keys=True), flush=True)
        with Game() as game:
            cs = start_final_assault(game, case['config'])
            case['entry'] = state(game, paused=True)
            game.debug_command(f'BP {cs:04X}:{CONVERTED:04X}')
            resume(game)
            wait_at(game)
            case['screen'] = paused_screen(game)
            case['converted'] = state(game, paused=True)
            data = game.dump('DS', 0x4FF, 10)
            case['army'], case['enemy'] = struct.unpack_from('<hh', data)
            case['net_worth'] = read_real48(data[4:])
            print('\n--- conversion complete ---\n' + case['screen'], flush=True)
            print('RESULT ' + json.dumps(case, sort_keys=True), flush=True)
            fixtures[:] = [c for c in fixtures if c['name'] != name] + [case]
            args.output.write_text(json.dumps(fixtures, indent=2) + '\n')
            if args.command or args.interactive:
                game.debug_command('BPDEL *')
                resume(game)
                for command in args.command:
                    game.command(command)
                    print('\n--- ' + command + ' ---\n' + game.screen(), flush=True)
                if args.interactive:
                    while True:
                        try:
                            command = input('dosbox> ')
                        except EOFError:
                            break
                        if command == 'quit':
                            break
                        game.command(command)
                        print('\n--- ' + command + ' ---\n' + game.screen(), flush=True)


if __name__ == '__main__':
    main()
