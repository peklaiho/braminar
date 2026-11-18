# Random events

This is a working reference combining observed gameplay with text extracted from `dos/BRAMINAR.COM`. Keep it updated as mechanics are recovered. Details described as string findings are provisional: strings establish possible messages, but event grouping, prerequisites, probabilities, and arithmetic need gameplay or code verification. File offsets below are hexadecimal offsets into the original executable.

Recovered rules and their binary evidence belong in [mechanics.md](mechanics.md). File formats and extraction-tool usage are described in [data-format.md](data-format.md).

Preserve the player-visible experience when implementing these findings. Minor numeric precision details with no observable effect may be simplified under the guideline in `mechanics.md`; record intentional differences in [CHANGES.md](../CHANGES.md). Keep original observations distinct from those implementation choices.

The main loop selects one of 16 slots, with state-dependent substitutions and two separate hollow-tree routines. The recovered dispatch, healing, food consumption, starvation, and level advancement are described in [mechanics.md](mechanics.md#main-turn-loop). The main-game location bodies are implemented, including both tree routines and their distinct outcomes; the Keep now hands off to the exit stub after bookkeeping; the separate final battle remains deferred.

## Recorded playthroughs

Capture every encountered event, including encounters outside the current research focus. Keep raw screen snapshots and an input ledger, then summarize observed branches and state changes here. Binary-verified rules belong in `mechanics.md`; an observed message alone does not establish its inventory effect or probability.

- [2026-11-14, session 01](captures/2026-11-14-session-01.md): Pan's riddle, a goblin band, enchanted forest, occupied hollow tree, village market/tavern/store/inn, and a river vision. Includes the initial stats and intermediate status screens. The ledger distinguishes observed outcomes from rules subsequently recovered in the binary.
- [2026-11-14, combat research](combat-research.md): an ordinary playthrough with two gnome visits, village trading and lodging, and a fatal Demon fight; twelve separately labelled controlled combat experiments with memory checkpoints.
- [2026-11-14, session 03](session-03.md): five ordinary turns covering a city visit, a victorious Drow fight after failed flight, a river vision, Keep rejection, and servant illness; five separate controlled river checks.
- [2026-11-14, session 04](session-04.md): ordinary play reaching an empty gnome home, Anthrax's laboratory, and a Gypsy camp; eight separate controlled empty-home checks.

- [2026-11-15, town research](town-research.md#ordinary-discovery-pass): a separate ordinary five-turn pass paid Gremlins, declined three river visits, and treated servant flu; no town appeared. Forty controlled City/Village cases are documented separately in the same ledger.

Additional targeted research: [City and Village](town-research.md), [Pan](pan-research.md), and [storms and servant illness](storm-illness-research.md), with controlled comparisons and explicit blank-input behavior differences.

## Startup and turn effects

[G4 evidence](startup-turn-research.md) consolidates existing ordinary startup observations and controlled healing, promotion, food, ring, plague, and death comparisons. Two new controlled cases use an ineligible Keep solely to reach bookkeeping: with zero food and two males/no females, starvation announces a loss and leaves one male; with one male/no females, no loss or starvation announcement occurs. Both cases finish at 50.10 HP, zero food, and no assault flag. [Raw captures](captures/2026-11-16-turn-starvation.txt) retain every Keep rejection, pause, input, and state; compact cases are in [Keep fixtures](keep-checkpoints.json). These experiments edit temporary player state/RNG and stop before any final-assault code; they are not ordinary play. No arithmetic correction was needed. Promotion/starvation message readability is recorded for G9.

## City and Village

City and Village now have shared service implementations and complete location wrappers. [Forty controlled comparisons](town-research.md) establish the implemented scope; [mechanics](mechanics.md#city-and-village) records binary-verified rules. Historical observations and provisional string findings below are retained as the discovery record.

Observed village sequence in session 01: enter → market → sell (male and female numeric quantities) → buy → tavern → drink → socialize → darts → store. Selling zero of each sex was accepted. A two-gold drink produced `It wasn't so hot.`; socializing produced `No one to socialize with.`; darts offered a five-gold ante and was declined. This is one path through the location, not a complete branch map. See the capture ledger for store offers and the eventual exit state.

The combat research playthrough sold two males and five females for 81 gold at its fixed prices, bought a bow, twelve arrows, and ten food, and paid twenty gold for room 3. Female servants bandaged wounds during lodging; the next status showed three HP restored plus normal turn healing. Store offer prices differed from session 01, and `You have:` followed the gold balance after each purchase rather than item stocks. Price generation and inn healing have since been [recovered](mechanics.md#general-store).

Binary inspection confirms that male and female servant prices are generated separately during player setup as `Random(10) + 5`, giving prices from 5 to 14 gold pieces. They stay fixed during the game and are used for both buying and selling. The two prices are displayed after gender selection, followed by a keypress pause. See [the binary findings](mechanics.md#servant-prices) for offsets and arithmetic.

- Servant market
- Chance to enter tavern
  - Buy a drink
  - Socialize
  - Play darts
- Store
  - Sword
  - Bows
  - Daggers
  - 12 Arrows
  - Food
  - Medicine
  - Horses
  - Carts
- Inn
  - Choice of three different level rooms

Additional string findings:

- Village: notice a pickpocket and stop him.
- City marketplace: buy food at five units per gold piece or poison at 100 gold per dose. This appears separate from the general store.
- A cleric offers to cure all ailments in exchange for all your gold.
- Muggers attack on leaving town and steal gold and food; the ring can drive them away with lightning.
- Tavern drinks may heal or be poisoned; medicine can cure the poisoning.
- Socializing can recruit a patron as a servant.
- Rats may eat food while resting; female servants may bandage wounds.

Subsequent binary inspection places the extra marketplace after the City store, then the inn, cleric, and muggers. The Village instead checks for a pickpocket before shared services; it has none of those City-only extras. Muggers require no servants; the cleric can remove the ring before their protection check. [Recovered flow](mechanics.md#entry-and-location-differences).

Two [G3 controlled City visits](town-research.md#g3-city-interactions) now compare those interactions directly. Shared servant/store purchases followed by bargain-market food and poison leave 31 gold and, after consumption for twelve servants plus the player, 114 food. In a separate visit with no servants, the cleric takes all fifty gold and removes the ring before muggers steal zero gold and eighteen food; the final two food are unchanged by bookkeeping. Both visits match the remake without gameplay changes. These are temporary-memory/RNG experiments, not ordinary play; [raw captures](captures/2026-11-16-city-interactions.txt) retain all encounters, declined services, inputs, and states, with compact records in [town fixtures](town-checkpoints.json).

Session 03 entered a city and declined market, tavern, store, and inn in that order. It returned directly to status with only normal healing and food use. No extra marketplace, cleric, or mugger scene appeared on this visit; their triggers have since been recovered in the [town rules](mechanics.md#city-and-village).

## Combat

Strings near `0xD908` describe encountering a band of creatures that demands male servants, female servants, or gold. Choose to (F)ight their champion, (G)ive the demanded payment, or (R)un.

- Winning offers the band's hoard.
- Insufficient payment can lead to being robbed and killed.
- Fleeing may lose servants without a cart, succeed, or result in being caught and forced to fight.
- The ring can compel the player to fight.

Separate combat text exists for ordinary monsters, Anthrax the wizard, a dragon, the Mad King, and powerful creatures such as Cerberus, Lolth, and the Lich King. These may be called from other events rather than selected directly by the main loop.

Session 01 observed Goblins demanding 7 males, 9 females, and 7 gold. Choosing `R` produced `You got away!` with no servant loss. The [band and shared combat rules](mechanics.md#band-encounters) are now recovered and implemented: template selection, all three payment demands, horse/cart losses, escape conditions, equipment destruction, poison, attack and damage tables, pauses, experience, ring awards, and loot. The servant-loss branch requires a horse, no cart, and more than two servants, and reduces each sex's count to at most one.

The ordinary combat research run fought Demons, used all twelve arrows, continued unarmed, and died at -1.73 HP with the enemy at 31.37 HP. Separately labelled controlled probes verify victories, death, payment, fleeing, ring effects, and equipment behavior against the remake. The shared routine is called by bands and the river champion; the dragon and gate-creature routines are now implemented and compared as part of [grass-hut research](hut-research.md). Anthrax combat is now implemented and compared in [thicket research](thicket-research.md); Mad King combat is now independently recovered and compared in [castle research](castle-research.md).

G8 controlled probes now capture simultaneous deaths in shared Goblin, dragon, Cerberus, and Mad King combat. Each reports player death independently of the opponent's death/disappearance, grants no survivor rewards, and reaches Game Over after the result/caller pause. The dragon's hermit also skips doubling/betrayal. No gameplay correction was needed. [Setups, raw captures, and limits](combat-order-research.md). Anthrax's already captured experience award on mutual death remains a separate rule.

## Dark Castle of the Mad King

All three rooms and the separate Mad King fight are recovered and implemented. Entry can produce ghost minstrels and fear; survivors choose Dungeons, Throne Room, or Galley once, then leave.

The dungeon gives gold and can inflict smell damage, grant the Staff, rescue male servants, supply poison, or lock its door. A dagger or current servants permit escape; otherwise starvation is fatal. The throne room accepts the sequence `1 / = \ H` to bag gold. Incorrect recognized keys add noise without removing gold or resetting progress. Every key advances noise, including unrelated keys and Space; above fifteen, the King attacks. Waiting does not itself trigger him.

The galley offers random crystal-ball hints, a possible Talisman, zero to 199 food, and pits that halve each sex's servant count. The hints do not reflect item placement or possessions. King combat has independent attack tables, poison, ring effects, six shouts, experience/gold rewards, and retaliation even after a killing blow.

[Research notes and controlled captures](castle-research.md) record the comparison scope, reproducible fixtures, and the original uninitialized fear local. Exact draw order and rules are in [mechanics](mechanics.md#dark-castle).

## Enchanted forest

- Elves give food
- Nymph
- Faerie

Additional strings describe elves curing ugliness, a faerie curing plague or restoring all hit points, and a nymph luring the player into being lost in La La Land.

The forest's flow and arithmetic have been [recovered from the binary](mechanics.md#enchanted-forest) and implemented. Session 01 observed a female nymph, successful resistance, and elves' food gift: with nine servants, food went from 90 to 95 after the ten-unit gift and five-unit turn consumption.

[G5 controlled chains](ring-research.md) now compare ring persistence across event boundaries: a poisoned Rust Golem victory awards a ring, then the next forest excludes elves and leaves bad looks unchanged; accepting the guru's removal instead lets the next forest give eleven food and restore looks. Ring power goes zero → five at acquisition → ten after that turn → fifteen after the excluded forest; removal goes twenty → zero and stays zero. All four encounters and inputs are retained in [raw captures](captures/2026-11-16-ring-chains.txt) and [fixtures](ring-checkpoints.json). Only initial temporary state/RNG is edited per chain; these are not ordinary-play observations. Both match the remake without gameplay corrections.

The [G1 follow-up](forest-research.md) separately captures six controlled outcomes: decline, ring exclusion, elves restoring looks followed by faerie plague cure, faerie healing, faerie at full health, and fatal lure by a male nymph. All match the existing remake. The faerie cures plague or restores HP; she does not do both on the same visit. With ten servants, the elves add eleven food before five-unit turn consumption. A ring prevents both that gift and appearance restoration. Fatal lure skips elves/faerie, pauses, then prints `Game Over.`. Declining goes directly to the next status without an exit pause. These experiments edit temporary player state and RNG; they are distinct from ordinary play. The [raw transcript](captures/2026-11-16-forest-probes.txt) and [fixtures](forest-checkpoints.json) retain every encountered forest and input.

## Evil Overlord's Keep

- Initiate the final battle, if at least level 20 and have Staff of Aviatar

The Keep asks whether to use the Staff, then reads a case-sensitive command of at most five characters. A wrong command removes the Staff; a match sets the assault flag. Every completed path pauses before normal bookkeeping, and death during that bookkeeping prevents the handoff. The implemented branch calls `start_final_battle()` to print its deferred-battle message and exit; the assault itself remains unimplemented. [Research and controlled captures](keep-research.md) cover gates, command boundaries, bookkeeping, and Pan-to-Keep persistence. Session 03's ordinary level-nine rejection remains separate evidence. See [mechanics](mechanics.md#overlords-keep-prerequisite) for exact rules.

## Flowing river

The [G7 Staff-recovery sequence](quest-state-research.md) captures a level-twenty player losing the Staff to a wrong Keep command, recovering it from the next river, and receiving the attack offer at the next Keep. The final offer is declined; Staff and stored command `PRIOR` remain, HP is 50.30 and food eighty-five after three turns, and the assault flag stays false. [Raw capture](captures/2026-11-16-staff-recovery.txt) and [Keep fixtures](keep-checkpoints.json) retain all three encounters and inputs. Only initial temporary state/RNG is edited; this is controlled evidence, not ordinary play, and no final-assault code executes.

- Gaze into the river
  - Chance for combat
  - Chance to see a vision

Gazing produces one of twenty flavor visions, then independent chances of finding the Staff (9% if absent), finding the Talisman (5% if absent), and fighting a Troglodyte champion (25%). The vision itself is independent of game state and does not apply the depicted effect. This flow and its exact RNG order are now [verified from the binary](mechanics.md#flowing-river) and implemented.

Session 01 chose to gaze and saw `An attractive tavern patron following you.` After the pause, the next status reflected only normal healing and food consumption. No item award or combat was observed on that visit.

Session 03 saw `A trap door in the Holiday out hotel`, again without items or combat. Five separate controlled checks cover harmless visions, both quest-item awards together, already-held items, a poisoned champion, and declining to gaze. The champion path has a river exit pause after the combat result pause. See the [session ledger](session-03.md#controlled-river-checks).

## Hollow tree with a door

- Gnome smoking a pipe
- No-one is home, chance to (S)teal food, (T)ake from trunk or (L)eave

Trunk has a chance to have the Staff of Aviatar.

Additional strings describe the gnome announcing food or giving healing ointment, the trunk containing arrows or medicine, and releasing a Rune Guardian that burns the player. A later block repeats the empty-home encounter.

The main dispatch confirms two separate tree slots. Session 01 entered the occupied variant: the gnome announced three food and five HP of ointment. The HP award occurred; the food award did not. The [binary confirms this discrepancy](mechanics.md#gnomes-announced-food-gift). The second tree routine is selected independently by slot 13, while slot 6 can itself branch to an empty home.

The remake implements the occupied gnome branch and intentionally fixes its missing food award: the announced 3–8 food is added to inventory before normal turn consumption. Healing remains 1–5 HP when below `level * 10`, without a subsequent cap. See [CHANGES.md](../CHANGES.md#bug-fixes). Slot 6's empty-home choices are now implemented: steal 1–100 food, take a random trunk outcome (arrows, medicine, Staff or substitute medicine, Rune Guardian damage), or leave. See [recovered rules](mechanics.md#empty-gnome-home) and [session 04](session-04.md). Slot 13 is now independently implemented: its tree actions are judged by a deity rather than granting ordinary trunk/food loot; it can also select a hut with an old man or puppy. Declining help loses a level; leaving the entered tree or offering help earns one of five rewards, even without supplies. See [watched-home research](watched-home-research.md) for controlled evidence and [mechanics](mechanics.md#watched-tree-and-hut) for exact rules.

The ordinary combat research playthrough observed two more gnome visits: announced food/healing of five/one and three/two respectively. Both visits again showed healing applied but no food gift in the classic. See the [combat ledger](combat-research.md) for before/after stats.

The [G2 follow-up](gnome-research.md) separately captures three controlled occupied-home visits at level eight: 50 HP heals to 54 (54.10 after bookkeeping), 79 heals past the cap to 83, and 80 receives no ointment and stays 80. Every visit announces six food without adding it in the original, leaving 95 food after consumption. The remake's intentional gift addition instead leaves 101; all other captured state, event text, and pauses match. These experiments edit temporary player state and RNG and do not add ordinary-play observations. [Raw captures](captures/2026-11-16-gnome-probes.txt) and [fixtures](gnome-checkpoints.json) retain every encounter and input.

## Grass hut

The [G7 repeated-dragon comparison](quest-state-research.md) captures two consecutive hermit challenges starting with charm one. Both dragons submit, charm remains one throughout, and both hermits double possessions. Final state is 50.20 HP, twenty servants of each sex, eighty gold, 360 food, twelve medicine, and three experience. The earlier potion-acquisition chain is reused. [Raw capture](captures/2026-11-16-repeat-dragon.txt) and [thicket fixtures](thicket-checkpoints.json) retain both huts, all inputs, and intermediate/final states without inter-event edits. This is controlled evidence and matches the remake without gameplay corrections.

The hut is now implemented, including all occupied/quiet outcomes and the separate dragon and gate-creature fights. [Research notes](hut-research.md) distinguish binary rules, controlled comparisons, and remaining validation limits.

- Before asking entry, a roll announces either sounds or silence. Declining also pauses.
- In a noisy hut, meet the sick old man, hungry puppy, or hermit with a dragon. Helping the man uses one medicine if available, otherwise comforts him. Helping the puppy uses one food if available, otherwise pets it. These two scenes do not give rewards in slot 7: the similar reward-bearing scenes belong to the independently implemented [slot-13 event](watched-home-research.md).
- The hermit offers doubled possessions after defeating the dragon. On a betrayal outcome he announces taking female servants but does not actually subtract them. Refusing his challenge instead offers to kill him, provoking the dragon without a doubling/betrayal reward afterward.
- Quiet-hut finds include a sword, arrows, a rare Staff, gold, either sex of servant, a bow, an astral gate, or nothing. A gate can close or summon one of five creatures.
- A surviving creature fight grants experience and one of six rewards. All displayed “50 Lbs of food” outcomes actually add 250 food. The remake preserves both that discrepancy and the hermit's announced-only theft.

Session 04's ordinary empty-hut visit remains recorded separately: after its pause, status showed only the normal 0.10 HP healing and 42 food consumed for 84 servants plus the player. The new probes modify player state and RNG; they are not additional ordinary visits.

## Mountain trail and guru

Strings near `0x7587` describe a mountain trail and ask whether to climb it. At the top, a guru asks whether the player seeks the powers of good.

- Nearby messages say the Staff and ring disappear.
- A fortune-teller rumor says the guru can remove the Ring of Doom.

Binary inspection places the guru in main-loop slot 8 only when the ring byte is positive; otherwise that slot produces the band encounter. After climbing, answering yes to seeking good removes the Staff if held and clears the ring byte (`0x765E`–`0x76CF`). The event is now implemented. Five controlled original-game comparisons capture declining the climb, refusing the guru, accepting with and without the Staff, and removing a ring at power 100. All paths pause; refusal adds no response message. The Talisman is preserved. These experiments reset player memory and RNG and are separate from ordinary play. See [research notes](mountain-research.md).

## Sick servants

- Ask how many servants to cure
- Any uncured servants die

Session 03 observed 9 males and 1 female with Old-Timer's disease. Treating 2 males and 1 female consumed three medicine, removed seven males, and left all females alive. Independent sick-count rolls, disease selection, medicine allocation, losses, and an excessive-treatment bug are now [recovered from the binary](mechanics.md#servant-illness). Treatment is now implemented, including medicine priority, uncured losses, the original excessive-cure behavior, and input handling. Controlled checks cover these branches, zero/one-sex parties, and protection substitution; see the [storm/illness ledger](storm-illness-research.md). The disease label alone does not infect the player with plague.

Two [G6 controlled chains](illness-state-research.md) now cover state carried between encounters. Anthrax infects a previously unplagued player, bookkeeping grows plague from five to ten, and the next forest's faerie cures it without full healing. In the other chain, one laboratory potion grants servant-illness protection; the next slot-9 event substitutes a band, with no disease/treatment prompts. Paying the band's seven males, ten females, and five gold leaves forty males/twenty-three females, eighty-one gold, forty-eight food, three medicine, and protection intact. All four encounters, inputs, and states are retained in [raw captures](captures/2026-11-16-illness-chains.txt) and [thicket fixtures](thicket-checkpoints.json). These experiments edit temporary memory/RNG only before each chain and match the remake without gameplay corrections.

## Statue of Pan

- Ask a riddle, right answer gives the prime command

Session 01 asked `How much weight can an unladen swallow bear?` with choices `2 oz.`, `3 lbs.`, and `1.2 tons`. Choosing `1` produced `Nope.  The answer is # 2 The statue Disappears.` The [binary establishes](mechanics.md#statue-of-pan) that the answer number is rolled after the choice, rather than determined by the question. Questions use only the first 20 data records. The event is now implemented. Five [controlled cases](pan-research.md) additionally captured success, failure, decline, invalid answer keys, and records 1, 3, and 20; success replaced an existing command, while failure and decline preserved it. These experiments modified player memory and RNG and are separate from Session 01 ordinary play.

## Storm

- Multiple different storm types
- Ask if you want to take shelter
- If take shelter, wait for storm to pass
- Potentially lose servants if no shelter

Session 04 declined shelter from a tornado, lost servants, and then suffered starvation. Binary inspection separates the storm losses (five males, two females) from starvation (one of each). All eight storm types and shelter/loss arithmetic are now [documented](mechanics.md#storms); the event is now implemented and checked against controlled original-game captures. See the [storm/illness ledger](storm-illness-research.md).

## Thicket in the woods

The thicket is now implemented with its full laboratory flow, Anthrax fight, potions, and pentacle. [Research notes](thicket-research.md) distinguish controlled comparisons from ordinary discovery and document the remaining validation limits.

- Enter a laboratory with a displayed 1–8 lb roast. Anthrax appears on rolls 1–50 out of 100. Survivors take food equal to ten times the roast weight, then discover male/female servants and gold; they may also find the Staff.
- A potion offer contains one to five bottles. Accepting drinks the whole batch. Effects include losing one level, gaining two, changing hair, losing all servants as fleeing gremlins, a harmless urine sample, adding ten HP, illness protection, incrementing persistent dragon charm, curing plague, restoring good looks, or water. Only the first eight listed hair colors are reachable.
- A pentacle can award or replace the ring, deal fifteen HP with a freezing wind, throw a harmless pie, summon the same creature routine used by the hut's gate, or grant the Staff and raise a lower level to twenty. It preserves levels already above twenty and supplies no prime command.
- Anthrax has a separate nine-outcome combat routine. Any held ring defeats him immediately; poison also kills without a saving throw. Normal spells can change looks, servants, level, hair, or plague, damage the player, or rebound against the wizard. Retaliation still happens after a killing blow.

Session 04's ordinary laboratory victory after level drain, servant deaths, plague, staff damage, and lightning remains in [its ledger](session-04.md#anthrax-observations). The newer controlled cases reset player memory and RNG and include all potion effects, pentacle outcomes, wizard spells, and a two-event potion-to-dragon path with no edits between the events.

A separate five-turn [ordinary discovery pass](captures/2026-11-15-thicket-ordinary.txt) declined the Dark Castle, escaped Rust Golems, received the Keep's prerequisite rejection, then declined Village and forest entry. No thicket appeared; [the research ledger](thicket-research.md#ordinary-discovery-pass) records every encounter and visible result.

## Wandering Gypsy Camp

The current [research ledger](gypsy-research.md) also records a separate ordinary discovery pass: Village declined, hollow tree declined, and a Gremlin demand left at its choice prompt. Every encountered screen is retained; no camp appeared in that pass.

- Play a game of Dagger's Hand
- Fortune teller

The fortune teller offers advice for 10 gold. Rumors mention the Ring of Doom aiding combat and compelling or corrupting its owner, the guru removing it, and other encounter hints.

Session 04 played Dagger's Hand: the player succeeded, the opponent hit a finger, and the player won. After the result pause, the fortune teller offered advice; buying it produced `Pan will tell you the Prime Command.` Gold changed from 108 to 99 across the game and advice together. This ordinary observation is retained in [the ledger](session-04.md). Subsequent binary inspection recovered the accumulating pot, 1–15 injury thresholds, both participants' turns even after injury, and all 35 paid rumors. The camp is now implemented; [controlled research](gypsy-research.md) checks losses, wins, simultaneous injuries, multiple rounds, money boundaries, advice, and fatal injury followed by advice. These probes reset player state and RNG and are separate from the ordinary pass.

## Additional player state to investigate

State beyond the visible status screen includes the Ring of Doom, plague, protection from servant illnesses, a persistent dragon-charm counter, and knowledge of the prime command. Ring acquisition through shared combat, attack bonuses, forced fighting, growth, plague progression, the illness-protection gate, and prime-command generation are documented in `mechanics.md`. Other acquisition paths and effects still need reconstruction; these state variables are not additional main-loop events.

The main-loop slots are now mapped in `mechanics.md`. The completeness of nested encounters and their conditions still needs investigation.

A separate [three-turn ordinary pass](captures/2026-11-15-watched-home-ordinary.txt) declined a noisy hut, a hollow tree, and a river vision. The first two silently paused after refusal, matching slot 13's recovered behavior. Visible changes were normal healing and food consumption only; [the watched-home ledger](watched-home-research.md#ordinary-discovery-pass) records every encounter and the inference identifying the repeated locations.

## Final assault opening (controlled research, 2026-11-18)

The user opened the previously deferred assault module for research. A [repeatable memory probe](final-assault-research.md#reliable-entry-and-memory-map) now automates a synthetic successful Keep and restores the desired player/RNG state at the loaded assault entry. These runs skip adventure progression and are controlled experiments. Every synthetic Keep input and resulting screen is retained in the linked raw captures.

The [initial exploration](captures/2026-11-18-final-explore.txt) encountered the successful Keep glow/pause, `Now attacking Overlord's castle!`, an asset conversion report (882 gold, 126 soldiers versus 74), then an automatic army display with 109 surviving player soldiers and no enemies. It stopped before a duel or ending. The [repeatable baseline](captures/2026-11-18-final-baseline.txt) restores seed one at assault entry and instead reports 838 gold, 120 soldiers versus 160. This difference follows from the deliberately different RNG setup.

The [conversion batch](captures/2026-11-18-final-conversion.txt) and [boundary batch](captures/2026-11-18-final-boundaries.txt) stop before any army combat, including the synthetic zero-soldier case. Net worth uses servants, supplies, weapons, transport and a bonus above level twenty; inventory itself stays intact. [Recovered rules](mechanics.md#final-assault-asset-conversion), [checkpoints](final-checkpoints.json) and [next targets](final-assault-research.md#next-targets) separate the implemented opening from the remaining army/duel/ending work. A [separate timed continuation](captures/2026-11-18-final-flow.txt) shows the seed-one army defeated with 128 enemy survivors, then `Your army perished.  You are captured by the Overlord.` and return to DOS. It is not a complete combat trace. No duel or ending was encountered in this continuation.

## Final assault army battle (controlled research, 2026-11-18 follow-up)

[Army research](army-research.md) extends the preceding opening work. All new runs use temporary memory/RNG edits and the real assault loader; none is ordinary adventure play. [Event captures](captures/2026-11-18-army-events.txt) retain all ten descriptions under both Player and Overlord labels: low/high morale, captured magic user/loss of a magic user, successful assassin/assassinated captain, successful surprise/enemy surprise, and friendly/enemy magical weapons. Four small-army cases and surprise overkill are included. The same “your” descriptions occur under the Overlord label; exact texts and effects are in [mechanics](mechanics.md#final-assault-army-battle).

[Full battle captures](captures/2026-11-18-army-battles.txt) record a 120-versus-160 army defeated after 21 batches and two events, leaving 128 enemies, plus 1/1 and 5/5 defeats and an 8/2 victory. Defeats print the capture message and exit to DOS; victories stop before the duel. All intermediate casualty/event frames and RNG states are retained without inter-round edits. No army choice or keypress pause was encountered.

[Zero-army captures](captures/2026-11-18-army-zero.txt) show `Run-time error 02`/`Program aborted` and return to DOS when either initial percentage denominator is zero, after the routine still executes its first five casualty trials. These supersede an initial probe's diagnostic-spelling assertion failure, retained at the end of the full-battle capture. [Compact event](army-events-checkpoints.json) and [battle checkpoints](army-battles-checkpoints.json) support remake comparisons without the ignored transcripts. Army combat is implemented; the duel and endings remain explicit research targets.

## Overlord duel (controlled research, 2026-11-18 continuation)

[Initial exploration](captures/2026-11-18-duel-explore.txt), [controlled round captures](captures/2026-11-18-duel-rounds.txt), [complete duels](captures/2026-11-18-duel-full.txt) and the [level-twenty trace](captures/2026-11-18-duel-endgame.txt) continue through a real controlled army victory into the original duel. These are memory/RNG experiments, not ordinary play. [Research notes](duel-research.md) explain reusable stack-frame edits versus uninterrupted fights and retain the superseded preliminary full-batch capture.

Encountered all eight player attacks and all eight enemy attacks, the Talisman shortcut, normal victory, normal defeat, simultaneous death, both grave displays, attack-message clearing and post-death return to DOS. There are no duel choices or keypress pauses. Saved HP and inventory remain unchanged: the duel uses fresh local HP from level, and attacks do not require their named weapons. The Overlord retaliates after a killing blow; simultaneous death prints the player's death message even while displaying the enemy grave.

Victorious probes stop before the ending routine; no rewards, corruption/appearance ending or printer certificate was entered. The remake now implements the duel and retains a placeholder only for those remaining victory branches. [Rules](mechanics.md#final-assault-overlord-duel), [round checkpoints](duel-rounds-checkpoints.json) and [complete-duel checkpoints](duel-full-checkpoints.json) retain the observed state, texts and frame order.
