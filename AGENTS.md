# Braminar remake

Recreate the DOS adventure in modern C, matching its texts and mechanics. Preserve `dos/` as reference material. Replace slaves with servants; typos may be corrected.

## Game structure and initial scope

Focus on `dos/BRAMINAR.COM`. Defer `FINASSLT.CHN`; its separate entry function must exit the game until the final battle is implemented.

## Code

- Use simple, readable C99 for POSIX (Linux/BSD), without external dependencies. Defer Windows support.
- Preserve arithmetic affecting displayed values or gameplay; simplify incidental precision. See [mechanics](doc/mechanics.md). Record intentional differences in [CHANGES.md](CHANGES.md).
- Keep custom research tools in the project root and free of external dependencies.

## Reconstructing the game

- Default to targeted reconstruction; mix in ordinary play for discovery and interactions. Skip completed branches through normal game choices when useful.
- Consult and update the [strategy and coverage checklist](doc/reconstruction.md) each research session. Finish bounded targets through research, implementation, and comparison with the original.
- Distinguish observations, verified rules, and assumptions. Leave explicit placeholders linked to research notes for unresolved mechanics.
- Record every encountered event, including skipped branches. Save raw captures in `doc/captures/`, observations in [events](doc/events.md), and recovered rules in [mechanics](doc/mechanics.md). Clearly separate controlled experiments from ordinary play.

## Tools

Use `./dosbox_game.py` to play and capture the original. See [tool usage](doc/dosbox-tool.md) for setup, commands, controlled probes, and comparison checks.
