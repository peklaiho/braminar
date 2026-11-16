# Occupied gnome home — G2

Completed 2026-11-16. Three controlled comparisons close G2 in [remaining gaps](remaining-gaps.md#bounded-completion-checklist). The existing remake matches the original's healing, text, and pauses, with the already intentional food-gift fix. No gameplay change was needed.

## Evidence and rules

Reinspection of `dos/BRAMINAR.COM` confirms the [documented occupied-home rules](mechanics.md#gnomes-announced-food-gift). After accepting entry, a roll below 56 selects the gnome (`0x67C0`–`0x67D4`). The food roll and announcement at `0x6876`–`0x68D5` do not add food in the original. The HP comparison at `0x68D8`–`0x68EF` skips ointment when HP is already at least `level * 10`. Otherwise the gnome adds 1–5 HP without a subsequent clamp (`0x68F2`–`0x696A`). The branch joins the common exit pause at `0x6CB5`–`0x6CBB`.

[Raw captures](captures/2026-11-16-gnome-probes.txt) retain all three controlled visits, inputs, screens, and initial/event/final states. These experiments edit temporary player memory and RNG through `dosbox_game.Game`; they do not change original DOS files and are not ordinary play. Every encountered event is an occupied slot-6 home, with no unrelated encounters or skipped branches. [Compact fixtures](gnome-checkpoints.json) preserve the comparisons without the ignored transcript.

Baseline: female, level eight, Black hair, good looks, five servants of each sex, eight gold, 100 food, three medicine, two arrows, zero experience, and no other equipment, transport, quest items, ring, or plague. Initial HP is the only difference between cases. Seed 8442 selects the occupied home and a six-food announcement; injured cases receive four HP of ointment. The seed is a reproducibility aid, not a parity requirement. A displayed status can predate the memory edits; each `STATE initial` gives the actual setup.

## Compared outcomes

| Case | Initial HP | HP at the original exit pause | HP after turn bookkeeping | Visible ointment outcome |
| --- | --- | --- | --- | --- |
| Injured | 50 | 54 | 54.10 | Announces four HP restored |
| Healing past cap | 79 | 83 | 83 | Announces four HP restored; no clamp to level-eight maximum of 80 |
| Full health | 80 | 80 | 80 | No ointment announcement or healing |

Each visit asks for entry and ends in one exit pause. Original food stays 100 during the event and falls to 95 afterward for the eleven-person party. The remake adds the announced six food, leaving 106 at the exit pause and 101 after consumption, as required by [CHANGES.md](../CHANGES.md#bug-fixes). Other captured state matches. Above-cap HP receives no extra turn healing; full-health HP also stays unchanged.

## Reproduction and completion limits

From the project root:

```sh
python3 tools/gnome_probe.py > doc/captures/2026-11-16-gnome-probes.txt
python3 tools/check_gnome.py --sanitize
```

Named case arguments select cases; `--output PATH` supports a separate fixture file. The probe reuses the forest tool's baseline state setup/readers. The checker reuses `tools/check_forest.c`'s event-dispatch/bookkeeping harness and the Python state comparison helpers; no new C harness is needed. It compares both event and post-turn state, complete normalized event text, survival, and the two input reads. Only expected food is adjusted by the captured announced gift. Exact RNG equality is not checked.

All three comparisons pass with address and undefined-behavior sanitizers. The cases establish the three requested healing outcomes; extra seeds, every ointment amount, and artificial extreme states are unnecessary. Existing [empty-home comparisons](session-04.md#controlled-empty-home-checks-and-implementation) cover the other slot-6 branch and declining entry. No ordinary integration session was added; that remains G9. G2 is complete; G3 City interactions are next.
