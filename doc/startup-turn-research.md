# Startup and turn effects — G4

Completed 2026-11-16. This review consolidates existing binary rules, ordinary observations, and controlled comparisons for G4 in [remaining gaps](remaining-gaps.md#bounded-completion-checklist). Only two new original-game probes were needed, both for small-party starvation. No arithmetic or outcome correction was needed. A presentation difference is recorded below for G9.

## Startup evidence

The [character-generation rules](mechanics.md#character-generation) and [price rules](mechanics.md#servant-prices) agree with `generate_player_stats()`, `create_player()`, and `display_stats()` on review. Reinspection of `BRAMINAR.COM` at `0xA36B`–`0xA3C8` confirms level 7–9 and a sum of one 1–10 HP roll per level; `0xA470`–`0xA4F9` confirms independent servant counts, gold, initial supplies/equipment, and the two prices. The remaining initial fields and hair palette have the existing binary evidence in mechanics.md.

These are generation rules, not conclusions inferred from a few starting characters: levels 7–9; HP from level through ten times level, using the sum of rolls; four initial hair colors; good looks; 0–14 servants of each sex; 0–24 gold; 100 food; three medicine; no weapons, poison, transport, or quest items; zero experience/ring/plague/protection/charm; and independently generated male/female prices of 5–14, retained for later markets. Name/gender input precedes prices, which have a pause before the first status. HP displays to two decimals; counts, prices, gold, and food display as integers.

Representative ordinary startup observations already exist:

| Source | Observed initial state | Observed male/female prices |
| --- | --- | --- |
| [Session 01](captures/2026-11-14-session-01.md#initial-character) | Level 8, 49 HP, Blonde, one male/eight female servants, two gold | 6 / 8 |
| [Combat playthrough](combat-research.md#ordinary-playthrough) | Level 7, 31 HP, Red, five male/ten female servants, five gold | 8 / 13 |
| [Session 03](session-03.md) | Level 9, 58 HP, Brown, fourteen male/four female servants, nineteen gold | 11 / 13 |

All three start with good looks, 100 food, three medicine, and empty equipment/quest inventory. The combat playthrough's later servant sale corroborates its stored prices; controlled town market comparisons establish the transaction rules. The title's 7–10 claim is reproduced text, while both executables actually generate 7–9. No new startup probe, statistical survey, matching startup seed, or complete RNG sequence is needed for G4.

## Turn evidence consolidated

Binary rules are in [end-of-turn bookkeeping](mechanics.md#end-of-turn-bookkeeping); review against `finish_turn()` confirms the following order. The table links representative evidence rather than requesting every combination.

| Effect and visible consequence | Existing evidence |
| --- | --- |
| Heal living, injured players by 0.10 HP before promotion; do not add ordinary healing at or above the current cap. | [Ordinary session 01](captures/2026-11-14-session-01.md#encounter-ledger) shows successive 0.10 increases. [G1 full-health](forest-research.md#compared-outcomes) stays at 80; [G2](gnome-research.md#compared-outcomes) stays at 83 after ointment crosses the cap. Real48 precision is an accepted difference. |
| Combat supplies experience. Exactly five does not promote; above five promotes once, subtracting five and adding one level/ten HP. | Shared combat's [Goblin victory](combat-research.md#controlled-original-game-experiments) gives 1.25 experience. Hut fixture `dragon-fight` ends at level 8 with exactly five experience. `creature-reward-5` has level 8/80 HP/11 experience at the exit pause and level 9/90 HP/six experience afterward, confirming a single promotion. [Hut fixtures](hut-checkpoints.json). |
| The encounter uses the old level before bookkeeping can promote it. | Keep fixture `gate-level-promotion` rejects level 19 with the Staff, then becomes level 20, 60.10 HP, one experience. [Keep evidence](keep-research.md#baseline-and-comparison-scope). |
| Ring power grows by five after the encounter while positive and below 100. | Combat's [ring award](combat-research.md#controlled-original-game-experiments) goes from five to ten at turn end. [G1 ring exclusion](forest-research.md#compared-outcomes) goes from twenty to twenty-five. Keep `success-bookkeeping` goes from 95 to 100. Overshoot/overflow surveys are unnecessary. |
| Plague grows by five after healing/promotion and can kill before the next turn or handoff. | Keep `success-bookkeeping`: level 20/50 HP/six experience, ring 95, plague 90 becomes level 21/60.10 HP/one experience, ring 100, plague 95. `success-plague-death`: plague 95 becomes 100 and HP zero; food still falls from 100 to 95, and no handoff occurs. [Keep fixtures](keep-checkpoints.json). |
| Consume integer `(male + female + 1) / 2` food using the party remaining after the encounter. Zero servants consume zero. | [Session 01](captures/2026-11-14-session-01.md#encounter-ledger): nine servants consume five. [G3 shopping](town-research.md#g3-city-interactions): twelve servants consume six; the no-servant mugging consumes zero. Storm fixtures `storm-empty-party` and `storm-tidal` corroborate no-servant behavior. |
| Only a negative food balance triggers shortage handling: clamp to zero; with more than one servant, announce starvation and remove one of each positive sex. A sole servant is spared. | [Storm/starvation evidence](storm-illness-research.md#controlled-original-game-evidence), Keep `success-starvation`, and the two focused G4 cases below. Hut `puppy-last-food` spends its last food before the party starves. These establish post-event consumption and losses; the zero-versus-negative predicate is also explicit at `0xF2BC`–`0xF2CE`. |
| Handle death and any successful Keep handoff after all the above. | [Keep comparison scope](keep-research.md#baseline-and-comparison-scope) includes starvation before handoff and fatal plague suppressing handoff. [Fatal forest](forest-research.md#compared-outcomes) captures its outer pause followed by `Game Over.`. |

Promotion/starvation messages appear during bookkeeping after the event's own final pause. Normal surviving play then redraws status and waits before the next encounter. Reinspection of `0xF180`–`0xF237` and `0xF297`–`0xF350` confirms single promotion and the small-party starvation gate. The other turn rules retain their existing binary evidence.

## Focused small-party comparisons

The [raw transcript](captures/2026-11-16-turn-starvation.txt) and two added [Keep fixtures](keep-checkpoints.json) capture controlled experiments, not ordinary play. `keep_probe.py` uses the shared `dosbox_game.Game` runner, edits temporary player state/RNG and prices, and stops at the existing breakpoint after bookkeeping (`0xF350`). Original files are untouched; no final-assault code executes. Each encountered event is an ineligible Keep, including its rejection text and normal exit pause. Every screen and input is retained.

Both cases use level eight, 50 HP, zero food, zero females, no Staff, zero experience/ring/plague, and otherwise the established Keep baseline. The initially displayed status can predate memory edits; `STATE initial` records the setup. No outcome within the Keep changes the party or supplies before bookkeeping.

| Case | Starting male servants | Original result after bookkeeping, matched by remake |
| --- | --- | --- |
| `turn-two-servants-starvation` | 2 | Food shortage prints the starvation message; one male remains, food zero, HP 50.10. Only the positive sex loses a servant. |
| `turn-last-servant-starvation` | 1 | One male remains, food zero, HP 50.10; no starvation message. |

Reproduce only those missing outcomes, then compare the retained suite:

```sh
python3 keep_probe.py turn-two-servants-starvation turn-last-servant-starvation > doc/captures/2026-11-16-turn-starvation.txt
python3 check_keep.py --sanitize
```

All 22 Keep/turn scenarios pass with address and undefined-behavior sanitizers, including input/state checkpoints, post-bookkeeping values, normalized text, and the new cases' presence/absence of starvation text. RNG remains fixture metadata; exact equality is no longer asserted by the Keep checker. No separate core-system harness or new startup captures were added. Existing encounter evidence was reused without replaying its original probes.

## Limits and next work

G4 establishes the requested normal-play values and effect ordering. It does not exhaust numeric thresholds, random seeds, attack combinations, artificial extremes, or long-term interactions. Ring persistence, illness acquisition/cure chains, and charm/quest persistence remain G5–G7; ordinary integration remains G9.

Presentation observation for G9: the original promotion path includes sound/waits at `0xF1BA`–`0xF1ED`, and negative-food processing waits 500 ms at `0xF34A`–`0xF34D`, even for a sole servant. The remake's `finish_turn()` has no corresponding waits, so promotion/starvation messages can be cleared immediately by the next status redraw. G9 should review their readability in a real terminal. Exact DOS sound or timing reproduction is not required, and this does not reopen G4's verified state arithmetic.
