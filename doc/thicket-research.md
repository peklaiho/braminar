# Thicket and Anthrax research

## Evidence and scope

The complete slot-5 thicket/laboratory (`dos/BRAMINAR.COM`, file offsets `0xDEE7`–`0xEB4E`) and separate Anthrax routine (`0x3E6C`–`0x4B66`) are recovered and implemented. Exploration, all eleven potion effects, the eight reachable hair colors, and all five pentacle outcomes are included. The pentacle's creature reuses the independently recovered gate-creature routine, not band combat. [Mechanics](mechanics.md#thicket-and-laboratory) gives the exact draws, thresholds, effects, and pause order.

The old ordinary laboratory victory remains in [session 04](session-04.md#anthrax-observations). It supplied discovery evidence for level drain, plague, servant priority, and retaliation after the killing blow. This session separately inspects control flow and runs controlled probes; ordinary observations alone are not used to infer probabilities.

## Controlled captures and reproduction

The initial 61 named cases in [checkpoints](thicket-checkpoints.json) combine these captures; G6 adds two illness-state chains and G7 adds repeated dragon meetings, bringing the current total to 64:

- [Primary probes](captures/2026-11-15-thicket-probes.txt): 60 cases covering exploration, potion effects, pentacle outcomes, Anthrax, and death.
- [Fresh good-looks probes](captures/2026-11-15-thicket-ugly-probes.txt): repeat `wizard-ugly-saved` and `wizard-ugly-failed` with good looks initially true, superseding their primary fixtures so both preservation and loss can be observed directly. The initial primary runs are retained in the raw transcript.
- [Chained charm path](captures/2026-11-15-thicket-charm-path.txt): one additional two-event case acquires charm in the laboratory and uses it at the next hut. No memory or RNG changes occur between the two events.

All these are explicitly labelled controlled experiments. `tools/thicket_probe.py` resets temporary player memory, servant prices 8/13, and RNG before each independent case. Original DOS files remain untouched. The baseline is level 8, 50 HP, Black hair, bad looks, five servants of each sex, 20 gold, 100 food, three medicine, two swords, three bows, four arrows, one dagger, horse/cart, and plague 10. Ring, Staff, Talisman, charm, experience, poison, and illness protection are zero. Configurations record the exceptions, including fresh looks, higher HP/level, preexisting charm, held quest items, and zero male or total servants.

Every input boundary and waiting/combat screen is captured. The initial displayed status precedes memory changes; `STATE initial` records the actual probe state. The initial encounters are thickets or their nested fights, plus the explicitly recorded hut in the charm chain. G6 additionally records forest and band follow-ups; G7 records two consecutive huts starting with retained charm. No unrelated encounters are skipped. Current `./tools/thicket_probe.py` reproduces all 64 cases in one run; named arguments rerun subsets, and `--output PATH` selects a separate checkpoint file.

## Compared behavior

| Area | Retained cases and observations |
| --- | --- |
| Entry and appearance | Decline without a pause; wizard absent at exactly 51 and present at exactly 50. |
| Exploration | Roast food arithmetic, male/female/gold draw order, Staff success at 4 and failure at 5, and a qualifying Staff roll while already held. Loot follows both a skipped wizard and surviving fights. |
| Potion offer | Present at exactly 25, absent at 26, and refusal after the count is drawn. |
| Potion effects | All eleven effects as single-bottle batches; all eight reachable hair colors; five bottles drunk in sequence; charm increment from zero to one and from two to three; plague cleared, bad looks cured, and protection acquired. Ten-HP addition starts at 85 HP to verify the lack of a cap. |
| Pentacle offer | Present at exactly 10, absent at 11, and refusal. |
| Pentacle results | All five outcomes; ring power 100 replaced by five (then ten after bookkeeping); level 21 preserved by the Staff/level-twenty outcome; summoned creature with poison; fatal freezing wind. |
| Wizard bypasses | Any held ring defeats Anthrax while preserving poison, after his three initial level/HP draws. Poison kills without a save and follows normal outcome/experience processing. |
| Wizard retaliation | All nine retaliation indices captured after a first-round killing blow. Ugly, drain, and plague have both failed and successful saves. Servant frying tests male-first priority, female fallback, and no servants. |
| Normal wizard rounds | A complete fight and the same fight with poison offered and declined compare player/wizard HP text, extra damage draws, state-changing spells, RNG, and pauses. |
| Death | A killing player attack followed by fatal lightning makes both participants dead. The original still awards wizard experience before the parent skips loot and reaches the outer pause. Wind death retains loot acquired before the pentacle. |

`./tools/check_thicket.py --sanitize` compares player state at every encounter input, quest items/charm, normalized text and prompt order, exact input counts, and requested delays. Since G6, RNG remains recorded metadata and exact equality is not asserted. Text comparison removes whitespace to accommodate DOS 80-column wrapping inside words, normalizes slave terminology to servants, and accepts the approved band typo corrections. Delay expectations count the entry's half second, each accepted potion's one second, spell-specific waits, the creature's half second, and G6's band-payment delay. These are requested delays, not measurements of emulated elapsed time.

Fatal cases compare the final event state at the outer pause, before turn bookkeeping, because DOS reclaims DS on exit. The remake checker suppresses bookkeeping for those cases; it does not claim a post-death memory comparison. Normal cases include bookkeeping, promotions, ring/plague progression, healing, and food use.

## Charm integration

Seed 1178019 produces a five-pound roast, no wizard, one charm potion, and no pentacle. After the laboratory, the next event is the noisy hut with the hermit and dragon. The player accepts the challenge; the dragon submits and the hermit doubles possessions.

Only the initial case setup edits memory/RNG. A retained `turn` checkpoint records the first event's state after normal bookkeeping. From there the script advances through normal choices, and the C comparison runs the same second dispatch with the existing player and RNG state. The counter stays at one, poison is unused, and no dragon combat rounds occur. Final state includes 50.20 HP, 36 male/48 female servants, 510 gold, 216 food, two experience, and charm one. This establishes the potion-to-dragon interaction under controlled initial conditions, not an unmodified ordinary acquisition run.

G7 reuses that acquisition evidence and adds `charm-two-dragons`, seed 10217: start with charm one and plague zero, enter two consecutive noisy huts, and accept both hermit challenges. Both dragons submit and charm stays one through both turns. Final state includes 50.20 HP, twenty servants of each sex, eighty gold, 360 food, twelve medicine, and three experience. [Raw capture](captures/2026-11-16-repeat-dragon.txt) retains all inputs and states without inter-event edits; [G7 notes](quest-state-research.md) record scope and results. No gameplay correction was needed.

## Illness-state integration (G6)

Two [G6 chains](illness-state-research.md), captured on 2026-11-16, use the same runner and two-turn C harness without editing state or RNG between encounters. `plague-to-faerie` starts plague at zero: Anthrax's killing-blow retaliation infects the player, plague grows from five to ten at turn end, then the next forest's faerie cures it. `protection-to-band` acquires protection from a single potion, then the next slot-9 dispatch becomes a band encounter. Paying through normal choices preserves protection and leaves medicine untouched.

The [raw transcript](captures/2026-11-16-illness-chains.txt) retains every screen, input, and state across these four encounters; the two compact cases are in the existing fixture file. Both use the established baseline with plague zero, rather than the earlier baseline's ten. These are controlled experiments, not ordinary play. No gameplay correction was needed. Reproduce just these cases with `python3 tools/thicket_probe.py plague-to-faerie protection-to-band`; G6's 63 then-retained comparisons passed `python3 tools/check_thicket.py --sanitize`.

## Ordinary discovery pass

A separate [five-turn ordinary pass](captures/2026-11-15-thicket-ordinary.txt) uses no memory/RNG edits. ThicketWalk starts female, level 7, 52 HP, Red hair, good looks, one male and twelve female servants, two gold, 100 food, three medicine, and no equipment/transport/quest items. Servant prices are 10/13.

| Turn | Encounter and normal choice | Visible state after bookkeeping |
| --- | --- | --- |
| 1 | Dark Castle: decline entry | 52.10 HP, 93 food |
| 2 | Rust Golems demand ten males, seven females, six gold: run; escape succeeds | 52.20 HP, 86 food |
| 3 | Keep: lacks Staff and level twenty; advance rejection pause | 52.30 HP, 79 food |
| 4 | Village: decline entry | 52.40 HP, 72 food |
| 5 | Enchanted forest: decline entry | 52.50 HP, 65 food |

Party, gold, medicine, and equipment remain unchanged. The pass stops at the next status screen. Every encountered screen and input is retained; there was no thicket and no new unresolved branch. These observations add ordinary interaction coverage, not thicket validation.

## Verification and remaining limits

All 64 current thicket cases pass with address and undefined-behavior sanitizers, including G6 illness-state chains and G7 repeated dragons. G7 also reran all 23 Keep cases, including Pan-to-Keep and Staff recovery. `make`, Python syntax, and whitespace checks pass. Other suites were not rerun for G7 because game code and the reused C harnesses did not change; earlier sessions' verification remains historical evidence.

Binary evidence establishes the full attack tables, precise saving thresholds, and unconditional post-fight experience award. Captures do not exhaust every player attack/damage value, exact equality at every spell-save boundary, every multi-potion ordering, illness-protection/plague combinations, or extreme integer overflow. The unreachable six hair-table entries are recorded as data, not selectable outcomes. Fractional arithmetic follows the documented double-precision policy. The prime command and final assault remain separate work; receiving the Staff/level twenty here does not implement those deferred branches.
