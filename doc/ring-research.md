# Ring persistence — G5

Completed 2026-11-16. Two controlled two-event chains close G5 in [remaining gaps](remaining-gaps.md#bounded-completion-checklist). Acquisition persists into a later forest visit; guru removal restores access to the elves' gifts. Both chains match the existing remake without gameplay changes.

## Rules and evidence

Existing [combat rules](mechanics.md#shared-combat), [guru rules](mechanics.md#mountain-trail-and-guru), and [forest rules](mechanics.md#enchanted-forest) establish the component behavior. Reinspection confirms the combat ring test/store at `BRAMINAR.COM` file offsets `0x3C5B`–`0x3CEA`, the guru's zeroing of ring power at `0x76CC`–`0x76CF`, and the forest's living-player/zero-ring gate at `0x8D76`–`0x8D9C`. A positive ring excludes elves regardless of its strength. Normal turn bookkeeping grows a positive ring below 100 by five; clearing it prevents regrowth.

[Raw captures](captures/2026-11-16-ring-chains.txt) and [compact fixtures](ring-checkpoints.json) retain each screen, input, and initial/input/intermediate/final state. These are controlled experiments using `dosbox_game.Game`, not ordinary play. Initial temporary player memory and RNG are edited once per chain; no state or RNG edits occur between its two encounters. Original DOS files are untouched. Every encountered event is retained: band then forest, and mountain/guru then forest. No unrelated encounters occur.

Both chains start female, level eight, 50 HP, Black hair, bad looks, five servants of each sex, eight gold, 100 food, three medicine, two arrows, zero experience/plague, and no weapons, transport, quest items, charm, or illness protection. The acquisition chain starts without a ring and with one poison; removal starts with ring power twenty and no poison. Status displayed before the initial memory edits can be stale; `STATE initial` is the actual setup. Seeds select the bounded sequences, not encounter-frequency evidence or a parity requirement.

## Compared sequences

| Chain | First encounter and intermediate state | Later forest result |
| --- | --- | --- |
| `acquire-then-forest`, seed 6661 | Choose F against Rust Golems and use poison. Blades are absent, so rust removes nothing. Victory adds 1.5 experience, awards the ring at five, and adds eleven males, seventeen females, four gold, and twenty-eight food. Bookkeeping leaves ring ten, 50.10 HP, sixteen males/twenty-two females, twelve gold, 109 food, and no poison. | Enter the forest with the acquired ring intact. No nymph appears; no elves, food gift, appearance cure, or faerie follow. After the pause and bookkeeping: ring fifteen, 50.20 HP, ninety food, looks still bad. |
| `guru-then-forest`, seed 10530 | Climb and accept the powers of good. The guru removes ring twenty; no Staff is held. At the next status, ring stays zero, HP 50.10, food ninety-five, looks still bad. | Enter the forest without a ring. No nymph appears; elves give eleven food and restore looks, with no faerie. After the pause and bookkeeping: ring zero, 50.20 HP, 101 food, good looks. |

The acquisition fixture retains ring five at the combat exit pause, ten between encounters, ten at the forest exit pause, and fifteen afterward. Removal retains zero at the guru's pause, between encounters, and through the forest. Input/state comparisons cover these transitions as well as the visible narratives and final supplies.

## Reproduction and completion limits

From the project root:

```sh
python3 tools/ring_probe.py > doc/captures/2026-11-16-ring-chains.txt
python3 tools/check_ring.py --sanitize
```

Named case arguments select a chain; `--output PATH` supports a separate fixture file. The probe reuses the forest tool's baseline setup/readers and adds the initial poison setting. The checker reuses `tools/check_thicket.c`'s existing two-turn dispatch/bookkeeping harness; no new C harness is needed. It compares input/intermediate/final state, normalized captured text, prompt order, input counts, and pauses. Normalization accepts whitespace, servant terminology, and the already approved combat/guru typo corrections. Both chains pass with address and undefined-behavior sanitizers. Exact RNG equality is not checked.

Existing component evidence already covers forced fighting, attack bonuses, ring acquisition, guru alternatives, and cleric removal/protection loss within a City visit. G5 does not require every acquisition/removal pairing or waiting for forced-fight strength after earning a ring. These two chains establish persistence in each direction; further ring probes are unnecessary absent a concrete normal-play discrepancy. No ordinary integration session was added; that remains G9. G6 illness-state chains are next.
