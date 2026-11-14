# Random events

This is a working reference combining observed gameplay with text extracted from `dos/BRAMINAR.COM`. Keep it updated as mechanics are recovered. Details described as string findings are provisional: strings establish possible messages, but event grouping, prerequisites, probabilities, and arithmetic need gameplay or code verification. File offsets below are hexadecimal offsets into the original executable.

Recovered rules and their binary evidence belong in [mechanics.md](mechanics.md). File formats and extraction-tool usage are described in [data-format.md](data-format.md).

The main loop selects one of 16 slots, with state-dependent substitutions and two separate hollow-tree routines. The recovered dispatch, healing, food consumption, starvation, and level advancement are described in [mechanics.md](mechanics.md#main-turn-loop). Location bodies remain incomplete in the remake except for the enchanted forest.

## Recorded playthroughs

Capture every encountered event, including encounters outside the current research focus. Keep raw screen snapshots and an input ledger, then summarize observed branches and state changes here. Binary-verified rules belong in `mechanics.md`; an observed message alone does not establish its inventory effect or probability.

- [2026-11-14, session 01](captures/2026-11-14-session-01.md): Pan's riddle, a goblin band, enchanted forest, occupied hollow tree, village market/tavern/store/inn, and a river vision. Includes the initial stats and intermediate status screens. The ledger distinguishes observed outcomes from rules subsequently recovered in the binary.

## City and Village

City and Village are similar.

Observed village sequence in session 01: enter → market → sell (male and female numeric quantities) → buy → tavern → drink → socialize → darts → store. Selling zero of each sex was accepted. A two-gold drink produced `It wasn't so hot.`; socializing produced `No one to socialize with.`; darts offered a five-gold ante and was declined. This is one path through the location, not a complete branch map. See the capture ledger for store offers and the eventual exit state.

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

The placement of the cleric and mugger encounters within city or village visits still needs verification.

## Combat

Strings near `0xD908` describe encountering a band of creatures that demands male servants, female servants, or gold. Choose to (F)ight their champion, (G)ive the demanded payment, or (R)un.

- Winning offers the band's hoard.
- Insufficient payment can lead to being robbed and killed.
- Fleeing may lose servants without a cart, succeed, or result in being caught and forced to fight.
- The ring can compel the player to fight.

Separate combat text exists for ordinary monsters, Anthrax the wizard, a dragon, the Mad King, and powerful creatures such as Cerberus, Lolth, and the Lich King. These may be called from other events rather than selected directly by the main loop.

Session 01 observed Goblins demanding 7 males, 9 females, and 7 gold. Choosing `R` produced `You got away!` with no servant loss. Binary inspection at `0xDDB5`–`0xDE34` shows the servant-loss branch requires a horse, no cart, and more than two servants; the warning's reference to having no cart alone is incomplete. Full combat and escape probabilities still need reconstruction.

## Dark Castle of the Mad King

- Choice of three rooms:
  - Dungeons
  - Throne room
  - Galley

Chance to steal gold in the throne room by successfully pressing a key combination: `1 / = \ H`. Mistyped key means dropped gold. Space bar to leave the room. If stay too long, Mad King will catch and initiate combat.

Galley has a crystal ball the player can gaze into. Galley has chance to find food and lose servants that fall down to pits.

Additional string findings:

- Dungeon: find gold, the Staff, male servants, or poison; the smell may overwhelm the player.
- The dungeon door can close. A dagger or servants can help the player escape; otherwise the player starves.
- The galley can contain the Talisman of Braminar.
- Crystal-ball visions describe possible locations of the Staff and Talisman. Whether visions reflect current item placement needs verification.

## Enchanted forest

- Elves give food
- Nymph
- Faerie

Additional strings describe elves curing ugliness, a faerie curing plague or restoring all hit points, and a nymph luring the player into being lost in La La Land.

The forest's flow and arithmetic have now been [recovered from the binary](mechanics.md#enchanted-forest) and implemented. Session 01 observed a female nymph, successful resistance, and elves' food gift: with nine servants, food went from 90 to 95 after the ten-unit gift and five-unit turn consumption. Other forest branches are verified from code but still await live captures. The faerie cures plague or restores HP; she does not do both on the same visit.

## Evil Overlord's Keep

- Initiate the final battle, if at least level 20 and have Staff of Aviatar

The Keep also asks for the prime command before allowing the attack. Binary inspection confirms that level 20 is sufficient and a failed command removes the Staff; see [mechanics.md](mechanics.md#overlords-keep-prerequisite). The location remains unimplemented.

## Flowing river

- Gaze into the river
  - Chance for combat
  - Chance to see a vision

Additional strings describe finding the Staff or Talisman in the river and being attacked by a Troglodyte champion. Visions include hints and humorous scenes; their relationship to actual game state is not yet established.

Session 01 chose to gaze and saw `An attractive tavern patron following you.` After the pause, the next status reflected only normal healing and food consumption. No item award or combat was observed on that visit.

## Hollow tree with a door

- Gnome smoking a pipe
- No-one is home, chance to (S)teal food, (T)ake from trunk or (L)eave

Trunk has a chance to have the Staff of Aviatar.

Additional strings describe the gnome announcing food or giving healing ointment, the trunk containing arrows or medicine, and releasing a Rune Guardian that burns the player. A later block repeats the empty-home encounter.

The main dispatch confirms two separate tree slots. Session 01 entered the occupied variant: the gnome announced three food and five HP of ointment. The HP award occurred; the food award did not. The [binary confirms this discrepancy](mechanics.md#gnomes-announced-food-gift). The second tree routine is selected independently by slot 13, while slot 6 can itself branch to an empty home.

## Grass hut

- Chance to find nothing
- Feed hungry puppy, receive reward for good deed
- Chance to find sick old man and heal him, receive reward for good deed

Additional strings in the hut's surrounding block (`0x6CD3` to `0x7558`) suggest:

- An old hermit with a dragon offers to double possessions if the player beats the dragon.
- The hermit may disappear with female servants; the player can be offered a choice to kill him, provoking the dragon.
- Find a rusty sword, arrows, the Staff, gold, servants, or a bow.
- Discover an astral gate through which a creature can appear.

These branches need control-flow verification before their grouping under the hut is considered confirmed. A later block repeats the old-man and puppy scenes and describes a watching deity rewarding a good deed or reducing the player's level. Possible rewards include servants, food, poison, and the Talisman.

## Mountain trail and guru

Strings near `0x7587` describe a mountain trail and ask whether to climb it. At the top, a guru asks whether the player seeks the powers of good.

- Nearby messages say the Staff and ring disappear.
- A fortune-teller rumor says the guru can remove the Ring of Doom.

Binary inspection places the guru in main-loop slot 8 only when the ring byte is positive; otherwise that slot produces the band encounter. After climbing, answering yes to seeking good removes the Staff if held and clears the ring byte (`0x765E`–`0x76CF`). The location remains unimplemented and has not yet been captured live.

## Sick servants

- Ask how many servants to cure
- Any uncured servants die

## Statue of Pan

- Ask a riddle, right answer gives the prime command

Session 01 asked `How much weight can an unladen swallow bear?` with choices `2 oz.`, `3 lbs.`, and `1.2 tons`. Choosing `1` produced `Nope.  The answer is # 2 The statue Disappears.` The [binary establishes](mechanics.md#statue-of-pan) that the answer number is rolled after the choice, rather than determined by the question. Questions use only the first 20 data records. The event remains unimplemented.

## Storm

- Multiple different storm types
- Ask if you want to take shelter
- If take shelter, wait for storm to pass
- Potentially lose servants if no shelter

## Thicket in the woods

- Chance for combat

Strings from around `0xDEF5` describe a dark dwelling or laboratory, with a roast cooking over a fire. Anthrax the evil wizard may appear.

- Find food and wine, servants, gold, or the Staff.
- Find potions and choose whether to try them.
- Potion messages describe losing one level, gaining two levels, changing hair color, losing servants transformed into fleeing gremlins, restoring 10 hit points, preventing servant illnesses, charming the next dragon, curing plague, curing ugliness, or drinking water or a zombie urine sample.
- On leaving, discover a pentacle and choose whether to step into it.
- Pentacle messages describe receiving a golden ring, being thrown out by a freezing wind, being hit by a pie, a figure appearing, or being raised to level 20 and given the Staff.

These are string findings; potion selection, effects without explicit numeric messages, pentacle outcomes, and their conditions need code verification.

## Wandering Gypsy Camp

- Play a game of Dagger's Hand
- Fortune teller

The fortune teller offers advice for 10 gold. Rumors mention the Ring of Doom aiding combat and compelling or corrupting its owner, the guru removing it, and other encounter hints.

## Additional player state to investigate

State beyond the visible status screen includes the Ring of Doom, plague, protection from servant illnesses, the ability to charm a dragon on the next meeting, and knowledge of the prime command. Ring growth, plague progression, the illness-protection gate, and prime-command generation are now documented in `mechanics.md`. Acquisition paths and other effects still need reconstruction; these state variables are not additional main-loop events.

The main-loop slots are now mapped in `mechanics.md`. The completeness of nested encounters and their conditions still needs investigation.
