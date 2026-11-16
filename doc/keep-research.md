# Overlord's Keep research

## Evidence and scope

The complete slot-16 Keep routine at `dos/BRAMINAR.COM` file offsets `0xA0CD`–`0xA2B7` is recovered and implemented. The main loop's decision at `0xF350`–`0xF385` establishes the order of bookkeeping, death handling, and final-assault handoff. The remake calls its existing `start_final_battle()` exit stub at that handoff. No final-assault gameplay or `FINASSLT.CHN` changes are included.

[Mechanics](mechanics.md#overlords-keep-prerequisite) records exact rules, texts, input limits, and draw order. The prerequisite is level at least twenty and a held Staff; the Talisman and whether Pan supplied the command are not additional gates. After agreeing to attack, a case-sensitive five-character command match sets the assault flag. A mismatch removes the Staff. Every completed path pauses before bookkeeping.

## Controlled captures and reproduction

`./keep_probe.py > doc/captures/2026-11-15-keep-probes.txt` now reproduces twenty-three named cases and writes [checkpoints](keep-checkpoints.json): the original twenty below, two G4 starvation cases, and G7 Staff recovery. Named arguments select cases in the requested order; `--output PATH` permits independent batches. Each sequence uses a fresh DOSBox runner and resets temporary player state, prices 8/13, command, assault flag, and RNG only at its start. This is controlled research, not ordinary play. Original files in `dos/` remain untouched. Initial displayed status predates memory edits; `STATE initial` records the actual setup.

The retained fixture batches are the four completed gate/decline cases from [early probes](captures/2026-11-15-keep-probes.txt), wrong-command and promotion cases from the [fixed-breakpoint probes](captures/2026-11-15-keep-breakpoint.txt), and the fourteen [remaining cases](captures/2026-11-15-keep-remaining.txt). The [initial rejection probe](captures/2026-11-15-keep-initial.txt) also completed. Early wrong-command and [promotion](captures/2026-11-15-keep-promotion.txt) attempts used the debugger's transient CS register to place the breakpoint; those attempts stopped without a final fixture and are superseded. A [second early wrong-command attempt](captures/2026-11-15-keep-wrong.txt) is retained for the same tooling issue.

The current probe derives the program's code segment from the initialized stack base at `DS:018C`, subtracting `0xFA0`. Startup at `0x09F2` adds `0x0F50` paragraphs for DS and another `0x0050` for the stack base, from the parameter block at `0x2C7F`. It verifies the code bytes and places an absolute breakpoint at code segment:`F450` (file `0xF350`). Each original visit stops there after bookkeeping, with DS still valid and before the program can enter the final assault or return to DOS on death. No final-assault code executes in these probes.

Screens, choices, submitted command lines, per-input memory, and the post-bookkeeping checkpoint are retained. This stopping point permits a real original-state comparison for a correct command even when plague kills the player during bookkeeping. The remake's actual exit stub is exercised by the comparison harness; an exit-time checkpoint records state after it prints its deferred-battle message.

## Baseline and comparison scope

The baseline is level twenty, 50 HP, Black hair, bad looks, five servants of each sex, 20 gold, 100 food, three medicine, two swords, three bows, four arrows, one dagger, horse/cart, the Staff, and stored command `PRIOR`. Talisman, ring, plague, charm, poison, protection, and experience are absent. Configurations record exceptions.

The twenty cases cover:

- Level nineteen with the Staff, level twenty without it, and neither prerequisite. A level-nineteen character with six experience is rejected before bookkeeping promotes them to twenty.
- Declining an eligible attack while keeping the Staff.
- Wrong, lowercase, short, empty, and leading-space commands; the comparison neither uppercases nor trims input.
- Correct commands at levels twenty and twenty-one; input beyond five characters, including trailing spaces, is ignored by the comparison.
- Startup's stored five spaces: an empty input fails, but five literal spaces match. This follows the original string length/comparison behavior and adds no requirement to have visited Pan.
- Successful entry followed by starvation: food and servant losses occur before the stub.
- Successful entry followed by fatal plague progression: the assault flag stays set, but HP becomes zero and the stub is not called.
- Successful entry with promotion, ring growth, and plague progression in the same bookkeeping pass.
- A two-event Pan-to-Keep sequence with no memory or RNG edits between events, described below.

`./check_keep.py --sanitize` compares state, command, assault flag, normalized text/prompt order, input counts, and requested delays; exact RNG equality is no longer asserted after G4. It invokes the same `run_turn()` used by the real main loop, exercises the actual exit stub, and checks that its printed message precedes the exit-time state checkpoint. EOF checks include every distinct prompt and each successful event's final pause, ensuring EOF cannot bypass bookkeeping into the stub. Fatal plague checks death text and suppression of the stub. The original twenty cases passed with address and undefined-behavior sanitizers, including five distinct EOF prompts and EOF at each successful final pause. All eleven existing sanitizer suites, `make`, Python syntax checks, and whitespace checks also passed in that session. Wiring the existing stub removes its former unused-function warning.

G4 adds `turn-two-servants-starvation` and `turn-last-servant-starvation`, using level-eight/no-Staff rejection to isolate bookkeeping. Their [capture](captures/2026-11-16-turn-starvation.txt) confirms loss from two males to one and preservation of a sole male at zero food, including the presence/absence of the starvation message. All 22 then-retained cases passed the sanitizer checker. [G4 notes](startup-turn-research.md) consolidate turn evidence and identify promotion/starvation pacing as a separate G9 presentation follow-up; requested-delay assertions do not establish parity with those original bookkeeping waits.

G7 adds `staff-loss-reacquisition`, seed 117042, using the level-twenty/Staff/`PRIOR` baseline. Submit `WRONG` at the first Keep, gaze into the next river to recover the Staff, and decline the next Keep's renewed attack offer. Intermediate Staff states are absent then held, while command `PRIOR` persists. Final state is 50.30 HP, eighty-five food, Staff held, level twenty, command unchanged, and assault flag false. [Raw capture](captures/2026-11-16-staff-recovery.txt) retains every input/state without inter-event edits; [G7 notes](quest-state-research.md) record scope and results. The runner places its bookkeeping breakpoint only at the last encounter's pause; no assault executes. All 23 current Keep cases pass with sanitizers, including the earlier Pan-to-Keep sequence. No gameplay or C-harness change was needed.

## Pan-to-Keep interaction

The chained case starts from controlled level-twenty/Staff state with the startup blank command, then answers Pan through normal choices. Pan supplies `GJVDY` from seed 1346. The next normal RNG dispatch selects the Keep. The probe submits the exact command Pan printed and stops after the Keep's bookkeeping. A `turn` checkpoint records the intervening state. No command, inventory, or RNG edits occur between the two encounters.

This validates acquisition, persistence, command input, and handoff ordering under controlled initial conditions. It is not an ordinary playthrough earning level twenty and the Staff. Final state at the handoff is 50.20 HP, 90 food, the Staff and command preserved, and assault flag true. The reconstruction checklist retains the broader ordinary integration follow-up.

## Limits and intentional differences

The final assault remains deferred by explicit scope; the exit stub replaces the original chain into `FINASSLT.CHN`. The probe captures original state before that chain, not outcomes inside it. POSIX line editing and EOF handling follow the existing shared reader; DOS-specific control characters and its line-editor capacity are not exhaustively compared. Ordinary five-character, short, empty, whitespace, lowercase, and overlength inputs are covered. Fractional HP/experience follow the documented double-precision policy; extreme integer overflow remains outside this target.
