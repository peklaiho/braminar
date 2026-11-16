# Combat death and reward ordering (G8)

G8 is complete. Four controlled original/remake comparisons confirm that a killing blow still receives retaliation, both participants can die in the same round, and these four routines grant no survivor rewards. No gameplay correction was needed. These are normal combat rules reached through chosen starting health and RNG, not ordinary-play frequency observations. No original files were modified; no final-assault code ran.

## Controlled outcomes

The probes reuse seeds from existing surviving fights and lower starting HP so the last retaliation is fatal. State is edited only before the encounter; every attack, retaliation, and pause then runs normally. Each final player HP is -0.05, avoiding exact-zero precision boundaries.

| Routine / fixture | Setup | Final opponent HP and messages | Rewards withheld |
| --- | --- | --- | --- |
| Shared Goblin / `simultaneous-death` | Seed 6427, HP 12.57, level 8, unarmed; choose F | -1.42; `You did not survive the fight.` then `The monster is dead.` | Experience stays zero; no ring, servants, gold, or food awarded. Twelve servants of each sex, 100 gold, and 100 food remain at the result pause. |
| Dragon / `dragon-simultaneous-death` | Seed 26677, HP 28.95; accept the hermit's challenge without charm or poison | -1.40; `You did not survive the battle.` then `The dragon is slain.` | Experience stays zero; no hermit doubling or betrayal. Five servants of each sex, 20 gold, 100 food, and equipment remain unchanged. |
| Gate creature / `creature-simultaneous-death` | Seed 286812, HP 16.95; enter the quiet hut's astral gate | Cerberus -0.90; `You did not survive the battle.` then `Cerberus is gone.` | Experience stays zero; no deity reward or recovery. Supplies, quest items, and equipment remain unchanged. |
| Mad King / `king-simultaneous-death` | Seed 1490650557, HP 79.65, ring one, plague zero; enter throne room and send sixteen X keys | -2.60; `You did not survive the battle.` then `The King disappears.` | Experience stays zero and gold stays 20; no flee-before-rematerialization message. Ring and other inventory remain unchanged at the room pause. |

The hut and castle baseline inventories and all per-input states are retained in their fixtures. The shared probe uses its existing twelve-servant/100-gold baseline. King combat retains the ring's attack bonus; its starting HP is below the level-eight cap. None of these cases uses poison, temporary combat-HP edits, or changes between rounds.

Each fight retains its round pause, then its existing result/caller pause, then Game Over. Final state comparisons stop before turn bookkeeping because DOS reclaims DS at exit. Thus food consumption, ring progression, and death bookkeeping are not newly tested here; their existing G4/Keep evidence is reused.

## Evidence and reproduction

Raw captures:

- [Shared Goblin](captures/2026-11-16-combat-order-shared.txt)
- [Dragon and Cerberus](captures/2026-11-16-combat-order-hut.txt)
- [Mad King](captures/2026-11-16-combat-order-king.txt)

Reproduce from the project root:

```sh
python3 tools/combat_probe.py --seed 6427 --hp 12.57
python3 tools/hut_probe.py dragon-simultaneous-death creature-simultaneous-death
python3 tools/castle_probe.py king-simultaneous-death
python3 tools/check_combat.py --sanitize
python3 tools/check_hut.py --sanitize
python3 tools/check_castle.py --sanitize
```

The compact [shared](combat-checkpoints.json), [hut](hut-checkpoints.json), and [castle](castle-checkpoints.json) fixtures preserve the captured states and required outcome text. Hut/castle probes regenerate their named fixtures; the shared fixture was extracted from its raw `CONFIG` and `STATE checkpoint` records. All 13 shared, 43 hut, and 47 castle scenarios pass with address and undefined-behavior sanitizers. Hut/castle comparisons also check normalized captured narrative, prompt order, input counts, and requested waits; the shared case checks both outcome messages and absence of reward announcements. These three checkers now ignore exact RNG state equality while retaining seeds for controlled selection.

## Rules and stopping point

Existing binary rules already establish the independent outcomes and survivor gates: [shared combat](mechanics.md#shared-combat), [dragon](mechanics.md#dragon-combat), [gate creature](mechanics.md#gate-creature-combat), and [Mad King](mechanics.md#mad-king-combat). The captures confirm those rules without requiring changes to the implementation.

Anthrax is a separate, already captured exception: a killing attack followed by fatal lightning still awards wizard experience before the parent skips exploration loot. That award survives until later death bookkeeping clears experience. Reuse the [wizard death evidence](thicket-research.md); do not apply the four routines' survivor-only rule to Anthrax.

The four missing representative outcomes are now covered. Further simultaneous-death seeds, every monster/creature type, attack-table pairings, save thresholds, exact RNG equality, and integer overflow are outside this bounded task. G9 ordinary play and terminal flow is next; the final assault remains excluded.
