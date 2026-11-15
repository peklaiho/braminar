# Recovered game mechanics

This reference records rules recovered from the original binaries, the evidence supporting them, and remaining uncertainties. Keep binary offsets alongside the rules they support. Observed encounters and provisional string findings are catalogued in [events.md](events.md); file layouts and extraction-tool usage are documented in [data-format.md](data-format.md).

The remake preserves the player-visible experience, while allowing simpler arithmetic for minor precision details that make no observable difference to a human player. For example, turn healing adds 0.10 HP using ordinary C arithmetic; emulating Pascal Real48 representation and intermediate rounding is not required. Preserve calculations that affect displayed values, costs, probabilities, thresholds, or gameplay outcomes. Original binary details below remain evidence about the classic game, not requirements to reproduce every low-order bit. Record adopted simplifications in [CHANGES.md](../CHANGES.md), separately from unresolved rules.

Unless a different file is named, offsets are hexadecimal byte offsets into `dos/BRAMINAR.COM`, not DOS runtime addresses. `DS:` addresses identify variables in the original program's data segment.

## Random-number generation

The bounded random routine is at `0x0FDA`. It saves the requested bound, calls the state-update routine at `0x0FE6`, shifts the returned 16-bit word right by one, and returns the remainder after division by the bound.

The state-update routine uses two 16-bit words at `DS:0x01FC` and `DS:0x01FE`, forming a 32-bit state. Its byte rearrangement and rotate-through-carry instructions compute a left shift by seven bits. It then adds the old state and the constant `0x361962E9`, retaining the low 32 bits. It returns the high 16 bits of the new state.

Equivalent arithmetic for a positive bound:

```c
state = (state * 129 + 0x361962E9) modulo 2^32;
result = (state >> 17) % bound;
```

Thus each price is `5 + ((next_state >> 17) % 10)`. The remainder operation gives a slight distribution bias: among the 32,768 possible 15-bit outputs, residues 0 through 7 occur 3,277 times each, while residues 8 and 9 occur 3,276 times each. The remake preserves this mapping rather than replacing it with an unbiased bounded sampler.

The remake uses this generator for character stats, prices, the recovered main event selector, band encounters and shared combat, the enchanted forest, and the gnome branch. It seeds the state from the current time. The order of random calls within character generation and the implemented encounter branches is reproduced, but the original initial seed and full order of random calls elsewhere are not yet reproduced, so identical complete runs across the original and remake are not claimed.

## Character generation

The initialization routine starts at file offset `0xA2B8`. After reading name and gender, it initializes the following visible stats. Here `Random(n)` means the recovered bounded generator above and returns a value from 0 through `n - 1`.

| Stat | Initial value or algorithm | File offsets | Original data address |
| --- | --- | --- | --- |
| Level | `Random(3) + 7`: 7, 8, or 9 | `0xA36B`–`0xA376` | `DS:0x0277` |
| Hit Points | Start at zero; add `Random(10) + 1` once per level | `0xA377`–`0xA3C8` | `DS:0x0279` |
| Hair Color | `Random(4) + 1`: 1 = Blonde, 2 = Brown, 3 = Black, 4 = Red | `0xA3C9`–`0xA445` | `DS:0x027F` |
| Good Looks | True | `0xA446`–`0xA44B` | `DS:0x0294` |
| Staff of Aviatar | False | `0xA458`–`0xA45D` | `DS:0x0297` |
| Talisman of Braminar | False | `0xA45E`–`0xA463` | `DS:0x0298` |
| Male servants | `Random(15)`: 0–14 | `0xA470`–`0xA478` | `DS:0x029C` |
| Female servants | `Random(15)`: 0–14 | `0xA479`–`0xA481` | `DS:0x029E` |
| Gold | `Random(25)`: 0–24 | `0xA482`–`0xA492` | `DS:0x02A0` |
| Food | 100 | `0xA493`–`0xA4A0` | `DS:0x02A6` |
| Medicine | 3 | `0xA4A1`–`0xA4A6` | `DS:0x02AC` |
| Poison, swords, bows, arrows, daggers | All zero | `0xA4A7`–`0xA4C4` | `DS:0x02AE` through `DS:0x02B6` |
| Horse, cart | Both false | `0xA4D6`–`0xA4E1` | `DS:0x02B8`, `DS:0x02B9` |

The title screen's claim of starting at levels 7–10 does not match the executable's `Random(3) + 7`, which excludes level 10. The remake follows the executable's calculation.

Hit points are the sum of multiple rolls, not one random value selected from the combined range. A level `L` character starts with `L` through `10 * L` hit points, with totals clustered around `5.5 * L`. Generation adds integers only, even though the value is stored as a six-byte Pascal real. The remake uses `double`; these initial integer sums are represented exactly. The runtime helpers at `0x1873`, `0x1893`, `0x18A6`, and `0x1A2F` load, store, add, and convert values for those real calculations. Gold and food are also stored as six-byte reals in the original, although their initial values are integers and the current remake keeps them as integers.

Preserve the sequence of random calls: level, one hit-point roll per level, hair color, male servant count, female servant count, gold, male price, then female price. Generating hit points with one roll or reordering fields would change every subsequent result for the same initial random state. The player's gender does not change these formulas.

These are generation values, before events and turn bookkeeping. The original's later turn processing adds fractional health and consumes food, so a subsequent status screen can differ from the initial values. See the main turn loop below.

## Servant prices

Disassembly confirms the following setup operations.

| File offsets | Operation |
| --- | --- |
| `0xA4E2` to `0xA4ED` | Call the bounded random routine with 10, add 5, store male price at `DS:0x03EE` |
| `0xA4EE` to `0xA4F9` | Call it again with 10, add 5, store female price at `DS:0x03F0` |
| `0xA50A` and `0xA542` | Start of the two original price message strings |
| `0xA57B` | Call the keypress-pause routine after displaying prices |

Each price is between 5 and 14 gold pieces, inclusive. Neither price is derived from the other or from the player's gender. The only direct assignments to these two variables found in the main binary are the setup assignments above.

The market routine reads these stored prices when calculating both sales (`0xAC31` and `0xAC55`) and purchases (`0xACDD`, `0xAD0F`, `0xAD21`, and `0xAD53`). It does not reroll them. The final assault module also reads them when valuing servants (`FINASSLT.CHN`, instructions beginning at `0x29DC` and `0x29F8`). This supports treating the prices as game-wide values fixed for the session.

## Main turn loop

Recovered from `0xEF7D`–`0xF385`. Each iteration displays status through `0xA585` and waits for a key, selects and resolves one encounter, then performs bookkeeping in the order below. There is no universal pause after encounters: individual event routines decide whether to pause. For example, declining the forest immediately advances to the next status; entering it ends with a pause.

### Event selection

At `0xEF97`–`0xEF9E`, the program calls the wrapper at `0x2C9F` with 16. The wrapper calls `Random(bound)` and adds one (`0x2CA6`–`0x2CAF`). The dispatch therefore uses `Random(16) + 1`, rather than selecting uniformly from the 14 locations initially listed in the remake. All 16 residues occur equally often among the generator's 32,768 possible outputs; this does not establish statistical independence across turns.

| Slot | Routine | Encounter or condition |
| --- | --- | --- |
| 1 | `0xC2B9` | Village |
| 2 | `0xC4C1` | City |
| 3 | `0xCB86` | Wandering Gypsy Camp |
| 4 | `0xD8FA` | Band demanding payment or combat |
| 5 | `0xDEE7` | Thicket / laboratory |
| 6 | `0x6769` | Hollow tree; can contain the gnome or be empty |
| 7 | `0x6CC5` | Grass hut |
| 8 | `0x757A` or `0xD8FA` | Mountain trail if ring state `DS:0x0295 > 0`; otherwise band encounter |
| 9 | `0x76FE` or `0xD8FA` | Servant illness unless protection flag `DS:0x0299` is true; otherwise band encounter |
| 10 | `0x7A5B` | Weather |
| 11 | `0x7E15` | Dark Castle |
| 12 | `0x8BA7` | Enchanted forest |
| 13 | `0x8F9B` | Separate hollow-tree routine, beginning with an empty home |
| 14 | `0x96E7` | River |
| 15 | `0x9DC8` | Statue of Pan |
| 16 | `0xA0CD` | Overlord's Keep |

The two guarded slots do not reroll when their conditions fail. At character creation, ring state and protection are zero (`0xA44C`–`0xA467`), so slots 4 and 8 dispatch to the band encounter, while slot 9 dispatches to illness. When the total servant count is zero, the illness routine instead prints `There seems to be a bug going around.` and returns after a delay without a keypress pause (`0x7708`–`0x7714`, `0x7A1C`–`0x7A54`).

### End-of-turn bookkeeping

1. If `0 < HP < level * 10`, add approximately 0.10 HP (`0xF112`–`0xF15A`). The classic's six-byte real literal at `0xF151` is `7D CC CC CC CC 4C`, slightly below 0.10; the remake intentionally uses 0.10 without reproducing that representation detail. The comparison happens before addition; there is no clamp afterward. If HP is nonpositive, reset the real accumulator at `DS:0x02BA` to zero (`0xF15D`–`0xF17D`). Combat adds to this accumulator after a victory (`0x3C24`–`0x3C58`), supporting its interpretation as experience.
2. If experience is strictly greater than 5, announce a level increase, add one level, subtract 5 experience, and add 10 HP (`0xF180`–`0xF205`). This is a single `if`, not a loop; exactly 5 does not qualify. Shared combat awards `enemy level / 4.0`; specialized combat routines still need separate investigation.
3. If the ring byte is greater than zero and less than 100, add 5 (`0xF208`–`0xF237`). A value such as 99 becomes 104; there is no clamp to 100. It starts at zero. Combat bonuses and forced fights are described below; effects in other encounters remain to be recovered.
4. If the plague byte is positive, add 5, storing the low byte. If the resulting byte is at least 100, print `You have died from the plague` and set HP to zero (`0xF23A`–`0xF294`). Other routines establish the identification: plague acquisition increments this byte at `0x48B2`–`0x48BA`; the forest cure clears it at `0x8F0B`–`0x8F0E`.
5. Subtract integer `(male servants + female servants + 1) / 2` from food (`0xF297`–`0xF2B9`). The signed integer division precedes conversion to real: this is not half a unit per person. With zero servants, consumption is zero; with nine servants, it is five. Counts are assumed nonnegative and within the signed 16-bit arithmetic range here; overflow behavior for extreme counts has not been reconstructed in C.
6. If food is strictly negative, set it to zero. If the combined servant count exceeds one, announce starvation and decrement each positive servant count by one (`0xF2BC`–`0xF34D`). Reaching exactly zero does not cause starvation that turn. A sole remaining servant is not removed by this code. Starvation here does not damage the player.
7. Continue unless HP is nonpositive or the final-assault flag `DS:0x04F8` is true (`0xF350`–`0xF36D`). After leaving the loop, positive HP invokes `FINASSLT.CHN`; otherwise print `Game Over.` (`0xF370` onward). A successful Keep encounter sets the flag before bookkeeping, so bookkeeping still happens before entering the final assault.

The [first recorded session](captures/2026-11-14-session-01.md) independently shows HP increasing by 0.10 per completed turn and food decreasing by five for a party with nine servants. An elves' gift adds ten food before the five-unit consumption. These observations corroborate the binary calculations; they do not independently test every boundary above.

The remake implements the dispatch, bookkeeping, band encounters and shared combat, river, forest, both branches of the slot-6 hollow tree, storms, servant illness, Pan, City/Village, and the Keep's prerequisite rejection, with explicit placeholders for other encounters. Some event bodies and their random calls are still missing, so subsequent event sequences will differ even with equal initial seeds. The remake intentionally uses `double` for HP and experience without Real48 emulation, as recorded in `CHANGES.md`. Food is currently integer; whether inventory calculations require visible fractional quantities remains unresolved and must be checked as those mechanics are recovered. The final-assault transition is still deferred with the eligible Keep branch; `start_final_battle()` remains its intended stub entry point.

## Enchanted forest

The routine at `0x8BA7`–`0x8F9A` was checked against the nymph/resistance/elves path in the [recorded session](captures/2026-11-14-session-01.md). Other branches below are verified from the binary, but have not yet been observed live in this session. Here `Roll(n)` means `Random(n) + 1`; preserve the strict comparisons and conditional random-call order.

- Ask whether to enter. Declining prints `You move on.` and returns without a pause (`0x8BEB`–`0x8BF1`, `0x8F7B`–`0x8F94`).
- Roll 1–100. A nymph appears if the result is less than `level + 5` (`0x8BF4`–`0x8C11`). Male players see a female nymph; female players see a male nymph (`0x8C14`–`0x8C97`).
- Only when a nymph appears, roll another 1–100. Resist if the result is less than `50 + 2 * level`; otherwise print that the player is lost in La La Land and set HP to zero (`0x8CD4`–`0x8D73`). This can be fatal regardless of otherwise healthy HP.
- After that branch, elves greet the player only if HP is positive and the ring byte is zero (`0x8D76`–`0x8D9C`). They add `male servants + female servants + 1` food and restore good looks (`0x8E0D`–`0x8E75`).
- Only in the elves' branch, roll another 1–100. A faerie appears for a result less than 21 (`0x8E78`–`0x8E8C`). If plague is positive, clear it without healing HP. Otherwise announce full healing and raise HP to `level * 10` only if currently below that value (`0x8ED5`–`0x8F6C`).
- All entered paths, including death or exclusion by the ring, end in a keypress pause (`0x8F6F`–`0x8F75`). Normal turn bookkeeping follows, including food use and healing where applicable.

## Statue of Pan

The [first session](captures/2026-11-14-session-01.md) records one incorrect answer. Binary inspection establishes these additional rules:

- Approaching reads one of the first 20 records of `BRAM.DAT`, using `Random(20)` as the zero-based record index (`0x9E3E`–`0x9E64`). It does not select the ten empty records.
- After accepting a key from `1`, `2`, `3`, the game rolls `Random(3) + 1` and compares it with the chosen digit (`0x9F54`–`0x9FAD`). The answer is random, not a stored factual answer to the question. On failure it displays that rolled answer number.
- Success generates five letters independently with `Random(26) + 1 + 64` (`A`–`Z`) and stores them in the prime-command string at `DS:0x04F9` (`0x9FCF`–`0xA007`). It displays that string and tells the player to remember it. Startup initializes this string to five spaces (`0xED26`–`0xED38`); success replaces its five characters without changing its length.
- Either answered outcome pauses; declining the initial approach prints `You move on.` without that pause (`0xA0A1`–`0xA0C6`).
- Invalid answer keys retry before the answer roll (`0x9F54`–`0x9F85`); they consume no random draws. Failure and decline leave the current command unchanged. A subsequent success overwrites it.
- No inventory, HP, or experience changes occur within Pan's routine. Ordinary end-of-turn bookkeeping still follows, including after declining.

Pan is implemented with all 20 question records embedded in C; record 3 changes only `=` to a space as recorded in [CHANGES.md](../CHANGES.md#typo-corrections). Five [controlled DOS cases](pan-research.md) compare command, state, RNG progression, text, and input counts. Success overwrote `PRIOR` with `SKJFJ`, `WINIP`, and `YYDGX`; failure and decline preserved it. All 20 embedded questions are separately checked against the declared data strings. The Keep's command input and transition remain the next target.

## Gnome's announced food gift

In the occupied branch of the hollow-tree routine, the original announces a gift of 3–8 food (`Roll(6) + 2`, `0x6876`–`0x68C9`) but never adds it to inventory on this path. It proceeds directly to a possible healing ointment and then exits the encounter. This matches the live capture: the gnome announced three food, yet food changed from 95 to 90 after ordinary five-unit turn consumption.

The remake intentionally fixes this classic bug: add the announced 3–8 food to inventory before normal turn consumption. With the captured example's starting food of 95, a gift of three and consumption of five now leave 93 food, rather than the classic's 90. This difference is recorded in [CHANGES.md](../CHANGES.md#bug-fixes); the original capture remains evidence of classic behavior.

After entering slot 6's tree, `Roll(100) < 56` selects the occupied gnome branch (`0x67C0`–`0x67D4`). If HP is below `level * 10`, the gnome rolls 1–5, announces that many HP restored, and adds it without a subsequent cap (`0x68D8`–`0x696A`). The captured five-HP ointment changed 49.30 HP to 54.40 after turn healing. The remake implements this occupied branch, including its pause, and pauses after declining entry as the original does (`0x6C9C`–`0x6CBB`). The empty-home branch is now implemented as described below. The separate slot 13 tree remains unfinished.

## Empty gnome home

Session 04 entered main-loop slot 6's empty home and took the Staff from its trunk during ordinary play. The remainder of this branch was checked against the executable and eight separately labelled [controlled DOS captures](captures/2026-11-14-tree-probes.txt); [saved checkpoints](tree-checkpoints.json) retain their final states. See [the pass ledger](session-04.md) for the distinction between ordinary and controlled observations.

- The initial entry choice and occupied/empty split are shared with the occupied branch above. An occupied roll is `Roll(100) < 56`; rolls 56–100 select the empty home, irrespective of inventory. Do not reuse this implementation for slot 13 without inspecting that separate routine.
- The empty home offers `(S)teal food, (T)ake from trunk or (L)eave.` Read one character, case-insensitively, retrying invalid choices (`0x6A48`–`0x6A87`). There is one action per visit.
- S rolls 1–100, announces that quantity, and adds it to food (`0x6A94`–`0x6AE6`). Unlike the occupied gnome's gift, this food addition exists in the classic.
- T rolls 1–4 with equal probability (`0x6AF4`–`0x6B03`). Result 1 adds 1–20 arrows (`0x6B0B`–`0x6B45`); result 2 adds 1–10 medicine (`0x6B53`–`0x6B8E`). Result 3 awards the Staff if absent, without another random draw (`0x6B9C`–`0x6BDA`); if already held, it instead adds 1–10 medicine (`0x6BE0`–`0x6C1C`). Result 4 releases a Rune Guardian and subtracts 1–5 HP (`0x6C2A`–`0x6C96`). This is direct damage, with no combat choice or retaliation.
- L consumes no further random numbers and prints no outcome line. All three actions end at the same keypress pause (`0x6CB5`–`0x6CBB`). Declining initial entry prints `You move on.` and also pauses, without rolling occupied/empty.
- Normal end-of-turn healing and food consumption follow. There is no extra reward, experience gain, or recorded theft penalty on these paths.

The controlled cases start at level 8, 50 HP, five servants of each sex, 8 gold, 100 food, 3 medicine, 2 arrows, no ring/plague, and the Staff absent except in `staff-held`. Arrows yielded five; medicine one; the held-Staff substitute nine medicine; the Guardian dealt five HP; theft added 57 food. Final food was 95 except for theft (152), with 0.10 turn healing applied to every case. `./check_tree.py --sanitize` compares inventory, HP, experience, level, RNG state, and expected input counts. These checks cover the captured seeds; full random ranges follow from binary arithmetic. Extreme signed-integer overflow remains outside this reconstruction.

## Overlord's Keep prerequisite

The gate tests `level < 20 OR NOT staff` at `0xA106`–`0xA121`. Level 20 is sufficient when the Staff is held. Rejection prints `You cannot attack due to lack of Staff or 20th level.` and takes the common exit pause (`0xA124`–`0xA166`, `0xA2A8` onward). This branch was observed in [session 03](session-03.md) and is implemented. On a successful prime-command comparison it sets `DS:0x04F8` to true (`0xA23B`–`0xA23E`); a failed comparison removes the Staff (`0xA2A2`–`0xA2A5`). The command input and comparison have not yet been implemented in the remake.

## Flowing river

The routine at `0x96E7`–`0x9DC7` is now implemented. [Session 03](session-03.md) records an ordinary harmless vision and five separately labelled controlled checks, with [compact final states](river-checkpoints.json).

- Declining to gaze prints `You Move on.` and returns without a pause or encounter RNG draws (`0x9728`–`0x9734`, `0x9DA8`–`0x9DC7`).
- Gazing selects one of twenty visions with `Roll(20)` (`0x976A`–`0x9C3B`). These are independent flavor texts: the selection does not inspect inventory, future events, or item placement, and none directly applies its depicted effect.
- Always roll 1–100 for the Staff, even when already held. Award it only if absent and the roll is strictly below 10: 9% (`0x9C3E`–`0x9CAC`).
- Always roll 1–100 for the Talisman next, even when already held. Award it only if absent and the roll is strictly below 6: 5% (`0x9CAF`–`0x9D14`). Both awards can happen on the same visit.
- Roll 1–100 next. Strictly below 26 invokes shared combat against the Troglodyte template, level 11: 25% (`0x9D17`–`0x9D99`). There are no payment or fleeing choices. Poison and all other shared-combat rules apply; the HP roll is made by that routine.
- Every gazed path has a final keypress pause (`0x9D9C`–`0x9DA2`). After combat this is an additional pause after the shared routine's result pause, including on death. Normal turn bookkeeping follows.

Preserve the random-call order: vision, Staff, Talisman, champion, then any combat draws. Already owning either quest item does not skip its roll. Typo corrections in visions are enumerated in [CHANGES.md](../CHANGES.md#typo-corrections).

## Storms

Following session 04's tornado, the routine at `0x7A5B`–`0x7E14` was inspected. It is now implemented. The ordinary pass observed the tornado/no-shelter path; separately labelled controlled checks cover all eight storm types and the boundary cases in the [storm/illness ledger](storm-illness-research.md).

- Roll 1–8 at `0x7AB4`–`0x7ABB`: rain storm, Hail storm, Snow storm, Wind storm, Tornado, Hurricane, Monsoon, Tidal Wave, in that order. All offer shelter.
- Sheltering preserves servants and food apart from ordinary turn consumption. The waiting loop runs `storm type * Roll(5)` iterations, each delaying one second and printing a dot, then prints `The storm is gone.` (`0x7C5B`–`0x7D17`). This is one random call, with duration scaled by the storm's type, not an arbitrary fixed wait.
- Without shelter, type 1 (rain) makes no inventory changes and consumes no further random numbers. Types 2–7 independently subtract `Roll(5)` males and then `Roll(5)` females, clamping each count to zero (`0x7D1D`–`0x7DB9`). These rolls occur even if that sex's count is zero.
- Without shelter, type 8 clears both servant counts and food (`0x7DBC`–`0x7E02`). It does not directly damage HP.
- Every path takes the common keypress pause (`0x7E05`–`0x7E0B`), then returns for normal turn bookkeeping. Starvation after storm losses is a separate effect.

Session 04's combined loss of six males and three females, with food exhausted, matches five/two storm losses followed by one/one starvation. The full distribution follows from binary arithmetic. Controlled checks now corroborate all eight types without shelter, rain/tidal-wave shelter waits of one/eight dots, clamping losses in small or empty parties, one-sex parties, and storm losses followed by starvation. Requested delays are compared with the original loop's dot counts; debugger captures do not establish exact elapsed timing.

## Servant illness

The routine at `0x76FE`–`0x7A5A` was inspected after [session 03](session-03.md) encountered Old-Timer's disease. The remake now implements the recovered treatment rules, including the excessive-treatment behavior, the no-servant delay, and the protection substitution. See the [controlled probe ledger](storm-illness-research.md) for cases and validation limits.

- With any servants, independently roll 1–10 for sick males and sick females, each capped at the number of that sex (`0x7717`–`0x775C`). Both rolls occur even if one sex has zero servants.
- Roll 1–10 for the displayed disease (`0x77AF`–`0x7918`): The Plague, Small Pox, Chicken Pox, Turkey Pox, Leprocy, Old-Timer's disease, A fatal cold, Bubonic plague, Scarlet fever, The Flu. All take the same treatment path. These names do not set the player's plague byte.
- Ask for males to cure, then females, using numeric line input; negative numbers retry the numeric read. Each requested quantity is capped to remaining medicine. Each cure consumes one medicine, and each sick servant left untreated is subtracted from that sex's count (`0x791B`–`0x79E8`). Males therefore receive medicine first.
- The original does not cap treatment to the sick count. An excessive request can consume unnecessary medicine and even increase the servant count through subtraction of a negative untreated count. Controlled probes confirm this behavior: one male/two females, twenty medicine, and requests of ten/ten yield ten of each sex and zero medicine. The remake preserves it.
- Print `Any not cured pass away.`, pause, then delay one second before returning (`0x79EB`–`0x7A51`). With no servants, print `There seems to be a bug going around.` and take only that delay. Main-loop illness protection substitutes a band encounter before reaching this routine.

The observed 9 sick males / 1 sick female, treated with 2 / 1 from three medicine, removed seven males and consumed all medicine. Subsequent turn consumption was fourteen food for 27 remaining servants plus the player, matching the recovered rules. New controlled checks establish that negative input retries without repeating the prompt, leading whitespace is skipped, and text after a whitespace-delimited number is discarded. `$0002` is accepted as hexadecimal two. Nonnumeric `abc` and decimal 32768 abort with `I/O error 10`, followed by `Program aborted`; bookkeeping does not run afterward. These are original-game input rules; the remake now uses the intentionally simplified shared reader described below and in CHANGES.md.

### Numeric treatment input

The original token reader at `0x2618`–`0x2653` skips whitespace within the active read and reads a token up to whitespace; an empty `ReadLn` can return without assigning its destination. The parser at `0x105D`–`0x10CB` accepts decimal and `$`-prefixed hexadecimal, with an optional leading minus. Decimal input is bounded to signed 16-bit values; hexadecimal can represent a full 16-bit word, interpreted as signed before the event tests for negativity. Negative treatment values retry without another prompt. `0x2654`–`0x2660` sets the I/O error on an invalid token or overflow. These original limits are reference evidence; they are no longer the remake's input policy. Controlled checks cover decimal negatives, blank input, whitespace-delimited trailing text, positive hexadecimal, and the two abort cases above. A blank male-treatment line in the controlled run advanced to the female prompt and reused a stale local value, consuming all three medicine. The destination is not initialized at routine entry (`0x7705`); an empty read can leave it untouched (`0x2618`–`0x2694`). The remake deliberately retries blank lines instead of emulating uninitialized stack contents; this difference is recorded in CHANGES.md. Bare signs/prefixes and original input-length limits remain outside the original-game comparisons. Extreme inventory arithmetic remains unresolved.

The shared `read_integer` now accepts only nonnegative decimal digits with optional surrounding whitespace, up to C `INT_MAX`. Other nonempty input silently yields zero; blank/whitespace-only lines retry and EOF returns failure. Hexadecimal, signs, mixed text, and fractions are rejected as zero. Nonblank lines filling the 159-character buffer also yield zero. These intentional changes replace the original negative retries, token parsing, and invalid-number aborts; see CHANGES.md. Illness and town quantities use this generic reader. The retained original fixtures are unchanged; comparison checks explicitly account for the new input policy.

## Implementation checks, 2026-11-14

`make` succeeds, retaining the existing warning about the unused final-battle stub. A temporary C harness built with AddressSanitizer and UndefinedBehaviorSanitizer checked bookkeeping against the documented boundaries: HP at and just below the limit, zero versus negative food, starvation with one or two servants, experience exactly five versus above five, one promotion per turn, dead-player experience reset, ring growth past 100, plague death, and byte wraparound. It also reproduced the captured forest resistance/elves state change with a selected seed. These are remake checks against the recovered rules, not additional observations of original-game boundary behavior.

A terminal-based check of the full remake passed character creation, an encounter, and the next status with the expected HP and food changes. All 18 dispatch call sites, including guarded alternatives, were checked directly against the executable bytes. `git diff --check` passed. Most event bodies remain outside this validation; exact Real48 emulation is intentionally outside the remake's requirements.

## Band encounters

The routine at `0xD8FA`–`0xDEE6` is now implemented. It selects one of nine templates with `Random(9) + 1` (`0xD947`–`0xDA3A`). Startup assigns the following names and levels at `0xEC37`–`0xED23`:

| Roll | Name | Level | Template at DS |
| --- | --- | --- | --- |
| 1 | Goblin | 5 | `0x0410` |
| 2 | Gremlin | 6 | `0x03F3` |
| 3 | Rust Golem | 6 | `0x042D` |
| 4 | Orc | 7 | `0x044A` |
| 5 | Ogre | 9 | `0x0467` |
| 6 | Pirate | 10 | `0x0484` |
| 7 | Troglodyte | 11 | `0x04A1` |
| 8 | Drow Elf | 12 | `0x04BE` |
| 9 | Demon | 14 | `0x04DB` |

Each template is 29 bytes: a Pascal string of capacity 20 (21 bytes including length), a signed 16-bit level, and six bytes for HP. The band copies the full template to its stack; the shared combat routine generates fresh HP.

After choosing the template, roll three independent `Random(10) + 1` demands in order: male servants, female servants, gold (`0xDA7A`–`0xDAAD`). The offer requires all three resources. Read F/G/R case-insensitively, repeating invalid input. If the ring byte is strictly greater than 45, print that it compels fighting and choose F without reading a key (`0xDBDC`–`0xDC5B`).

- Fight calls the shared routine below (`0xDC6B`–`0xDC7B`).
- Give succeeds only with enough of each demanded resource, and subtracts all three demands. If any is insufficient, print the assassination message and set HP to zero; this branch does not actually clear the remaining inventory (`0xDC89`–`0xDDA7`). It returns after a delay, without a keypress pause.
- Run with a horse, no cart, and more than two servants sets each sex's count to one if it was greater than one. This is not a loss of just one servant (`0xDDB5`–`0xDE34`).
- On every run attempt, roll `Random(100) + 1`, even with a horse or a low-level enemy. The player is caught only if there is no horse, enemy level is greater than nine, and the roll is strictly less than `enemy level * 3` (`0xDE37`–`0xDE78`). Caught players enter shared combat. Otherwise print `You got away!`. The routine delays after the outcome, without adding a separate keypress pause.

These rules were checked in [controlled original-game probes](combat-research.md), including successful and failed payment, a horse escape, and a caught escape. Controlled probes alter only temporary emulator state and are distinct from the ordinary playthrough.

## Shared combat

`0x2DB4`–`0x3E6B` implements the shared routine called by band encounters and the river's champion path (`0x9D99`). Wizard, dragon, Mad King, and gate-creature fights use separate routines; these shared rules must not be applied to them. Dragon and gate-creature combat are now recovered and implemented for the grass hut below; Anthrax combat is recovered below; Mad King combat remains unfinished.

### Setup and round selection

1. A Rust Golem clears all swords and daggers before any poison choice (`0x2DC4`–`0x2E28`). A Drow Elf clears bows and arrows (`0x2E2B`–`0x2E91`). The effects are based on the monster name, not on a chance roll.
2. If poison is available, offer to use it. Yes consumes one dose. Regardless of the answer, next roll `Random(7) + 1` and multiply by enemy level to generate HP. If poison was used, set that generated HP to zero (`0x2E94`–`0x2F1D`). Thus a poisoned enemy still consumes the HP random call and equipment destruction still occurs.
3. While both participants have positive HP, run a round. Roll `Random(9) + 1` for the player's attack; add two if any ring is held (`0x2F4B`–`0x2F70`).
4. If the player has a sword or dagger, or has both a bow and at least one arrow, roll `Random(100) + 1`; add one to the attack result if this roll is less than 26. Skip this random call entirely when unarmed. Clamp the final attack result to nine (`0x2F73`–`0x2FD8`). Weapon counts do not add further bonuses.
5. Choose blade text if any sword or dagger is held, otherwise bow text if a bow and arrows are available, otherwise unarmed text. A bow shot consumes one arrow, including a miss. Blades take precedence over a bow, so holding both does not consume arrows. Damage depends on the attack result, not on which of these three text categories is used (`0x2FDB`–`0x382E`).
6. The enemy retaliates even if that attack reduced its HP to zero or below. An enemy of level ten or less rolls `Random(9) + 1`; higher-level enemies roll `Random(8) + 2`. The player's level does not affect these rolls (`0x3831`–`0x3863`).
7. Show both HP totals and pause for a key after every round, including the fatal round (`0x3AFC`–`0x3B5F`). The player does not choose an attack or weapon each round. Recheck both HP totals only after that pause.

### Damage table

The table gives the ordinary decimal values represented by the classic's constants. The remake uses these decimals with `double`, following the numeric-simplification guideline. Preserve significant values such as 0.01 and 1.02, which can affect displayed HP and survival.

| Result | Damage to monster | Damage to player |
| --- | --- | --- |
| 1 | 0 | 0 |
| 2 | 0.01 | 0 |
| 3 | 0.20 | 0.01 |
| 4 | 2.10 | 0.10 |
| 5 | 3.20 | 1.02 |
| 6 | 3.90 | 2.10 |
| 7 | 4.60 | 3.20 |
| 8 | 5.10 | 4.10 |
| 9 | 6.20 | 5.00 |

Player constants occur at `0x3178`, `0x3259`, `0x333A`, `0x3449`, `0x3538`, `0x3627`, `0x3728`, `0x3825`; enemy constants occur at `0x38FA`, `0x394C`, `0x399E`, `0x39EC`, `0x3A3B`, `0x3A98`, `0x3AF0`. The enemy's result-two message, `He nudged you slightly.`, does not subtract HP (`0x389E`–`0x38C2`).

### Outcome, experience, and loot

The routine reports the player's survival and the monster's survival separately (`0x3B62`–`0x3C21`), so both may die in the same round. Surviving players gain `enemy level / 4.0` experience (`0x3C24`–`0x3C58`), including victories obtained with poison. The real division preserves fractional experience: a level-five Goblin gives 1.25, a level-eleven Troglodyte gives 2.75. Main-loop level advancement follows later.

Always roll `Random(100) + 1` after the fight, even if the player died or already owns a ring. If that roll is less than player level, the player is alive, and the ring byte is zero, award a ring by setting the byte to five (`0x3C5B`–`0x3CEA`). The ordinary end-of-turn increment then raises a newly acquired ring to ten.

Survivors next roll male servants 1–20, female servants 1–20, and gold 1–50, in that order. Add all three to inventory and add food equal to the sum of those two servant awards (`0x3CED`–`0x3E57`). No loot calls are made for a dead player. Both success and death end with a keypress pause (`0x3E5A`–`0x3E60`). End-of-turn bookkeeping then runs, consuming food for the enlarged party and checking for level advancement or death.

The [probe ledger](combat-research.md) records scenarios and validation. Round HP, arrow use, equipment loss, experience, ring acquisition, rewards, end-of-turn state, and RNG progression were compared against original memory checkpoints. These checks establish the recorded cases, not complete parity for every possible state or for the specialized combat routines.

## City and Village

The Village (`0xC2B9`–`0xC4C0`) and City (`0xC4C1`–`0xCB85`) call the same servant market, tavern, store, and inn routines. The remake uses `event_town(player, city)` and shared service functions. These rules come from binary inspection, with [40 controlled comparisons and an ordinary pass](town-research.md). `Roll(n)` below means `Random(n) + 1`.

### Entry and location differences

- Both ask whether to enter. Declining prints `You go on.` and returns without a pause or encounter random draw. Normal turn bookkeeping still follows.
- On entry, ring power strictly greater than 10 triggers a warning. The Village peasant says `Mercy is for the Weak!` without a random draw. The City centurion rolls 1–100: below 51 gives the liars warning; otherwise the Ford remark (`0xC30E`–`0xC358`, `0xC513`–`0xC5C2`).
- The Village then rolls 1–100 for a pickpocket: below 8 (7%) triggers a detection roll. Below `25 + level` prints the three-line discovery/nose-punch message. Otherwise silently subtract `Roll(trunc(gold))` gold (`0xC35B`–`0xC479`). With zero gold, this calls `Random(0)` and aborts the original with runtime error 04. The remake preserves termination, using a POSIX diagnostic instead of reproducing the Pascal runtime's address report. This is not a player death and does not run turn bookkeeping.
- Both visit servant market → tavern → store in that order. The City then rolls for its extra marketplace; both next visit the inn. The City alone then rolls for a cleric and muggers. The Village has no extra marketplace, cleric, or muggers. The wrappers do not test HP between services, so a fatal drink does not immediately skip later prompts.
- None of the four common services ends with a keypress pause. Successful lodging requests a two-second delay. The City mugger encounter has an explicit result pause; declining or skipping everything else returns directly to status.

### Servant market

The common routine is `0xAB46`–`0xAD79`. After accepting the market invitation, independently ask to sell and to buy. Each accepted action reads male then female quantities before applying either transaction. It uses the fixed [session prices](#servant-prices) and consumes no random numbers.

Sales cap each quantity to the count held, subtract those servants, and add the corresponding gold. Purchases process males first: if their entire requested cost exceeds current gold, buy zero males; otherwise buy the full quantity and deduct its cost. Then do the same for females against the remaining gold. Exact funds suffice. There is no partial affordable-quantity purchase, transaction retry, or failure message. Each accepted action prints `Done`, even if both quantities are zero or unaffordable.

The remake uses the shared nonnegative integer reader. Its documented malformed/negative/blank-input policy replaces Pascal input behavior; normal decimal transactions preserve the above arithmetic. Extreme signed-integer overflow remains outside the reconstructed scope.

### Tavern

The common routine is `0xAD7A`–`0xB7EF`. Once inside, ask about a drink, socializing, then darts, in that order, and print `You leave the tavern.` No outcome adds an input pause.

- A drink requires at least two gold; otherwise silently skip its effects. Deduct two gold and always draw `Roll(5)` for potential healing first. Next draw `Roll(100)`; below 30 (29%) prints that healing amount, adds it, then caps HP at `level * 10` even if HP was already above that maximum (`0xAE13`–`0xAED8`).
- Only when the healing check fails, draw another 1–100. Below 15 (14% conditional) selects poison. Draw another 1–100: below `level + 20` lets the player notice and save the poison, adding one poison dose. Otherwise consume one medicine if available, or set HP to zero if none. The remaining drink outcome prints `It wasn't so hot.` (`0xAEDB`–`0xB078`).
- Accepted socializing draws 1–100: 1–10 meets a girl, 11–20 a guy, 21–100 nobody. An encountered patron always consumes a hair roll and an appearance roll, each 1–5, before asking `Will you Move in?` Hair is Blonde, Gold, Brown, Black, or bald; appearance spans ugly through beautiful/gorgeous. Accepting consumes a 1–100 charm roll even when the player lacks good looks. A roll below 20 together with good looks adds one servant of the patron's sex; otherwise the patron rejects the player. Declining consumes no charm roll (`0xB07B`–`0xB62A`). Player gender does not select the patron.
- Darts asks every tavern visitor. If accepted with at least five gold, request a 500 ms delay, then draw player score 1–50 and opponent score 1–50. Player score greater than or equal to the opponent's wins and adds ten gold; no ante is deducted first. A loss subtracts five gold unless ring power is strictly greater than 60, in which case the player takes the money and runs without changing gold (`0xB62D`–`0xB7C7`). Insufficient funds silently skip the game and its draws/delay.

### General store

The shared store is `0xB7F0`–`0xBEDB`. On entry, offer each item once, in this order: sword, bow, dagger, twelve arrows, food, medicine, horse if absent, cart if absent. There is no return-to-menu loop.

| Offer | Price | Award |
| --- | --- | --- |
| Sword, bow, dagger | Independent `Roll(100)` for each | One of the selected weapon |
| Twelve arrows | `Roll(20)` | Twelve arrows |
| Food | One gold per unit | Requested units |
| Medicine | Three gold per unit | Requested units |
| Horse, only if absent | `Roll(150)` | Set horse flag |
| Cart, only if absent | `Roll(250)` | Set cart flag |

Prices are generated immediately before each applicable offer, whether or not the player can afford it. Already owning a horse/cart skips both its price draw and prompt. A cart does not require a horse. `You have:` displays current gold, not inventory. For the sword, the price precedes that balance; later offers show the balance first.

Yes/no purchases silently do nothing when unaffordable; exact funds suffice. Food and medicine repeatedly read quantities until their full cost fits the remaining gold, without a new question or error message on retry (`0xBC14`–`0xBC34`, `0xBCE8`–`0xBD0D`). Their quantities use the remake's shared integer-input policy. The code checks affordability before multiplication to avoid introducing C integer overflow for oversized requested quantities; original extreme 16-bit overflow is not emulated.

### City marketplace

After the general store and before the inn, draw 1–100. Below 18 (17%) offers `It seems that the Market Place is open. Enter?` (`0xC5E0`–`0xC63F`). Declining prints `You proceed on.` with no pause or further marketplace draws.

Accepted entry first asks about food, then poison. Food's entered quantity is gold to spend: subtract that amount and add five times that amount to food, rather than charging one fifth of a gold per individually requested unit (`0xC642`–`0xC726`). Poison costs 100 gold per requested dose (`0xC732`–`0xC807`). Each quantity retries with `? ` until affordable, then applies the purchase; zero is accepted. No marketplace prices or transaction outcomes consume random draws. The store does not sell poison.

### Inn

The shared routine is `0xBEDC`–`0xC2B8`. Accepted entry reads one of the keys 1, 2, 3 (retry invalid keys). Costs are 5, 10, 20 gold. Deduct the chosen cost first; if gold becomes negative, print the insufficient-funds ejection, reset gold to zero, and return without healing or delay (`0xC087`–`0xC161`). Exact funds qualify for lodging.

Successful lodging adds `Roll(room cost)` HP without a maximum clamp. Only if there are no male servants, draw 1–100; below 15 (14%) lets rats eat one food, but only if food is positive. The rat roll is consumed even when food is zero (`0xC193`–`0xC24C`). More than one female servant prints the bandaging line and adds integer `female servants / 2` HP, also without clamping. Then delay two seconds and return (`0xC24F`–`0xC2AF`). Room quality affects the healing roll, not rat probability. Normal turn healing follows only if HP is still below its ordinary maximum.

### City cleric and muggers

After the inn, always draw 1–100 for a cleric. Above 88 (12%) offers a cure for all current gold (`0xC832`–`0xC8EA`). Acceptance sets gold to zero, restores good looks and Brown hair, clears plague and ring, and raises HP to `level * 10` only when below it. It does not remove quest items or clear the servant-illness protection flag. Declining prints `He leaves.` Either response delays 500 ms, without a keypress pause (`0xC8ED`–`0xC9A5`).

Then always draw another 1–100, even with servants. Only when the combined servant count is zero and that roll is below 11 (10%) do muggers appear (`0xC9A8`–`0xC9D8`). Ring power strictly greater than 30 drives them away with lightning, with no loss draws or robbery delay. Otherwise draw gold loss 1–100, cap it to current gold, then food loss 1–20, cap it to current food, announce and subtract both, and delay one second. Either mugger outcome ends in a keypress pause (`0xC9DB`–`0xCB62`). Cleric removal of the ring happens before this protection check.

## Mountain trail and guru

Slot 8 selects the mountain trail only when ring power is positive; without a ring it calls the band encounter instead. The event body (`0x757A`–`0x76FD`) makes no random calls and requests no delay.

- Ask `You come upon a mountain trail.  Climb it?` (`0x7587`). N prints `You move on.` and proceeds to the common keypress pause.
- Y introduces the guru and asks `Do you seek the powers of good?` (`0x75CD`–`0x765B`). N proceeds straight to the pause without another message or state change.
- Y checks the Staff flag at `DS:0297`; if held, print `Your staff disappears.` and clear it (`0x765E`–`0x7690`). Then print `The ring disappears from around your finger.` and clear ring power (`0x7693`–`0x76CF`). The Talisman and other player fields are untouched.
- Both questions use the shared case-insensitive Y/N reader. Every completed path pauses once (`0x76EE`–`0x76F4`), followed by ordinary turn bookkeeping. A retained ring therefore gains five power when below 100; a removed ring stays zero.

Five [controlled comparisons](mountain-research.md) check state, RNG, normalized text, and input counts, including invalid keys and ring powers 1 and 100. The remake corrects `lookes` to `looks`, as recorded in CHANGES.md.

## Wandering Gypsy camp

Recovered from file offsets `0xCB86`–`0xD8F9`. Slot 3 always offers entry. N prints `You move on.` and returns without a pause or further random draw. Y offers Dagger's Hand, then the fortune teller regardless of whether the game was played (`0xCBD9`–`0xCD24`, `0xCF7C`).

### Dagger's Hand

- Ask `Play the game?` even when gold is zero. Play requires Y and strictly positive gold (`0xCD09`–`0xCD24`). Otherwise, gold at or below zero prints `You have insufficient funds.` even if the answer was N; a funded refusal is silent (`0xCF3E`–`0xCF79`). Neither skipped path pauses or draws RNG. No dagger, level, ring, servant, or appearance prerequisite is checked.
- Start a pot at zero, with both previous rolls at 100. Repeat while gold is positive and both previous rolls exceed 15 (`0xCD27`–`0xCD6D`). There is no further choice between rounds.
- Each round prints `You Ante`, subtracts one gold, and adds two to the pot (`0xCD70`–`0xCDA3`). The opponent's contribution is represented by the pot increment; it is not taken from the player.
- Delay 500 ms, print `You go...` without a newline, and draw `Roll(100)`. Below 16 (rolls 1–15) prints the finger injury, subtracts the Real48 constant at `0xCE11` (approximately 0.4 HP), and prints `You lose.` Otherwise print `You made it.` (`0xCDA6`–`0xCE4C`).
- Always delay another 500 ms and let the opponent draw `Roll(100)`, even when the player was injured or HP is nonpositive. Print `He goes...` without a newline. Below 16 prints his finger injury, adds the whole accumulated pot to player gold, and prints `You win!` Otherwise print `He made it.` (`0xCE4F`–`0xCEEF`). Both participants can therefore hit a finger, producing both loss and win messages; the pot is still awarded.
- Delay 500 ms after the opponent's outcome, then reevaluate the loop. Both successful rolls continue if any gold remains. A last-coin win restores gold before this check; a last-coin round without an opponent injury leaves zero gold.
- After any played game, zero gold prints `You ran out of money.` The result always ends with a single keypress pause (`0xCEFB`–`0xCF38`), then the fortune teller is offered. There is no HP gate before advice, so even a fatal injury allows that interaction before turn bookkeeping.

The injury threshold occupies 15 of the 100 bounded results. The existing generator's remainder mapping is preserved; this is not a claim of perfectly independent or unbiased rolls. The C remake uses 0.4 HP under the documented double-precision policy.

### Fortune teller

After any entered visit, ask `will you buy some advice?` for ten gold (`0xCF7C`–`0xD039`). Acceptance requires at least ten gold, including equality. A refusal or insufficient funds silently ends the visit without a pause or RNG call.

A funded Y draws `Roll(35)` and displays the corresponding fixed rumor (`0xD046`–`0xD8B3`). All 35 branches are populated; none is conditional on player inventory or prior knowledge. It then subtracts ten gold and pauses once (`0xD8B6`–`0xD8D4`). Repeated visits may give the same rumor. Hints only print text; buying advice does not grant quest items, change flags, or generate Pan's command. Slave references become servants in the remake; other rumor wording is preserved.

See [research and comparison coverage](gypsy-research.md) and [retained checkpoints](gypsy-checkpoints.json).

## Grass hut

Recovered from `0x6CC5`–`0x7579`, dispatched by slot 7. This is separate from the repeated old-man/puppy scenes around `0x91D0`–`0x95xx`; helping someone in this hut does not call that later good-deed reward routine. See [research and validation limits](hut-research.md).

### Entry and occupants

The hut draws `Roll(100)` before the entry answer. Values 51–100 announce sounds within; 1–50 announce silence (`0x6CFA`–`0x6D57`). Both ask `Will you enter?`. Declining prints `You move on.` and pauses once. Every completed hut path ends with this same single outer keypress pause (`0x756A`–`0x7570`); fights may additionally pause after each round.

Entering a noisy hut draws `Roll(3)` (`0x6D93`–`0x6DA2`):

| Roll | Occupant | Behavior |
| --- | --- | --- |
| 1 | Sick old man | Ask to cure him. Y uses one medicine if positive, otherwise comforts him. Both print that he seems better, then delay one second. N does nothing further. No reward, HP gain, or experience change (`0x6DAA`–`0x6E9D`). |
| 2 | Hungry puppy | Ask to share food. Y uses one food if positive, otherwise pets the puppy, then delays one second. N does nothing further. No reward or experience change (`0x6EAB`–`0x6FAD`). Starvation can follow at bookkeeping. |
| 3 | Hermit with dragon | Offers to double possessions for defeating the dragon. See below. |

### Hermit

Accepting the challenge calls the separate dragon routine (`0x7065`–`0x706B`). If the player is alive afterward, draw `Roll(100)`; 1–20 announces that the hermit disappears with `min(Roll(10), female servants)` females. This prints the count but never subtracts it (`0x7083`–`0x710F`), preserving another original discrepancy between text and state. The count draw occurs even with zero females.

Values 21–100 double male servants, female servants, gold, medicine, food, poison, swords, bows, arrows, and daggers, in that order (`0x7112`–`0x71DD`). HP, experience, level, quest items, ring, charm, horse, and cart are not doubled. The parent skips both outcome draws and rewards after death.

Refusing the challenge instead asks `Kill the hermit?` (`0x71E3`–`0x720C`). Y announces his death and calls the dragon routine; this path never doubles possessions or rolls for betrayal. N proceeds to the outer pause.

### Quiet-hut finds

Entering a quiet hut draws `Roll(9)` (`0x7255`–`0x7264`).

| Roll | Result | Extra draws and state changes |
| --- | --- | --- |
| 1 | Rusty sword | +1 sword |
| 2 | Quiver | +12 arrows |
| 3 | Staff or nothing | Always draw `Roll(100)`, including if already held. Award the Staff only if absent and the roll is 1–4; otherwise print nothing found (`0x72E5`–`0x735F`). |
| 4 | Gold | Add `Roll(25)` gold |
| 5 | Female servant | +1 female |
| 6 | Male servant | +1 male |
| 7 | Bow | +1 bow |
| 8 | Astral gate | Draw `Roll(100)`: 1–12 summons a creature and calls `0x4B67`; 13–100 closes the gate. |
| 9 | Nothing | No extra draws |

### Dragon combat

The separate routine is `0x56C6`–`0x5DBD`. It does not use shared band combat, weapon bonuses, ring bonuses, or arrows.

- A positive signed charm word at `DS:029A` prints submission, adds `Roll(3)` experience, and returns without other combat draws or a pause. The word is not consumed or cleared (`0x56D0`–`0x5731`). Startup clears it (`0xA467`–`0xA46D`); a laboratory potion increments it (`0xE7B1`–`0xE7B7`). Despite the potion's “Next meeting” text, the recovered dragon routine leaves it persistent. Acquisition through that potion is now implemented as part of the thicket below.
- Without charm, draw dragon level `Roll(14)`, then HP `Roll(50) + 30` (31–80). If poison is positive, offer its use. Y consumes one dose, sets dragon HP to zero without a saving throw, and prints use (`0x5734`–`0x57F2`).
- While both HP values are positive, draw player attack `Roll(7)`, then dragon attack `Roll(7)`. The dragon retaliates even if the player's attack killed it. Weapons, servants, ring, and level do not alter these draws or damage.
- Display both HP values to two decimals and pause after each round (`0x5C5E`–`0x5CC5`). At the end print player victory/death and dragon victory/slain independently. A surviving player gains integer `dragon level / 2` experience. No final combat pause, loot, or ring draw; the hut performs the outer pause and any hermit reward.

| Roll | Player damage to dragon | Dragon damage to player |
| --- | --- | --- |
| 1 | 0 | 0 |
| 2 | 0.1 | 0.1 |
| 3 | 0.7 | 0.6 |
| 4 | 1.2 | 1.4 |
| 5 | 2 | 2 |
| 6 | 4.3 | 3.4 |
| 7 | 5.1 | 4.2 |

Fractional values simplify embedded Real48 constants according to the existing precision policy.

### Gate-creature combat

The separate routine is `0x4B67`–`0x56C5`. Draw `Roll(5)` for the type, then its HP die (`0x4B71`–`0x4CD8`). Original spellings are retained.

| Roll | Name | Level | Starting HP |
| --- | --- | --- | --- |
| 1 | Cerberus | 16 | `Roll(5) * 14` |
| 2 | Collosus | 16 | `Roll(7) * 12` |
| 3 | Lolth | 18 | `Roll(10) * 10` |
| 4 | Vaprak | 20 | `Roll(11) * 10` |
| 5 | The Lich King | 22 | `Roll(14) * 12` |

Announce the creature and delay 500 ms. If poison is positive, offer its use; Y consumes one dose and draws `Roll(100)`. A value strictly greater than creature level sets its HP to zero; equality or below saves (`0x4D0B`–`0x4DD8`). Refusal has no poison draw.

Each round draws player attack `Roll(7)` followed by creature attack `Roll(7)`, with any extra player-damage draw between them. The creature retaliates even after a killing hit. No equipment, ring, servant, or player-level modifier applies. Display HP to two decimals and pause after each round (`0x4DDB`–`0x532D`).

| Roll | Player damage to creature | Creature damage to player |
| --- | --- | --- |
| 1 | 0 | 0 |
| 2 | 0.2 | 1 |
| 3 | 1.4 | 3 |
| 4 | 2.5 | 5 |
| 5 | 3.4 | 7 |
| 6 | `Random(10)`, 0–9 | 9 |
| 7 | `Roll(5) + 7`, 8–12 | 11.5 |

At the end print player victory/death and creature survival/disappearance independently. A surviving player gains integer `creature level / 2` experience and receives one unconditional `Roll(6)` reward (`0x5330`–`0x56BF`). No final combat pause; return to the hut's outer pause.

| Reward roll | State effect |
| --- | --- |
| 1 | Award Staff if absent; otherwise +250 food |
| 2 | Award Talisman if absent; otherwise +250 food |
| 3 | +250 food |
| 4 | +5 poison |
| 5 | Set HP to `player level * 10`, even if previously above this |
| 6 | +20 food, +10 medicine, +2 poison; set horse and cart true |

All three food paths announce `50 Lbs of food.` but add 250 (`0x54EC`–`0x54F5`, `0x5566`–`0x556F`, `0x55A5`–`0x55AE`). The remake preserves both the message and the arithmetic. Experience promotion and normal food consumption follow at turn bookkeeping, after the hut pause.

## Thicket and laboratory

Recovered from `0xDEE7`–`0xEB4E`, dispatched by slot 5. [Research notes](thicket-research.md) separate binary rules, controlled comparisons, the chained charm/dragon check, and ordinary discovery play. All probabilities below describe bounded roll results; the existing RNG remainder mapping is preserved.

### Visit sequence and loot

- Ask entry first, without extra random draws. N prints `You proceed on.` and returns without a pause (`0xDEF1`–`0xDF38`, `0xEB2C`–`0xEB45`).
- Y describes the laboratory, draws `Roll(8)` for the displayed roast weight, and delays 500 ms. Then draw `Roll(100)`; 1–50 calls Anthrax, 51–100 skips him (`0xDFB1`–`0xE0B2`).
- Only a player with positive HP proceeds to exploration after the fight. Death skips all loot, Staff, potion, and pentacle draws, but still reaches the common outer keypress pause (`0xE0B5`–`0xE0C7`, `0xEB20`–`0xEB26`).
- Add `roast weight * 10` food, despite the smaller displayed pound count (`0xE0CA`–`0xE0E4`). Then draw and award males `Roll(60)`, females `Roll(60)`, and gold `Roll(250)`, in that order (`0xE119`–`0xE233`). These are laboratory loot, not a reward inside Anthrax's routine.
- Always draw `Roll(100)` for the Staff. Award it only when absent and the roll is 1–4; an already-held Staff still consumes the draw (`0xE23F`–`0xE2A7`).
- Draw `Roll(100)` for potions; 1–25 offers them, 26–100 skips them. An offer draws a count `Roll(5)` before asking whether to try the batch. Declining skips every potion effect draw (`0xE2AA`–`0xE317`).
- After potions, including refusal or absence, draw `Roll(100)` for a pentacle; 1–10 offers entry, 11–100 skips it (`0xE874`–`0xE8E8`). Every entered thicket path ends with one outer pause, in addition to any combat-round pauses. There is no extra loot/potion pause.

### Potions

Accepting the batch drinks all announced potions, without further choices. For each, print `Drinking Potion #n` and four dots, each followed by a 250 ms delay, then draw `Roll(11)` and apply the effect (`0xE31A`–`0xE871`). Effects run in sequence; an earlier level or servant change affects later effects and bookkeeping. There is no clamp on potion level changes or the ten-HP addition.

| Roll | Effect |
| --- | --- |
| 1 | Subtract one player level; HP and experience untouched. |
| 2 | Add two player levels; HP and experience untouched. |
| 3 | Draw `Roll(8)` and replace hair color with Black, Brown, Blonde, Red, Blue, White, Orange, or Plaid. |
| 4 | Set both servant counts to zero; they become fleeing gremlins. |
| 5 | Zombie urine sample: text only. |
| 6 | Add ten HP without a cap. |
| 7 | Set servant-illness protection true. |
| 8 | Increment the signed dragon-charm word. Dragon submission does not consume it. |
| 9 | Clear plague. |
| 10 | Set good looks true. |
| 11 | Water: text only. |

The hair table also contains Purple, Avocado, Silver, Magenta, Hot Pink, and Green, but neither the potion nor Anthrax's hair spell can select them: both call `Roll(8)`, not 14. The implemented palette follows the reachable control flow (`0xE442`–`0xE451`, `0x45FC`–`0x460B`).

### Pentacle

Declining the pentacle proceeds to the common outer pause. Acceptance draws `Roll(5)` (`0xE8EB`–`0xE8FA`):

| Roll | Outcome |
| --- | --- |
| 1 | Set ring power to five, replacing any existing ring power. Normal turn progression subsequently makes it ten. |
| 2 | Subtract fifteen HP; death is processed after the outer pause. |
| 3 | Pie in the face: text only, without an appearance change. |
| 4 | A figure forms; call the already recovered gate-creature routine `0x4B67`, including its poison saving throw, combat, experience, and reward. |
| 5 | Raise level to twenty only if currently below it, and set Staff true. HP and experience are untouched; levels above twenty are preserved despite the message. |

The pentacle does not supply Pan's command. Its creature caller adds no extra final combat pause beyond the thicket's common pause.

## Anthrax combat

The separate wizard routine is `0x3E6C`–`0x4B66`; it does not use shared band combat. Weapons, arrows, servants, illness protection, and charm provide no combat bonus. Servants can instead be targeted by one retaliation.

Always draw wizard level `Roll(10) + 2` (3–12), then wizard HP `Roll(wizard level) * Roll(10)` (`0x3E8B`–`0x3ECA`).

Any positive ring power then vaporizes Anthrax, adds `Roll(4) + 1` (2–5) experience, and returns without poison questions, combat outcomes, or another pause (`0x3ECD`–`0x3F84`). The initial level/HP draws still happen. The ring is neither consumed nor changed by this routine.

Without a ring, offer poison if positive. Y consumes one dose and sets wizard HP to zero, with no saving throw (`0x3F87`–`0x400F`). Normal outcome messages and integer `wizard level / 2` experience still follow.

### Wizard rounds

While both HP values are positive, draw player attack `Roll(9)` (`0x4012`–`0x4388`), followed by wizard retaliation `Roll(9)` (`0x438B`–`0x4A14`). Any extra attack damage roll occurs before retaliation; any spell saving throw or damage roll occurs during retaliation. The wizard retaliates even if the player just killed him, and even if the player's diving attack killed the player. No HP or level scaling applies to attack damage.

| Player roll | Action | Wizard HP damage | Other effect |
| --- | --- | --- | --- |
| 1 | Funny face | 0 | None |
| 2 | Frog | 0.3 | None |
| 3 | Tear skin | 1.4 | None |
| 4 | Raccoon | 1.5 | None |
| 5 | Test tube | `Roll(10) + 2`, 3–12 | Extra damage draw |
| 6 | Dive while he teleports | 1.1 | Player also loses 0.3 HP |
| 7 | Bubbling brew | `Roll(5) + 3`, 4–8 | Extra damage draw |
| 8 | Nick staff | 5.1 | None |
| 9 | Hellen Reddy picture | 6.4 | None |

Fractional values simplify the embedded Real48 constants under the existing precision policy.

| Wizard roll | Retaliation and exact rule |
| --- | --- |
| 1 | Miss. |
| 2 | Ugly stick: delay 500 ms, print saving-throw prefix, delay 500 ms, draw/display `Roll(100)`. Save only if strictly greater than `79 - current player level`; otherwise set good looks false. A save preserves current looks rather than restoring them. |
| 3 | Fry one servant: subtract one male if positive, otherwise one female if positive; no random sex draw. Zero servants still prints the spell text. |
| 4 | Level drain: draw/display `Roll(100)`; above 40 saves, 40 or below subtracts one level. HP and experience are not adjusted. |
| 5 | Replace hair with the shared eight-color laboratory palette, using one draw. |
| 6 | Plague: print saving-throw prefix, delay 500 ms, draw/display `Roll(100)`. Save only if strictly greater than `80 - current player level`; otherwise add five to plague, storing the low byte. Existing plague is not cleared by a save. Illness protection does not prevent it. |
| 7 | Staff strike: player loses `Roll(3)` HP. |
| 8 | Shot hits mirror: wizard loses `Roll(2) + 1` (2–3) HP; player takes none. |
| 9 | Lightning: player loses `Roll(5)` HP. |

After each round display player and wizard HP to two decimals and pause (`0x4A17`–`0x4A76`). At the end print player victory/death and wizard survival/disappearance independently. Add integer `wizard level / 2` experience unconditionally, even after player death (`0x4A7C`–`0x4B5D`); normal death bookkeeping later clears experience. There is no additional pause or inventory reward inside this routine. The laboratory parent awards loot only to survivors.
