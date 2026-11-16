# Illness-state persistence — G6

Completed 2026-11-16. Two controlled two-event chains close G6 in [remaining gaps](remaining-gaps.md#bounded-completion-checklist): player plague is acquired, progresses, and is cured; a potion grants servant-illness protection that persists into the next slot-9 dispatch. Both chains match the existing remake without gameplay changes.

## Rules and evidence

The component rules are already recovered in [Anthrax combat](mechanics.md#anthrax-combat), [potions](mechanics.md#potions), [forest](mechanics.md#enchanted-forest), [dispatch](mechanics.md#event-selection), and [turn bookkeeping](mechanics.md#end-of-turn-bookkeeping). Reinspection of `BRAMINAR.COM` confirms plague acquisition at `0x48B2`–`0x48BA`, the protection flag assignment at `0xE777`–`0xE77A`, the slot-9 protection gate at `0xF065`–`0xF083`, and the faerie cure at `0x8F0B`–`0x8F0E`. Plague uses the player's separate progression byte; servant protection controls which event slot 9 calls.

[Raw captures](captures/2026-11-16-illness-chains.txt) retain every screen, input, and initial/input/intermediate/final state. The two added [thicket fixtures](thicket-checkpoints.json) are `plague-to-faerie` and `protection-to-band`. These are controlled experiments through `dosbox_game.Game`, not ordinary play. Temporary player state, servant prices 8/13, and RNG are set only before each chain; no edits occur between its encounters. Original DOS files remain untouched. All four encountered events are retained: laboratory/Anthrax then forest, and laboratory/potion then band. No unrelated encounters are skipped.

Both chains use the established thicket baseline except plague starts at zero: female, level eight, 50 HP, Black hair, bad looks, five servants of each sex, twenty gold, 100 food, three medicine, two swords, three bows, four arrows, one dagger, horse/cart, zero experience, and no poison, ring, quest items, charm, or illness protection. The initial displayed status can predate the edits; `STATE initial` records the actual setup. Seeds select short reproducible sequences; exact RNG equality is not a completion requirement.

## Compared sequences

| Chain | First encounter and intermediate state | Later encounter and final state |
| --- | --- | --- |
| `plague-to-faerie`, seed 1146089 | Enter the laboratory and defeat Anthrax with the first attack. His killing-blow retaliation casts plague; saving throw fourteen fails. Plague is five at the combat and laboratory exit pauses, then ten after bookkeeping. Laboratory loot leaves sixty males/seventeen females, 234 gold, 131 food, two experience, 50.10 HP, and bad looks. No potion or pentacle appears. | Enter the next forest. No nymph appears; elves give seventy-eight food and restore looks, then the faerie cures plague without healing HP. Plague stays zero through bookkeeping. Final HP 50.20, food 170, good looks, medicine three; party, gold, and equipment are preserved. |
| `protection-to-band`, seed 203267 | Enter the laboratory with no wizard, then drink its single protection potion. Protection changes zero → one at the exit pause and stays one after bookkeeping. Intermediate state: forty-seven males/thirty-three females, eighty-six gold, eighty food, 50.10 HP, plague zero. No pentacle appears. | The next dispatch is slot 9 and produces a band rather than sickness/treatment prompts. Choose G and pay seven males, ten females, and five gold. Final party forty males/twenty-three females, eighty-one gold, forty-eight food, 50.20 HP, protection one, medicine three, plague zero. |

The plague chain captures infection before turn progression and cure before the next progression pass. HP receives only ordinary 0.10 healing on each turn, corroborating that faerie plague treatment does not also restore full health. The protection chain retains its original intermediate RNG state; applying the recovered selector to that state identifies the next slot as nine. This establishes the guarded dispatch rather than inferring its slot from band text, which is shared with other slots. The remake compares resulting state and flow without asserting RNG equality.

## Reproduction and completion limits

Reproduce just these chains and compare all retained thicket cases:

```sh
python3 tools/thicket_probe.py plague-to-faerie protection-to-band > doc/captures/2026-11-16-illness-chains.txt
python3 tools/check_thicket.py --sanitize
```

The existing tool now supports these two follow-up events and writes both cases by name into its fixture file; `--output PATH` supports a separate file. The existing two-turn C harness invokes the actual event dispatch and bookkeeping. Comparisons cover state at inputs and between turns, final supplies/flags/health, normalized narrative, prompt order, input counts, and requested delays. Normalization additionally accepts the already approved band typo corrections. RNG remains recorded research metadata and is no longer asserted by the thicket checker.

All 63 thicket comparisons pass with address and undefined-behavior sanitizers. The new plague chain requests the entry's half second plus the plague spell's half second; the protection chain requests the entry's half second, one second for its potion, and one second after band payment. No new C harness or gameplay correction was needed.

Existing `potion-9` evidence covers a laboratory plague cure; [City cleric comparisons](town-research.md#g3-city-interactions) cover cleric cure; [G1 forest evidence](forest-research.md) covers plague cure versus healing. These are reused without repeating all cure combinations. Fatal progression is already covered by [G4](startup-turn-research.md). No ordinary integration session or additional illness probe is required for G6; ordinary play remains G9. G7 charm/quest persistence is next.
