# Recovered game mechanics

This reference records rules recovered from the original binaries, the evidence supporting them, and remaining uncertainties. Keep binary offsets alongside the rules they support. Observed encounters and provisional string findings are catalogued in [events.md](events.md); file layouts and extraction-tool usage are documented in [data-format.md](data-format.md).

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

The remake now uses this generator for character stats, prices, and placeholder event selection. It seeds the state from the current time. The order of random calls within character generation is reproduced, but the original initial seed and full order of random calls elsewhere are not yet reproduced, so identical complete runs across the original and remake are not claimed. Equal event weights remain placeholders.

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

These are generation values, before events and turn bookkeeping. The original's later turn processing adds fractional health and consumes food, so a subsequent status screen can differ from the initial values. Turn processing is not implemented by this change.

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
