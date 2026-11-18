# Final assault: first reconstruction session

Follow-up: [automatic army combat](army-research.md) is now implemented and compared, including all ten special events and zero-army aborts. The sections below retain the initial opening session's evidence and scope; the [Overlord duel](duel-research.md) is also implemented and compared. Current outstanding work is victory rewards and endings.

## Scope and evidence

2026-11-18: the user opened `FINASSLT.CHN` for reconstruction. The bounded implemented target is its opening asset conversion and army generation, reached through the real Keep handoff. Army combat, the Overlord duel, and endings remain explicit placeholders. Original files in `dos/` are unchanged.

All runs in this session are controlled experiments, not ordinary play. The probe edits player memory, servant prices, prime command, assault flag and RNG in a temporary emulator. A synthetic successful Keep visit loads the original assault module, then the requested experiment state is restored at its entry. This removes both adventure play and Keep bookkeeping from subsequent measurements.

Retained evidence:

- [Initial exploration](captures/2026-11-18-final-explore.txt): forced successful Keep, loaded-module entry, conversion, then an automatically progressing army display. This earlier experiment retained post-Keep state and seed rather than resetting them at assault entry: 882 gold of net worth, 126 soldiers against 74, followed by an army display with 109 player soldiers and zero enemies. It ended before the duel. Raw debugger output is included.
- [Baseline](captures/2026-11-18-final-baseline.txt): first completed repeatable entry using seed one, with exact state restored after the loader.
- [Conversion batch](captures/2026-11-18-final-conversion.txt): independent fresh launches covering repeatability, level bonus, weapon quantities, servant prices and excluded assets.
- [Boundary batch](captures/2026-11-18-final-boundaries.txt): zero/one soldier rounding boundary, seven gold, and mixed asset valuation below level twenty. This supersedes the earlier conversion batch's exploratory 6.9/7-gold boundary setups.
- [Flow discovery](captures/2026-11-18-final-flow.txt): separate seed-one continuation beyond conversion. After eight seconds, the army display shows zero player soldiers and 128 enemy survivors; after another eight seconds, `Your army perished.  You are captured by the Overlord.` and the DOS prompt appear. These timed screens establish defeat/exit flow, not a complete combat trace.
- [Compact checkpoints](final-checkpoints.json): entry and converted memory, RNG, exact conversion screen, net worth and both army sizes. These suffice for comparison without the ignored raw captures.

## Reliable entry and memory map

`tools/final_probe.py` exports `start_final_assault(game, config)` for use with a fresh `Game`. It automates only character creation and one forced successful Keep, using the established Keep setup and five-character command `PRIOR`. No adventure progression or on-disk patch is needed. At the initial status screen derive the program code segment as `word(DS:018C) - 0xFA0`; verify the `BF 79 02` instruction at code segment:`F450` before proceeding. Do not derive it from the debugger's transient CS while stopped inside DOS or BIOS input.

The chain loader at `BRAMINAR.COM` file `0x2C4D`–`0x2C6E` reads the module into code segment:`2D7C`. Consequently an assault file offset maps to runtime IP `0x2D7C + offset`. The entry jump at assault file `0x20` targets file `0x2985`, runtime `5701`. Install an absolute breakpoint there before dismissing the successful Keep's final pause. The real chain initialization runs first; the tool verifies the loaded entry bytes against `dos/FINASSLT.CHN`.

The debugger's return prompt signals that the sole installed breakpoint stopped execution. Curses sometimes redraws only changed address digits, so waiting for the complete textual CS:IP is unreliable. An early attempt using that text timed out and was superseded. Memory reads and writes at a breakpoint use `dump()` and `write_memory(..., paused=True)` without toggling another debugger context. Every write is verified by readback. Clear the entry breakpoint, restore the desired state and RNG, set the assault flag, and resume. This keeps the module's initialized stack, segments and runtime state; directly launching the `.CHN` or jumping out of a pending input call has not been used or established as valid.

The existing player offsets remain usable through the loaded module's current DS. Relevant additional addresses:

| DS offset | Meaning |
| --- | --- |
| `01FC` | 32-bit RNG state |
| `03EE`, `03F0` | Signed 16-bit male/female servant prices |
| `04F8` | Main-game assault flag; restored to one at loaded entry |
| `04FF` | Signed 16-bit player soldiers |
| `0501` | Signed 16-bit enemy soldiers |
| `0503` | Pascal Real48 net worth |

A second breakpoint at runtime `59C5`, assault file `0x2C49`, stops after conversion text and its two-second delay, before the army routine starts. Reaching it does not consume any army-combat draws. The zero-soldier case also stops here; its later behavior is unresolved.

## Recovered opening rules

See [mechanics](mechanics.md#final-assault-asset-conversion) for the complete implemented formula and random-call order. Conversion does not sell, clear or otherwise change player inventory: both original checkpoints and the remake preserve player state. The resulting army is separate state. The original opening uses one second of delay before valuation and two seconds after its report, without asking for input.

Seed one produces category values 25, 75, 14 and 73 for swords, bows, arrows and daggers. With the baseline two swords, three bows, four arrows and one dagger, those contribute 404 gold. Other assets contribute 434 gold, giving 838 net worth and 120 soldiers. The fifth draw gives 100; with half the player army added, the enemy has 160. Independent fresh launches produce identical entry/conversion checkpoints despite randomized character creation.

The runtime call preceded by `MOV CX,10` near the weapon valuations is a stack-space check, not division by ten. The count pushed before it is multiplied directly by a category roll. The conversion to soldiers calls the runtime's rounding entry (`BRAMINAR.COM` file `0x19F6`), not its truncation entry at `0x19FA`.

## Observed and inspected later flow

The initial exploratory continuation showed an automatic army battle with two army counts and percentage bars. No battle choice was encountered. Its winning army display is observed; no claim about victory probability follows from it.

At the end of the opening session, binary inspection identified these next bounded areas. The army follow-up linked above now completes the battle/event rows within its stated limits:

| Assault file offset | Evidence / next work |
| --- | --- |
| `0x0085`–`0x00B7` | Positive-limit inclusive random helper; nonpositive limit returns zero without a draw. |
| `0x00B8`–`0x025E` | Percentage bars and display scaling. |
| `0x025F`–`0x077A` | Random army events: morale, captured magic users, assassinations, surprise attacks and magical weapons. Exact effects need controlled comparisons. |
| `0x077B`–`0x09DA` | Automatic army fight. Inspection shows five casualty trials before a refresh/event test; boundary handling and event interactions still need probes. |
| `0x0E19` onward | Random victory reward and gender-dependent reward texts, including marriage candidates and equipment. Strings alone do not establish gameplay effects. |
| `0x1D81` onward | Ring of Doom corruption, unattractive ruler message, printer question and ending flow. Exact priority and prerequisites need probes. |
| `0x2059`–`0x2984` | Separate Overlord duel; inspection shows an enemy HP calculation using player level, a Talisman shortcut, and unarmed/blade attack text. Do not reuse shared combat without recovering its formulas and order. |
| `0x2C4C`–`0x2CB7` | Army fight, then duel if player soldiers remain positive; otherwise the capture message and exit. |

## Validation and limits

`python3 tools/check_final.py --sanitize` compares net worth, both armies, exact RNG state after conversion, unchanged player memory, normalized original text and the three seconds of requested delays. All thirteen retained conversion scenarios pass with address and undefined-behavior sanitizers. The checker compiles the actual game sources. The Keep checker now includes the split sources, checks the new opening followed by its placeholder exit, and retains fatal-plague suppression and EOF-before-handoff checks. All twenty-three Keep scenarios and EOF checks pass with both sanitizers. `make`, Python syntax checks and whitespace checks pass.

The initial Keep checker failed to link before this work because its harness included only `braminar.c` after the source split. Its harness now includes the actual combat/events/town/final modules under its input and delay instrumentation. Other pre-existing comparison harnesses have the same source-split issue and are outside this bounded change.

Validation does not establish full-run RNG parity, DOS startup seeding, fractional-gold fidelity, extreme signed-16-bit overflow behavior, the zero-army battle, or any later combat or ending. The remake continues using its existing integer inventory and ordinary double arithmetic. No new compatibility or save format is introduced.

## Next targets

1. Completed in the [army follow-up](army-research.md): casualty order, all event effects, negative counts and zero-army behavior, with state/RNG retained between rounds.
2. Completed in [duel research](duel-research.md): HP generation, Talisman, all attacks, retaliation/death order, complete fights and saved-state preservation.
3. Recover reward/ending priority, ring corruption, looks, status certificate and printer behavior. The remake will write the same certificate message and final player stats to stdout instead of using a printer, without a printer-connection prompt. This is the agreed output policy; exact certificate content still needs reconstruction.

`final.c` now runs the army battle and duel after conversion, then exits on defeat or at the remaining victory-reward/ending placeholder.
