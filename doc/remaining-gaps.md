# Remaining main-game gaps

Overview reviewed on 2026-11-16 against `braminar.c`, the [reconstruction checklist](reconstruction.md), recovered mechanics, research ledgers, and retained comparison tools. The initial overview used existing evidence; later [G1 forest](forest-research.md), [G2 gnome](gnome-research.md), [G3 City](town-research.md#g3-city-interactions), [G4 startup/turn](startup-turn-research.md), [G5 ring](ring-research.md), [G6 illness](illness-state-research.md), [G7 charm/quest](quest-state-research.md), and [G8 combat ordering](combat-order-research.md) follow-ups consolidate evidence and add controlled comparisons where needed.

Scope: remaining work concerns mechanics and text a player encounters in normal play. Exact RNG state, matching startup seeds or complete random sequences, integer overflow, and artificial extreme states are outside scope. Preserve encounter chances and gameplay rules without requiring identical random outcomes between runs.

## Current situation

All sixteen `BRAMINAR.COM` event slots have implementations, including state-dependent band substitutions, both independent tree routines, nested encounters, and the separate shared-monster, Anthrax, dragon, gate-creature, and Mad King fights. No known reachable main-game event body is still a placeholder. Implementation coverage is substantially ahead of live comparison coverage in a few older branches.

The Keep's gates, command handling, Staff loss, and successful transition after bookkeeping are implemented and compared. `start_final_battle()` prints the deferred-battle message and exits. Reconstructing or implementing `FINASSLT.CHN` is excluded from this backlog.

Verification at the initial overview:

- `make` succeeds; a separate fresh build with `cc -O2 -Wall -Wextra -Wpedantic -std=c99` succeeds without warnings.
- All twelve `check_*.py --sanitize` suites pass with address and undefined-behavior sanitizers: 349 retained scenarios, including checks for documented intentional differences. The suites also check integer input, embedded Pan questions, and selected EOF paths.
- Five additional [river fixtures](river-checkpoints.json) survive from an earlier temporary comparison harness, but have no retained dedicated checker. These five are outside the 349 scenarios above.

G1 adds six passing forest comparisons via `python3 check_forest.py --sanitize`, bringing retained automated scenario coverage to 355 across thirteen suites. No gameplay correction was needed. The initial twelve suites were not rerun for this follow-up because game code did not change.

G2 adds three passing occupied-gnome comparisons via `python3 check_gnome.py --sanitize`, bringing coverage to 358 scenarios across fourteen suites. Healing, text, and pauses match with the documented food-gift fix; no gameplay correction was needed. Other suites were not rerun because game code and the reused harness did not change.

G3 adds two City interaction cases to the town suite, bringing coverage to 360 scenarios across fourteen suites. All 42 town comparisons pass with sanitizers, including the new narrative checks. No gameplay correction was needed; exact RNG equality is no longer asserted by the town checker. Other suites were not rerun because game code did not change.

G4 consolidates startup and turn evidence, adding only two small-party starvation cases to the Keep suite. All 22 Keep/turn comparisons pass with sanitizers; total coverage is 362 scenarios across fourteen suites. No arithmetic or outcome correction was needed. The Keep checker no longer asserts exact RNG equality. A promotion/starvation message-readability difference is recorded for G9; other suites were not rerun because game code did not change.

G5 adds two passing two-event ring chains via `python3 check_ring.py --sanitize`, bringing coverage to 364 scenarios across fifteen suites. Acquisition persists into forest exclusion; guru removal restores the elves' food/appearance gifts. No gameplay correction was needed. Exact RNG equality is not checked, and other suites were not rerun because game code and the reused harness did not change.

G6 adds two illness-state chains to the thicket suite, bringing coverage to 366 scenarios across fifteen suites. All 63 thicket comparisons pass with sanitizers. Anthrax-acquired plague progresses before faerie cure, and potion-acquired protection persists into slot-9 band substitution. No gameplay correction was needed; the thicket checker no longer asserts exact RNG equality. Other suites were not rerun because game code and the reused harness did not change.

G7 adds repeated-dragon and Staff-recovery sequences, bringing coverage to 368 scenarios across fifteen suites. All 64 thicket and 23 Keep comparisons pass with sanitizers, including the reused potion-to-dragon and Pan-to-Keep cases. Charm persists through both submissions; Staff recovery restores eligibility and preserves the command. No gameplay correction was needed, and no final-assault code ran in the new original-game captures.

G8 adds four simultaneous-death outcomes, bringing coverage to 372 scenarios across fifteen suites. All 13 shared-combat, 43 hut, and 47 castle comparisons pass with sanitizers. Independent death messages, pauses, and absence of survivor rewards match; no gameplay correction was needed. Exact RNG equality is no longer asserted by these three checkers. Anthrax's already captured death-time experience award remains a separate rule.

Passing these fixtures establishes their recorded states and flows, not exhaustive coverage of normal play. Existing suites also compare RNG state as a research aid; exact RNG equality is not a completion requirement. Several fatal fixtures stop at the final event pause before bookkeeping because DOS reclaims the original data segment on exit. The Keep suite does compare post-bookkeeping death and exercises the actual exit stub. See the individual ledgers for those limits.

## Bounded completion checklist

This checklist is the completion gate for main-game reconstruction. The sections below explain its evidence and context; they are not additional mandatory work. Check an item when its stated outcomes are supported by recovered rules, the implementation, and representative comparisons. Reuse existing evidence first. Each item needs a short evidence link and any correction made, not a new exhaustive suite.

- [x] G1 — Forest outcomes: six controlled comparisons cover fatal lure, ring exclusion, appearance restoration with plague cure, healing, a full-health visit, and decline. Visible messages, state, survival, and pauses match; both gender strings were inspected. No gameplay correction was needed. [Evidence and completion limits](forest-research.md). The earlier resistance/food path was not repeated.
- [x] G2 — Occupied gnome: three controlled comparisons confirm an injured visit, healing past the normal HP cap, and a full-health visit without ointment. Messages, pauses, and resulting state match with the intentional food-gift fix. No gameplay correction was needed. [Evidence and completion limits](gnome-research.md).
- [x] G3 — City interactions: two controlled visits confirm shared-service spending followed by bargain-market shopping, and cleric ring removal followed by a mugging. Updated balances, supplies, ring state, text, and pauses match without gameplay changes. [Evidence and completion limits](town-research.md#g3-city-interactions).
- [x] G4 — Startup and turn effects: starting stats/prices, healing, experience/promotion, food consumption, starvation, ring growth, and plague progression/death are supported by consolidated binary/ordinary/controlled evidence. Only two new small-party starvation comparisons were needed; state and order match. [Evidence and completion limits](startup-turn-research.md). Promotion/starvation message readability remains part of G9's terminal-flow review.
- [x] G5 — Ring persistence: two controlled two-event chains confirm that a combat-awarded ring excludes elves on the next forest visit, while guru removal restores their gifts on the next visit. State, supplies, text, and pauses match without edits between encounters or gameplay corrections. [Evidence and completion limits](ring-research.md). Other component checks are reused; no additional ring pairings are required.
- [x] G6 — Illness state: two controlled two-event chains confirm Anthrax infection → plague progression → faerie cure, and laboratory protection potion → slot-9 band substitution. State, supplies, text, and pauses match without edits between encounters or gameplay corrections. [Evidence and completion limits](illness-state-research.md). Existing potion/cleric cure comparisons are reused; further cure combinations are unnecessary.
- [x] G7 — Charm and quest persistence: existing acquisition/command evidence is retained. Two new controlled sequences confirm consecutive dragon submissions with charm intact, and wrong-command Staff loss → river recovery → restored Keep eligibility, with command preserved. State, text, and pauses match without edits between encounters or gameplay corrections. [Evidence and completion limits](quest-state-research.md). No progression grind or further item/command combinations are required.
- [x] G8 — Combat death/reward ordering: four controlled simultaneous-death comparisons confirm independent death messages, pauses, and absence of survivor rewards in shared monster, dragon, gate-creature, and Mad King fights. No gameplay correction was needed. Anthrax's already captured experience award on mutual death is preserved separately. [Evidence and completion limits](combat-order-research.md). No additional attack-table or save-threshold matrix is needed.
- [ ] G9 — Ordinary play and terminal flow: capture one ordinary session of up to twenty completed turns in each version, allowing up to three starts per version if characters die sooner. Review cumulative health, supplies, equipment, prompts, and pacing. Different random events are expected. Stop at that budget or the Keep handoff; use the existing controlled handoff evidence if normal play does not reach it. A complete naturally earned victory prerequisite run is optional.
- [ ] G10 — Close the review: fix any concrete normal-play discrepancies found by G1–G9, rerun affected checks, and link the results here and in the reconstruction index. Reconcile current claims of unfinished mechanics with later evidence while preserving historical session notes. Mark each mandatory item complete or name the specific unresolved player-visible rule that prevents closure.

### Probe budget and stopping rule

Start each item by identifying the exact unanswered gameplay question. If existing binary rules and comparisons already answer it, record that evidence and move on. Otherwise capture one representative example per missing, materially different outcome; combine outcomes in one case where practical. Controlled setups are appropriate for rare branches and avoid waiting for them in ordinary play.

For each question, stop once the rule is established and a representative comparison agrees with the remake. If a mismatch appears, investigate that mismatch and check the correction; it does not automatically reopen adjacent completed branches. More seeds, repeated successes, every flavor string, every threshold equality, and every cross-event permutation are not completion requirements. A missing capture alone does not establish a missing mechanic.

After G1–G10, close main-game reconstruction unless a concrete discrepancy affects normal-play text, choices, costs, resources, progression, survival, or quest access. Newly discovered discrepancies can become separately bounded fixes. Do not keep expanding this checklist merely to increase probe coverage.

### Optional maintenance, outside the completion gate

- [ ] M1 — Retain a reusable river checker using the five existing fixtures; no new river probes are required.
- [ ] M2 — Remove or clarify the unreachable placeholder after the event-dispatch switch.

Additional flavor-text captures, such as the King's sixth shout, and an ordinary run earning all Keep prerequisites may be collected opportunistically. They do not block completion.

## Highest-value follow-up comparisons

These are implemented, binary-documented rules with incomplete original-game comparison coverage, rather than known missing mechanics.

| Area | Remaining work | Evidence |
| --- | --- | --- |
| Enchanted forest | G1 complete: six representative original/remake comparisons cover the previously missing outcomes, and both gender strings match the binary. No further forest probes are required absent a concrete normal-play discrepancy. | [Evidence and limits](forest-research.md), [rules](mechanics.md#enchanted-forest) |
| Occupied gnome home | G2 complete: injured, over-cap healing, and full-health visits match with the documented food-gift fix. No further occupied-home probes are required absent a concrete normal-play discrepancy. | [Evidence and limits](gnome-research.md), [rules](mechanics.md#gnomes-announced-food-gift) |
| City service combinations | G3 complete: purchases carry through shared services and the bargain marketplace, and cleric ring removal prevents protection during a later mugging. The 42 town cases include these two representative visits. No further combinations are required absent a concrete normal-play discrepancy. | [Evidence and limits](town-research.md#g3-city-interactions) |
| River regression | A reusable checker for the five existing original fixtures would preserve their state and flow comparisons. This is maintenance rather than a known gameplay gap; the champion already uses the tested shared fight routine. | [Earlier comparisons](session-03.md#controlled-river-checks) |
| Startup and bookkeeping | G4 complete: generation and normal turn values/order are supported by existing evidence plus two small-party starvation comparisons. No exhaustive core-system suite is required. Promotion/starvation message readability is recorded under G9. | [Evidence and limits](startup-turn-research.md), [turn rules](mechanics.md#end-of-turn-bookkeeping) |

## Integration and hidden state

Most recent research isolates an event by editing initial player state and RNG. Two retained controlled chains already validate potion-acquired charm followed by the hut dragon, and Pan's command followed by the Keep. They do not establish a full ordinary playthrough.

- Use the bounded ordinary sessions in G9 to review integration. A longer run that earns the command, Staff, and level twenty is optional; the controlled Pan-to-Keep handoff already covers command persistence and transition. Retain every encountered event, including skipped branches, and stop at the exit stub if reached.
- G5–G7 interaction scopes are complete: [ring chains](ring-research.md), [illness chains](illness-state-research.md), [charm/quest sequences](quest-state-research.md), and reused acquisition/command evidence suffice. Further ring, cure, Staff, or command combinations are unnecessary absent a concrete normal-play discrepancy.
- Use longer play sequences to check cumulative health, experience, supplies, and item changes. Controlled setups can help reach a particular interaction, but identical random sequences are unnecessary.

Use [the reconstruction workflow](reconstruction.md#choosing-and-finishing-a-target) to keep each comparison bounded. Ordinary observations establish what happened in that run; binary evidence or controlled probes can resolve uncertain gameplay rules without exhaustively testing every threshold.

## Other player-visible follow-ups

| Area | Current evidence | Useful follow-up |
| --- | --- | --- |
| Displayed quantities | Original gold and food are Real48; the remake stores integers. Documented ordinary awards, purchases, and consumption use integer quantities, and no fractional-inventory mismatch is established by this review. | Investigate only if ordinary play or a recovered rule reveals a difference in displayed quantities, costs, or outcomes. A representation audit or Real48 emulation is unnecessary on its own. [Precision policy](mechanics.md). |
| Combat outcomes | Full attack tables are recovered, and retained fights cover the main equipment, poison, reward, and death paths. G8 now compares simultaneous deaths in shared combat, dragon/gate creatures, and Mad King; Anthrax already has coverage, including its different experience-award rule. The King's sixth shout also lacks a retained capture. | G8 is complete: [evidence and limits](combat-order-research.md). Sample remaining text opportunistically during normal combat; exhaustive attack pairings and poison/save equality cases are not required. [Combat](combat-research.md), [hut](hut-research.md#verification-and-limits), [thicket](thicket-research.md#verification-and-remaining-limits), [castle](castle-research.md#intentional-difference-and-limits). |
| Terminal flow | Comparisons generally normalize whitespace and terminology. G4 identified that `finish_turn()` omits the original promotion/starvation waits, allowing messages to clear immediately on the next status redraw. The remake uses ANSI output and does not reproduce speaker sounds. | G9 should check these messages' readability along with normal input, prompts, and pacing. Exact DOS layout, control-key behavior, sound reproduction, and wall-clock timing are not completion requirements. [G4 presentation observation](startup-turn-research.md#limits-and-next-work), [town comparison method](town-research.md#reproduction-and-limits), [Keep input limits](keep-research.md#limits-and-intentional-differences). |

## Documentation and maintenance

- The TODO and `This encounter is not implemented yet.` after `run_event()`'s switch are stale: `random_number(16) + 1` only yields 1–16, and every case returns. They do not identify a missing event. Remove or clarify this dead tail in a future code cleanup.
- Older session ledgers correctly describe their historical state but should not be treated as today's backlog. Examples: session 03's illness/town placeholders, session 04's unfinished Anthrax and slot-13 tree, Pan research's unfinished Keep, and older unused-final-battle-stub warnings. Later ledgers and the current code supersede them; the fresh build has no such warning.
- The broad hidden-state investigation paragraph in [events.md](events.md#additional-player-state-to-investigate) and older uncertainty wording in mechanics need reconciliation with later research. Known acquisition/removal paths and specialized fights now have recovered rules. Preserve historical observations while distinguishing remaining validation from unfinished implementation.
- Raw captures are ignored; compact fixtures make most comparisons reproducible without DOSBox. Preserve useful checks rather than relying on temporary harness descriptions, especially for the river and core boundaries.

## Intentional differences already settled

[CHANGES.md](../CHANGES.md) records servant terminology, corrected typos, double-precision HP/experience and 0.10 turn healing, the fixed gnome food gift, defined blank-treatment and castle fear behavior, the shared nonnegative integer reader, and the zero-gold pickpocket diagnostic. These are accepted differences, not restoration tasks. Additional research into original malformed numeric input is optional historical work unless that policy changes.

Follow G1–G10 as the bounded remaining scope, reusing existing evidence throughout. The broader follow-up descriptions above do not require extra probes once those items are satisfied. Exact RNG parity, startup-seed reconstruction, integer overflow, and artificial extreme-state research are not remaining tasks for this remake.
