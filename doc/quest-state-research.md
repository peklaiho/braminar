# Charm and quest persistence — G7

Completed 2026-11-16. Existing potion-to-dragon and Pan-to-Keep evidence is retained; two new controlled sequences close the remaining G7 work in [remaining gaps](remaining-gaps.md#bounded-completion-checklist). Charm works on consecutive dragon meetings, and recovering a lost Staff restores Keep eligibility. Both sequences match the existing remake without gameplay changes.

## Evidence reused

| Existing comparison | Established scope |
| --- | --- |
| Thicket `potion-charm-to-hut` | A laboratory potion increments charm from zero to one. The next hut's dragon submits; charm remains one. [Charm integration](thicket-research.md#charm-integration). |
| Keep `pan-to-keep` | Pan supplies `GJVDY`; the next Keep accepts that command with the already held Staff and level twenty, after normal bookkeeping. [Pan-to-Keep interaction](keep-research.md#pan-to-keep-interaction). |
| Pan success/failure/decline cases | Success replaces an existing command; failure and decline preserve it. [Pan research](pan-research.md). |

These captures remain completed evidence and were not regenerated. Both existing chained cases were rerun in the affected sanitizer suites. No new command variations or naturally earned level-twenty run are required.

## New controlled sequences

The [repeat-dragon capture](captures/2026-11-16-repeat-dragon.txt) and [Staff-recovery capture](captures/2026-11-16-staff-recovery.txt) retain every encountered screen, input, and initial/input/intermediate/final state. Compact cases are `charm-two-dragons` in [thicket fixtures](thicket-checkpoints.json) and `staff-loss-reacquisition` in [Keep fixtures](keep-checkpoints.json).

Both use `dosbox_game.Game` and edit temporary player state, servant prices 8/13, and initial RNG once per sequence. No state or RNG edits occur between encounters. Original DOS files remain untouched. These are controlled experiments, not ordinary play. Initial displayed status can predate the edits; `STATE initial` records the actual setup. All five encounters are retained: two hermit/dragon huts, then a separate Keep → river → Keep sequence. There are no unrelated encounters to skip.

### Repeated dragon submission

Seed 10217 starts with the established thicket baseline except plague zero and charm one: level eight, 50 HP, five servants of each sex, twenty gold, 100 food, three medicine, two swords, three bows, four arrows, one dagger, horse/cart, bad looks, and no ring, poison, or quest items. The initial charm value represents the already demonstrated acquisition; a second potion capture is unnecessary.

Enter each consecutive noisy hut and accept each hermit's challenge. The dragon submits both times, without combat rounds. Charm stays one at both result pauses, between encounters, and at the end. The first submission awards two experience and the second one; both hermits double possessions. After the first turn: 50.10 HP, ten servants of each sex, forty gold, 190 food, six medicine, two experience. After the second: 50.20 HP, twenty servants of each sex, eighty gold, 360 food, twelve medicine, three experience. Equipment is doubled twice; charm is preserved rather than doubled or consumed.

The [dragon rules](mechanics.md#dragon-combat) explain this behavior. Reinspection of `BRAMINAR.COM` at `0x56D0`–`0x5731` confirms that a positive charm word selects submission and adds experience before exiting, without decrementing the word. Together with the existing potion-acquisition chain, the new case establishes repeated use across encounters.

### Staff loss, recovery, and eligibility

Seed 117042 uses the [Keep baseline](keep-research.md#baseline-and-comparison-scope): level twenty, 50 HP, Staff held, command `PRIOR`, five servants of each sex, twenty gold, 100 food, three medicine, the listed equipment/transport, and zero ring/plague/charm/experience.

| Encounter and normal choice | Original result, matched by remake |
| --- | --- |
| Keep: accept attack, submit `WRONG`, dismiss the result | Staff disappears; stored command remains `PRIOR`. After bookkeeping: Staff absent, 50.10 HP, ninety-five food, assault flag false. |
| River: gaze, dismiss the result | Vision is `The Overlord wearing your outfit!!`; the Staff is found. No Talisman or champion follows. After bookkeeping: Staff held, 50.20 HP, ninety food; command unchanged. |
| Keep: decline the newly offered attack, dismiss the pause | The attack prompt confirms restored eligibility with the recovered Staff. Final Staff held, level twenty, command `PRIOR`, 50.30 HP, eighty-five food, assault flag false. |

The [Keep gate/command rules](mechanics.md#overlords-keep-prerequisite) and [river item rules](mechanics.md#flowing-river) establish the component behavior. The sequence proves that Staff loss is recoverable and that the stored command survives. Declining the last attack keeps the original final assault unentered. The runner also stops at its existing post-bookkeeping breakpoint; no final-assault code executes.

## Reproduction and completion limits

```sh
python3 thicket_probe.py charm-two-dragons > doc/captures/2026-11-16-repeat-dragon.txt
python3 keep_probe.py staff-loss-reacquisition > doc/captures/2026-11-16-staff-recovery.txt
python3 check_thicket.py --sanitize
python3 check_keep.py --sanitize
```

Named cases update the existing fixture files by name; both runners support `--output PATH`. The thicket runner already handles two-event chains; its harness is reused for the two huts. The Keep runner now retains intermediate states through the river and places its bookkeeping breakpoint only at the last encounter's pause. Its existing C harness already handles three turns and line input, so no C changes were needed.

All 64 thicket and 23 Keep scenarios pass with address and undefined-behavior sanitizers, including the earlier acquisition/command chains. Comparisons cover per-input/intermediate/final state, charm, Staff, stored command where applicable, normalized narrative, prompt order, pauses, and requested delays. Both new sequences request no event delays. Exact RNG equality is not checked. Other suites were not rerun because game code and the C harnesses did not change.

G7 requires no further Staff source/loss combinations, repeat potion captures, command variations, or ordinary progression grind. G8 combat death/reward ordering is next; ordinary integration and the already identified message-readability issue remain G9.
