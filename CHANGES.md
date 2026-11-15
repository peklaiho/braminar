# Differences from classic Braminar

This document records intentional differences between the classic DOS game and the remake. Unfinished features and unresolved mechanics are tracked in the research notes under `doc/`.

## Terminology and text

- Replace slaves with servants throughout the remake.

## Typo corrections

| Location | Classic text | Remake text |
| --- | --- | --- |
| Pan, question 3 answer 3 | `Do Lake:=1 to 5` | `Do Lake: 1 to 5` |
| Mountain trail | `He lookes up` | `He looks up` |
| Character status | `Mecidine:` | `Medicine:` |
| Enchanted forest | `The nymph it luring you deep into the forest.` | `The nymph is luring you deep into the forest.` |
| Band encounter | `Things seem strangely quite when out of no where jumps` | `Things seem strangely quiet when out of nowhere jumps` |
| Band encounter | `or you may fight their champ, and win their horde.` | `or you may fight their champ, and win their hoard.` |
| Forced combat | `Your ring compells you to fight.` | `Your ring compels you to fight.` |
| Unarmed combat | `You bump him abit.` | `You bump him a bit.` |
| Blade combat | `You severe a finger.` | `You sever a finger.` |
| Blade combat | `You nearly severe a limb.` | `You nearly sever a limb.` |
| Combat reward | `You find a shiny ring on your opponents finger & take it.` | `You find a shiny ring on your opponent's finger & take it.` |
| River vision | `A diety watching over a hollow tree home.` | `A deity watching over a hollow tree home.` |
| River vision | `A diety giving you a box of Duncin' Doughnuts` | `A deity giving you a box of Duncin' Doughnuts` |
| River vision | `A monsterous beast waiting in a pentacle` | `A monstrous beast waiting in a pentacle` |
| River vision | `A needed treasure in the Gnomes trunk` | `A needed treasure in the Gnome's trunk` |
| Storm losses | `You may of lost some slaves in the storm.` | `You may have lost some servants in the storm.` |
| Tidal wave | `All slaves & food is lost.` | `All servants & food are lost.` |

## Numeric precision

- Turn healing adds 0.10 HP instead of reproducing the classic executable's slightly smaller Pascal Real48 constant.
- HP and experience use C `double` arithmetic instead of Pascal Real48 storage and intermediate rounding.

## Bug fixes

- The gnome's food gift adds the announced 3–8 units to inventory. The classic game displays the gift without adding food. Normal end-of-turn food consumption still follows.

- Blank treatment lines retry instead of using the classic routine's uninitialized local value, which can consume medicine or change servant counts unpredictably.

- The castle proceeds to its room menu when no ghost minstrels appear. The original reads an uninitialized fear byte on that path; the remake confines fear to the minstrel encounter. No RNG draw is added. See [castle research](doc/castle-research.md).

## Integer input

- Use the shared `read_integer` for nonnegative decimal quantities. Negative numbers, letters, signs, hexadecimal, fractions, mixed text, overflow beyond C `INT_MAX`, and lines of 159 or more characters silently become zero. Surrounding whitespace and leading zeroes are accepted; empty or whitespace-only lines retry. EOF still ends input.
- This replaces the classic treatment reader's negative-number retries, hexadecimal/token parsing, signed 16-bit limit, and invalid-number aborts. The same policy applies to servant-market, store, and City-marketplace quantities.

## Runtime diagnostics

- An undetected Village pickpocket with zero gold still terminates the game, as the original does when it calls `Random(0)`. The remake prints `Division by zero.` to standard error and exits unsuccessfully instead of showing the Pascal runtime error number and code address.
