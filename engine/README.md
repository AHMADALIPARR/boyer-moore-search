# Engine — CompCert-lowered Boyer-Moore core

Boyer-Moore string search, with the search core lowered to
assembly by the CompCert verified compiler. This repo is the
engine: the assembly in `asm/bm_core.s` is the product, and the
C it was lowered from sits beside it in `src/bm_core.c`.

## The verified core

CompCert 3.12 (`ccomp`, x86-64 Linux target) compiles
`src/bm_core.c` to `asm/bm_core.s`. CompCert carries a
machine-checked theorem that the generated assembly behaves
as the C source does, so the assembly is verified against the
C by construction, not by testing alone.

The six lowered functions (verbatim from
SNAPKITTYWEST/boyer-moore-search, `bin/bm.c` and
`bin/bm_goodsuffix.c`):

- `build_bad_character_table` — LAST(P, c) table
- `boyer_moore_search` — first-match search, bad-char + good-suffix
- `compute_border_array` — KMP prefix function
- `compute_goodsuffix_table` — good-suffix shifts, every entry >= 1
- `compute_suffix_shift` — shift for a matched suffix
- `combined_shift` — max of both rules, at least 1

No change to the C was needed for CompCert: the core as written
already fits the CompCert C dialect (C99, no VLAs, no pthreads,
no inline assembly). The only differences from the original files
are layout — I/O, the CLI, and pthreads live in the GCC harness.

## Outside the verified fragment

The harness is GCC-compiled and is not covered by CompCert's
theorem: file I/O and the CLI (`src/harness.c`), the parallel
search (`src/bm_parallel.c`, pthreads), and the UTF-8 helpers
(`src/bm_unicode.c`). They link against the CompCert core, and
CompCert output is ABI-compatible with GCC code, so the binaries
are mixed: verified core, plain harness.

One fix in the staged parallel copy: the original
`parallel_search_worker` records the first thread to finish,
not the lowest offset, so it intermittently returns a later
match instead of the first one. The staged copy records the
smallest offset found. The original repo still has the race.
See STATUS.md.

## Build and verify

```bash
make                    # CompCert core + GCC harness + tests
make test               # 4,265-check suite against the CompCert core
make verify             # suite + CLI comparison vs the GCC reference
```

Verified 2026-11-18 on this machine (x86-64 Linux):

- Test suite: 4,265 checks, 0 failures, 10 consecutive runs.
- CLI comparison (`tests/cli_compare.sh`): 16 cases, byte-for-byte
  identical stdout, stderr, and exit codes vs the GCC binary.

Linking the CompCert object prints a benign warning:
`missing .note.GNU-stack section implies executable stack`.
CompCert 3.12 does not emit that section. The assembly file is
kept byte-identical to CompCert's output, so the warning stands.

## Target

This build is x86-64 Linux (CompCert `arch=x86 model=64`).
The Jetson is AArch64: an AArch64 build needs a CompCert install
configured for that target, which this machine does not have.
The C core is portable; only the CompCert configuration changes.

## License

Sovereign Leviathan, see `license`. Commercial repository,
Apache-2.0 OR GPL-3.0-or-later, no MIT license.
