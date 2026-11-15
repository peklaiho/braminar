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

`0x2DB4`–`0x3E6B` implements the shared routine called by band encounters and the river's champion path (`0x9D99`). Wizard, dragon, Mad King, and other specialized fights use separate routines and remain unfinished; these rules must not be applied to them without checking their code.

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
