# Running and capturing the original game

`tools/dosbox_game.py` launches Braminar in `dosbox-debug`, sends keyboard input, and
reads the emulated text screen directly. It uses Python 3's standard library;
there are no Python packages to install. On Debian, install the system tools with:

```sh
apt-get install python3 dosbox-debug xvfb xdotool
```

Each run gets its own virtual display and temporary copy of `dos/`. The original
files stay untouched. The tool terminates its DOSBox and Xvfb processes and deletes
its temporary files on exit, including EOF, Ctrl+C, and SIGTERM.

## Interactive use

Run from the project root:

```sh
./tools/dosbox_game.py
```

The tool waits for Braminar's welcome screen and prints it. Enter tool commands
at the `dosbox>` prompt; each command prints its command label and the resulting screen:

```text
key space
text Probe
key Return
key m
key space
```

This advances past the welcome screen, types a name, submits it, chooses male,
and advances from prices to the initial character status.

| Command | Effect |
| --- | --- |
| `screen` | Capture the current screen without sending input |
| `key space` | Send one space keypress |
| `key Return` | Press Enter |
| `key y` | Answer a single-character prompt |
| `text Probe` | Type literal text, without pressing Enter |
| `wait 1` | Let the game run for one second, then capture |
| `quit` | End the session |

`key` accepts xdotool key names and combinations, such as `Escape`, `BackSpace`,
or `shift+m`. Prefer sending one response at a time so the game reaches the next
prompt before more input arrives. `text` preserves spaces after the command's
first separating space. The command line itself is not passed to a shell.

## Repeatable command sequences

Repeat `--command` (or `-c`) to run a sequence and exit:

```sh
./tools/dosbox_game.py \
    -c 'key space' \
    -c 'text Probe' \
    -c 'key Return' \
    -c 'key m' \
    -c 'key space' > /tmp/braminar-session.txt
```

Output includes the initial screen and a labeled screen after each command.
Commands can also be piped into the interactive mode, one per line; EOF ends the
session. Diagnostics and interactive prompts go to standard error. Failures
produce a nonzero exit status.

The default delay after input is 0.2 seconds. Use `--delay 0.5` to allow more time,
or `wait SECONDS` for a particular transition. Waiting is limited to 60 seconds
per command. A repeated input sequence does not reproduce the random seed or
guarantee identical encounters. Captures are screen snapshots, so text that
scrolls away between captures is not retained.

## Capture mechanism

The tool enters the debugger with Alt+Pause and uses `MEMDUMPBIN` to read the BIOS
video information. It then dumps the active text page at segment `B800` (`B000`
for monochrome text mode), decodes the character bytes as CP437, and resumes with
F5. It preserves line spacing and removes trailing spaces and empty outer rows.
Colors are not included in the text output. Graphics modes are rejected.

This was verified against the original welcome screen, name and gender prompts,
prices, and character status. Captures retain original wording and typos; the
remake's text changes do not apply to this inspection tool. Pausing affects real
elapsed time, so this tool does not establish timing-sensitive mechanics.

## Controlled mechanics probes

`tools/combat_probe.py` imports the same `Game` runner to capture a band encounter with
chosen player state and RNG seed. It labels its output `CONTROLLED PROBE`, records
the configuration, captures inputs and screens, and prints `STATE` JSON records
with inventory, HP, experience, ring state, and RNG state at each checkpoint.

```sh
./tools/combat_probe.py --seed 6427 > /tmp/goblin-fight.txt
./tools/combat_probe.py --seed 6427 --bows 1 --arrows 2 > /tmp/arrow-exhaustion.txt
./tools/combat_probe.py --seed 6450 --choice g > /tmp/band-payment.txt
./tools/combat_probe.py --seed 6443 --swords 2 --daggers 3 --poison 1 --use-poison
```

Use `./tools/combat_probe.py --help` for player-state options. Seeds must select slot 4;
the script rejects other seeds. Starting seeds for the nine band types are:
Goblin 6427, Gremlin 6435, Rust Golem 6443, Orc 6450, Ogre 6458, Pirate 6466,
Troglodyte 6409, Drow Elf 6411, Demon 6419. Changing equipment can change all later
rolls because armed rounds consume an additional random number.

The probe changes temporary emulated memory, including the seed at `DS:01FC`;
it does not modify the original DOS files. Keep these experiments distinct from
ordinary playthrough observations. The initially displayed status predates the
memory edits; the `STATE initial` record describes the actual probe state. After
death the probe captures the exit screen but omits a memory checkpoint because
DOS has regained control of the data-segment register.

For further Python probes, `game.read_memory(segment, offset, size)` reads bytes;
`game.write_memory('DS', offset, data)` writes up to 256 bytes and verifies them by
readback. Writes also support `SS`. `game.debugger()` pauses/resumes through a
context manager; `game.dump()` and `game.debug_command()` operate while paused.
Do not nest debugger contexts. Capturing a screen reads the current display;
editing player memory does not itself redraw the status screen. The Real48
conversion functions in `tools/combat_probe.py` serve binary inspection only; the C
remake uses ordinary arithmetic as described in `CHANGES.md`.

Run `./tools/check_combat.py` to compare the remake with the saved controlled captures,
or `./tools/check_combat.py --sanitize` to enable address and undefined-behavior
sanitizers. This requires a C compiler (`CC`, default `cc`); it builds
`tools/check_combat.c` in a temporary directory and runs the checks without DOSBox.
The compact fixtures in `doc/combat-checkpoints.json` are sufficient to run the
checks without the raw transcripts, which the repository ignores. Individual
case names may be passed as arguments, for example `./tools/check_combat.py arrows poison`.
See the
[combat ledger](combat-research.md) for cases and evidence limits.

## Forest outcome probes

`python3 tools/forest_probe.py > doc/captures/2026-11-16-forest-probes.txt`
captures G1's six controlled forest outcomes and writes
`doc/forest-checkpoints.json`. Named case arguments select cases; `--output PATH`
keeps a separate fixture file. Player memory and RNG are edited only in the
temporary emulator. Fatal lure runs last; its final retained state is at the
event pause, followed by the captured death screen.

`python3 tools/check_forest.py --sanitize` compares state, normalized event text,
survival, and pause/input counts using the actual remake source. It requires no
DOSBox or raw captures and does not check exact RNG equality. See
[forest research](forest-research.md) for outcomes, binary evidence, and limits.

## Ring-persistence chains

`python3 tools/ring_probe.py > doc/captures/2026-11-16-ring-chains.txt` captures G5's
combat-acquisition → forest and guru-removal → forest chains, saving
`doc/ring-checkpoints.json`. Named case arguments select a chain; `--output PATH`
supports a separate fixture file. Only initial temporary player state/RNG is
edited; neither is changed between encounters.

`python3 tools/check_ring.py --sanitize` reuses the thicket two-turn C harness to
compare state, captured narrative, prompt order, and pauses without DOSBox or
exact RNG equality. See [ring research](ring-research.md) for results and limits.

## Occupied-gnome probes

`python3 tools/gnome_probe.py > doc/captures/2026-11-16-gnome-probes.txt`
captures G2's three occupied-home healing cases and writes
`doc/gnome-checkpoints.json`. Named case arguments and `--output PATH` work as
in the forest probe. Only temporary emulator state and RNG are changed.

`python3 tools/check_gnome.py --sanitize` reuses the forest C harness to compare
event/post-turn state, normalized text, survival, and pause/input counts. Expected
food includes the intentional gift fix; exact RNG equality is not checked.
See [gnome research](gnome-research.md) for evidence and completion limits.

## Empty-home probes

`./tools/tree_probe.py > doc/captures/2026-11-14-tree-probes.txt` captures eight
controlled slot-6 empty-home branches and writes `doc/tree-checkpoints.json`.
It uses deterministic seeds and resets temporary player memory between cases;
the header and each case explicitly label these modifications. Status screens
before the next turn can contain values from the previous case. The final
`STATE` records are memory reads after the next status screen.

Run `./tools/check_tree.py --sanitize` to compare the remake with those saved original
checkpoints without DOSBox. The checks include the four trunk results, the
held-Staff medicine substitute, food theft, leaving, and declining entry, along
with RNG progression and input counts. See [session 04](session-04.md) and the
[recovered rules](mechanics.md#empty-gnome-home).

## Storm and illness probes

`./tools/event_probe.py > doc/captures/2026-11-14-storm-illness-probes.txt` captures
controlled storms and illness, resetting temporary player memory and selecting
seeds before dispatch. Completed cases are saved immediately in
`doc/storm-illness-checkpoints.json`. To rerun selected cases and update their
entries, pass names, for example `./tools/event_probe.py storm-tidal illness-excess`.
The invalid/overflow cases deliberately abort the original; the tool restarts
it if further cases remain. Every encountered screen and input is captured.

`./tools/check_events.py --sanitize` runs comparisons without DOSBox. It checks state,
RNG, normalized text order, input counts, and requested delays. Input cases explicitly check the documented shared-reader differences, and the
suite also checks integer parsing directly. See the
[research ledger](storm-illness-research.md) for evidence and validation limits.

## Pan probes

`./tools/pan_probe.py > doc/captures/2026-11-15-pan-probes.txt` captures five controlled
Pan cases and saves `doc/pan-checkpoints.json`. It resets temporary player state,
RNG, and an existing prime command before each case. Captures include success,
failure, decline, invalid answer keys, and question records 1, 3, and 20.

`./tools/check_pan.py --sanitize` compares state, command, RNG, normalized text, and
input counts without DOSBox, and checks all 20 embedded question records against
`BRAM.DAT` with the approved correction. See the [ledger](pan-research.md) for
scope and limits.

## City and Village probes

`./tools/town_probe.py > doc/captures/2026-11-15-town-probes.txt` captures 42 controlled
City/Village cases and saves `doc/town-checkpoints.json`. It resets player memory,
servant prices (8/13), and RNG before each case and records memory at every input
boundary. Pass case names to update selected cases. `--output PATH` allows
independent capture batches. The runner restarts after fatal poisoning or the
original zero-gold pickpocket abort when more cases remain.

`./tools/check_towns.py --sanitize` compares state, prompt order, input counts, and
requested delays without DOSBox. `--fixtures PATH` selects another checkpoint
file. Fatal poisoning compares event state before bookkeeping because DOS has
reclaimed DS after death; the abort case checks unsuccessful termination.
Exact RNG equality is not checked. The two G3 interaction cases additionally
compare normalized captured narrative. Reproduce just those visits with:

```sh
python3 tools/town_probe.py city-services-then-marketplace city-cleric-then-muggers > doc/captures/2026-11-16-city-interactions.txt
```

See [town research](town-research.md) for captures, reproduction, and limits.

## Mountain trail probes

`./tools/mountain_probe.py > doc/captures/2026-11-15-mountain-probes.txt` captures five
controlled trail/guru cases and writes `doc/mountain-checkpoints.json`. It resets
temporary player memory and RNG before each case.

`./tools/check_mountain.py --sanitize` compares state, RNG, normalized text, and input
counts without DOSBox, and checks EOF at both questions. See the
[research notes](mountain-research.md) for scope and limits.

## Gypsy camp probes

`./tools/gypsy_probe.py > doc/captures/2026-11-15-gypsy-probes.txt` captures 51 controlled
camp cases and saves `doc/gypsy-checkpoints.json`. These include all 35 advice
results, gambling outcomes, and resource boundaries. Pass case names to update
selected entries, or use `--output PATH` for a separate fixture file. The probe
waits for automatic gambling rounds to finish before advancing their pause.

`./tools/check_gypsy.py --sanitize` compares state and RNG at each input, normalized
event text, key counts, and requested delays without DOSBox. Fatal injury
compares event state before bookkeeping because DOS reclaims DS after death.
See [research notes](gypsy-research.md) for scope and limits.

## Grass hut probes

`./tools/hut_probe.py > doc/captures/2026-11-15-hut-probes.txt` captures 41 controlled
hut cases and saves `doc/hut-checkpoints.json`. The runner resets temporary
player memory and RNG, records every choice and combat-round pause, and restarts
after death if more cases remain. Pass case names to update selected entries;
`--output PATH` selects a separate checkpoint file.

`./tools/check_hut.py --sanitize` compares state, quest items, charm, RNG, normalized
text, key counts, and requested delays without DOSBox. Fatal cases compare event
state before turn bookkeeping, because DOS reclaims DS on exit.
See [research notes](hut-research.md) for scope and remaining limits.

## Thicket and Anthrax probes

`./tools/thicket_probe.py > doc/captures/2026-11-15-thicket-probes.txt` captures 64
controlled cases and saves `doc/thicket-checkpoints.json`. Pass case names to
update selected entries; `--output PATH` selects another checkpoint file.
The probe waits through potion animation, captures combat pauses, and restarts
after death when needed. `potion-charm-to-hut` preserves state and RNG between
the thicket and the next hut, recording a `turn` checkpoint between them.

`./tools/check_thicket.py --sanitize` compares per-input and inter-turn state,
quest items/charm, text, key counts, and requested delays without DOSBox. Text
comparison ignores whitespace, including DOS line wrapping inside words, and
normalizes servant terminology and approved band typo corrections. Exact RNG
equality is not checked. Fatal cases compare event state before
bookkeeping because DOS has reclaimed DS after exit. See the
[research notes](thicket-research.md) for captures and validation limits.

G6's plague/protection chains preserve state and RNG from the initial laboratory
through the next forest or slot-9 band. Reproduce only these two cases with:

```sh
python3 tools/thicket_probe.py plague-to-faerie protection-to-band > doc/captures/2026-11-16-illness-chains.txt
```

See [illness-state research](illness-state-research.md) for outcomes and the
existing cure evidence reused to keep this task bounded.

G7's `charm-two-dragons` case starts with charm one and records two consecutive
hut challenges without inter-event edits. Reproduce just this repeated-use case
with `python3 tools/thicket_probe.py charm-two-dragons`; the existing potion-acquisition
case is retained. See [charm/quest research](quest-state-research.md).

## Dark Castle and Mad King probes

`./tools/castle_probe.py > doc/captures/2026-11-15-castle-probes.txt` captures controlled
castle visits and writes `doc/castle-checkpoints.json`. Pass case names to select
visits (in the requested order); `--output PATH` selects an independent fixture
file. Every case resets temporary player state and RNG; drop probes wait through
the sound before sampling the next input boundary. Fatal cases restart the
original when more cases remain. Punctuation in the theft sequence is sent as
literal text because xdotool does not accept `/` or `=` as key names.

`./tools/check_castle.py --sanitize` compares per-input and final state, RNG, normalized
text/prompt order, key counts, and requested delays without DOSBox. Fatal cases
compare event state at the final pause, before bookkeeping. See
[castle research](castle-research.md) for evidence, limits, and the deliberate
resolution of the original's uninitialized fear local.

## Watched tree and hut probes

`./tools/watched_home_probe.py > doc/captures/2026-11-15-watched-home-probes.txt`
captures 32 controlled slot-13 cases and writes
`doc/watched-home-checkpoints.json`. Named arguments select cases in the
requested order; `--output PATH` selects an independent fixture file. The
runner resets temporary player state and RNG and waits through the care delay
before recording the next input boundary.

`./tools/check_watched_home.py --sanitize` compares state, RNG, normalized text and
prompt order, key counts, requested delays, and EOF handling without DOSBox.
See [research notes](watched-home-research.md) for how the watched tree and hut
differ from the independently implemented slot-6 and slot-7 events.

## Overlord's Keep probes

`./tools/keep_probe.py > doc/captures/2026-11-15-keep-probes.txt` captures twenty-three
controlled cases and writes `doc/keep-checkpoints.json`. Each case starts a
fresh DOSBox runner, resets temporary player state, command, assault flag, prices
and RNG, then stops at an absolute breakpoint after turn bookkeeping and before
the final-assault handoff. The original assault module never executes. Named
arguments select cases; `--output PATH` selects an independent fixture file.
The Pan-to-Keep case retains state/RNG between its two normally chosen encounters.

`./tools/check_keep.py --sanitize` compares per-input and final state, command, assault
flag, normalized text/prompt order, logical key/line counts, requested delays,
and EOF handling. It invokes the actual `run_turn()` and final-assault entry,
including checking that fatal plague suppresses the assault. See
[research notes](keep-research.md) for breakpoint details, captures, and limits.
Exact RNG equality is not checked. G4's two small-party starvation cases reuse
this runner and harness; no assault is initiated. Reproduce just those with:

```sh
python3 tools/keep_probe.py turn-two-servants-starvation turn-last-servant-starvation > doc/captures/2026-11-16-turn-starvation.txt
```

[G4 notes](startup-turn-research.md) link the existing startup/turn evidence and
distinguish state comparisons from the remaining message-readability review.

G7's `staff-loss-reacquisition` case records Keep → river → Keep with no edits
between encounters. The final attack is declined to retain the Staff and avoid
the assault. The existing breakpoint stops after the last bookkeeping pass.
Reproduce just this case with `python3 tools/keep_probe.py staff-loss-reacquisition`;
see [charm/quest research](quest-state-research.md) for captures and limits.

## G8 simultaneous-death comparisons

[Combat ordering research](combat-order-research.md) records four controlled setups, raw captures, and reproduction commands. The retained checkers now cover 13 shared-combat, 43 hut, and 47 castle scenarios. These checkers compare gameplay state and outcome text without requiring exact RNG equality. Hut and castle named probes regenerate their new fixtures; the shared fixture is extracted from its raw configuration/checkpoint records, as with the earlier shared cases.

## Repeatable final-assault entry

`python3 tools/final_probe.py > doc/captures/final-probes.txt` launches fresh original-game instances, automates character creation and a synthetic successful Keep, then restores controlled player state and RNG at the loaded assault module's entry. No adventure play or disk patch is needed. It stops after asset conversion, before army combat, and saves `doc/final-checkpoints.json`. Named cases and `--output PATH` select a subset or separate fixture file.

To start at the final assault for repeated play:

```sh
python3 tools/final_probe.py baseline --interactive
```

At `dosbox>` use the same `screen`, `key`, `text`, `wait`, and `quit` commands as the ordinary runner. Army combat progresses automatically; `wait 1` captures a later screen. Redirect output to retain the complete transcript. For a scripted continuation:

```sh
python3 tools/final_probe.py baseline -c 'wait 8' -c 'wait 8' --output /tmp/final-flow.json > doc/captures/final-flow.txt
```

For custom experiments, import `start_final_assault(game, config)` from `final_probe` with a fresh `Game` and a modified named-case configuration. It returns the program code segment with the debugger paused at the initialized module entry, with requested state restored. Use `dump()` and `write_memory(..., paused=True)` there; do not nest debugger contexts. `resume(game)` resumes execution. The runner's conversion breakpoint is code segment:`59C5`; module file offsets map to runtime IP by adding `0x2D7C`. The [research ledger](final-assault-research.md) records byte checks, DS offsets, RNG restoration and validation limits.

`python3 tools/check_final.py --sanitize` compares saved original conversion state, exact RNG progression, unchanged player inventory, normalized text and requested delays using the actual remake sources, without requiring DOSBox. `python3 tools/check_keep.py --sanitize` validates main-turn handoff ordering through the opening and army combat, ending on capture or at the duel placeholder.


## Army battle and event probes

Run `python3 tools/army_probe.py events > doc/captures/army-events.txt` for all ten events under both labels plus small limits and overkill. It saves `doc/army-events-checkpoints.json`. Cases reuse an initialized army frame, resetting only temporary army/RNG state at the event-check boundary; inter-case setup batches are captured too.

Run `python3 tools/army_probe.py battles > doc/captures/army-battles.txt` for full retained battles and zero-denominator aborts. It saves `doc/army-battles-checkpoints.json`. Named cases select a subset, for example `python3 tools/army_probe.py battles equal-one small-win`. Both modes accept `--output PATH`. Full battle runs edit state only once after conversion, retain every casualty batch and applied/cleared event, and stop before the duel on victory. Losing battles capture the original caller's message and DOS exit.

All breakpoint offsets passed to `stop_at()` are assault file offsets. Useful stops: `0x08C4` after five casualties/before bars, `0x08DD` before the event check, `0x06DA` after applying an event/before its three-second display delay, `0x08FD` after message clearing/before pacing and termination, and `0x09D4` after final refresh/clear and before returning to the caller. Use paused memory methods at these stops.

`python3 tools/check_army.py --sanitize` compares all event/battle checkpoints without DOSBox, including exact RNG, casualty order, signed effects, original terminal frames, event clearing, delays, unchanged player state and zero-denominator termination. See [army research](army-research.md) for evidence, controlled reset boundaries and remaining numeric limits. The earlier conversion checker stays bounded to the conversion report; the Keep checker follows the actual army battle.
