# Grass hut research

## Evidence and scope

The entire slot-7 hut (`dos/BRAMINAR.COM`, file offsets `0x6CC5`–`0x7579`) is recovered and implemented. This includes its dragon caller and astral gate, so this target also recovers the separate dragon routine (`0x56C6`–`0x5DBD`) and gate-creature routine (`0x4B67`–`0x56C5`). The shared band-combat routine is not used for either fight. [Mechanics](mechanics.md#grass-hut) contains the exact draws, damage tables, text/state discrepancies, prerequisites, and reward order.

Control-flow inspection resolves an earlier string-grouping assumption: the sick old man and puppy in this hut do not award good-deed rewards. Similar scenes and reward strings later in the executable are separate code, now independently recovered and compared with the [slot-13 watched tree/hut](watched-home-research.md). The old ordinary empty-hut visit remains in [session 04](session-04.md); it establishes that visit only.

## Controlled captures

[Raw capture](captures/2026-11-15-hut-probes.txt) contains 41 controlled cases, all explicitly labelled as memory/RNG experiments rather than ordinary play. `hut_probe.py` resets the temporary emulator state before every case. No original DOS files are modified. Each encountered event is a hut and its nested encounters; every input, combat-round pause, and intermediate waiting screen is captured. There are no unrelated encounters to skip.

The baseline is level 8, 50 HP, Black hair, good looks, five servants of each sex, 20 gold, 100 food, three medicine, two swords, three bows, four arrows, one dagger, horse/cart, and illness protection. Ring, plague, poison, Staff, Talisman, charm, and experience are zero; servant prices are fixed at 8/13. Individual cases vary HP, food, medicine, poison, female count, quest items, charm, transport, and choices. Initial screenshots precede memory changes; the `STATE initial` records establish each experiment's actual state.

[Saved checkpoints](hut-checkpoints.json) retain state and RNG at every input, the answer and visible screen, and final state. `./check_hut.py --sanitize` compares these against the actual remake code, including quest items, charm, key counts, text order, and requested delay totals.

## Captured outcomes

| Scope | Cases and observations |
| --- | --- |
| Entry | Decline at occupancy rolls exactly 50 and 51. Both announcement variants end in a pause. |
| Old man | Accept with medicine, accept without medicine, refuse. One medicine is consumed when present; comforting needs no medicine. No reward or experience; accepted care requests a one-second delay. |
| Puppy | Feed, pet without food, spend the last food, refuse. Accepted care delays one second. With no food, normal bookkeeping reduces five servants of each sex to four; this is starvation, not a direct puppy effect. |
| Quiet finds | All nine outcomes, including Staff success at roll 4, failure at 5, held-Staff failure despite a qualifying roll, all equipment/servant/gold finds, and gate closure at 13. |
| Hermit refusal | Leave the hermit alive; kill him and use charm or poison against the dragon. Killing grants no possession-doubling/betrayal outcome. |
| Hermit challenge | Charm/poison victories and a normal fight; doubling at exactly 21, betrayal at exactly 20. Announced betrayal clamps its count to female servants but leaves that count unchanged, including a zero-female case. |
| Dragon charm | Submission bypasses level/HP/poison draws, adds experience, leaves poison unused, and preserves the charm counter at one. Charm acquisition was deferred in this session; it is now implemented and compared in [thicket research](thicket-research.md). |
| Dragon combat | Full unpoisoned battle; poison offered and declined; poison used. Equipped blades/bows do not change attack rolls or consume arrows. The retained normal fight ends at 71.10 HP after turn healing, with five experience and doubled possessions. |
| Gate creatures | All five types captured with successful poison; all six reward indices. Held Staff/Talisman substitutes both add 250 food despite the 50-food text. Those cases end with 345 food after normal consumption. |
| Creature combat | Gate opening at exactly 12, a full unpoisoned Cerberus fight, and a Cerberus saving against poison followed by combat. Extra random damage draws and retaliation after the player's attack are included in the state/RNG comparison. |
| HP reward | Starting above the level cap, the restore reward sets HP to the old cap before promotion. The captured level-8, 90-HP case ends at level 9, 90 HP, six experience after a single promotion. |
| Death | Dragon and creature death, including round pauses and the outer hut pause. No hermit outcome or creature reward is drawn after death. |

Fatal fixtures retain final event state before bookkeeping, using the last outer-pause checkpoint; DOS reclaims DS when the original exits. The remake checker suppresses bookkeeping for these comparisons, so these are not claims about post-exit memory. Nonfatal fixtures include normal turn bookkeeping and any resulting promotion or starvation.

## Verification and limits

The 41 hut comparisons pass with address and undefined-behavior sanitizers. `make` and the existing combat, tree, storm/illness, Pan, City/Village, mountain, and Gypsy sanitizer suites pass. Python syntax and whitespace checks pass. The existing unused-final-battle-stub compiler warning remains.

Binary evidence establishes the full damage tables and ranges; the retained fights do not exhaust every attack/counterattack combination, poison-save equality boundary for every creature, simultaneous deaths, or extreme integer overflow. Fractional combat constants follow the documented double-precision policy. Charm is tested through controlled state, not through ordinary acquisition. Other callers were not validated in this session; the pentacle caller is now implemented and compared in [thicket research](thicket-research.md). This session contains targeted controlled reconstruction, with no new ordinary-play pass.
