# Reconstruction checklist

Use this strategy and work index alongside the core rules in [AGENTS.md](../AGENTS.md#reconstructing-the-game). Rules and binary offsets belong in [mechanics.md](mechanics.md); observations and possible branches belong in [events.md](events.md). Update this index after each research session without duplicating those detailed ledgers.

## Choosing and finishing a target

Choose one bounded branch, identify its evidence gaps, inspect the binary, probe the original, implement, and compare. Prefer closing well-understood gaps before opening another large location. Use ordinary play periodically for discovery and interactions; decline or leave completed branches through normal game choices when useful, retaining every encountered screen and input.

Select common, consequential gaps and recovered rules awaiting implementation. Split large locations into bounded branches. Use binary control flow and controlled seeds/player states to resolve specific uncertainties about choices, resource boundaries, item prerequisites, probabilities, and fatal outcomes. Retain reproducible tools and compact checkpoints to avoid repeating established research.

Mix in ordinary play after several completed targets, when an interaction needs checking, or when discovery would help choose the next target. Skip fully reconstructed branches only through normal in-game choices, unless exercising a regression or interaction; memory/RNG edits make a run a controlled experiment. Capture every encountered event, including skipped branches and unexpected events outside the target. Save texts, choices, inputs, resulting screens, and visible state changes in `doc/captures/`, linking summaries from events.md and rules from mechanics.md. Investigate surprises, but record unrelated unresolved work for later rather than automatically expanding implementation scope.

For each target:

- [ ] Recover exact texts, choices, prerequisites, arithmetic, and random-call order; link evidence and identify unresolved rules.
- [ ] Capture consequential original-game outcomes and relevant boundaries, separating ordinary play from controlled experiments.
- [ ] Implement the verified behavior, leaving explicit placeholders for unresolved branches and recording intentional differences in [CHANGES.md](../CHANGES.md).
- [ ] Compare resulting state and player-visible flow, including pauses and RNG progression where relevant; retain useful probes/checkpoints and run affected checks.
- [ ] Update this checklist with the completed scope, validation limits, and next gap.

The boxes above are a reusable workflow, not claims that every target needs every possible seed or a new test harness. Completion requires evidence for the stated scope. A captured example alone does not establish a probability or make an entire event complete.

## Next targets

- [x] Storms: implemented and compared across all eight types, shelter, zero/small parties, tidal losses, and starvation. [Evidence and limits](storm-illness-research.md).
- [x] Servant illness: implemented and compared for medicine allocation, zero/one-sex parties, excessive cures, protection, and numeric input. Original excessive cures are preserved; integer input now follows the intentional shared-reader policy in CHANGES.md. Original bare-token/input-length behavior remains research-only. [Evidence and limits](storm-illness-research.md).
- [x] Pan: implemented all 20 decoded questions, random answers, and prime-command generation; compared success, failure, decline, invalid answer keys, and command replacement/preservation. [Evidence and limits](pan-research.md).
- [x] City/Village: shared servant market, tavern, store, inn, and both complete location wrappers; City marketplace/cleric/muggers and Village pickpocket. Forty controlled comparisons. [Evidence and limits](town-research.md).
- [x] Mountain trail/guru: climb, refusal, ring removal, optional Staff loss, and pause implemented; five controlled comparisons. [Evidence and limits](mountain-research.md).
- [x] Gypsy camp: Dagger’s Hand and all 35 fortune-teller hints implemented; controlled comparison scope in [research notes](gypsy-research.md).
- [x] Grass hut: all occupants/finds, hermit outcomes, dragon combat, and gate-creature combat/rewards implemented. [Evidence and limits](hut-research.md).
- [ ] Town follow-up: compare City shared-service purchases plus the bargain marketplace in one visit, and cleric/ring/mugger interactions. Continue ordinary integration play for discovery.
- [ ] Keep command branch (deferred in priority until endgame work): recover exact input/comparison behavior and connect successful entry to the final-battle stub after turn bookkeeping. Check level/Staff gates, wrong command, and correct command.
- [ ] When returning to endgame work, run an ordinary integration pass checking Pan's command followed by the Keep.

City/Village took priority at the user's request; the Keep is for later endgame work. The separate slot-13 tree is the next location candidate: verify its independent flow and the later good-deed scenes before sharing any code. Split specialized combat work into bounded targets when selecting it.

## Core systems

“Recovered” means the documented scope has binary evidence. “Partial” means only some rules or branches are established. Validation describes what has actually been checked, not exhaustive parity.

| System | Rules / implementation | Validation and next gap |
| --- | --- | --- |
| RNG | Recovered / implemented | State progression compared in combat, river, and tree cases. Original startup seed remains unresolved. [Rules](mechanics.md#random-number-generation) |
| Character generation and servant prices | Recovered / implemented | Ordinary initial statuses and binary arithmetic; no retained dedicated comparison suite. Original title says levels 7–10; executable generates 7–9. [Rules](mechanics.md#character-generation) |
| Event dispatch and substitutions | Recovered / implemented | All dispatch call sites checked against binary. Missing event bodies still change subsequent RNG sequences. [Rules](mechanics.md#event-selection) |
| Turn healing, experience, ring/plague progression, food, starvation, death | Recovered / implemented | Temporary boundary harness plus encounter comparisons; no retained standalone boundary suite. Successful Keep transition remains unconnected. [Rules and check limits](mechanics.md#main-turn-loop) |
| Band choices and shared combat | Recovered / implemented | Twelve retained controlled comparisons via `./check_combat.py --sanitize`. Remaining validation gaps include simultaneous deaths and resource boundaries; specialized fights are separate. [Ledger](combat-research.md) |
| Question data | Recovered / extractor and gameplay implemented | All 20 populated records are used and checked against the original data. Record 3 replaces `=` with a space as recorded in CHANGES.md. [Format](data-format.md) |

## Main-game event coverage

Slot numbers follow the [recovered dispatch](mechanics.md#event-selection). “Placeholder” means the event body is unfinished even if an introductory message exists. An implemented row can still have validation gaps; consult the last column before treating a branch as completed for ordinary-play skipping.

| Slot | Event or branch | Rules / implementation | Validation and next gap |
| --- | --- | --- | --- |
| 1, 2 | Village/city entry and visit sequence | Recovered / implemented | Both wrappers share services; controlled entry/decline/skip order and ring warnings. [Evidence and limits](town-research.md) |
| 1, 2 | Servant market | Recovered / implemented | Sale caps, exact funds, male-first spending, unaffordable request checked. Shared integer policy applies. [Rules](mechanics.md#servant-market) |
| 1, 2 | Tavern: drinks, socializing, darts | Recovered / implemented | All drink outcomes including fatal poison, both recruitment sexes/looks gate/decline, darts win/tie/loss/ring checked. Remaining combinations in [ledger](town-research.md). |
| 1, 2 | Store | Recovered / implemented | All merchandise, exact/insufficient funds, supply retries, owned transport prompts/draws checked. Prices follow binary. [Rules](mechanics.md#general-store) |
| 1, 2 | Inn | Recovered / implemented | All rooms, uncapped healing, bandaging, exact/insufficient funds, rat food loss checked. Rat roll at zero food lacks retained probe. [Rules](mechanics.md#inn) |
| 1, 2 | Pickpocket, extra marketplace, cleric, muggers | Recovered / implemented | Village-only pickpocket; City-only marketplace/cleric/muggers. Purchases, cures, ring protection, theft/abort checked; cross-service combinations remain. [Ledger](town-research.md) |
| 3 | Gypsy camp: Dagger's Hand and advice | Recovered / implemented | Betting, injuries, both-hit outcome, repeat rounds, last-coin results, advice affordability, and all 35 hints compared with the original. [Evidence and limits](gypsy-research.md). |
| 4; guarded 8, 9 | Band encounter | Recovered / implemented | Twelve combat scenarios cover payment, escape, forced fighting, and shared combat. [Ledger and limits](combat-research.md) |
| 5 | Thicket/laboratory, potions, pentacle | Partial / placeholder | Laboratory entry and wizard victory observed; recover encounter selection, exploration, potion/pentacle effects and conditions. [Evidence](events.md#thicket-in-the-woods) |
| 6 | Occupied gnome home | Recovered / implemented | Several ordinary food/healing observations. Food addition intentionally fixed; healing threshold/over-cap boundaries lack retained original probes. [Rules](mechanics.md#gnomes-announced-food-gift) |
| 6 | Empty gnome home and decline | Recovered / implemented | Eight retained comparisons via `./check_tree.py --sanitize`: all trunk results, held-Staff substitute, theft, leave, decline. [Scope](mechanics.md#empty-gnome-home) |
| 7 | Grass hut | Recovered / implemented | All finds and occupants, care with/without supplies, hermit outcomes, dragon/charm/poison, gate creature types/rewards, and death compared. No good-deed reward for the man or puppy in this routine. [Evidence and limits](hut-research.md). |
| 8 | Mountain trail/guru when ring held | Recovered / implemented | Five controlled comparisons cover declining, refusing, acceptance with/without Staff, ring power 1/100, invalid keys, and Talisman preservation. [Evidence and limits](mountain-research.md). |
| 9 | Servant illness | Recovered / implemented | Controlled medicine, excess-cure, zero/one-sex, protection, and input comparisons; no-servant delay restored. Shared nonnegative integer parsing intentionally differs from Pascal input; original bare-token/input-length behavior remains research-only. [Validation](storm-illness-research.md) |
| 10 | Storms | Recovered / implemented | Controlled checks for all eight types, two shelter durations, zero/small/one-sex parties, and starvation; requested delays checked against dots and binary loop. [Validation](storm-illness-research.md) |
| 11 | Dark Castle: dungeon, throne room, galley | String findings / placeholder | Recover room flow, traps/escape, theft timing, crystal ball, rewards, and Mad King triggers. [Evidence](events.md#dark-castle-of-the-mad-king) |
| 12 | Enchanted forest | Recovered / implemented | Ordinary nymph resistance/elves gift and temporary remake harness. Capture fatal lure, ring exclusion, faerie plague cure versus healing, and decline. [Rules](mechanics.md#enchanted-forest) |
| 13 | Separate empty-tree routine | String findings / placeholder | Inspect independently; do not reuse slot 6's rules without verifying them. [Evidence](events.md#hollow-tree-with-a-door) |
| 14 | Flowing river | Recovered / implemented | Five saved cases: harmless vision, both items, held items, poisoned champion, decline. Comparison harness was temporary; fixtures retained. [Ledger](session-03.md#controlled-river-checks) |
| 15 | Statue of Pan | Recovered / implemented | Five retained DOS comparisons cover success, failure, decline, invalid answer keys, command replacement/preservation, and question records 1, 3, 20. All 20 embedded records checked against data. [Evidence and limits](pan-research.md) |
| 16 | Keep prerequisite rejection | Recovered / implemented | Ordinary rejection; temporary harness checked level 19 with Staff and level 20 without it. [Rules](mechanics.md#overlords-keep-prerequisite) |
| 16 | Keep command and final-assault transition | Partial / placeholder | Gate, success flag, and wrong-command Staff loss inspected; input/comparison and transition remain unfinished. [Rules](mechanics.md#overlords-keep-prerequisite) |

## Cross-event and deferred work

| Area | Current evidence and next gap |
| --- | --- |
| Anthrax combat | Full ordinary victory captured, including drain, plague, servant deaths, and rewards; recover its separate arithmetic and saving throws. [Observations](session-04.md#anthrax-observations) |
| Dragon and gate-creature fights | Recovered / implemented for hut callers; other callers remain unfinished. Controlled normal fights, charm, poison, rewards and death. Not every damage-table result or simultaneous-death combination has a retained probe. [Evidence](hut-research.md). |
| Mad King and other specialized fights | Separate routines remain unfinished. Establish callers, prerequisites, damage, rewards, and death behavior independently of shared combat. [Combat overview](events.md#combat) |
| Hidden state and interactions | Some ring/plague/protection behavior recovered. Track remaining acquisition/removal paths, dragon charm, command changes, quest items, and effects across event boundaries. [Known gaps](events.md#additional-player-state-to-investigate) |
| Numeric behavior | HP/experience precision choices are recorded in CHANGES.md. Check gold/food calculations for player-visible fractions as new mechanics are recovered; extreme integer overflow is unresolved. [Guideline](mechanics.md) |
| Final battle | Deferred: reconstruct `FINASSLT.CHN` later. Keep the separate exit stub as the entry point until then; wiring the main game's successful Keep branch is still in scope. [Scope](../AGENTS.md#game-structure-and-initial-scope) |

Raw captures live in the ignored `doc/captures/` directory. Keep compact controlled fixtures and their configurations alongside research notes so useful comparisons remain reproducible without those captures. Available capture/check commands are documented in [dosbox-tool.md](dosbox-tool.md).
