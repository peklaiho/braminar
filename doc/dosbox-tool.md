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
at the `dosbox>` prompt; each command prints the resulting screen:

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
