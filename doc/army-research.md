# Final assault: automatic army battle

## Scope and evidence

2026-11-18 follow-up: the automatic army battle and all ten special events are recovered and implemented in `final.c`, within the non-overflow scope below. The Overlord duel and endings remain placeholders. This extends the [opening research](final-assault-research.md); conversion and the original `dos/` files are unchanged.

All new runs are controlled experiments. `tools/army_probe.py` reuses the real loaded-module entry and conversion checkpoint, then changes only temporary army counts and RNG before the battle or a chosen event check. Initial synthetic Keep inputs, conversion, every observed army frame/event, and termination screens are retained. No ordinary adventure play is claimed.

- [Event captures](captures/2026-11-18-army-events.txt) and [25 checkpoints](army-events-checkpoints.json): all ten events under both labels, four small-army limits, and surprise overkill. The runner reuses one initialized army frame. Each case explicitly resets counts/RNG before the event check, captures the applied effect before its display delay, then captures clearing. Between cases it restores 100/80 soldiers and seed one, allowing an ordinary five-trial batch before the next controlled setup; these discarded setup frames are also recorded. Original frame initial counts remain 120/160, so these injected counts do not redefine percentage denominators.
- [Battle captures](captures/2026-11-18-army-battles.txt) and [checkpoints](army-battles-checkpoints.json): a complete baseline defeat, one-versus-one, a small victory, and an equal small-party defeat. After initial count/seed injection there are no inter-round edits. Winning runs stop at the army routine's return before the caller can start the unreconstructed duel. Losing runs also capture the caller's capture message and return to DOS.
- [Zero-denominator captures](captures/2026-11-18-army-zero.txt): zero player and zero enemy initial armies. Memory is retained after the first five trials, before the percentage routine aborts. DS is not read after DOS reclaims the program. An earlier attempt, at the end of the battle capture above, correctly observed the abort but failed its diagnostic spelling assertion (`Run-time error`, not `Runtime error`); the separate zero batch supersedes it and supplies both compact fixtures.

## Recovered mechanics

All file offsets below refer to `dos/FINASSLT.CHN`. [Detailed formulas and texts](mechanics.md#final-assault-army-battle) belong in the mechanics ledger.

| Offset | Behavior |
| --- | --- |
| `0x077B`–`0x0790` | Save both initial army counts as fixed percentage denominators and the initial player count as the pacing input. |
| `0x082C`–`0x08C3` | Print current counts, then execute exactly five casualty trials. Inclusive random helper, strict comparison, no early termination. |
| `0x08C4`–`0x08FC` | Update percentage bars, draw `R(100)`, and call the event routine only for 1–4. |
| `0x025F`–`0x0289` | Event alert delay of 100 ms, then draw `R(2)` for label/side. |
| `0x02CC`–`0x0681` | Draw `R(10)` for event and calculate its signed magnitude using the same player/enemy count limits under either label. |
| `0x0682`–`0x06D9` | Positive magnitude kills the other side; negative magnitude kills the selected side. No immediate clamp. |
| `0x06DA`–`0x077A` | Hold event for three seconds, then blank 54 columns of each message row. |
| `0x08FD`–`0x093E` | Delay according to initial player army size, then test whether either count is below one. |
| `0x093F`–`0x09DA` | Clamp negative counts to zero, refresh counts/bars, 100 ms alert plus three-second delay, clear screen, return. |
| `0x2C55`–`0x2CB7` | Positive player survivors enter duel; otherwise print capture message and exit. |

Several tempting simplifications would change results. Equality loses a player soldier, not an enemy. All five trials finish even after elimination; the helper stops consuming RNG once its limit becomes nonpositive. The event check still runs after that batch. A positive effect removes opponents rather than recruiting friendly soldiers. The Overlord label does not swap the event text or the limit formulas. Counts lag the percentage bars by one five-trial batch and also lag the event's effects until the next batch/final refresh.

## Retained outcomes

| Case | Starting player/enemy; seed | Outcome |
| --- | --- | --- |
| Baseline loss | 120/160; 3080093198, exactly the opening baseline's converted state | 21 batches, two special events, final 0/128; RNG 2710549810. Original defeat/capture message and DOS exit observed. |
| Equal one | 1/1; 1 | First batch leaves player -4 before clamping; final 0/1; RNG 557549500. Demonstrates strict comparison, continued trials and nonpositive-limit draw suppression. |
| Small win | 8/2; 1 | Five trials overshoot enemy zero; final 8/0; RNG 3102664695. Captured before duel entry. |
| Equal small | 5/5; 99 | Final 0/5; RNG 2405404249. |
| Zero player | 0/0; 1 | First batch -5/0, no RNG draws; first percentage denominator aborts. |
| Zero enemy | 5/0; 1 | First batch captured; second percentage denominator aborts. |
| Small magic-user capture/loss | 2/3 | Quarter limits truncate to zero; the event amount consumes no RNG draw and changes no count. |
| Surprise overkill | 100/3 | Event makes enemy count -34 before final clamping. |

All 25 event cases match the original signed effects and RNG. Captures preserve the original spellings, including `moral`, `emeny`, `surprize`, and `Magic users is Captured`. They also preserve Player/Overlord labels and the same “your” descriptions under both labels.

## Presentation, validation and limits

The remake uses its existing ANSI terminal approach for the original count positions, two 75-character percentage bars and transient two-line event area. UTF-8 `▓`/`░` replace the DOS CP437 bytes representing the same glyphs. The DOS speaker tones remain silent; their 100 ms intervals are retained. This presentation difference is recorded in [CHANGES.md](../CHANGES.md). There are no army input prompts or added keypress pauses.

`python3 tools/check_army.py --sanitize` compiles the actual sources and checks all 25 event cases and six battle cases. It compares effects, every five-trial batch, exact RNG, player preservation, intermediate and cleared event text, full retained battle terminal frames, clamping, requested delays, victory/defeat routing, and the two aborts. A small ANSI screen interpreter lets the comparison include stale counts, bar fill and message clearing rather than normalizing away their positions. Fatal cases compare retained pre-abort memory and unsuccessful termination, using the documented POSIX diagnostic.

The baseline is a complete conversion-to-army-defeat continuation using the preceding session's converted seed and counts. It reproduces its independently observed 128 enemy survivors. Full army victories here stop before the duel; no duel/endings claim follows from winning a small army test. Existing thirteen conversion cases and twenty-three Keep cases remain separate checks. Keep checks now follow the real army battle and expect either capture or the duel placeholder; dedicated army comparisons check exact battle delays.

Remaining limits: signed-16-bit overflow in large army sums, conversions and event arithmetic is not reconstructed; pacing at the 10000-soldier threshold is established from binary, not a retained timing boundary experiment. Terminal name overflow and every Real48 rounding boundary are not exhaustively compared. The new zero-denominator probes resolve the prior zero-army question: preserve unsuccessful termination, with the existing `Division by zero.` diagnostic instead of Pascal `Run-time error 02` and its code address.

## Next target

Recover and compare the Overlord duel, starting with its Talisman shortcut, enemy HP, attack/retaliation order and fatal outcomes. Victory rewards, corruption/looks endings, certificate and printer behavior remain separate later work. `final.c` now reaches its explicit placeholder only after an army victory.


## Duel follow-up

The [Overlord duel](duel-research.md) is now implemented and compared. The historical army probes above still stop before it; current gameplay and Keep checks continue through the actual duel, exiting on defeat or at the rewards/ending placeholder. Rewards, corruption/looks endings and the stdout certificate remain the next target.


Follow-up: [victory endings and the stdout certificate](ending-research.md) are now implemented and compared. Final-assault victory no longer reaches a placeholder.
