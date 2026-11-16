# Dark Castle and Mad King research

## Evidence and scope

The slot-11 castle at `dos/BRAMINAR.COM` file offsets `0x7E15`–`0x8BA6` and separate Mad King combat at `0x5DBE`–`0x6768` are recovered and implemented, with 46 retained controlled comparisons. The three rooms are independent choices; there is no room-selection loop after completing a room. Binary control flow establishes the exact random-call order and thresholds in [mechanics](mechanics.md#dark-castle).

This session is targeted reconstruction with controlled experiments, not ordinary play. `tools/castle_probe.py` uses the existing `dosbox_game.Game` runner, resets temporary player memory and RNG, and fixes servant prices to 8/13. It never edits `dos/`. Every case records its configuration, inputs, screens, and memory at input boundaries. Initial displayed status predates the edits; `STATE initial` describes the actual setup.

The baseline is level 8, 50 HP, Black hair, bad looks, five servants of each sex, 20 gold, 100 food, three medicine, two swords, three bows, four arrows, one dagger, horse/cart, and plague 10. Quest items, ring, charm, poison, experience, and protection start absent. Fight cases use higher HP or explicitly test death; individual configurations record all exceptions.

## Captures and reproduction

Run `./tools/castle_probe.py > doc/captures/2026-11-15-castle-probes.txt` to reproduce the cases and write [checkpoints](castle-checkpoints.json). Named arguments select cases; `--output PATH` permits independent batches. The fixtures combine the [primary combat batch](captures/2026-11-15-castle-probes.txt), [room batch](captures/2026-11-15-castle-rooms.txt), [theft boundaries](captures/2026-11-15-castle-boundaries.txt), and [settled drop probes](captures/2026-11-15-castle-drops.txt). The primary batch was stopped after the last combat case to avoid repeating the completed room batch. [Initial tooling probes](captures/2026-11-15-castle-initial.txt) also retain an unsuccessful attempt to send punctuation using xdotool key names: its one-bag sequence did not actually reach DOS. The primary capture supersedes that case and uses literal text for punctuation. The first error-progress capture sampled memory during the drop sound, before the noise RNG draw completed; the settled drop probes supersede it and wait through that sound before recording the next input boundary.

All 46 cases pass `./tools/check_castle.py --sanitize`, which compares state, RNG, text/prompt order, input counts, and requested delays at every recorded input. Fatal cases compare the state at the final event pause before bookkeeping: DOS reclaims DS after game exit. Nonfatal cases include ordinary turn bookkeeping. EOF checks cover six distinct prompts. The fixtures suffice without DOSBox or ignored raw transcripts. `make`, Python syntax checks, and whitespace checks pass; all nine existing sanitizer suites also pass. The existing unused-final-battle-stub compiler warning remains.

## Findings and comparison scope

- Declining entry immediately continues; accepting can encounter ghost minstrels, flee in fear, or reach the three-room menu.
- Dungeon probes cover gold, smell damage, Staff acquisition and an already held Staff, male-servant recruitment, poison, locked-door escape with servants or a dagger, and starvation without either. The door tests run after recruitment, so newly found servants can help.
- Galley probes cover all seven hints and seeing nothing, declining the ball, Talisman acquisition/preservation, food from zero through 199, and pits halving each sex separately. Hints do not depend on held quest items or any placement state.
- Throne-room probes cover complete bags, lowercase H, repeated bags, wrong keys, ignored keys, leaving, and the noise threshold. Dropping gold does not subtract inventory gold or reset sequence progress. Noise depends on inputs, not elapsed time.
- King probes cover poison acceptance/refusal, ordinary combat, ring attacks, bows, unarmed fighting, death, rewards, and the final room pause. His weapon RNG draw is `Random(1)`: it changes RNG state while adding zero damage. Bows do not consume arrows here. The King retaliates even after a killing blow.

## Intentional difference and limits

The DOS castle's fear local at `[BP-5]` is assigned only when minstrels appear, then read even when none appeared. The remake enters the menu when there are no ghosts instead of reading an uninitialized local. This deterministic choice is recorded in [CHANGES.md](../CHANGES.md). It does not add a new roll.

Binary evidence establishes all damage-table entries, six shouts, and precise thresholds. The captures sample combat and do not independently exercise every attack/counter pairing, the sixth shout (five were observed), simultaneous deaths, or extreme signed-integer overflow. Delays are requested durations recovered from the binary; debugger pauses prevent measurements of original wall-clock timing. Speaker sounds are outside the remake's existing terminal interface; their waits remain represented. The final battle remains deferred.

## G8 death-ordering follow-up

The [G8 comparisons](combat-order-research.md) close this ledger's earlier simultaneous-death validation gap. Both participants die, independent outcome messages print, and survivor rewards are withheld; no gameplay correction was needed. All 47 castle scenarios pass with sanitizers. Exact RNG equality is no longer asserted by this checker; seeds remain for controlled selection. Earlier session counts and limits above describe their original scope. Final comparisons stop at the event pause before death bookkeeping.
