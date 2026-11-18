#!/usr/bin/env python3
"""Probe the Keep, stopping the original before its final-assault handoff."""

import argparse
import json
from pathlib import Path
import struct

from combat_probe import real48, read_real48
from castle_probe import rolls
from dosbox_game import Game


def cases():
    seed = next(n for n in range(100000) if rolls(n, (16,))[0] == 16)
    result = []
    def add(name, **options):
        config = dict(seed=seed, hp=50, gold=20, food=100, male=5, female=5,
                      medicine=3, poison=0, ring=0, plague=0, looks=False,
                      horse=True, cart=True, staff=True, talisman=False, charm=0,
                      protection=False, level=20, swords=2, bows=3, arrows=4, daggers=1,
                      command='PRIOR', experience=0)
        for key in list(options):
            if key in config:
                config[key] = options.pop(key)
        result.append(dict(name=name, config=config, options=options))
    add('gate-level-19', level=19)
    add('gate-level-promotion', level=19, experience=6)
    add('gate-no-staff', staff=False)
    add('gate-neither', level=19, staff=False)
    add('decline', attack=False)
    add('wrong', text='WRONG')
    add('lowercase', text='prior')
    add('short', text='PRIO')
    add('empty', text='')
    add('leading-space', text=' PRIOR')
    add('correct', text='PRIOR')
    add('correct-above-20', level=21, text='PRIOR')
    add('correct-suffix', text='PRIORextra')
    add('correct-trailing-spaces', text='PRIOR  ')
    add('startup-spaces', command='     ', text='     ')
    add('startup-empty', command='     ', text='')
    add('success-starvation', food=0, text='PRIOR')
    add('success-plague-death', plague=95, text='PRIOR')
    add('success-bookkeeping', ring=95, plague=90, experience=6, text='PRIOR')
    add('turn-two-servants-starvation', level=8, staff=False, male=2, female=0, food=0)
    add('turn-last-servant-starvation', level=8, staff=False, male=1, female=0, food=0)
    for value in range(1000000):
        r = rolls(value, (16,20,3,26,26,26,26,26,16))
        if r[0] == 15 and r[-1] == 16:
            add('pan-to-keep', seed=value, command='     ', pan_answer=str(r[2]),
                text=''.join(chr(64+n) for n in r[3:8]), turns=2)
            break
    else:
        raise RuntimeError('No Pan-to-Keep seed')
    add('staff-loss-reacquisition', seed=117042, text='WRONG', turns=3, staff_recovery=True)
    return result


def state(game, paused=False):
    read = game.dump if paused else game.read_memory
    base = 0x1FC
    data = read('DS', base, 0x1F6)
    result = {'seed': int.from_bytes(data[:4], 'little')}
    for name, address in (('level',0x277),('charm',0x29A),('male',0x29C),('female',0x29E),
                          ('medicine',0x2AC),('poison',0x2AE),('swords',0x2B0),
                          ('bows',0x2B2),('arrows',0x2B4),('daggers',0x2B6)):
        result[name] = struct.unpack_from('<h', data, address-base)[0]
    for name, address in (('hp',0x279),('gold',0x2A0),('food',0x2A6),('experience',0x2BA)):
        result[name] = round(read_real48(data[address-base:address-base+6]),8)
    for name, address in (('staff',0x297),('talisman',0x298),('looks',0x294),('ring',0x295),
                          ('plague',0x296),('protection',0x299),('horse',0x2B8),('cart',0x2B9)):
        result[name] = data[address-base]
    length = data[0x27F-base]
    result['hair'] = data[0x280-base:0x280-base+length].decode('ascii')
    flags = read('DS',0x4F8,7)
    result['ready'] = flags[0]
    result['command'] = flags[2:2+flags[1]].decode('ascii')
    return result


def paused_screen(game):
    bios = game.dump(0x40,0,0x100)
    assert bios[0x49] == 3 and int.from_bytes(bios[0x4A:0x4C],'little') == 80
    offset = int.from_bytes(bios[0x4E:0x50],'little')
    data = game.dump(0xB800,offset,4000)
    return '\n'.join(data[n:n+160:2].replace(b'\0',b' ').decode('cp437').rstrip()
                     for n in range(0,4000,160)).strip('\n')


def prepare(game, config, paused=False):
    read = game.dump if paused else game.read_memory
    data = bytearray(read('DS',0x277,0x49))
    struct.pack_into('<h',data,0,config['level'])
    data[2:8] = real48(config['hp'])
    data[8:14] = b'\x05Black'
    for address, name in ((0x294,'looks'),(0x295,'ring'),(0x296,'plague'),(0x297,'staff'),
                           (0x298,'talisman'),(0x299,'protection'),(0x2B8,'horse'),(0x2B9,'cart')):
        data[address-0x277] = config[name]
    for address, name in ((0x29A,'charm'),(0x29C,'male'),(0x29E,'female'),(0x2AC,'medicine'),
                           (0x2AE,'poison'),(0x2B0,'swords'),(0x2B2,'bows'),(0x2B4,'arrows'),(0x2B6,'daggers')):
        struct.pack_into('<h',data,address-0x277,config[name])
    for address,name in ((0x2A0,'gold'),(0x2A6,'food'),(0x2BA,'experience')):
        data[address-0x277:address-0x277+6] = real48(config[name])
    game.write_memory('DS',0x277,data,paused=paused)
    game.write_memory('DS',0x3EE,struct.pack('<hh',8,13),paused=paused)
    game.write_memory('DS',0x1FC,config['seed'].to_bytes(4,'little'),paused=paused)
    game.write_memory('DS',0x4F8,b'\0\x05'+config['command'].encode('ascii'),paused=paused)


def event_text(screen, pan=False):
    marker = 'You find a Statue of Pan.' if pan else "You find the Evil Overlord's Keep."
    if 'You come upon a flowing river.' in screen:
        marker = 'You come upon a flowing river.'
    return screen[screen.index(marker):] if marker in screen else screen


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases',nargs='*')
    parser.add_argument('--output',type=Path,default=Path('doc/keep-checkpoints.json'))
    args = parser.parse_args()
    available = cases()
    selected = ([c for name in args.cases for c in available if c['name']==name]
                if args.cases else available)
    if not selected or set(args.cases)-{c['name'] for c in selected}:
        parser.error('Unknown case')
    fixtures = json.loads(args.output.read_text()) if args.cases and args.output.exists() else []
    print('CONTROLLED PROBES: temporary player memory, prices 8/13, command and RNG edited.',flush=True)
    print('A breakpoint stops each case after bookkeeping, before any final-assault code executes.',flush=True)
    for case in selected:
        print('\nCASE '+case['name']+' '+json.dumps(case,sort_keys=True),flush=True)
        with Game() as game:
            print(game.screen(),flush=True)
            def command(text):
                game.command(text)
                screen = game.screen()
                print('\n--- '+text+' ---\n'+screen,flush=True)
                return screen
            for text in ('key space','text KeepProbe','key Return','key f','key space'):
                command(text)
            prepare(game,case['config'])
            print('STATE initial '+json.dumps(state(game),sort_keys=True),flush=True)
            screen = command('key space')
            case['steps'] = []
            turns = case['options'].get('turns',1)
            completed = 0
            pan = turns==2
            staff_recovery = case['options'].get('staff_recovery',False)
            for step in range(20):
                if screen.startswith('*** KeepProbe ***') and screen.endswith('Any key.'):
                    assert pan or staff_recovery
                    completed += 1
                    checkpoint = dict(kind='turn',state=state(game))
                    case['steps'].append(checkpoint)
                    print('STATE turn '+json.dumps(checkpoint,sort_keys=True),flush=True)
                    pan = False
                    screen = command('key space')
                    continue
                lines = [line.strip() for line in screen.splitlines() if line.strip()]
                if lines[-1] == '(Y/N)':
                    answer = 'y' if pan or case['options'].get('attack',True) else 'n'
                    if staff_recovery and completed==2:
                        answer = 'n'
                    kind,prompt = 'key',lines[-2]+'\n(Y/N)'
                elif lines[-1].endswith('Prime Command (in caps):'):
                    answer = case['options']['text']
                    kind,prompt = 'line',lines[-1]
                elif pan and lines[-1] == 'Choose (1,2,3):':
                    answer = case['options']['pan_answer']
                    kind,prompt = 'key','Choose (1,2,3):'
                elif lines[-1] == 'Press any key...':
                    answer,kind,prompt = ' ','key',lines[-1]
                else:
                    raise RuntimeError('Unexpected prompt '+repr(lines[-1]))
                checkpoint = dict(kind=kind,answer=answer,prompt=prompt,
                                  screen=event_text(screen,pan),state=state(game))
                case['steps'].append(checkpoint)
                print('STATE checkpoint '+json.dumps(checkpoint,sort_keys=True),flush=True)
                if kind=='key' and answer==' ' and completed+1==turns:
                    stack_base = int.from_bytes(game.read_memory('DS',0x18C,2),'little')
                    code_segment = stack_base - 0xFA0
                    assert game.read_memory(code_segment,0xF450,3)==bytes.fromhex('bf7902')
                    with game.debugger():
                        game.debug_command(f'BP {code_segment:04X}:F450')
                    print(f'--- breakpoint {code_segment:04X}:F450 (file 0xF350) ---',flush=True)
                    game.command('key space')
                    game.settle(1)
                    case['final'] = state(game,paused=True)
                    case['final_screen'] = event_text(paused_screen(game))
                    print(case['final_screen'],flush=True)
                    print('STATE final '+json.dumps(case['final'],sort_keys=True),flush=True)
                    break
                if kind=='line':
                    if answer:
                        command('text '+answer)
                    screen = command('key Return')
                else:
                    screen = command('key '+('space' if answer==' ' else answer))
            else:
                raise RuntimeError('No Keep exit pause')
        fixtures[:] = [old for old in fixtures if old['name']!=case['name']]
        fixtures.append(case)
        args.output.write_text(json.dumps(fixtures,indent=2)+'\n')


if __name__=='__main__':
    main()
