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

Current overview: [remaining main-game gaps](remaining-gaps.md), reviewed 2026-11-16. All twelve retained sanitizer comparison suites pass; remaining work is principally validation, integration, and unresolved fidelity boundaries. Final assault work is excluded.

Bounded follow-ups G1–G8 are complete: [forest](forest-research.md), [occupied gnome](gnome-research.md), [City interactions](town-research.md#g3-city-interactions), [startup/turn evidence](startup-turn-research.md), [ring persistence](ring-research.md), [illness state](illness-state-research.md), [charm/quest persistence](quest-state-research.md), and [combat death/reward ordering](combat-order-research.md). These comparisons needed no gameplay corrections beyond the already documented intentional differences. Exact RNG equality is not a completion requirement.

G7 adds consecutive dragon submissions with charm preserved and Staff loss → river recovery → restored Keep eligibility. All 64 thicket and 23 Keep comparisons pass with sanitizers, including the reused acquisition/command chains. G8 adds four matching simultaneous-death outcomes; all 13 shared-combat, 43 hut, and 47 castle comparisons pass with sanitizers. G9 ordinary play and terminal flow is next. Promotion/starvation message readability remains recorded for G9.

- [x] Storms: implemented and compared across all eight types, shelter, zero/small parties, tidal losses, and starvation. [Evidence and limits](storm-illness-research.md).
- [x] Servant illness: implemented and compared for medicine allocation, zero/one-sex parties, excessive cures, protection, and numeric input. Original excessive cures are preserved; integer input now follows the intentional shared-reader policy in CHANGES.md. Original bare-token/input-length behavior remains research-only. [Evidence and limits](storm-illness-research.md).
- [x] Pan: implemented all 20 decoded questions, random answers, and prime-command generation; compared success, failure, decline, invalid answer keys, and command replacement/preservation. [Evidence and limits](pan-research.md).
- [x] City/Village: shared servant market, tavern, store, inn, and both complete location wrappers; City marketplace/cleric/muggers and Village pickpocket. Forty-two controlled comparisons, including G3 interaction visits. [Evidence and limits](town-research.md).
- [x] Mountain trail/guru: climb, refusal, ring removal, optional Staff loss, and pause implemented; five controlled comparisons. [Evidence and limits](mountain-research.md).
- [x] Gypsy camp: Dagger’s Hand and all 35 fortune-teller hints implemented; controlled comparison scope in [research notes](gypsy-research.md).
- [x] Grass hut: all occupants/finds, hermit outcomes, dragon combat, and gate-creature combat/rewards implemented. [Evidence and limits](hut-research.md).
- [x] Thicket/laboratory: exploration, all potion effects, pentacle, and Anthrax implemented; 64 controlled cases include potion-acquired charm, G6 illness-state chains, and G7 repeated dragon meetings. [Evidence and limits](thicket-research.md).
- [x] Dark Castle/Mad King: all three rooms, ghost minstrels, theft/noise, crystal-ball hints, traps/rewards, and independent King combat implemented; 47 retained controlled comparisons pass. [Evidence and limits](castle-research.md).
- [x] Slot-13 watched tree/hut: separate tree judgments, old-man/puppy help, all five rewards and held-Talisman substitute implemented; 32 retained controlled comparisons pass, plus a separate ordinary decline pass. [Evidence and limits](watched-home-research.md).
- [x] Town follow-up (G3): shared-service purchases plus bargain-market shopping, and cleric ring removal followed by muggers, compared in two complete visits. No gameplay correction needed. [Evidence](town-research.md#g3-city-interactions). Ordinary integration remains the separate G9 task.
- [x] Keep command branch: level/Staff gates, optional attack, case-sensitive five-character comparison, wrong-command Staff loss, and post-bookkeeping handoff to the exit stub implemented; twenty-three retained controlled comparisons include G4 starvation and G7 Staff recovery. [Evidence and limits](keep-research.md).
- [ ] Future ordinary integration: earn Pan's command, the Staff, and level twenty through normal play, then reach the Keep. A controlled Pan-to-Keep sequence is retained in [Keep research](keep-research.md); the final assault remains deferred.

The Keep branch now reaches the existing final-battle exit stub after bookkeeping. The main-game event bodies are implemented within their documented scope; continue ordinary integration play and the listed validation follow-ups. The final assault is explicitly deferred at the user's request.

## Core systems

“Recovered” means the documented scope has binary evidence. “Partial” means only some rules or branches are established. Validation describes what has actually been checked, not exhaustive parity.

| System | Rules / implementation | Validation and next gap |
| --- | --- | --- |
| RNG | Recovered / implemented | State progression compared in combat, river, and tree cases. Original startup seed remains unresolved. [Rules](mechanics.md#random-number-generation) |
| Character generation and servant prices | Recovered / implemented | G4 consolidates ordinary initial statuses, binary formulas, and implementation review; no dedicated startup suite is needed. Original title says levels 7–10; executable generates 7–9. [Evidence](startup-turn-research.md#startup-evidence), [rules](mechanics.md#character-generation) |
| Event dispatch and substitutions | Recovered / implemented | All dispatch call sites checked against binary. Full-run RNG parity and original startup seeding remain unverified. [Rules](mechanics.md#event-selection) |
| Turn healing, experience, ring/plague progression, food, starvation, death | Recovered / implemented | G4 consolidates retained encounter comparisons, adding only two small-party starvation cases. Healing/promotion order, ring/plague growth, food/losses, and death/handoff are supported without an exhaustive boundary suite. Message readability remains G9. [Evidence and limits](startup-turn-research.md), [rules](mechanics.md#main-turn-loop) |
| Band choices and shared combat | Recovered / implemented | Thirteen retained controlled comparisons via `./check_combat.py --sanitize`. G8 confirms simultaneous-death messages and absence of survivor rewards; specialized fights are separate. [G8 evidence](combat-order-research.md). [Ledger](combat-research.md) |
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
| 1, 2 | Pickpocket, extra marketplace, cleric, muggers | Recovered / implemented | Village-only pickpocket; City-only marketplace/cleric/muggers. Purchases, cures, ring protection, theft/abort checked; G3 closes shared-service/marketplace and cleric/mugger interaction gaps. [Ledger](town-research.md#g3-city-interactions) |
| 3 | Gypsy camp: Dagger's Hand and advice | Recovered / implemented | Betting, injuries, both-hit outcome, repeat rounds, last-coin results, advice affordability, and all 35 hints compared with the original. [Evidence and limits](gypsy-research.md). |
| 4; guarded 8, 9 | Band encounter | Recovered / implemented | Thirteen combat scenarios cover payment, escape, forced fighting, and shared combat. [Ledger and limits](combat-research.md) |
| 5 | Thicket/laboratory, potions, pentacle | Recovered / implemented | Entry, loot/Staff boundaries, all 11 potion effects/eight reachable hair colors, five pentacle outcomes, fatal paths, and charm-to-dragon sequence compared. [Evidence and limits](thicket-research.md). |
| 6 | Occupied gnome home | Recovered / implemented | Ordinary observations plus three retained G2 comparisons: injured healing, healing past the HP cap, and full health without ointment. State/text/pauses match with the intentional food-gift fix. [Evidence and limits](gnome-research.md) |
| 6 | Empty gnome home and decline | Recovered / implemented | Eight retained comparisons via `./check_tree.py --sanitize`: all trunk results, held-Staff substitute, theft, leave, decline. [Scope](mechanics.md#empty-gnome-home) |
| 7 | Grass hut | Recovered / implemented | All finds and occupants, care with/without supplies, hermit outcomes, dragon/charm/poison, gate creature types/rewards, and death compared. No good-deed reward for the man or puppy in this routine. [Evidence and limits](hut-research.md). |
| 8 | Mountain trail/guru when ring held | Recovered / implemented | Five controlled comparisons cover declining, refusing, acceptance with/without Staff, ring power 1/100, invalid keys, and Talisman preservation. [Evidence and limits](mountain-research.md). |
| 9 | Servant illness | Recovered / implemented | Controlled medicine, excess-cure, zero/one-sex, protection, and input comparisons; no-servant delay restored. G6 additionally compares potion-acquired protection through the next slot-9 band substitution. Shared nonnegative input parsing intentionally differs from Pascal. [Illness validation](storm-illness-research.md), [G6 chain](illness-state-research.md) |
| 10 | Storms | Recovered / implemented | Controlled checks for all eight types, two shelter durations, zero/small/one-sex parties, and starvation; requested delays checked against dots and binary loop. [Validation](storm-illness-research.md) |
| 11 | Dark Castle: dungeon, throne room, galley | Recovered / implemented | Controlled room, ghost, item, food/pit, theft/noise, poison, equipment/ring, full-combat, and fatal comparisons. Original uninitialized fear local intentionally resolved. [Evidence and limits](castle-research.md). |
| 12 | Enchanted forest | Recovered / implemented | Ordinary resistance/elves gift plus six retained G1 comparisons: fatal lure, ring exclusion, appearance restoration/plague cure, healing, full-health healing, and decline. Both gender strings inspected; all G1 outcomes match without gameplay changes. [Evidence and limits](forest-research.md) |
| 13 | Watched empty tree and old-man/puppy hut | Recovered / implemented | Independent tree and hut selection, bad-deed level loss, all rewards, supply boundaries, held-Talisman substitute, and invalid choice. [Evidence and limits](watched-home-research.md). |
| 14 | Flowing river | Recovered / implemented | Five saved cases: harmless vision, both items, held items, poisoned champion, decline. Comparison harness was temporary; fixtures retained. [Ledger](session-03.md#controlled-river-checks) |
| 15 | Statue of Pan | Recovered / implemented | Five retained DOS comparisons cover success, failure, decline, invalid answer keys, command replacement/preservation, and question records 1, 3, 20. All 20 embedded records checked against data. [Evidence and limits](pan-research.md) |
| 16 | Keep prerequisite rejection | Recovered / implemented | Ordinary rejection plus controlled level/Staff combinations and rejection before promotion to level twenty. [Evidence](keep-research.md). |
| 16 | Keep command and final-assault transition | Recovered / implemented to exit stub | Optional attack, input boundaries, wrong-command Staff loss, success, post-turn death suppression, and controlled Pan-to-Keep sequence. Final assault remains unimplemented. [Evidence and limits](keep-research.md). |

## Cross-event and deferred work

| Area | Current evidence and next gap |
| --- | --- |
| Anthrax combat | Recovered / implemented | Ring/poison bypasses, all nine retaliations, successful/failed saves, servant priority, full fights, killing-blow retaliation, and simultaneous deaths compared. Exact save boundaries and every attack combination are not all retained probes. [Evidence and limits](thicket-research.md). |
| Dragon and gate-creature fights | Recovered / implemented for hut and thicket pentacle callers. Controlled normal fights, charm, poison, rewards and death. Not every damage-table result or simultaneous-death combination has a retained probe. [Hut evidence](hut-research.md), [pentacle evidence](thicket-research.md). |
| Mad King combat | Recovered / implemented independently of shared combat: full fights, ring, weapons/bows/unarmed, poison, death, rewards and RNG compared. Captures do not exhaust every damage/shout combination or simultaneous deaths. [Evidence and limits](castle-research.md). |
| Hidden state and interactions | G5–G7 close the bounded ring, illness, charm, and quest-persistence chains. Further combinations are unnecessary absent a concrete normal-play discrepancy; ordinary integration remains G9. [Ring evidence](ring-research.md), [illness evidence](illness-state-research.md), [charm/quest evidence](quest-state-research.md), [remaining gaps](remaining-gaps.md#bounded-completion-checklist) |
| Numeric behavior | HP/experience precision choices are recorded in CHANGES.md. Check gold/food calculations for player-visible fractions as new mechanics are recovered; extreme integer overflow is unresolved. [Guideline](mechanics.md) |
| Final battle | Deferred: reconstruct `FINASSLT.CHN` later. Successful Keep entry now calls the separate exit stub after bookkeeping if the player survives; implementing the battle is excluded from this task. [Scope](../AGENTS.md#game-structure-and-initial-scope) |

Raw captures live in the ignored `doc/captures/` directory. Keep compact controlled fixtures and their configurations alongside research notes so useful comparisons remain reproducible without those captures. Available capture/check commands are documented in [dosbox-tool.md](dosbox-tool.md).
