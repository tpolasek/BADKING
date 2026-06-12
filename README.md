# The Bad King — Linux Port

A Linux terminal port of **The Bad King**, a 1991 text-based RPG by Griffin Knodle (Flatrat Production), originally released for Windows 3.x.

Built by decompiling `BADKING.EXE` with Ghidra and fixing the output to compile and run as a native Linux terminal application.

## Building

```
make
```

Requires GCC. Produces the `badking` binary.

## Playing

```
./badking
```

Player name is hardcoded to "thomas". Terminal runs in raw mode (no Enter needed for commands).

**Travel commands:** `m`ove, `l`ook, `t`alk, `s`earch, `c`ast, `a`tatus, `b` save, `q`uit  
**Battle commands:** `f`ight, `c`ast, `r`un

## How It Works

The original `BADKING.EXE` is a 16-bit NE (New Executable) for Windows 3.x, compiled with Borland C++. Ghidra decompiled it to C, but the output had ~40 compilation errors, missing string data, and DOS/Windows system calls that don't exist on Linux.

The port fixes this by:

1. **Embedding string data** — 292 game strings extracted from the binary's data segment into `strings.c`
2. **Embedding the data segment** — the full 20KB data segment (navigation tables, character classification, item data) extracted into `dataseg.c`
3. **Replacing DOS/Windows I/O** — `swi(0x21)` interrupt stubs and Windows API calls replaced with standard C (`printf`, `getchar`, `fopen`, `fscanf`, `fprintf`, `time`, `exit`)
4. **Fixing 16-bit→64-bit issues** — data segment offsets converted to proper pointers via `DS()` macro, 16-bit table reads fixed, function signatures corrected
5. **Terminal raw mode** — `termios` for character-at-a-time input matching the original Windows message loop behavior

## Files

| File | Description |
|---|---|
| `decompiled.c` | Main game code — Ghidra decompilation with all Linux fixes applied |
| `strings.c` | 292 game string literals extracted from the binary (auto-generated) |
| `dataseg.c` | Full data segment from `BADKING.EXE` embedded as a C array (auto-generated) |
| `compat.h` | Type definitions (`undefined1`, `undefined2`, etc.), calling convention stubs, Ghidra helper macros |
| `decls.h` | Global variable declarations (game state, data segment references) |
| `dataseg.h` | Macros for data segment access (`DS()`, `DS_MUT()`, buffer names) |
| `decompiled_strings.txt` | Reference file mapping string offsets to their text content |
| `BADKING.EXE` | Original 16-bit Windows 3.x binary (NE format, 67KB) |
| `Makefile` | Build: `gcc -Wno-all decompiled.c strings.c dataseg.c -o badking` |

## Known Limitations

- Player name is hardcoded to "thomas" (original line input was replaced)
- Windows GUI functions (scrolling, caret, painting) are stubs — the game was always text-mode through an EasyWin window
- Some C runtime internals (locale, timezone parsing) are stubbed out
- 23 `swi(0x21)` DOS interrupt stubs remain in dead code paths (Windows init, C runtime internals) — none are reachable from the game
