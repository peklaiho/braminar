# Storm and servant-illness reconstruction, 2026-11-14

This session used targeted reconstruction rather than another ordinary playthrough. Prior ordinary observations are in [session 03](session-03.md) (illness) and [session 04](session-04.md) (tornado and starvation). Binary evidence and rules are in [mechanics.md](mechanics.md#storms) and [the illness section](mechanics.md#servant-illness).

## Controlled original-game evidence

[Main raw transcript](captures/2026-11-14-storm-illness-probes.txt), [initial nonnumeric-input probe](captures/2026-11-14-illness-invalid.txt), and [separate overflow probe](captures/2026-11-14-illness-overflow.txt), and [abort/restart verification](captures/2026-11-14-numeric-aborts.txt). Every command and resulting screen is retained. All runs explicitly label temporary memory/RNG modifications; none is evidence of encounter frequency during ordinary play. Original DOS files remain untouched.

The main run completed 27 cases, then its attempted restart after the invalid-input abort reached character creation incorrectly. That run's final `illness-overflow` section is a failed setup, not evidence about illness. The separate overflow capture starts a fresh game and establishes the actual overflow result. The probe tool now waits for the welcome screen before replaying creation after an abort and saves each completed case immediately. A subsequent invalid/overflow run verified that restart sequence and both aborts.

`./event_probe.py` reproduces the cases and saves [compact configurations, states, and event screens](storm-illness-checkpoints.json). Named cases can be rerun individually and update only their entries. Memory is reset at a status prompt before each case; the initially displayed status can therefore be stale. `STATE initial` and `STATE final` are direct memory reads. After an abort DOS regains DS, so no final memory checkpoint is claimed.

Each case starts female, level 8, 50 HP, 100 gold, zero experience/ring/plague, no equipment, horse/cart, Staff or Talisman. Counts, medicine, food, and illness protection are specified per case. Treatment cases use illness seed 11489: three sick males and four sick females before caps, with Small Pox. The protected case sets illness protection and instead encounters a band, choosing G to pay its demands. The disease name never changes the player's plague byte.

The table's initial column is male/female counts; medicine; food. Final numbers are male / female / medicine / food, after normal turn bookkeeping. Surviving cases end at 50.10 HP; unchanged fields and final RNG states are in the fixtures. Inputs in quotes are numeric lines except storm Y/N and protected-band G.

| Case | Seed | Initial counts; medicine; food | Inputs | Final counts / medicine / food or outcome |
| --- | --- | --- | --- | --- |
| storm-rain | 12523 | 12/12; 3; 100 | 'n' | 12 / 12 / 3 / 88 |
| storm-hail | 12531 | 12/12; 3; 100 | 'n' | 10 / 7 / 3 / 91 |
| storm-snow | 12539 | 12/12; 3; 100 | 'n' | 9 / 8 / 3 / 91 |
| storm-wind | 12547 | 12/12; 3; 100 | 'n' | 8 / 11 / 3 / 90 |
| storm-tornado | 12555 | 12/12; 3; 100 | 'n' | 7 / 11 / 3 / 91 |
| storm-hurricane | 12505 | 12/12; 3; 100 | 'n' | 11 / 10 / 3 / 89 |
| storm-monsoon | 12507 | 12/12; 3; 100 | 'n' | 8 / 10 / 3 / 91 |
| storm-tidal | 12515 | 12/12; 3; 100 | 'n' | 0 / 0 / 3 / 0 |
| storm-shelter-1 | 12523 | 12/12; 3; 100 | 'y' | 12 / 12 / 3 / 88 |
| storm-shelter-8 | 12516 | 12/12; 3; 100 | 'y' | 12 / 12 / 3 / 88 |
| storm-small-party | 12555 | 1/2; 3; 100 | 'n' | 0 / 1 / 3 / 99 |
| storm-no-males | 12555 | 0/12; 3; 100 | 'n' | 0 / 11 / 3 / 94 |
| storm-empty-party | 12555 | 0/0; 3; 100 | 'n' | 0 / 0 / 3 / 100 |
| storm-starvation | 12555 | 12/12; 3; 0 | 'n' | 6 / 10 / 3 / 0 |
| illness-partial | 11489 | 12/12; 3; 100 | '2', '1' | 11 / 9 / 0 / 90 |
| illness-no-medicine | 11489 | 12/12; 0; 100 | '10', '10' | 9 / 8 / 0 / 91 |
| illness-full-cure | 11489 | 1/2; 3; 100 | '1', '2' | 1 / 2 / 0 / 98 |
| illness-excess | 11489 | 1/2; 20; 100 | '10', '10' | 10 / 10 / 0 / 90 |
| illness-no-males | 11489 | 0/12; 3; 100 | '0', '3' | 0 / 11 / 0 / 94 |
| illness-no-females | 11489 | 12/0; 3; 100 | '3', '0' | 12 / 0 / 0 / 94 |
| illness-empty-party | 11489 | 0/0; 3; 100 | none | 0 / 0 / 3 / 100 |
| illness-negative | 11489 | 12/12; 3; 100 | '-1', '2', '-2', '1' | 11 / 9 / 0 / 90 |
| illness-protected | 11489 | 12/12; 3; 100 | 'g' | 8 / 10 / 3 / 91 |
| illness-hex | 11489 | 12/12; 3; 100 | '$0002', '1' | 11 / 9 / 0 / 90 |
| illness-blank | 11489 | 12/12; 3; 100 | '', '2', '1' | 12 / 8 / 0 / 90 (original stale-value result; see below) |
| illness-whitespace | 11489 | 12/12; 3; 100 | ' 2 extra', '1' | 11 / 9 / 0 / 90 |
| illness-invalid | 11489 | 12/12; 3; 100 | 'abc' | I/O error 10; program aborted |
| illness-overflow | 11489 | 12/12; 3; 100 | '32768' | I/O error 10; program aborted |

## Initial findings and implementation

All eight storms are implemented, including harmless rain without shelter, independent losses and zero clamps for types 2–7, and the tidal wave clearing servants and food. Shelter consumes one duration roll and prints one dot per requested second, scaled by storm type. Captured rain and tidal shelter runs waited one/eight iterations respectively. The initial period in `Waiting for storm to pass.` is not a timed dot. The final pause is present even after an otherwise harmless storm. Starvation applies afterward to the remaining party.

Illness now rolls both sick counts and the disease in the original order, caps sick counts to inventory, asks for male then female cures, spends remaining medicine in that order, and subtracts untreated servants. It retains the original excessive-cure behavior: one male/two females and twenty medicine become ten of each sex when requesting ten/ten. With no servants, it prints the bug message and requests a one-second delay without an input pause or further random calls. Illness protection substitutes a band encounter before any illness draws.

Negative numbers retry without repeating the treatment prompt. Decimal and `$`-prefixed hexadecimal input are supported; the whitespace probe confirms a leading space and discarded text after the number. Invalid `abc` and decimal 32768 abort before turn bookkeeping. The initial remake retained that abort and its diagnostic, omitting the DOS program-counter address. The shared integer reader now deliberately replaces these input rules; see below.

Blank input exposed an original uninitialized-local bug: in the captured run, a blank male answer advanced to females and consumed all medicine through a stale value. The following `2` answered the female prompt; the later `1` was typed at the exit pause. The remake deliberately retries blank lines, so its blank/2/1 case instead has the same result as the ordinary 2/1 partial treatment. This intentional difference is recorded in [CHANGES.md](../CHANGES.md). The fixture retains the observed original state and marks `compare_as: illness-partial`; the check explicitly validates the chosen retry behavior against that reference, rather than claiming parity with the original blank case.

Remaining original-game input research gaps are bare signs/prefixes and input-length limits; the shared reader now intentionally treats these as invalid input. Extreme inventory overflow remains unresolved across the remake. Other disease labels were extracted from their Pascal strings and binary dispatch, rather than separately captured here. No assumptions about other events' numeric readers follow from this work.

## Initial validation

`./check_events.py --sanitize` exercises the actual event dispatch and turn processing: 27 original-game comparisons plus the documented blank-input difference. It compares HP, inventory, experience, ring/plague/protection, and final RNG where original memory is valid; normalized text order; input request counts; and requested delays. Aborted cases compare their input/diagnostic flow and lack of normal completion, without comparing memory after DOS regained control. Text normalization permits terminology replacements, recorded typo corrections, whitespace, and omission of the DOS program-counter address.

The C harness scripts line/key reads and records sleep requests, so the checks run without DOSBox or real waits. Delay assertions compare requested sleep totals with binary rules and original dot counts, not wall-clock measurements from debugger captures. A separate temporary PTY check used real terminal input and actual `fgets`/key reads for treatment 2/1, returning 11 males, 9 females, zero medicine, 90 food, and 50.10 HP.

The twelve existing combat cases and eight tree cases also pass with address/undefined-behavior sanitizers. `make` succeeds with only the existing unused-final-battle-stub warning; `git diff --check` passes.

## Shared integer reader update

`read_treatment` has been replaced by `read_integer`, beside the other input functions. It accepts nonnegative decimal quantities through C `INT_MAX`, with surrounding whitespace; invalid or negative input silently becomes zero, while empty lines retry. Hexadecimal and mixed text no longer follow Pascal token parsing. EOF still ends input. The policy and overlong-line handling are recorded in [CHANGES.md](../CHANGES.md#integer-input).

Original captures and fixtures remain evidence of the classic game. The current event checks compare 22 scenarios with original behavior and explicitly check six intentional input differences: blank, negative, hexadecimal, mixed text, alphabetical input, and the old 16-bit overflow case. `32768` is now a valid integer; `abc` is zero and does not abort. The check supplies a female answer after inputs that originally aborted. Eighteen direct reader checks cover valid values, whitespace, blank retries, invalid syntax, integer overflow, overlong lines, and EOF. All pass with address/undefined-behavior sanitizers.
