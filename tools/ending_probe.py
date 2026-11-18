#!/usr/bin/env python3
"""Controlled victory endings and certificate rewards in the original overlay."""

import argparse
import json
from pathlib import Path

from army_probe import roll, stop_at
from dosbox_game import Game
from duel_probe import setup
from final_probe import OVERLAY_BASE, cases, resume, wait_at
from keep_probe import paused_screen, prepare, state


def capture(game, label):
    result = dict(screen=paused_screen(game), state=state(game, True))
    print('CHECK ' + label + ' ' + json.dumps(result, sort_keys=True), flush=True)
    return result


def answer_at(game, cs, answer, offset):
    game.debug_command('BPDEL *')
    game.debug_command(f'BP {cs:04X}:{OVERLAY_BASE + offset:04X}')
    resume(game)
    game.command('key ' + answer)
    wait_at(game)


def seek(game, offset):
    # Reuse the same allocated frame, after its balanced calls have returned.
    game.debug_command(f'SR EIP {OVERLAY_BASE + offset:04X}')


def printer_to_screen(game):
    # Pascal text descriptor flags: output bit 0x40, device 3 = printer,
    # device 1 = console. Preserve the descriptor and all formatting code.
    if game.dump('DS', 0x150, 1) != b'\x43':
        raise RuntimeError('Unexpected Pascal printer descriptor')
    game.write_memory('DS', 0x150, b'\x41', paused=True)
    print('CONTROL: printer descriptor DS:0150 changed 43 -> 41 (console).', flush=True)


def config():
    return dict(cases()[0]['config'], talisman=True, looks=True, seed=77,
                hp=123.25, gold=987, food=654, male=12, female=34,
                medicine=5, poison=6, swords=7, daggers=8, bows=9, arrows=10,
                charm=3, protection=True, horse=False, cart=True)


def enter(game):
    cs = setup(game, config())
    stop_at(game, cs, 0x2975)
    capture(game, 'duel victory before ending')
    stop_at(game, cs, 0x1D94)
    return cs


def endings(output):
    result = []
    with Game() as game:
        cs = enter(game)
        for name, ring, looks, answer in (
                ('ring-one-ugly', 1, False, None),
                ('ring-hundred-good-looks', 100, True, None),
                ('ring-255', 255, False, None),
                ('good-looks-certificate', 0, True, 'y'),
                ('ugly-certificate', 0, False, 'y'),
                ('original-no-printer', 0, True, 'n')):
            case = dict(name=name, config=dict(config(), ring=ring, looks=looks), answer=answer)
            if name == 'ugly-certificate':
                case['config'].update(seed=5393, hp=0, level=21, gold=0, food=0,
                                      male=0, female=0, medicine=0, poison=0,
                                      swords=0, daggers=0, bows=0, arrows=0, charm=0,
                                      staff=False, talisman=False, protection=False,
                                      horse=True, cart=False)
            print('CASE ' + json.dumps(case, sort_keys=True), flush=True)
            prepare(game, case['config'], paused=True)
            game.write_memory('DS', 0x4F8, b'\x01', paused=True)
            game.write_memory('DS', 0x260, b'\x01\x00', paused=True)
            case['before'] = state(game, True)
            seek(game, 0x1D94)
            if ring:
                stop_at(game, cs, 0x2052)
                case['finished'] = capture(game, 'corruption returns without printer or status')
            else:
                stop_at(game, cs, 0x1FEE)
                case['prompt'] = capture(game, 'printer prompt')
                if answer == 'y':
                    answer_at(game, cs, answer, 0x135E)
                    # Reset descriptor because a previous controlled run redirected it.
                    game.write_memory('DS', 0x150, b'\x43', paused=True)
                    printer_to_screen(game)
                    stop_at(game, cs, 0x14C1)
                    case['header'] = capture(game, 'certificate header')
                    stop_at(game, cs, 0xE19)
                    case['stats'] = capture(game, 'certificate stats before reward')
                    stop_at(game, cs, 0x17C4)
                    case['certificate'] = capture(game, 'certificate complete')
                    stop_at(game, cs, 0x1D6C)
                else:
                    answer_at(game, cs, answer, 0x1D6C)
                case['status'] = capture(game, 'final status waiting for key')
                answer_at(game, cs, 'space', 0x2052)
                case['finished'] = capture(game, 'ending returns')
            result.append(case)
            output.write_text(json.dumps(result, indent=2) + '\n')


def rewards(output):
    result = []
    with Game() as game:
        cs = enter(game)
        stop_at(game, cs, 0x1FEE)
        capture(game, 'printer prompt')
        answer_at(game, cs, 'y', 0x135E)
        printer_to_screen(game)
        stop_at(game, cs, 0xE19)
        capture(game, 'certificate before reward loop')
        first = True
        for gender, encoded in (('M', 0), ('F', 1)):
            game.write_memory('DS', 0x260, encoded.to_bytes(2, 'little'), paused=True)
            for reward in range(1, 11):
                seed = next(s for s in range(10000) if roll(s, 10)[1] == reward)
                game.write_memory('DS', 0x1FC, seed.to_bytes(4, 'little'), paused=True)
                case = dict(name=f'{gender}-reward-{reward}', gender=gender, reward=reward, seed=seed,
                            before=state(game, True))
                print('CASE ' + json.dumps(case, sort_keys=True), flush=True)
                if not first:
                    seek(game, 0xE22)
                stop_at(game, cs, 0x1357)
                first = False
                case['finished'] = capture(game, 'reward returned without inventory changes')
                case['text'] = case['finished']['screen'].splitlines()[-1]
                result.append(case)
                output.write_text(json.dumps(result, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('endings', 'rewards'))
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    print('CONTROLLED: real loader, small army win, Talisman duel; player/RNG edited. '
          'Delay calibration 1. Reused allocated frames via debugger SR EIP. '
          'Printer output temporarily routed to console; dos/ unchanged.', flush=True)
    output = args.output or Path(f'doc/ending-{args.mode}-checkpoints.json')
    (endings if args.mode == 'endings' else rewards)(output)


if __name__ == '__main__':
    main()
