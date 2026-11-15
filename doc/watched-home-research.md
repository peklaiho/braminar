# Watched tree and hut research

## Evidence and scope

Main-loop slot 13 calls `dos/BRAMINAR.COM` file offsets `0x8F9B`–`0x96E6`. Binary inspection establishes that this routine contains both a hollow tree and a noisy hut, selected before asking entry. The tree repeats slot 6's empty-home description, but its actions have different consequences. The hut repeats slot 7's old-man/puppy scenes, but adds divine judgment. Neither earlier event's outcome processing should be reused. The complete slot-13 routine is implemented; [mechanics](mechanics.md#watched-tree-and-hut) gives exact thresholds, draw order, and arithmetic.

The targeted reconstruction uses controlled experiments; a separate ordinary discovery pass is recorded below. `watched_home_probe.py` uses the existing `dosbox_game.Game` runner, resetting temporary player state, servant prices 8/13, and RNG for each visit. Original files in `dos/` remain untouched. Configurations, screens, inputs, and memory at each input boundary are retained in [raw captures](captures/2026-11-15-watched-home-probes.txt) and compact [checkpoints](watched-home-checkpoints.json). Initial displayed status predates memory edits; `STATE initial` records the actual case setup.

The baseline is level 8, 50 HP, Black hair, bad looks, five servants of each sex, 20 gold, 100 food, three medicine, two swords, three bows, four arrows, one dagger, horse/cart, and no quest items, ring, plague, charm, protection, poison, or experience. Case configurations record exceptions. Food-boundary puppy cases have no servants so subsequent starvation does not obscure the one-unit feeding cost.

## Observed behavior and comparisons

The 32 named cases cover:

- Tree selection at roll 50 and hut selection at 51, and declining each. Both declines silently end at the common pause.
- Tree stealing and trunk-taking: no food/items are granted; the deity removes one level. A held Staff and a power-100 ring give no exemption. Level one can fall to zero, and HP above the new level's usual maximum is preserved.
- Leaving the entered tree: all five reward rolls, including the Talisman and the held-Talisman doughnut substitute.
- Both hut occupants, refusing help, and all five reward rolls for helping each. Refusing help removes one level.
- Helping the man with ordinary, last, or no medicine; helping the puppy with ordinary, last, or no food. Offering comfort or petting without supplies still earns a reward. Both held-Talisman substitutes are included.
- An invalid action key followed by lowercase `s`, establishing the retry without an extra draw or premature judgment.

`./watched_home_probe.py > doc/captures/2026-11-15-watched-home-probes.txt` reproduces the cases. Named arguments select cases in the requested order; `--output PATH` selects a separate fixture file. The runner waits through the one-second care delay before sampling the next input boundary.

`./check_watched_home.py --sanitize` compares state and RNG at every input and after bookkeeping, normalized text/prompt order, key counts, and requested delays. All 32 cases pass with address and undefined-behavior sanitizers. EOF terminates cleanly at six distinct prompts. All ten existing sanitizer suites also pass, as do `make`, Python syntax checks, and whitespace checks. The preexisting unused-final-battle-stub compiler warning remains. The five rewards are twelve male servants, twelve female servants, fifty food, one poison, or the Talisman (twelve food if already held). Servant rewards affect the same turn's food consumption.

## Limits and intentional differences

The binary establishes the full selection and reward rules; captures sample deterministic seeds rather than establishing probabilities statistically. Extreme signed-integer overflow, negative initial levels, and fractional inventory supplied through memory edits remain outside this target. The remake's ordinary double-precision policy applies. The two occurrences of `diety` become `deity`, recorded in CHANGES.md. No gameplay difference is introduced for this event.

## Ordinary discovery pass

The [three-turn ordinary pass](captures/2026-11-15-watched-home-ordinary.txt) makes no player-memory or RNG edits. TreeWalk starts female, level 7, 38 HP, Red hair, good looks, four servants of each sex, six gold, 100 food, three medicine, no equipment/transport, and no quest items. Servant prices are 7/7.

| Turn | Encounter and ordinary choice | Visible state after bookkeeping |
| --- | --- | --- |
| 1 | Noisy grass hut: decline entry, advance pause | 38.10 HP, 96 food |
| 2 | Hollow tree: decline entry, advance pause | 38.20 HP, 92 food |
| 3 | Flowing river: decline gazing; no pause | 38.30 HP, 88 food |

Level, party, gold, medicine, equipment, looks, hair, and quest items remain unchanged. The two silent entry refusals identify the first two encounters as slot 13 under the recovered control flow; this is an inference from the displayed behavior, not a memory/RNG read. Slot 6 and slot 7 would print a departure line on refusal. Every input and encountered screen is retained. The run ends at the next status screen, adds ordinary decline/turn-flow coverage, and establishes no new reward probabilities.
