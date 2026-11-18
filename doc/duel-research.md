# Final assault: Overlord duel

## Scope and evidence

2026-11-18 continuation: `final.c` now implements the Overlord duel after army victory, including its separate HP, all eight attacks per side, Talisman shortcut, animation, retaliation, simultaneous death and defeat text. Winning the duel still reaches an explicit victory-reward/ending placeholder. This extends [army research](army-research.md); original `dos/` files are unchanged.

All new runs are controlled experiments, not ordinary play. `tools/duel_probe.py` enters through the original loader and a controlled 8-versus-2 army victory. It restores the desired player and RNG at the actual duel entry, then lets the original create its stack frame and HP. No instruction bytes or return addresses are patched. The runner now also sets the runtime delay-loop calibration word at `DS:0012` to one, accelerating controlled captures without changing the requested delays, HP, RNG or choices. Earlier round/small-fight captures used the original calibration; the completed level-twenty capture uses one. A [fresh accelerated Talisman capture](captures/2026-11-18-duel-fast-regression.txt) exactly matches the earlier unaccelerated compact fixture. Original wall-clock timing is not compared; requested delay arguments and animation counts are recovered from binary and checked in the remake.

Evidence:

- [Initial exploration](captures/2026-11-18-duel-explore.txt): actual duel entry, stack inspection, first attack/retaliation frame. It established that debugger expressions `SS:EBP-10`, `SS:EBP-E` and `SS:EBP-8` access the current frame reliably.
- [Controlled rounds](captures/2026-11-18-duel-rounds.txt), [12 checkpoints](duel-rounds-checkpoints.json): one initialized frame reused for all eight attack pairs, killing-blow retaliation, simultaneous death and exact-zero boundaries. Each case writes local HP/RNG, verifies local writes by readback, and records applied and cleared screens. Before the next case, locals are restored positive at the cleared-frame breakpoint so a synthetic lethal round cannot end the reusable duel. Global player memory is read before/after each case.
- [Complete duels](captures/2026-11-18-duel-full.txt), [checkpoints](duel-full-checkpoints.json): Talisman victory, natural victory, natural defeat, simultaneous defeat, and an identical-seed fight with altered saved HP, ring, poison and weapons. No edits occur between combat rounds. An [early full-batch capture](captures/2026-11-18-duel-full-initial.txt) was interrupted while refining the runner and is superseded by this batch.
- [Full level-twenty duel](captures/2026-11-18-duel-endgame.txt): the same uninterrupted combat tracing at the normal end-game level, with saved HP deliberately set to one. Its compact result is included with the complete-duel checkpoints. The [first end-game attempt](captures/2026-11-18-duel-endgame-initial.txt) and [instruction-pane detection attempt](captures/2026-11-18-duel-endgame-pane-attempt.txt) captured all 36 rounds through victory but timed out waiting for the final stop. Changing the wait marker alone did not resolve this. The completed capture shortens runtime delay loops using the controlled calibration described below and supersedes both incomplete final-checkpoint attempts. A [last-round isolation probe](captures/2026-11-18-duel-finish-explore.txt) separately reached the grave with the same HP/RNG, and an [entry regression](captures/2026-11-18-duel-entry-regression.txt) reproduced the existing conversion baseline.

The retained complete-duel fixtures stop victories before the ending routine at file `0x2975`. Fatal runs retain global/local memory before returning to DOS and capture the exit screen afterward. Victory rewards and the printer flow remain outside the observed and implemented scope.

## Stack frame and breakpoints

All file offsets refer to `dos/FINASSLT.CHN`; add `0x2D7C` for runtime IP in the established program code segment. Duel entry is file `0x2059`. Do not infer the current frame from an absolute stack address: after prologue, `SS:EBP-E` holds the player's six-byte Real48 local HP and `SS:EBP-8` the Overlord's. The 16-byte dump at `SS:EBP-10` includes two leading bytes of animation-loop state, both HP values, and a copy of the current frame pointer.

| File offset | Retained boundary |
| --- | --- |
| `0x221F` | HP generated and opening animation complete, before Talisman check |
| `0x2299` | Start of an ordinary player attack |
| `0x27F0` | Both attacks/animations applied and HP displayed, before two-second hold |
| `0x2882` | Both message rows cleared, before loop termination test |
| `0x28AD` | Talisman applied, before death-marker processing |
| `0x291F` | Death marker displayed and final one-second delay complete |
| `0x2975` | Victory branch, before calling the unreconstructed ending routine |
| `0x297E` | Defeat text complete, before returning from the duel |

The runner uses one absolute breakpoint at a time and the paused debugger memory APIs. Its controlled round fixtures retain the previous cleared frame as presentation input. For the initial retained batch, these input frames are derived from its captured initialization and preceding cleared frames; subsequent captures record them directly. This matters because shorter HP values can leave trailing digits from previous displays. The comparison seeds the terminal renderer with the actual preceding frame, rather than assuming blank HP fields.

## Recovered rules and outcomes

[Mechanics](mechanics.md#final-assault-overlord-duel) records the formulas, complete attack tables and timing. The original uses local player HP equal to ten times level, not saved player HP. The enemy starts with `R(10 * level) + (10 * level) / 3`. Inventory does not select attacks or supply damage, and ring/poison do not bypass this fight. Talisman is the only shortcut in this routine. It is checked after the enemy-HP draw and opening animation and is not consumed.

Player and Overlord attacks always both execute before checking either HP, so a killing blow still receives retaliation. Negative HP is displayed without clamping. Both dead means player defeat; the enemy's grave is nevertheless drawn because death-marker selection tests enemy HP first. No local damage is written back to the main player record. In particular, the future certificate must not substitute duel-local HP for the saved HP used by the original ending.

| Complete case | Evidence |
| --- | --- |
| Talisman | Level 20, saved HP 0.1, seed 1: local player starts at 200; enemy starts at about 191.67 and becomes zero. One RNG draw total, no retaliation; item and saved HP remain intact. |
| Natural victory | Level 1, saved HP 1, seed 0: three rounds, local HP about 0.70 versus -2.77, RNG 840368863. Stops before ending. |
| Natural defeat | Level 1, saved HP 500, seed 5: one round, local HP about -1.20 versus 7.13, RNG 566136256. Death message and return to DOS observed. |
| Simultaneous defeat | Level 1, saved HP 500, seed 1: three rounds, local HP about -8.40 versus -10.37, RNG 1719420512. Enemy grave plus player death message, no ending. |
| Ignored inventory | Same level/seed as natural victory, saved HP 999, ring 100, poison and every weapon count 99: identical local combat and RNG. Global inventory unchanged. |
| End-game duel | Level 20, saved HP 1, seed 0: 36 rounds, local HP about 25.10 versus -1.83; RNG 3903989361. Full uninterrupted trace validates ordinary end-game scale and retained frame/timing behavior. |

The twelve isolated rounds establish all eight damage values per side. A 20-HP player killing an enemy at 0.1 HP with the strongest attack still takes 11.2 retaliation damage. Starting both at 0.1 produces simultaneous death. Exact enemy-zero (12.7 minus 12.7) and player-zero (11.2 minus 11.2) are also retained.

## Validation and limits

`python3 tools/check_duel.py --sanitize` compiles the actual game sources. Twelve controlled round cases compare local HP, exact RNG, all attack text, post-animation and cleared terminal frames, animation/hold delays and global-state preservation. All twelve round cases and six complete-duel cases pass with address and undefined-behavior sanitizers. Complete-duel comparisons call the actual `fight_overlord()`, independently check HP initialization, compare every retained attack-result frame and RNG, and verify grave selection, death text, victory routing, delays and unchanged saved player state. The screen model normalizes DOS glyph bytes 1/2/127 to their UTF-8 terminal glyphs and retains stale numeric characters. Raw local values are compared within the established C-double policy rather than requiring Real48 bit identity.

The earlier thirteen conversion, twenty-five army-event, six army-battle and twenty-three Keep scenarios remain separate regression suites. Keep handoff checks now follow the real duel after army victory and accept capture, duel death, or the victory-ending placeholder. No extra input pauses were added.

This is not an ordinary full adventure playthrough. Large signed-16-bit level multiplication/HP-roll overflow and exhaustive floating-point boundary parity remain unresolved. Endings and rewards are intentionally not invoked by these probes; their priority and effects remain the next bounded target. The agreed certificate destination is stdout, without a printer-connection prompt.
