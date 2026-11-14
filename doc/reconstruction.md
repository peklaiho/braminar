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
- [ ] Pan: implement the decoded questions and recovered random-answer/command generation; capture success, failure, and decline.
- [ ] Keep command branch: recover exact input/comparison behavior and connect successful entry to the final-battle stub after turn bookkeeping. Check level/Staff gates, wrong command, and correct command.
- [ ] After these bounded targets, run an ordinary integration pass, choosing unfinished branches and checking interactions such as Pan's command followed by the Keep.

This is a suggested order based on existing evidence, not a requirement to ignore new findings. Guru removal is another small candidate once its remaining flow and text are verified. Split city/village and specialized combat work into smaller targets when selecting them.

## Core systems

“Recovered” means the documented scope has binary evidence. “Partial” means only some rules or branches are established. Validation describes what has actually been checked, not exhaustive parity.

| System | Rules / implementation | Validation and next gap |
| --- | --- | --- |
| RNG | Recovered / implemented | State progression compared in combat, river, and tree cases. Original startup seed remains unresolved. [Rules](mechanics.md#random-number-generation) |
| Character generation and servant prices | Recovered / implemented | Ordinary initial statuses and binary arithmetic; no retained dedicated comparison suite. Original title says levels 7–10; executable generates 7–9. [Rules](mechanics.md#character-generation) |
| Event dispatch and substitutions | Recovered / implemented | All dispatch call sites checked against binary. Missing event bodies still change subsequent RNG sequences. [Rules](mechanics.md#event-selection) |
| Turn healing, experience, ring/plague progression, food, starvation, death | Recovered / implemented | Temporary boundary harness plus encounter comparisons; no retained standalone boundary suite. Successful Keep transition remains unconnected. [Rules and check limits](mechanics.md#main-turn-loop) |
| Band choices and shared combat | Recovered / implemented | Twelve retained controlled comparisons via `./check_combat.py --sanitize`. Remaining validation gaps include simultaneous deaths and resource boundaries; specialized fights are separate. [Ledger](combat-research.md) |
| Question data | Format and selection recovered / extractor implemented | First 20 populated records decoded; game does not yet use them. Record 3 contains a declared source-fragment answer needing an explicit text decision. [Format](data-format.md) |

## Main-game event coverage

Slot numbers follow the [recovered dispatch](mechanics.md#event-selection). “Placeholder” means the event body is unfinished even if an introductory message exists. An implemented row can still have validation gaps; consult the last column before treating a branch as completed for ordinary-play skipping.

| Slot | Event or branch | Rules / implementation | Validation and next gap |
| --- | --- | --- | --- |
| 1, 2 | Village/city entry and visit sequence | Partial / placeholder | Ordinary market/tavern/store/inn visits and city skip sequence. Recover entry/exit conditions and exact pauses. [Evidence](events.md#city-and-village) |
| 1, 2 | Servant market | Prices recovered; transactions partial / placeholder | Observed sales and zero-quantity input. Recover quantity limits, affordability, buying and selling flow. [Prices](mechanics.md#servant-prices) |
| 1, 2 | Tavern: drinks, socializing, darts | Partial / placeholder | Some ordinary outcomes; recover each activity's costs, probabilities, effects, and return flow. [Evidence](events.md#city-and-village) |
| 1, 2 | Store | Partial / placeholder | Purchases observed; recover price generation, quantities, affordability, and equipment effects. [Ledger](combat-research.md#ordinary-playthrough) |
| 1, 2 | Inn | Partial / placeholder | Room 3 lodging and bandaging observed; recover room prices, healing, rats, and prerequisites. [Evidence](events.md#city-and-village) |
| 1, 2 | Pickpocket, extra marketplace, cleric, muggers | String findings / placeholder | Establish grouping, triggers, ring interactions, and effects before implementation. [Evidence](events.md#city-and-village) |
| 3 | Gypsy camp: Dagger's Hand and advice | Partial / placeholder | One game victory and paid advice observed. Recover betting, losses, probabilities, rumor selection, and flow. [Pass](session-04.md) |
| 4; guarded 8, 9 | Band encounter | Recovered / implemented | Twelve combat scenarios cover payment, escape, forced fighting, and shared combat. [Ledger and limits](combat-research.md) |
| 5 | Thicket/laboratory, potions, pentacle | Partial / placeholder | Laboratory entry and wizard victory observed; recover encounter selection, exploration, potion/pentacle effects and conditions. [Evidence](events.md#thicket-in-the-woods) |
| 6 | Occupied gnome home | Recovered / implemented | Several ordinary food/healing observations. Food addition intentionally fixed; healing threshold/over-cap boundaries lack retained original probes. [Rules](mechanics.md#gnomes-announced-food-gift) |
| 6 | Empty gnome home and decline | Recovered / implemented | Eight retained comparisons via `./check_tree.py --sanitize`: all trunk results, held-Staff substitute, theft, leave, decline. [Scope](mechanics.md#empty-gnome-home) |
| 7 | Grass hut | Partial / placeholder | Empty visit observed; other strings' grouping and good-deed/reward rules need control-flow verification. [Evidence](events.md#grass-hut) |
| 8 | Mountain trail/guru when ring held | Partial / placeholder | Ring gate and Staff/ring removal inspected; capture climb/refusal/acceptance and verify remaining text/flow. [Evidence](events.md#mountain-trail-and-guru) |
| 9 | Servant illness | Recovered / implemented | Controlled medicine, excess-cure, zero/one-sex, protection, and input comparisons; no-servant delay restored. Shared nonnegative integer parsing intentionally differs from Pascal input; original bare-token/input-length behavior remains research-only. [Validation](storm-illness-research.md) |
| 10 | Storms | Recovered / implemented | Controlled checks for all eight types, two shelter durations, zero/small/one-sex parties, and starvation; requested delays checked against dots and binary loop. [Validation](storm-illness-research.md) |
| 11 | Dark Castle: dungeon, throne room, galley | String findings / placeholder | Recover room flow, traps/escape, theft timing, crystal ball, rewards, and Mad King triggers. [Evidence](events.md#dark-castle-of-the-mad-king) |
| 12 | Enchanted forest | Recovered / implemented | Ordinary nymph resistance/elves gift and temporary remake harness. Capture fatal lure, ring exclusion, faerie plague cure versus healing, and decline. [Rules](mechanics.md#enchanted-forest) |
| 13 | Separate empty-tree routine | String findings / placeholder | Inspect independently; do not reuse slot 6's rules without verifying them. [Evidence](events.md#hollow-tree-with-a-door) |
| 14 | Flowing river | Recovered / implemented | Five saved cases: harmless vision, both items, held items, poisoned champion, decline. Comparison harness was temporary; fixtures retained. [Ledger](session-03.md#controlled-river-checks) |
| 15 | Statue of Pan | Core rules recovered / placeholder | One wrong answer observed. Decode/use question texts and compare success, failure, and decline. [Rules](mechanics.md#statue-of-pan) |
| 16 | Keep prerequisite rejection | Recovered / implemented | Ordinary rejection; temporary harness checked level 19 with Staff and level 20 without it. [Rules](mechanics.md#overlords-keep-prerequisite) |
| 16 | Keep command and final-assault transition | Partial / placeholder | Gate, success flag, and wrong-command Staff loss inspected; input/comparison and transition remain unfinished. [Rules](mechanics.md#overlords-keep-prerequisite) |

## Cross-event and deferred work

| Area | Current evidence and next gap |
| --- | --- |
| Anthrax combat | Full ordinary victory captured, including drain, plague, servant deaths, and rewards; recover its separate arithmetic and saving throws. [Observations](session-04.md#anthrax-observations) |
| Dragon, Mad King, and other specialized fights | Separate routines remain unfinished. Establish callers, prerequisites, damage, rewards, and death behavior independently of shared combat. [Combat overview](events.md#combat) |
| Hidden state and interactions | Some ring/plague/protection behavior recovered. Track remaining acquisition/removal paths, dragon charm, command changes, quest items, and effects across event boundaries. [Known gaps](events.md#additional-player-state-to-investigate) |
| Numeric behavior | HP/experience precision choices are recorded in CHANGES.md. Check gold/food calculations for player-visible fractions as new mechanics are recovered; extreme integer overflow is unresolved. [Guideline](mechanics.md) |
| Final battle | Deferred: reconstruct `FINASSLT.CHN` later. Keep the separate exit stub as the entry point until then; wiring the main game's successful Keep branch is still in scope. [Scope](../AGENTS.md#game-structure-and-initial-scope) |

Raw captures live in the ignored `doc/captures/` directory. Keep compact controlled fixtures and their configurations alongside research notes so useful comparisons remain reproducible without those captures. Available capture/check commands are documented in [dosbox-tool.md](dosbox-tool.md).
