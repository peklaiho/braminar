# City and Village reconstruction

## Scope and implementation

This target implements both complete location wrappers and their shared servant market, tavern, general store, and inn routines. The City additionally has a conditional bargain marketplace, cleric, and muggers; the Village has a pickpocket. [Mechanics and binary offsets](mechanics.md#city-and-village) document exact rules, conditional draw order, and pauses. The original already shares the four common routines; the remake follows that structure through `event_town(player, city)`.

No final-battle work is included. The Keep remains deferred in priority at the user's request. No original DOS files were changed. The only intentional town differences are servant terminology, the established shared integer reader, and the diagnostic used for the original zero-gold pickpocket abort; see [CHANGES.md](../CHANGES.md).

## Controlled experiments

`town_probe.py` uses the `dosbox_game.py` Game runner, changing temporary player memory and the RNG before dispatch. These experiments are distinct from ordinary play. Base state: level 8, 50 HP, 500 gold, 100 food, five servants of each sex, three medicine, Black hair, good looks, zero ring/plague/experience/poison/weapons, no horse/cart/quest items, servant-illness protection set. Male/female servant prices are set to 8/13. Each case's configuration records overrides. Displayed status can predate a memory reset; direct memory checkpoints give the actual state.

All encountered screens and commands, including startup, are retained in these raw transcripts:

- [Shopping](captures/2026-11-15-town-shopping.txt): entry/decline, servant transactions, weapon/transport/supply purchases, exact and insufficient funds. Its final held-transport attempt stalled because the first probe runner did not recognize a silent quantity retry; that unfinished attempt supplies no expected final state.
- [Shopping retries and marketplace](captures/2026-11-15-town-shopping-retries.txt): completed held-transport/supply retries and marketplace purchase/decline after fixing the runner.
- [City-specific events, pickpockets, and lodging](captures/2026-11-15-town-city-inn.txt).
- [Tavern](captures/2026-11-15-town-tavern.txt): drinks, recruitment, and fatal poisoning. Initial darts attempts sent a second key during the original's 500 ms delay; these raw attempts are retained but their timing-dependent checkpoints are superseded.
- [Darts](captures/2026-11-15-town-darts.txt): completed win, tie, loss, and ring-loss comparisons, waiting through the delay before responding to the next prompt.
- [Restart check](captures/2026-11-15-town-restart.txt): the runner restarts normally after the zero-gold abort and then captures fatal poisoning. Both cases pass the retained-state comparison.
- [Empty purse](captures/2026-11-15-town-empty-purse.txt): undetected theft at zero gold aborts with `Run-time error 04, PC=10DF` before the market invitation.

The [40 retained cases](town-checkpoints.json) contain only completed comparisons, recording configuration, actions, observed prompts, and direct memory state at every input boundary and after ordinary turn bookkeeping. For fatal poisoning, the final retained memory state is at the last inn prompt before declining, before bookkeeping; the raw capture also shows subsequent `Game Over.`. The checker compares the completed event without bookkeeping for this case, since DOS has reclaimed DS after death. The zero-gold abort has no claimed final player state.

| Cases | Evidence covered |
| --- | --- |
| village-decline, city-decline, village-skip, city-skip | Entry prompts, common service order, no extra exit pause, RNG with skipped services |
| market-sell-clamped | Selling beyond counts caps both sexes; correct credit and smaller-party food use |
| market-buy-sequential, market-buy-unaffordable | Exact funds, male-first spending, unaffordable male purchase does not block affordable female purchase |
| store-all, store-exact-gold, store-insufficient | Every merchandise category, exact funds, silent failure, changing balance |
| store-held-transport | Owned transport skips prompts/draws; food and medicine retry oversized quantities |
| marketplace-purchase, marketplace-decline | City open-market branch; two gold buys ten food; poison purchase; affordability retries; decline |
| cleric-decline, cleric-cure, cleric-ring-overcap | Donation decision, zeroed gold, plague/ring removal, hair/looks restoration, HP maximum without lowering already-high HP, protection retained |
| muggers-loss, muggers-ring | No-servant gate, losses capped to available resources, ring protection and result pause |
| pickpocket-noticed, pickpocket-theft, pickpocket-empty | Detection text, silent gold loss, zero-gold runtime abort |
| village-ring-warning | Ring-triggered peasant text without an extra RNG draw |
| inn-room-1, inn-room-2, inn-room-3, inn-insufficient, inn-rats | All rooms, uncapped HP, female bandaging, insufficient funds lost, exact funds, rat loss with no male servants |
| drink-good, drink-plain, drink-saved, drink-cured, drink-fatal | Healing cap, ordinary drink, saved poison dose, medicine consumption, fatal poison and later service prompts |
| social-girl, social-guy, social-ugly, social-decline | Both servant sexes, appearance draws, charm requires good looks, declining skips charm draw |
| darts-win, darts-tie, darts-loss, darts-ring | Scores, tie counts as victory, ten-gold win, five-gold loss, ring prevents loss without adding gold |

## Ordinary discovery pass

A separate [five-turn ordinary pass](captures/2026-11-15-town-ordinary.txt) used `dosbox_game.py` without memory or RNG edits. TownWalk began female, level 9, Black hair, good looks, 56 HP, twelve male/one female servants, 23 gold, 100 food, three medicine, and no equipment or quest items; servant prices were 6/5.

| Turn | Encounter and normal choices | Next visible state |
| --- | --- | --- |
| 1 | Gremlins demand four males, one female, nine gold; pay G | 56.10 HP, eight males/no females, 14 gold, 96 food |
| 2 | River; decline gaze | 56.20 HP, 92 food |
| 3 | River; decline gaze | 56.30 HP, 88 food |
| 4 | Six males and zero females have The Flu; cure three males, zero females; dismiss outcome | 56.40 HP, five males/no females, zero medicine, 85 food |
| 5 | River; decline gaze | 56.50 HP, 82 food |

The run stopped at the next status screen. Every encountered event and input is retained, including the three skipped river visits. No town appeared, so this ordinary pass adds interaction/discovery evidence but no further town validation. It showed no new unresolved branch; town behavior comparisons remain controlled experiments.

## Reproduction and limits

Run `./town_probe.py > doc/captures/2026-11-15-town-probes.txt` to regenerate the full fixture set, or pass case names to update selected entries. `--output PATH` supports independent capture batches; merge completed batches by case name before running the full comparison. The runner restarts the game after aborts/death when further cases remain. It waits through darts' delay without sending buffered extra answers.

`./check_towns.py --sanitize` runs the actual remake event and turn functions against the saved observations without DOSBox. It compares state, RNG, prompt order, input counts, and requested delays. Prompt text comparison normalizes whitespace and servant terminology; it does not establish byte-for-byte screen layout or actual elapsed delays. Numeric parsing policy is exercised by the existing generic integer-reader checks rather than reproducing Pascal's malformed input quirks.

These cases do not exhaust all seeds, prices, appearance strings, exact probability cutoffs, or combinations across services. The remaining combinations follow recovered binary control flow. Follow-up comparisons could cover City transactions using its shared services, a store plus bargain-market purchase in one visit, cleric removal immediately followed by muggers, ring thresholds exactly 10/30/60, socializing with nobody or a random rejection, and rats with zero food. Extreme original 16-bit overflow and fractional externally supplied gold/food remain outside scope; these location routines create no fractional inventory values from ordinary integer state.
