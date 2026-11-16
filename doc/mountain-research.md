# Mountain trail and guru research

## Evidence

Binary inspection of `dos/BRAMINAR.COM`, file offsets `0x757A`–`0x76FD`, establishes the entire event body: two Y/N questions, optional Staff removal, ring removal, and a common keypress pause. The dispatch ring gate was already recovered. Detailed rules are in [mechanics](mechanics.md#mountain-trail-and-guru).

[Raw capture](captures/2026-11-15-mountain-probes.txt) contains five controlled experiments, not ordinary play. `tools/mountain_probe.py` resets temporary emulator state to level 8, 50 HP, five servants of each sex, eight gold, 100 food, three medicine, two arrows, no plague, and zero experience. The Talisman is held throughout. Seed 10473 selects slot 8; its sole draw leaves RNG state 908984402. All encountered events were mountain trails; no unrelated encounters were skipped.

## Captured outcomes

| Case | Choices and initial items | Original result after turn bookkeeping |
| --- | --- | --- |
| decline | N; Staff and ring power 5 | Staff retained, ring power 10; `You move on.` then pause |
| refuse | Y, N; Staff and ring power 5 | Staff retained, ring power 10; no refusal message, then pause |
| accept-staff | Y, Y; Staff and ring power 5 | Staff and ring removed, then pause |
| accept-no-staff | Y, Y; no Staff, ring power 1 | Only ring-removal message, ring removed, then pause |
| accept-strong-ring | x, Y, z, Y; Staff and ring power 100 | Invalid keys ignored; Staff and ring removed, then pause |

Every case preserves the Talisman and inventory, apart from normal food consumption (100 to 95). HP becomes 50.10, experience stays zero, and no additional RNG calls occur. The remake corrects the original `lookes` typo.

## Validation and limits

`./tools/check_mountain.py --sanitize` compares the saved original states in [checkpoints](mountain-checkpoints.json), RNG progression, normalized event text, and exact input counts. It also checks that EOF at either question ends the event before bookkeeping. `make` and the combat and town sanitizer comparisons pass; the existing unused final-battle-stub warning remains.

The event body has no unresolved branches. These captures do not constitute an ordinary playthrough acquiring a ring and later visiting the guru, nor a new controlled comparison of the already established no-ring band substitution.
