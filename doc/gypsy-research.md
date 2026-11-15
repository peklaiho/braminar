# Wandering Gypsy camp research

## Evidence and scope

Binary inspection of `dos/BRAMINAR.COM`, file offsets `0xCB86`–`0xD8F9`, recovers the complete slot-3 camp: entry, Dagger's Hand, and the fortune teller's 35 hints. [Detailed rules](mechanics.md#wandering-gypsy-camp) distinguish the accumulating pot, injury damage, random-call order, affordability tests, pauses, and delays. The camp is implemented without unresolved event-body placeholders.

The [raw controlled capture](captures/2026-11-15-gypsy-probes.txt) is separate from [session 04's ordinary visit](session-04.md). `gypsy_probe.py` resets temporary emulator memory and RNG before each case; original files are untouched. The baseline is level 8, 50 HP, 20 gold, 100 food, five servants of each sex, three medicine, Black hair, good looks, and illness protection. Equipment, transport, quest items, ring, plague, and experience are zero. Individual cases vary seed, gold, HP, and choices. Every encountered event in these experiments is a camp; no unrelated branches are skipped.

The runner saves [checkpoints](gypsy-checkpoints.json) immediately after each completed case and supports selected reruns. Gambling advances automatically; the runner captures intermediate screens while waiting for the result pause and never injects an extra key during a round.

## Controlled comparisons

The 51 cases cover:

- Entry refusal and an entered visit skipping both activities.
- Zero-gold acceptance and refusal of gambling; both print insufficient funds without a result pause.
- Player injury, opponent injury, and both participants hitting a finger. A normal loss changes 20 gold to 19 and HP from 50 to 49.70 after turn healing. A first-round win changes gold to 21. Simultaneous injuries also produce 21 gold, while retaining the HP injury and printing both loss and win messages.
- A two-round win: two antes and a four-gold pot yield 22 gold from 20.
- Exact injury boundaries: player/opponent rolls 15/16 and 16/15.
- Last-coin exhaustion (one gold becomes zero after both succeed) and last-coin victory (one gold becomes two when the opponent hits a finger).
- Advice refused, accepted with insufficient funds, and rendered unaffordable by a gambling loss (ten gold becomes nine). A win followed by paid advice changes 20 gold to 11.
- All 35 rumor indices captured individually, with exact ten-gold affordability on rumor 1 and eleven gold on the others. Purchases leave zero or one gold respectively and each pauses once.
- Fatal finger injury starting at 0.2 HP, followed by buying advice before death is processed. The comparison retains event state before bookkeeping because DOS reclaims DS on exit; it does not claim a post-death memory comparison.

`./check_gypsy.py --sanitize` compares state and RNG at every input boundary, event text visible in retained screens (with servant terminology normalized), exact key counts, final state, and requested delays. Gambling delay expectations follow the binary's three half-second requests per round. These are delay requests, not measurements of emulated elapsed time. Final HP comparisons allow the documented Real48/double precision difference.

## Ordinary discovery pass

A separate [ordinary capture](captures/2026-11-15-gypsy-ordinary.txt) makes no memory or RNG edits. CampWalk started female, level 8, 38 HP, Blonde hair, good looks, 11 male and 14 female servants, 16 gold, 100 food, and three medicine; servant prices were 10/14. Equipment and quest items were absent.

| Turn | Encounter and choice | Visible result |
| --- | --- | --- |
| 1 | Village, decline entry | 38.10 HP, 87 food; party and gold unchanged |
| 2 | Hollow tree, decline entry; advance its pause | 38.20 HP, 74 food; party and gold unchanged |
| 3, unfinished | Gremlins demand nine males, ten females, ten gold | Stopped at F/G/R before choosing; no outcome claimed |

The short pass ended when the temporary ordinary-play driver did not recognize the band prompt. All encountered screens and inputs are retained. The tree was not assigned a dispatch slot from text alone. No camp appeared, and this pass adds no camp validation or new unresolved event branch.

## Verification and limits

`make` and all existing combat, tree, storm/illness, Pan, town, and mountain sanitizer comparison suites pass. The existing unused final-battle-stub warning remains. Python syntax and whitespace checks pass.

The binary establishes the other money/HP ranges and ring-independent behavior; the retained probes use the states above. Extreme signed-integer overflow, fractional gold supplied through emulator edits, and the original startup seed remain outside this target. Advice text may describe branches still unimplemented elsewhere; its presence does not establish those branches' mechanics. The short ordinary pass is not an integration playthrough acquiring money and later reaching a camp.
