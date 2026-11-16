# Enchanted forest outcomes — G1

Completed 2026-11-16. This bounded follow-up closes G1 in [remaining gaps](remaining-gaps.md#bounded-completion-checklist). All six missing representative outcomes agree with the existing remake; no gameplay or intentional-difference change was needed.

## Rules and evidence

Reinspection of `dos/BRAMINAR.COM` file offsets `0x8BA7`–`0x8F9A` confirms the [existing forest rules](mechanics.md#enchanted-forest): optional nymph and fatal lure, elves only for living players without a ring, party-sized food, appearance restoration, and a faerie that cures plague or heals HP. Entered paths pause; declining does not. The strict chance comparisons come from binary control flow, not the frequency of these captures.

The gender test at `0x8C14` reads the word at `DS:0260`. Its zero branch uses `You are confronted by a beautiful Female Nymph.` (Pascal string at `0x8C25`); the other uses `You are confronted by an attractive Male Nymph.` (`0x8C64`). The remake's M/F selection matches these strings. The new fatal-lure capture uses a female player and observes the male nymph; the earlier ordinary session observed a male player's female nymph. No extra gender-only probe was needed.

The [raw transcript](captures/2026-11-16-forest-probes.txt) contains six controlled experiments, not ordinary play. `forest_probe.py` uses the existing `dosbox_game.Game` runner and edits temporary player memory and RNG before each dispatch. No original DOS files are changed. Each encountered event is a forest; there are no unrelated encounters or skipped branches. Every submitted input, screen, initial state, and available event/final state is retained in the transcript and [compact fixtures](forest-checkpoints.json).

Baseline: female, level eight, 50 HP, Black hair, good looks, five servants of each sex, eight gold, 100 food, three medicine, two arrows, zero experience, no other equipment, transport, quest items, ring, or plague. Configurations specify exceptions. The preexisting status screen can show the preceding case's values until the next turn redraws it; `STATE initial` records the actual controlled setup. Seeds select outcomes reproducibly and are not a parity requirement.

## Compared outcomes

| Case | Controlled setup / choice | Original event result | After bookkeeping |
| --- | --- | --- | --- |
| Decline | N | Immediately returns to status; no forest exit pause | 50.10 HP, 95 food |
| Ring exclusion | Ring 20, bad looks; Y | No elves, food gift, appearance restoration, or faerie; exit pause | 50.10 HP, 95 food, ring 25, looks still bad |
| Faerie cure and looks | Plague 50, bad looks; Y | Elves add eleven food and restore looks; faerie clears plague without healing; exit pause | 50.10 HP, 106 food, plague zero, good looks |
| Faerie healing | Injured, no plague; Y | Eleven food; faerie sets HP to 80; exit pause | 80 HP, 106 food |
| Faerie at full health | 80 HP, no plague; Y | Same healing announcement, HP stays 80; eleven food; exit pause | 80 HP, 106 food |
| Fatal lure | Y; failed resistance | Male nymph lures player; HP zero, no elves or food gift; exit pause | `Game Over.` after the pause |

Other captured inventory and progression values are unchanged except for the documented turn effects. On fatal lure, the fixture records state at the final event pause and the subsequent death screen. It does not claim a post-exit DOS memory state.

The decline's `You move on.` line is established by the binary at `0x8F7B`–`0x8F94`; the original clears it before the next captured status screen. The checker verifies that line in the remake, the single entry input, and the original post-turn state. The other five event narratives, including their pauses, are compared directly with captured screens.

## Reproduction and completion limits

From the project root:

```sh
python3 forest_probe.py > doc/captures/2026-11-16-forest-probes.txt
python3 check_forest.py --sanitize
```

Named case arguments select an outcome; `--output PATH` allows a separate fixture file. The default fixture file is updated by case name when selecting cases. Fatal lure runs last so the runner never needs to continue from exited DOS memory.

All six comparisons pass with address and undefined-behavior sanitizers. The checker builds the actual remake source through `check_forest.c`, invokes event dispatch and turn bookkeeping, and compares relevant state, normalized complete event text, survival/death, and input counts. Normalization accepts whitespace differences and the already approved nymph typo correction. Fatal bookkeeping executes in the remake, but only the event state and visible death outcome have an original reference. Exact RNG equality is not checked. The fixtures suffice without DOSBox or ignored raw transcripts.

The already captured ordinary resistance/elves-food path was not repeated. Full-health behavior is covered once; above-cap and exact probability-equality cases, extra seeds, and gender-only duplicate runs are outside G1's completion requirements. No ordinary integration session was added; that remains the separately bounded G9 task. G1 is complete; the next checklist item is G2, occupied-gnome comparisons.
