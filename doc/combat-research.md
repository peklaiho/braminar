# Combat research, 2026-11-14

This ledger separates ordinary play from controlled experiments. All captures preserve original wording. Intentional remake corrections are listed in [CHANGES.md](../CHANGES.md); recovered rules and offsets are in [mechanics.md](mechanics.md#band-encounters).

## Controlled original-game experiments

`tools/combat_probe.py` generated the captures below using the original executable in DOSBox. It changes only temporary emulator memory, sets a known RNG seed immediately before the next event, and records screens, inputs, configuration, and memory checkpoints. These are deliberately constructed states, not evidence of their frequency during ordinary play. No original DOS files are modified.

Unless overridden below, the probe starts at level eight, 50 HP, twelve servants of each sex, 100 gold, 100 food, no equipment, no poison, no horse/cart, no ring, and zero experience. It chooses F. The name is CombatProbe. Configuration records for the first three probes (bare, arrows, Drow) were added from their recorded invocation arguments after command-config logging was added to the tool; their screens and memory records are unchanged.

| Capture | Seed and overrides | Observed outcome |
| --- | --- | --- |
| [Bare-handed Goblin](captures/2026-11-14-combat-bare.txt) | 6427 | Victory; 1.25 experience; final HP 37.48; 20 males, 23 females, 148 gold, 97 food |
| [Arrow exhaustion](captures/2026-11-14-combat-arrows.txt) | 6427; one bow, two arrows | Both arrows consumed, then unarmed attacks; victory; final HP 40.69 |
| [Blade](captures/2026-11-14-combat-blade.txt) | 6427; one sword | Victory; final HP 38.69; sword retained |
| [Drow equipment loss](captures/2026-11-14-combat-drow.txt) | 6411; one bow, twelve arrows | Bow and all arrows destroyed before combat; victory; 3.00 experience; final HP 15.18 |
| [Rust Golem and poison](captures/2026-11-14-combat-poison.txt) | 6443; two swords, three daggers, one poison; use poison | Swords/daggers destroyed first; poison consumed; no combat rounds; 1.50 experience and normal loot |
| [Payment](captures/2026-11-14-combat-payment.txt) | 6450; choose G | Orc demands two males, five females, seven gold; all deducted; final 10 males, 7 females, 93 gold, 91 food |
| [Insufficient payment](captures/2026-11-14-combat-payment-death.txt) | 6450; choose G; zero males | Assassination and Game Over despite sufficient female servants and gold |
| [Horse escape](captures/2026-11-14-combat-flee-horse.txt) | 6466; choose R; horse, no cart | Both servant counts reduced from twelve to one; escaped; final food 99 |
| [Caught escape](captures/2026-11-14-combat-flee-caught.txt) | 6419; choose R; one poison; use poison | Demon catches the player; poison defeats it; 3.50 experience and loot |
| [Forced fight](captures/2026-11-14-combat-ring-forced.txt) | 6427; ring byte 46 | No F/G/R key read; ring forces combat and boosts attacks; victory; ring reaches 51 at turn end |
| [Ring acquisition](captures/2026-11-14-combat-ring-gift.txt) | 6410; one poison; use poison | Poison defeats Troglodyte; 2.75 experience; awarded ring at five, reaching ten at turn end |
| [Fatal round](captures/2026-11-14-combat-fight-death.txt) | 6427; 0.10 HP | Death in the first round; no loot or experience; outcome pause, then Game Over |

Every positive-HP final status above includes main-loop healing and food consumption. Servant loot increases party food needs; consume food for the enlarged party, not the starting party. After Game Over, DOS regains the data-segment register, so these captures intentionally omit a final memory dump. Death captures still include pre-outcome/final-pause checkpoints where the original is waiting for input, except assassination, which exits without another input prompt.

`./tools/check_combat.py --sanitize` builds the C checkpoint harness in a temporary directory and compares the remake with all twelve saved scenarios in `doc/combat-checkpoints.json`. The harness includes the actual game source, substitutes scripted keyboard reads, and records state at input boundaries and after turn bookkeeping. Inventory, HP, experience, ring, and RNG state match the saved original checkpoints; HP comparisons allow insignificant representation differences. Original snapshots taken during delays may add checkpoints between input boundaries. The raw transcripts are ignored by the repository; the compact JSON fixtures preserve their configurations and memory checkpoints for repeatable tests. The check requires the final surviving state to match and verifies death for fatal cases. It does not launch DOSBox again.

The suite covers equipment effects, arrow exhaustion, poison's HP-roll consumption, the extra armed-round random call, a fatal round, rewards, and ring effects. It does not exhaust all seeds, resource boundaries, simultaneous deaths, or specialized wizard/dragon combat.

## Ordinary playthrough

[Full unmodified session](captures/2026-11-14-combat-01.txt). Inputs and screens are labelled by the runner. No player state or RNG memory was changed in this run. Every encountered event is retained.

Initial character Fighter: male, level seven, red hair, good looks, 31.00 HP, five male and ten female servants, five gold, 100 food, three medicine. Other inventory is empty. Male/female servant prices are eight/thirteen gold.

| Turn | Observed flow and inputs | Result |
| --- | --- | --- |
| 1 | Hollow tree; enter; gnome announces five food and one HP ointment | Next status HP 32.10, food 92; confirms classic's missing food gift again |
| 2 | Village; market; sell two males/five females; decline buying servants and tavern; enter store; buy bow and twelve arrows; buy ten food; decline medicine/horse/cart; enter inn and choose room 3 | Next status HP 35.20, three males/five females, nine gold, 98 food, one bow/twelve arrows |
| 3 | Hollow tree; enter; gnome announces three food and two HP ointment | Next status HP 37.30, food 94; other inventory unchanged |
| 4 | Demon band demanding eight males, one female, ten gold; choose F | Twenty-one rounds; twelve arrows exhausted, then unarmed attacks; player dies at -1.73 HP with Demon still at 31.37; outcome pause, then Game Over |

Village findings: selling the servants raised gold from five to 86, agreeing with the stored prices. Offers were sword 92, bow 31, dagger 86, twelve arrows 16, horse 119, cart 46; all unpurchased except bow/arrows. Ten food cost ten gold. Room 3 cost twenty gold and printed `You lay back and relax for awhile.` followed by female servants bandaging wounds. HP increased by three plus ordinary 0.10 turn healing in this visit; this one outcome does not establish the inn's healing formula.

The store's `You have:` figure followed the gold balance: 86 before the bow, 55 afterward, 39 after the arrows, and 29 after food. This corrects the earlier session's uncertainty about those lines: they do not show current stocks of the offered item. Prices differed from session 01; their generation formula remains to be recovered.

## G8 death-ordering follow-up

The [G8 comparisons](combat-order-research.md) close this ledger's earlier simultaneous-death validation gap. Both participants die, independent outcome messages print, and survivor rewards are withheld; no gameplay correction was needed. All 13 shared-combat scenarios pass with sanitizers. Exact RNG equality is no longer asserted by this checker; seeds remain for controlled selection. Earlier session counts and limits above describe their original scope. Final comparisons stop at the event pause before death bookkeeping.
