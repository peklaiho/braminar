# Final assault victory endings and certificate

2026-11-18 follow-up: the remaining victory branches in `FINASSLT.CHN` are implemented in `final.c`. This closes the bounded final-assault reconstruction through game exit, within the existing numeric limits. Ordinary adventure-to-victory integration remains G9; these are controlled experiments.

## Evidence and controlled entry

The [ending probe](../tools/ending_probe.py) uses the established real chain loader, a controlled 8/2 army victory and Talisman duel. It stops at file `0x2975` before the victorious duel calls the ending. Add `0x2D7C` to assault file offsets for runtime IP. Player flags, saved HP, inventory, gender and RNG are restored at the ending boundary, independently of duel-local HP. Runtime delay calibration is shortened as documented in [duel research](duel-research.md).

Two probe modes retain compact evidence:

- [Ending checkpoints](ending-endings-checkpoints.json): three corruption cases (ring powers 1, 100 and 255, including both appearance states), attractive and unattractive certificate winners, and the original's no-printer branch. The unattractive certificate deliberately supplies saved HP zero, empty supplies, level 21 and opposite equipment/protection/transport flags after the already-won duel. This is a controlled output boundary, not an ordinarily reachable zero-HP victory.
- [Reward checkpoints](ending-rewards-checkpoints.json): all ten rolls for each gender, exact reward text, before/after player memory and RNG. Selection uses controlled seeds, not a claim about observed frequencies.

The probe reuses allocated ending/reward stack frames with debugger `SR EIP` only after balanced calls return: ending body `0x1D94` through `0x2052`, reward body `0x0E22` through `0x1357`. It does not rerun their stack allocation/prologues. Every case's state is captured; no saved files or reference binaries are patched.

Original printer output is captured by temporarily changing the Pascal printer descriptor's flags at `DS:0150` from `0x43` (output, printer device 3) to `0x41` (output, console device 1). The original certificate/reward formatting and code still execute. The descriptor's first word is not a DOS output handle: an initial attempted handle replacement did not capture the certificate and timed out; the corrected flag edit succeeds. No physical printer is used.

Raw captures:

- [Initial exploration](captures/2026-11-18-ending-explore.txt), including the unsuccessful handle edit.
- [Corrected console exploration](captures/2026-11-18-ending-console.txt), certificate and final status with saved HP 50.
- [Initial complete ending batch](captures/2026-11-18-endings.txt), all six branches with populated inventory.
- [Boundary ending batch](captures/2026-11-18-endings-boundaries.txt), retained six-case fixture with zero saved HP/empty-inventory unattractive winner.
- [All rewards](captures/2026-11-18-ending-rewards.txt), twenty controlled selections.

## Recovered flow

The ending clears the duel screen. Any nonzero ring power immediately prints the Ring of Doom corruption paragraph and evil laugh, then returns to game exit. Appearance does not override it. There is no certificate, reward draw, final status or input pause on this branch. The ring and all saved player fields remain unchanged.

Without the ring, absent good looks adds the three-line unattractive ruler message. Both appearance states then reach the original printer question. Original `Y` prints the certificate and consumes exactly one reward draw; original `N` skips both. Both original answers then display the same final player status and wait for one key before exit. The remake always takes the certificate path, writes it to stdout and omits the printer question, as requested. It preserves the final status pause; EOF also exits cleanly. The status follows the certificate without clearing it from the terminal.

The certificate congratulates the new King regardless of gender, names the player twice, and prints level, saved HP, good looks, Talisman, servant illness protection, dragon charm credits, both servant counts, gold, food, medicine, poison, swords, daggers, bows, arrows, horse and cart. It does not print duel-local HP or surviving army counts. All saved assets remain present: conversion did not consume them. The original HP uses an 18-column scientific field with ten decimal places; gold and food use seven columns with no decimals. The remake retains these fields using its existing C double/integer policy. Printer line-feed/form-feed page controls are replaced by ordinary stdout line breaks. Servant terminology and the shared corrected `Medicine` status label follow project policy.

The reward consumes `R(10)` once, prints a line and changes no player state. Male gender (original `DS:0260 = 0`) selects princesses for rolls 1–2; female selects princes. Neither appearance nor Talisman changes selection. The full table and branch offsets are recorded in [mechanics](mechanics.md#final-assault-victory-endings).

## Comparison and remaining limits

`python3 tools/check_ending.py --sanitize` compiles the actual game sources and compares all twenty rewards and six ending cases. It checks exact reward text/RNG, unchanged player bytes, ring precedence and no-input/no-draw return, certificate fields and saved HP, appearance messages, final status and its one-key/EOF exit. Certificate/status whitespace is normalized and servant/medicine terminology follows the documented changes. The original no-printer case explicitly checks the agreed difference: the remake still prints a certificate and consumes its reward draw.

The affected conversion (13), army events/battles (25/6), duel rounds/full fights (12/6), and Keep (23 plus six EOF prompts) comparisons also pass with address and undefined-behavior sanitizers. The Keep harness excludes the newly reached final-status key from its Keep input ledger; the ending checker covers that pause directly.

No victory placeholder remains. Existing open limits are ordinary full-adventure integration, startup/full-run RNG parity, exact Real48 rounding and extreme signed-16-bit arithmetic behavior; the ending work does not resolve those broader boundaries.
