# Running and capturing the original game

`dosbox_game.py` launches Braminar in `dosbox-debug`, sends keyboard input, and
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
./dosbox_game.py
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
./dosbox_game.py \
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

`combat_probe.py` imports the same `Game` runner to capture a band encounter with
chosen player state and RNG seed. It labels its output `CONTROLLED PROBE`, records
the configuration, captures inputs and screens, and prints `STATE` JSON records
with inventory, HP, experience, ring state, and RNG state at each checkpoint.

```sh
./combat_probe.py --seed 6427 > /tmp/goblin-fight.txt
./combat_probe.py --seed 6427 --bows 1 --arrows 2 > /tmp/arrow-exhaustion.txt
./combat_probe.py --seed 6450 --choice g > /tmp/band-payment.txt
./combat_probe.py --seed 6443 --swords 2 --daggers 3 --poison 1 --use-poison
```

Use `./combat_probe.py --help` for player-state options. Seeds must select slot 4;
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
conversion functions in `combat_probe.py` serve binary inspection only; the C
remake uses ordinary arithmetic as described in `CHANGES.md`.

Run `./check_combat.py` to compare the remake with the saved controlled captures,
or `./check_combat.py --sanitize` to enable address and undefined-behavior
sanitizers. This requires a C compiler (`CC`, default `cc`); it builds
`check_combat.c` in a temporary directory and runs the checks without DOSBox.
The compact fixtures in `doc/combat-checkpoints.json` are sufficient to run the
checks without the raw transcripts, which the repository ignores. Individual
case names may be passed as arguments, for example `./check_combat.py arrows poison`.
See the
[combat ledger](combat-research.md) for cases and evidence limits.

## Empty-home probes

`./tree_probe.py > doc/captures/2026-11-14-tree-probes.txt` captures eight
controlled slot-6 empty-home branches and writes `doc/tree-checkpoints.json`.
It uses deterministic seeds and resets temporary player memory between cases;
the header and each case explicitly label these modifications. Status screens
before the next turn can contain values from the previous case. The final
`STATE` records are memory reads after the next status screen.

Run `./check_tree.py --sanitize` to compare the remake with those saved original
checkpoints without DOSBox. The checks include the four trunk results, the
held-Staff medicine substitute, food theft, leaving, and declining entry, along
with RNG progression and input counts. See [session 04](session-04.md) and the
[recovered rules](mechanics.md#empty-gnome-home).

## Storm and illness probes

`./event_probe.py > doc/captures/2026-11-14-storm-illness-probes.txt` captures
controlled storms and illness, resetting temporary player memory and selecting
seeds before dispatch. Completed cases are saved immediately in
`doc/storm-illness-checkpoints.json`. To rerun selected cases and update their
entries, pass names, for example `./event_probe.py storm-tidal illness-excess`.
The invalid/overflow cases deliberately abort the original; the tool restarts
it if further cases remain. Every encountered screen and input is captured.

`./check_events.py --sanitize` runs comparisons without DOSBox. It checks state,
RNG, normalized text order, input counts, and requested delays. Input cases explicitly check the documented shared-reader differences, and the
suite also checks integer parsing directly. See the
[research ledger](storm-illness-research.md) for evidence and validation limits.

## Pan probes

`./pan_probe.py > doc/captures/2026-11-15-pan-probes.txt` captures five controlled
Pan cases and saves `doc/pan-checkpoints.json`. It resets temporary player state,
RNG, and an existing prime command before each case. Captures include success,
failure, decline, invalid answer keys, and question records 1, 3, and 20.

`./check_pan.py --sanitize` compares state, command, RNG, normalized text, and
input counts without DOSBox, and checks all 20 embedded question records against
`BRAM.DAT` with the approved correction. See the [ledger](pan-research.md) for
scope and limits.

## City and Village probes

`./town_probe.py > doc/captures/2026-11-15-town-probes.txt` captures 40 controlled
City/Village cases and saves `doc/town-checkpoints.json`. It resets player memory,
servant prices (8/13), and RNG before each case and records memory at every input
boundary. Pass case names to update selected cases. `--output PATH` allows
independent capture batches. The runner restarts after fatal poisoning or the
original zero-gold pickpocket abort when more cases remain.

`./check_towns.py --sanitize` compares state, RNG, prompt order, input counts, and
requested delays without DOSBox. `--fixtures PATH` selects another checkpoint
file. Fatal poisoning compares event state before bookkeeping because DOS has
reclaimed DS after death; the abort case checks unsuccessful termination.
See [town research](town-research.md) for captures, reproduction, and limits.

## Mountain trail probes

`./mountain_probe.py > doc/captures/2026-11-15-mountain-probes.txt` captures five
controlled trail/guru cases and writes `doc/mountain-checkpoints.json`. It resets
temporary player memory and RNG before each case.

`./check_mountain.py --sanitize` compares state, RNG, normalized text, and input
counts without DOSBox, and checks EOF at both questions. See the
[research notes](mountain-research.md) for scope and limits.

## Gypsy camp probes

`./gypsy_probe.py > doc/captures/2026-11-15-gypsy-probes.txt` captures 51 controlled
camp cases and saves `doc/gypsy-checkpoints.json`. These include all 35 advice
results, gambling outcomes, and resource boundaries. Pass case names to update
selected entries, or use `--output PATH` for a separate fixture file. The probe
waits for automatic gambling rounds to finish before advancing their pause.

`./check_gypsy.py --sanitize` compares state and RNG at each input, normalized
event text, key counts, and requested delays without DOSBox. Fatal injury
compares event state before bookkeeping because DOS reclaims DS after death.
See [research notes](gypsy-research.md) for scope and limits.
