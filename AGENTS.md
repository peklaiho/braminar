# Braminar remake

Recreate the original DOS text adventure Braminar in modern C. The original source code is unavailable; the binaries, data files, and documentation are in `dos/`. Preserve these files as references.

Match the original game's texts and mechanics, but typos in the original text may be corrected. As documented in `README.md`, replace slaves with servants.

Preserve the player-visible experience rather than reproducing incidental numeric precision. Simplify minor rounding and floating-point representation details when they make no observable difference to a human player; for example, restore health by 0.10 per turn without emulating Pascal Real48. Preserve arithmetic that affects displayed values, costs, probabilities, thresholds, or gameplay outcomes. Record intentional differences from the classic game in [CHANGES.md](CHANGES.md), and distinguish those choices from unresolved mechanics in the research notes.

## Game structure and initial scope

The original game consists of two separate parts: the main gameplay in `dos/BRAMINAR.COM` and the final battle in `dos/FINASSLT.CHN`.

Focus initially on the main game. Implement the final battle later, with a separate function as its entry point. Until then, that function should be a stub that exits the game when the final battle is reached.

## Code

- Write clean, readable C99 targeting POSIX systems, primarily Linux and BSD. Use the C standard library and POSIX APIs without external libraries or dependencies. Windows support is deferred.
- Prefer simple functions, clear names, and straightforward control flow.

## Running the game

Run `./dosbox_game.py` from the project root to play the original game and capture its text screen directly from video memory. Use `key`, `text`, `screen`, `wait`, and `quit` interactively, or repeat `-c 'COMMAND'` for scripted sequences. See [DOSBox tool usage](doc/dosbox-tool.md) for setup, examples, and capture limitations. The tool runs a temporary copy of the DOS files and cleans up its processes on exit.

Use `./combat_probe.py --seed 6427` for a controlled original-game band encounter and `./check_combat.py --sanitize` to compare the remake with saved combat checkpoints. See the same usage document for options and the [combat ledger](doc/combat-research.md) for evidence. Keep controlled memory/seed experiments explicitly labelled and separate from unmodified playthroughs.

## Reconstructing the game

Capture information from every encounter as it occurs during play, including random events outside the current research focus. Save raw transcripts in `doc/captures/`, recording texts, choices, inputs, resulting screens, and visible state changes; summarize findings in `doc/events.md` and recovered rules in `doc/mechanics.md`, linking to the evidence. Do not restrict a playthrough to recording one targeted event and discard other encounters. Build the remake incrementally from confirmed texts and observed flow, then increase its precision as further gameplay and binary analysis establish conditions, arithmetic, and probabilities. Leave explicit placeholders and focused TODO comments linking to the research notes for unresolved mechanics, rather than silently inventing rules; keep observations distinct from verified rules.

- Build the first version by extracting the original texts and playing the DOS game to understand its flow and behavior.
- Reverse engineer the binaries where needed to recover the mechanics accurately. Combat probabilities and arithmetic that affect the player's experience are especially important; apply the numeric-simplification guideline above to incidental precision details.
- Distinguish verified behavior from observations and assumptions. Do not treat a plausible implementation as a confirmed original rule.
- Check reconstructed mechanics against binary evidence and observed gameplay.
- Create custom tools as needed to inspect the binaries or read the game's data files. Keep these tools in the project root alongside the other project files so the relevant work stays together. Keep tools free of external dependencies as well.
