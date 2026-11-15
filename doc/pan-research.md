# Statue of Pan reconstruction

## Scope and evidence

This bounded target implements main-loop slot 15, the routine at `BRAMINAR.COM` file offsets `0x9DC8`–`0xA0CC`. [Recovered rules](mechanics.md#statue-of-pan) establish question selection, accepted answer keys, the random answer check, command generation, and exit flow. Question data comes from the first 20 declared records of `BRAM.DAT`; the original files remain unchanged.

The remake embeds these strings in C, avoiding a runtime dependency on DOS data files. At the user's request, record 3's third answer changes only `=` to a space: `Do Lake: 1 to 5`. No other question or answer text is changed. The [extractor](data-format.md) still returns the original strings.

## Controlled original-game comparisons

The [raw transcript](captures/2026-11-15-pan-probes.txt) records all screens and inputs, including startup. These are controlled experiments, not ordinary play: `pan_probe.py` resets temporary player memory, sets a chosen RNG seed before dispatch, and initializes the prime command to `PRIOR` before each case. No unrelated encounters occurred.

Each case starts at level 8, 50 HP, five servants of each sex, eight gold, 100 food, three medicine, two arrows, no ring/plague, and neither quest item. Other equipment and experience are zero. Previously drawn status values can remain on screen after the next memory reset; the recorded initial memory state describes the actual case.

| Case | Seed | Actions | Observed outcome |
| --- | --- | --- | --- |
| success | 1465 | Approach; invalid `x`, invalid `0`, then `2`; dismiss result | Record 1; guessed right; command becomes `SKJFJ` |
| failure | 1465 | Approach; `3`; dismiss result | Record 1; answer reported as 2; command stays `PRIOR` |
| decline | 1465 | Decline | Returns directly to status without a result pause; command stays `PRIOR` |
| malformed | 1328 | Approach; `2`; dismiss result | Record 3 displays the original malformed third answer; command becomes `WINIP` |
| last-question | 1457 | Approach; `3`; dismiss result | Record 20; command becomes `YYDGX` |

All cases end at 50.10 HP and 95 food after ordinary turn bookkeeping, with other resources unchanged. Compact [checkpoints](pan-checkpoints.json) retain the configurations, final memory state, and event text. Decline's `You move on.` scrolls away when status redraws; its exact text follows the binary at `0xA0AD`–`0xA0C6`. The saved approach screen plus the immediate status return establishes that no extra keypress is required.

## Reproduction and validation limits

Run `./pan_probe.py > doc/captures/2026-11-15-pan-probes.txt` to repeat the DOS captures and regenerate checkpoints. Run `./check_pan.py --sanitize` without DOSBox to compare final state, stored command, RNG, normalized text order, and input counts. The checker allows only the approved answer correction in its text comparison. It also exercises every question record against `BRAM.DAT` and verifies that character generation initializes the command to five spaces.

The five DOS cases cover question records 1, 3, and 20, not all question/answer combinations or generated commands. Full ranges and random-call order follow from binary inspection. Invalid keys are observed to leave the question pending; final RNG equality verifies that they consume no draws. Screen layout and real-time terminal behavior are not compared byte for byte. No ordinary integration pass was run for this bounded target; Keep command handling and its transition are still unfinished and are next in the checklist.
