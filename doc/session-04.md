# Five-turn gameplay pass, 2026-11-14

This was ordinary play without memory or seed edits. Every input and encountered screen was captured in [the raw transcript](captures/2026-11-14-session-04.txt), including the entire wizard fight. The run stopped at the status screen after turn five. Snapshots repeat earlier screen lines; a command label marks each new capture. During delayed transitions, a space can advance to status rather than start the next event.

Trailblazer began female, level 8, black hair, good looks, 39 HP, five servants of each sex, 8 gold, 100 food, and 3 medicine. No equipment, poison, horse, cart, Staff, or Talisman. Servant prices were 6 male / 9 female.

| Turn | Encounter and input | Next status |
| --- | --- | --- |
| 1 | Hollow tree: enter Y, empty home, take from trunk T. `You find the STAFF of AVIATAR!!!!!` | Staff TRUE, HP 39.10, food 95; other inventory unchanged |
| 2 | Thicket: enter Y. Laboratory with a six-pound roast; Anthrax appears and attacks. Advance each combat pause, then the victory/reward pause | Level 7, HP 35.20, males 32, females 52, gold 108, food 113; Staff retained |
| 3 | Gypsy camp: enter Y, play Dagger's Hand Y. Player succeeds; opponent hits a finger; `You win!`. Advance pause; buy advice Y | `Pan will tell you the Prime Command.` After another pause: HP 35.30, gold 99, food 71; servants unchanged |
| 4 | Grass hut: enter Y. `You find nothing.` Advance pause | HP 35.40, food 29; other inventory unchanged |
| 5 | Tornado: shelter N. `You may of lost some slaves in the storm.` Advance pause; starvation is announced before the next status | HP 35.50, males 26, females 49, gold 99, food 0 |

## Anthrax observations

The first displayed round ended at player 39.10 / wizard 31.00 HP after `You dunk his head in some bubbling brew`. Anthrax attempted a level drain; saving throw 9 failed. Later rounds included three announcements that he fried a servant, a successful plague spell (saving throw 67), a staff strike reducing HP by 3, and a lightning bolt reducing HP by 1. Other attacks missed, including a shot that hit a mirror behind the player. The fatal round reduced wizard HP to -5.40, but he still fried a servant before the result pause.

The wizard vanished from his cloak. The player took the roast and wine, then found an announced 30 males, 47 females, and 100 gold. Three fried-servant messages and the final males count are consistent with three males lost before that award; this single run does not establish how the game selects victims. Food rose from 95 to 113 after a 42-unit party consumption, consistent with an unannounced addition of 60. That inferred addition and the wizard's experience award still need binary verification. The plague was announced but its hidden byte was not read during ordinary play. The round-by-round HP and exact text remain in the capture.

This combat is separate from the implemented shared monster routine. Wizard attacks, saving throws, level drain, plague, rewards, and encounter chance remain unresolved implementation work; none was inferred from ordinary monster combat.

## Other observations and evidence limits

The winning Dagger's Hand game plus ten-gold advice reduced gold from 108 to 99, consistent with a net one-gold game prize. This does not establish the game's full betting arithmetic or probabilities. Advice followed the game's result pause; the advice had its own pause. The empty grass hut changed only normal healing and food consumption in this visit.

The tornado plus starvation removed six males and three females. Binary inspection separates those losses: storm damage rolls 1–5 independently for each sex; starvation then removes one of each positive count. Thus the observed storm loss was five males and two females. Other storm branches were inspected but not played in this pass; see [mechanics](mechanics.md#storms).

## Controlled empty-home checks and implementation

After the ordinary Staff discovery, eight separate experiments reset temporary emulated memory and select deterministic slot-6 seeds. They are explicitly labelled controlled probes in [their raw transcript](captures/2026-11-14-tree-probes.txt). `tools/tree_probe.py` reproduces them and writes [compact checkpoints](tree-checkpoints.json). These runs are separate from Trailblazer's ordinary playthrough.

Each starts at level 8, 50 HP, five males, five females, 8 gold, 100 food, 3 medicine, 2 arrows, and no ring/plague. Staff is absent except in `staff-held`. Inventory is reset between cases; displayed status text immediately after those edits can be stale. Final states are read from memory after the next status appears.

| Case | Seed | Choice | Original result before turn bookkeeping |
| --- | --- | --- | --- |
| arrows | 8517 | Y, T | Add 5 arrows |
| medicine | 8519 | Y, T | Add 1 medicine |
| staff | 8522 | Y, T | Award Staff |
| staff-held | 8522 | Y, T | Add 9 medicine instead of a duplicate Staff |
| guardian | 8514 | Y, T | Subtract 5 HP |
| food | 8517 | Y, S | Add 57 food |
| leave | 8517 | Y, L | No inventory change or outcome line |
| decline | 8517 | N | `You move on.`; no occupied/empty roll |

The remake now implements this whole slot-6 branch from binary-verified rules, including random-call order and its exit pause. It leaves the separately dispatched slot-13 tree unfinished. No new intentional difference from the classic was introduced.

`make` succeeds with the existing unused-final-battle-stub warning. `./tools/check_tree.py --sanitize` passes all eight original-game state comparisons (including RNG and expected input counts); `./tools/check_combat.py --sanitize` passes all twelve existing combat scenarios. Both use address and undefined-behavior sanitizers. `git diff --check` passes. This validation does not establish wizard, Gypsy, hut, or storm implementation parity.
