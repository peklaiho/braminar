# Five-turn research pass, 2026-11-14

This was ordinary play without memory edits. Every encountered event and input was captured in [the raw transcript](captures/2026-11-14-session-03.txt). Screens can repeat old lines; commands label each snapshot. The game was stopped at the status screen after turn five.

Wayfarer began male, level 9, brown hair, good looks, 58 HP, 14 male and 4 female servants, 19 gold, 100 food, and 3 medicine. No weapons, poison, horse, cart, Staff, or Talisman. Servant prices were 11 male / 13 female.

| Turn | Encounter and choices | Resulting status |
| --- | --- | --- |
| 1 | City: enter Y; market N, tavern N, store N, inn N | HP 58.10, food 91; other inventory unchanged |
| 2 | Drow Elves demand 1 male, 7 females, 10 gold; R fails, forcing combat | After 26 unarmed rounds: player 20.30 HP, enemy -1.45 HP. Loot: 13 males, 3 females, 31 gold, 16 food. Next status: HP 20.40, males 27, females 7, gold 50, food 90 |
| 3 | River: gaze Y | Vision: `A trap door in the Holiday out hotel`. No item or fight. Next status HP 20.50, food 73 |
| 4 | Overlord's Keep | `You cannot attack due to lack of Staff or 20th level.` A keypress returns to status: HP 20.60, food 56 |
| 5 | 9 males and 1 female have Old-Timer's disease; treat 2 males, 1 female | Seven males lost; all 3 medicine consumed. Next status HP 20.70, males 20, females 7, gold 50, food 42 |

The Drow encounter corroborates the existing combat and fleeing rules. The river's harmless vision and the Keep's rejection are now implemented. Illness remains a placeholder, with newly recovered arithmetic documented in [mechanics.md](mechanics.md#servant-illness). City nested branches remain unfinished; this visit establishes the skip sequence and its lack of an additional exit pause on this path.

## Controlled river checks

Separate from the ordinary pass, five experiments edited temporary emulated memory using the existing `Game` API. [Raw captures](captures/2026-11-14-river-probes.txt) label these controlled probes. [Compact final states](river-checkpoints.json) preserve their results outside the ignored captures directory.

Before each case: level 8, HP 50, 12 servants of each sex, 100 gold, 100 food, 3 medicine, 1 poison, zero experience/ring/plague/weapons, no horse/cart. Staff and Talisman are initially absent except in `held`. The seed is set immediately before the main dispatch. Cases share one emulator session, with those fields reset before each case; screens displayed before the next status can therefore contain stale values. Results are read directly from memory after the next status appears.

| Case | Seed | Choice | Outcome |
| --- | --- | --- | --- |
| plain | 312 | Gaze Y | Vision 14; no items or combat |
| both | 798 | Gaze Y | Vision 16; acquire both Staff and Talisman |
| held | 798 | Gaze Y | Same rolls as `both`, both items already held; no duplicate award messages, identical final RNG |
| fight | 324 | Gaze Y, poison Y | Vision 16; Troglodyte champion; two further pauses, one for combat result and one for river exit; XP 2.75 and loot applied |
| decline | 798 | Gaze N | No vision or pause; only the dispatch consumes a random draw |

A temporary C harness exercised the actual `run_event()` and `finish_turn()` under address/undefined-behavior sanitizers. All five cases matched original HP, experience, inventory, quest items, and final RNG; input counts were 2 for harmless visions, 4 for the poisoned champion, and 1 for declining. Separate checks confirmed the Keep rejection pauses once for level 19 with the Staff and level 20 without it. The existing twelve combat regression scenarios also pass with sanitizers. These checks do not establish every combat outcome of the river independently; it calls the already researched shared combat routine.
