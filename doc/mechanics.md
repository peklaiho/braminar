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

The remake uses this generator for character stats, prices, the recovered main event selector, and the enchanted forest. It seeds the state from the current time. The order of random calls within character generation is reproduced, but the original initial seed and full order of random calls elsewhere are not yet reproduced, so identical complete runs across the original and remake are not claimed.

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
2. If experience is strictly greater than 5, announce a level increase, add one level, subtract 5 experience, and add 10 HP (`0xF180`–`0xF205`). This is a single `if`, not a loop; exactly 5 does not qualify. Combat's full experience award formula remains to be recovered.
3. If the ring byte is greater than zero and less than 100, add 5 (`0xF208`–`0xF237`). A value such as 99 becomes 104; there is no clamp to 100. It starts at zero. Other ring effects remain to be recovered.
4. If the plague byte is positive, add 5, storing the low byte. If the resulting byte is at least 100, print `You have died from the plague` and set HP to zero (`0xF23A`–`0xF294`). Other routines establish the identification: plague acquisition increments this byte at `0x48B2`–`0x48BA`; the forest cure clears it at `0x8F0B`–`0x8F0E`.
5. Subtract integer `(male servants + female servants + 1) / 2` from food (`0xF297`–`0xF2B9`). The signed integer division precedes conversion to real: this is not half a unit per person. With zero servants, consumption is zero; with nine servants, it is five. Counts are assumed nonnegative and within the signed 16-bit arithmetic range here; overflow behavior for extreme counts has not been reconstructed in C.
6. If food is strictly negative, set it to zero. If the combined servant count exceeds one, announce starvation and decrement each positive servant count by one (`0xF2BC`–`0xF34D`). Reaching exactly zero does not cause starvation that turn. A sole remaining servant is not removed by this code. Starvation here does not damage the player.
7. Continue unless HP is nonpositive or the final-assault flag `DS:0x04F8` is true (`0xF350`–`0xF36D`). After leaving the loop, positive HP invokes `FINASSLT.CHN`; otherwise print `Game Over.` (`0xF370` onward). A successful Keep encounter sets the flag before bookkeeping, so bookkeeping still happens before entering the final assault.

The [first recorded session](captures/2026-11-14-session-01.md) independently shows HP increasing by 0.10 per completed turn and food decreasing by five for a party with nine servants. An elves' gift adds ten food before the five-unit consumption. These observations corroborate the binary calculations; they do not independently test every boundary above.

The remake implements the dispatch, bookkeeping, forest, and occupied gnome branch of the hollow tree, with explicit placeholders for other encounters. Some event bodies and their random calls are still missing, so subsequent event sequences will differ even with equal initial seeds. The remake intentionally uses `double` for HP and experience without Real48 emulation, as recorded in `CHANGES.md`. Food is currently integer; whether inventory calculations require visible fractional quantities remains unresolved and must be checked as those mechanics are recovered. The final-assault transition is still deferred with the Keep implementation; `start_final_battle()` remains its intended stub entry point.

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

## Gnome's announced food gift

In the occupied branch of the hollow-tree routine, the original announces a gift of 3–8 food (`Roll(6) + 2`, `0x6876`–`0x68C9`) but never adds it to inventory on this path. It proceeds directly to a possible healing ointment and then exits the encounter. This matches the live capture: the gnome announced three food, yet food changed from 95 to 90 after ordinary five-unit turn consumption.

The remake intentionally fixes this classic bug: add the announced 3–8 food to inventory before normal turn consumption. With the captured example's starting food of 95, a gift of three and consumption of five now leave 93 food, rather than the classic's 90. This difference is recorded in [CHANGES.md](../CHANGES.md#bug-fixes); the original capture remains evidence of classic behavior.

After entering slot 6's tree, `Roll(100) < 56` selects the occupied gnome branch (`0x67C0`–`0x67D4`). If HP is below `level * 10`, the gnome rolls 1–5, announces that many HP restored, and adds it without a subsequent cap (`0x68D8`–`0x696A`). The captured five-HP ointment changed 49.30 HP to 54.40 after turn healing. The remake implements this occupied branch, including its pause, and pauses after declining entry as the original does (`0x6C9C`–`0x6CBB`). The empty-home branch and separate slot 13 tree remain unfinished.

## Overlord's Keep prerequisite

The gate tests `level < 20 OR NOT staff` at `0xA106`–`0xA121`. Level 20 is sufficient when the Staff is held. On a successful prime-command comparison it sets `DS:0x04F8` to true (`0xA23B`–`0xA23E`); a failed comparison removes the Staff (`0xA2A2`–`0xA2A5`). The command input and comparison have not yet been implemented in the remake.

## Implementation checks, 2026-11-14

`make` succeeds, retaining the existing warning about the unused final-battle stub. A temporary C harness built with AddressSanitizer and UndefinedBehaviorSanitizer checked bookkeeping against the documented boundaries: HP at and just below the limit, zero versus negative food, starvation with one or two servants, experience exactly five versus above five, one promotion per turn, dead-player experience reset, ring growth past 100, plague death, and byte wraparound. It also reproduced the captured forest resistance/elves state change with a selected seed. These are remake checks against the recovered rules, not additional observations of original-game boundary behavior.

A terminal-based check of the full remake passed character creation, an encounter, and the next status with the expected HP and food changes. All 18 dispatch call sites, including guarded alternatives, were checked directly against the executable bytes. `git diff --check` passed. Most event bodies remain outside this validation; exact Real48 emulation is intentionally outside the remake's requirements.
