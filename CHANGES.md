# Differences from classic Braminar

This document records intentional differences between the classic DOS game and the remake. Unfinished features and unresolved mechanics are tracked in the research notes under `doc/`.

## Terminology and text

- Replace slaves with servants throughout the remake.

## Typo corrections

| Location | Classic text | Remake text |
| --- | --- | --- |
| Character status | `Mecidine:` | `Medicine:` |
| Enchanted forest | `The nymph it luring you deep into the forest.` | `The nymph is luring you deep into the forest.` |

## Numeric precision

- Turn healing adds 0.10 HP instead of reproducing the classic executable's slightly smaller Pascal Real48 constant.
- HP and experience use C `double` arithmetic instead of Pascal Real48 storage and intermediate rounding.

## Bug fixes

- The gnome's food gift adds the announced 3–8 units to inventory. The classic game displays the gift without adding food. Normal end-of-turn food consumption still follows.
