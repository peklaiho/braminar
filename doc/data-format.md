# Original game data

The original files in `dos/` are the reference for the remake. The extraction tool reads them without modifying them. Extracted text retains the original spelling and terminology; corrections belong in the remake.

## Extraction tool

Build the standalone C99 tool from the project root:

```sh
make extract_data
```

Decode the question records:

```sh
./extract_data questions dos/BRAM.DAT
```

Extract printable ASCII runs of at least four bytes from either binary:

```sh
./extract_data strings dos/BRAMINAR.COM
./extract_data strings dos/FINASSLT.CHN
```

Output goes to standard output and can be redirected to a file. Errors go to standard error and produce a nonzero exit status. The tool requires no external libraries and does not require an interactive terminal.

All displayed offsets are hexadecimal byte offsets from the start of the input file. They are not DOS runtime addresses. Question field offsets point to the length byte; string-scan offsets point to the first printable byte.

## File inventory

| File | Size in bytes | Purpose |
| --- | ---: | --- |
| `BRAMINAR.COM` | 62,456 | Main gameplay executable, including text and mechanics |
| `FINASSLT.CHN` | 11,449 | Separate final assault module |
| `BRAM.DAT` | 4,320 | Question and answer records |
| `BRAMINAR.DOC` | 2,981 | Original player documentation |
| `FILES.TXT` | 298 | Original file descriptions |

The main executable contains a Borland copyright signature. Its code and data are consistent with an early Turbo Pascal program; the precise compiler version is not yet established.

## BRAM.DAT record layout

Inspection of the supplied file establishes a layout of 30 consecutive records, each 144 bytes long. Each field uses a Pascal short-string representation: one unsigned length byte followed by a fixed-capacity character area. There are no separators between records or fields.

| Field | Offset within record | Character capacity | Total storage |
| --- | ---: | ---: | ---: |
| Question | 0 (`0x00`) | 80 bytes | 81 bytes |
| Answer 1 | 81 (`0x51`) | 20 bytes | 21 bytes |
| Answer 2 | 102 (`0x66`) | 20 bytes | 21 bytes |
| Answer 3 | 123 (`0x7B`) | 20 bytes | 21 bytes |

For record number `n`, numbered from 1, the record starts at `(n - 1) * 144`. Read only the number of characters specified by each field's length byte. Strings are not null-terminated. The tool quotes the decoded text, escapes quotes and backslashes, and renders non-ASCII or control bytes as `\xNN` without assuming a character encoding.

The tool rejects partial records and lengths greater than the field capacity. It accepts any number of complete records, rather than assuming every input is the supplied 30-record file.

### Observed contents

- Records 1 through 20 contain questions and three answer choices.
- Records 21 through 30 have zero lengths for all four fields. Their storage still contains nonzero bytes, so empty fields must be identified by length rather than by their entire storage area.
- Unused space after string contents includes old text and Pascal source fragments such as `Player.Daggers:=Player.Daggers+1;`. These bytes are outside the declared strings and must not be displayed as question or answer text.
- Record 3's third answer is actually declared as `Do Lake:=1 to 5`, including the apparent source fragment. This is inside its 15-byte string length, so the extractor preserves it. Whether to correct it for gameplay needs separate investigation.

The first record decodes to:

| Field | Length-byte file offset | Length | Text |
| --- | --- | ---: | --- |
| Question | `0x00000000` | 39 | What does the Harkonen frog taste like? |
| Answer 1 | `0x00000051` | 7 | Spinach |
| Answer 2 | `0x00000066` | 15 | Orange Sherbert |
| Answer 3 | `0x0000007B` | 13 | Guacamole Dip |

### Rules still to recover

There is no separate answer-key field in this record layout. The binary must be examined to establish how the correct choice is determined, which records can be selected, and how the question event awards the prime command. Empty records do not establish whether the original code can select them.

## Binary text extraction

The `strings` mode finds consecutive bytes in the printable ASCII range `0x20` through `0x7E`, including spaces. It reports runs of at least four bytes and scans through the end of the file without a fixed maximum string length.

This is a discovery aid. It does not identify Pascal string boundaries or distinguish text from machine code. Runs can include printable length bytes, adjacent code, or several strings combined. Text shorter than four bytes and characters outside ASCII are omitted. Use the offsets to inspect surrounding bytes and code before treating a run as an exact game message.

Useful starting points in `BRAMINAR.COM` include:

| File offset | Readable text begins |
| --- | --- |
| `0x0000A640` | `Gender:` |
| `0x0000ED97` | `Welcome to:` |
| `0x0000EE0B` | `You will start out a 7-10 level player` |
| `0x0000F3BB` | `FINASSLT.CHN` |

These mark text bytes, which can lie inside a longer run in the scanner output. Extracted text helps locate relevant routines, but does not establish combat arithmetic, event probabilities, initialization rules, or the mapping of shared state into the final battle module.
