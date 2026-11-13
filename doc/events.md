# Random events

This is a working reference combining observed gameplay with text extracted from `dos/BRAMINAR.COM`. Keep it updated as mechanics are recovered. Details described as string findings are provisional: strings establish possible messages, but event grouping, prerequisites, probabilities, and arithmetic need gameplay or code verification. File offsets below are hexadecimal offsets into the original executable.

Recovered rules and their binary evidence belong in [mechanics.md](mechanics.md). File formats and extraction-tool usage are described in [data-format.md](data-format.md).

Each turn has chance for one of the listed random events.

Additionally each turn restores little Hit Points.
Each turn also consumes food (more servants = more food consumed). If no food, servants start to die from starvation.

## City and Village

City and Village are similar.

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

Additional strings describe elves curing ugliness, a faerie curing plague and restoring all hit points, and a nymph luring the player into being lost in La La Land. The conditions for resisting the nymph are not yet known.

## Evil Overlord's Keep

- Initiate the final battle, if over level 20 and have Staff of Aviatar

The introduction says level 20 is sufficient, and the Keep asks for the prime command before allowing the attack. The exact level comparison still needs code verification. A failed attempt can make the Staff disappear.

## Flowing river

- Gaze into the river
  - Chance for combat
  - Chance to see a vision

Additional strings describe finding the Staff or Talisman in the river and being attacked by a Troglodyte champion. Visions include hints and humorous scenes; their relationship to actual game state is not yet established.

## Hollow tree with a door

- Gnome smoking a pipe
- No-one is home, chance to (S)teal food, (T)ake from trunk or (L)eave

Trunk has a chance to have the Staff of Aviatar.

Additional strings describe the gnome giving food or healing ointment, the trunk containing arrows or medicine, and releasing a Rune Guardian that burns the player. A later block repeats the empty-home encounter; whether this is a separate selectable event is not yet known.

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

The exact effects, prerequisites, and position in the event-selection logic remain unverified.

## Sick servants

- Ask how many servants to cure
- Any uncured servants die

## Statue of Pan

- Ask a riddle, right answer gives the prime command

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

The strings imply state beyond the visible status screen: possession of the Ring of Doom, plague, protection from servant illnesses, the ability to charm a dragon on the next meeting, and knowledge of the prime command. These are mechanics to recover, not additional confirmed random events.

The completeness of this list and the distinction between main-loop events and nested encounters must be checked against the binary's event-selection code.
