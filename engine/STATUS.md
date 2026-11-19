# STATUS — engine (CompCert-lowered core)

Verification scope and limits, stated as plain facts.

## Verified

- CompCert 3.12 lowered `src/bm_core.c` (six pure functions) to
  `asm/bm_core.s` on 2026-11-18, x86-64 Linux, with no source
  change required. CompCert's machine-checked compiler
  correctness theorem is the guarantee: the assembly preserves
  the C semantics.
- The CompCert core passes the full 4,265-check suite
  (`tests/test_bm.c`: bad-char table, good-suffix table, fixed
  cases, 4,000 randomized trials vs a naive oracle, UTF-8
  helpers) with 0 failures, 10 consecutive runs.
- The CompCert-core CLI binary matches the GCC reference binary
  byte for byte (stdout, stderr, exit code) on 16 cases in
  `tests/cli_compare.sh`, including a 200 KB input and binary
  data with embedded NUL bytes.

## Not verified

- The GCC harness (I/O, CLI, pthreads, UTF-8 helpers) is not
  covered by CompCert's theorem. It is covered by the test suite
  and the CLI comparison only.
- The parallel search is not a CompCert target: pthreads are
  outside the CompCert C fragment. Its correctness rests on the
  test suite.
- No AArch64 build exists. The Jetson target needs a CompCert
  install configured for AArch64.
- `malloc`/`free` (used by `boyer_moore_search`) and `memcmp`
  (used by `compute_goodsuffix_table`) are external calls in the
  CompCert model: CompCert guarantees the code around them, and
  the calls themselves resolve to the system libc at link time.

## Known defect found during verification

- The original `parallel_search_worker` (in
  SNAPKITTYWEST/boyer-moore-search, `bin/bm_parallel.c`)
  records the first thread to finish instead of the lowest
  offset, so the parallel search intermittently returns a
  later match. Measured before the fix: the original GCC test
  binary failed 3–9 of 4,265 checks per run, and the
  CompCert-core binary failed 3–10 per run, all failures in
  the parallel tests, 0 in the core. The staged copy in
  `src/bm_parallel.c` records the smallest offset found, and
  the suite then passes 10/10 runs. The original repo was not
  modified and still has the race.

## Linker note

- Linking `asm/bm_core.o` warns `missing .note.GNU-stack
  section implies executable stack`. CompCert 3.12 does not
  emit `.note.GNU-stack`. The assembly is kept byte-identical
  to CompCert's output, so the warning is documented, not
  silenced by editing the generated file.
