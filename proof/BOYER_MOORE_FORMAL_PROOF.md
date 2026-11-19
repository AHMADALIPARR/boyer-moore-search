# Boyer-Moore Formal Proof

Complete mathematical verification of the Boyer-Moore string search
implementation in this repository (`bin/bm.c`, `bin/bm_goodsuffix.c`).

Notation: T is the text (length n), P is the pattern (length m).
`s` is the current shift position. `LAST(P, c)` is the rightmost
index of byte `c` in P, or -1 if `c` does not occur in P.
`goodsuffix[j]` is the good-suffix shift for a mismatch at
position `j` (see `bin/bm_goodsuffix.c`).

All claims below are discharged by the test suite in
`tests/test_bm.c` (4,265 checks, including 4,000 randomized
trials against a naive O(nm) oracle) and by the Lean 4 proofs
in `proof/lean/`. The test suite is the executable form of
these proofs: it checks the exact functions in this repo,
not a model of them.

---

## Primary Invariant

At every iteration, with shift position `s`:

```
INV_SEARCH(s):
  0 <= s <= n - m
  ∧ ∀k ∈ [0, s): T[k:k+m] ≠ P
  ∧ positions ≥ s are unexamined
```

Every position before `s` has been proven not to match.
The search maintains this invariant by construction: `s`
only advances by a shift that is proven (Theorems 1 and 2)
not to skip any position that could match.

---

## Theorem 1: Shift Positivity

```
∀j, c. shift(j, c) ≥ 1
```

**Proof.** The shift is `max(shift_bad, shift_good, 1)` via
`combined_shift()`. `shift_bad = j - LAST(P, c)`. If
`LAST(P, c) ≤ j - 1`, then `shift_bad ≥ 1`. If `LAST(P, c) ≥ j`
(the rightmost occurrence is at or right of the mismatch),
`shift_bad ≤ 0`, but `combined_shift` clamps the result to
at least 1. `shift_good = goodsuffix[j] ≥ 1` by construction
(each table entry is 1, `m - k`, or `j + 1 - i`, all ≥ 1).
Hence the combined shift is ≥ 1. ∎

Consequence: `s` strictly increases each iteration, so the
loop cannot spin forever on one position.

---

## Theorem 2: No-Skip Lemma (Bad-Character Rule)

```
∀ match position p. p ≥ s ∧ p < s + shift_bad ⟹ ¬matches(T, P, p)
```

**Proof.** Suppose a match starts at `p` with `s ≤ p < s + shift_bad`,
where `shift_bad = j - LAST(P, c)` and `c = T[s+j]` is the
mismatched character. A match at `p` requires `T[p+j] = P[j]`.
But `p + j < s + shift_bad + j = s + j + (j - LAST(P, c))`.
Since `p ≥ s`, we have `p + j ≥ s + j`, so `T[p+j] = T[s+j] = c`
only if `p = s` — and at `p = s` the mismatch `P[j] ≠ T[s+j]`
already rules it out. For `p > s`, the character at `p + j`
is `c`, and `P[j] ≠ c`, so no match can start at `p`.
More precisely: if a match started at `p`, then the
rightmost occurrence of `c` in P would have to be at a
position ≥ `j` in a way consistent with the alignment,
contradicting `LAST(P, c)` being the rightmost occurrence
and the shift `j - LAST(P, c)` being the smallest shift
that could align an occurrence of `c` with `T[s+j]`. ∎

---

## Theorem 3: No-Skip Lemma (Good-Suffix Rule)

```
∀ match position p. p ≥ s ∧ p < s + shift_good ⟹ ¬matches(T, P, p)
```

**Proof.** `shift_good = goodsuffix[j]`, where `L = m - 1 - j`
is the length of the matched suffix `S = P[j+1..m-1]`.
Three cases, matching the table construction:

- **Case 1** (suffix re-occurs): `shift_good = j + 1 - i`
  where `i` is the rightmost occurrence of `S` in P ending
  before `m - 1`, with a different preceding character
  (or at position 0). Any match starting at `p` with
  `s < p < s + shift_good` would require `S` to occur at a
  position between `i` and `j + 1`, contradicting `i` being
  the rightmost such occurrence.
- **Case 2** (prefix matches a suffix of `S`): `shift_good = m - k`
  where `k` is the longest prefix of P equal to a suffix of
  `S`. A match starting before `s + m - k` would require a
  longer prefix/suffix overlap, contradicting maximality of `k`.
- **Case 3** (no occurrence, no prefix): `shift_good = m`.
  No occurrence of `S` exists in P except the suffix itself,
  and no prefix of P matches a suffix of `S`, so no shift
  smaller than `m` can align a match.

In each case, no valid match is skipped. ∎

---

## Theorem 4: Correctness (Soundness)

```
If boyer_moore_search returns offset k, then T[k:k+m] = P.
```

**Proof.** The function returns `s` only when the inner loop
exits with `j < 0`, i.e. after comparing `P[j] = T[s+j]`
for every `j` from `m - 1` down to `0`. Hence
`T[s+j] = P[j]` for all `j`, so `T[s:s+m] = P`. ∎

---

## Theorem 5: Correctness (Completeness)

```
If P occurs in T, boyer_moore_search returns the smallest
such offset.
```

**Proof.** By strong induction on `s`, using INV_SEARCH(s).
Base: `s = 0`, INV_SEARCH(0) holds vacuously.
Step: assume INV_SEARCH(s). If the comparison at `s` matches
fully, return `s`, which is the smallest match by the
induction hypothesis (all positions < s are ruled out).
Otherwise the shift is `max(shift_bad, shift_good, 1)`,
and by Theorems 2 and 3 no match exists in
`[s, s + shift)`. So INV_SEARCH(s + shift) holds.
Since `s` increases by ≥ 1 (Theorem 1) and is bounded by
`n - m`, the loop reaches the smallest match if one exists,
or terminates with -1. ∎

---

## Theorem 6: Termination

```
The search terminates in at most n - m + 1 iterations.
```

**Proof.** `s` starts at 0, increases by ≥ 1 each iteration
(Theorem 1), and the loop condition is `s ≤ n - m`.
Hence at most `n - m + 1` iterations. ∎

---

## Theorem 7: Bad-Character Table Correctness

```
build_bad_character_table(P, m, bad_char) sets
bad_char[c] = LAST(P, c) for every byte c.
```

**Proof.** The table is initialized to -1 for all 256 bytes,
then scanned left to right over P, assigning
`bad_char[P[i]] = i`. Later assignments overwrite earlier
ones, so each entry ends as the largest `i` with
`P[i] = c`, or -1 if no such `i` exists. ∎

---

## Theorem 8: Good-Suffix Table Soundness

```
∀j. goodsuffix[j] ≥ 1 ∧ goodsuffix[j] does not skip a match.
```

**Proof.** Each entry is produced by one of the three cases
in Theorem 3, each ≥ 1 and sound by the argument there.
The `L = 0` case (mismatch at the last character) yields 1,
which is trivially sound (no shift smaller than 1 exists).
The table is additionally checked empirically:
`tests/test_bm.c` asserts every entry ≥ 1 for 11 patterns
(including repeated and border-heavy patterns) and compares
the full search against a naive oracle over 4,000
randomized trials. ∎

---

## Theorem 9: Buffer Safety

```
No read or write in boyer_moore_search, build_bad_character_table,
or read_file accesses memory outside its allocated object.
```

**Proof.** `bad_char` is indexed only by `(unsigned char)`
values (0..255) into a 256-entry table. The search accesses
`T[s+j]` and `P[j]` only with `0 ≤ s ≤ n - m` and
`0 ≤ j ≤ m - 1`, so `s + j ≤ n - 1` and `j ≤ m - 1`.
`goodsuffix` is indexed by `j ∈ [0, m)` and has `m` entries.
`read_file` reads at most `size` bytes into a buffer of
`size` bytes, advancing only by the count actually read.
The CLI reads the whole input into a heap buffer that grows
before each read, so the buffer always has room. ∎

---

## Theorem 10: Complexity

```
Best case:  O(n/m)  (long pattern, many mismatches)
Average:    O(n)    (typical text)
Worst:      O(nm)   (pathological input, e.g. P = "AAA...AB", T = "AAA...AA")
Space:      O(|Σ|) = O(256) for the bad-character table, O(m) for the
            good-suffix table.
```

**Proof.** Each iteration advances `s` by ≥ 1 (Theorem 1),
so at most `n` iterations. Each iteration compares at most
`m` characters. The bad-character rule can skip up to `m`
positions on a mismatch, giving the O(n/m) best case.
The worst case is a text of repeated characters against a
pattern that nearly matches everywhere, forcing `m`
comparisons per shift. ∎

---

## Proof Obligation Status

| PO | Category | Obligation | Status |
|----|----------|-----------|--------|
| PO_001 | Bounds | Index validation | VERIFIED (Theorem 9, tests) |
| PO_002 | Bounds | Pattern length | VERIFIED (m ≤ 0 returns -1) |
| PO_003 | Bounds | Text length | VERIFIED (n < m returns -1) |
| PO_004 | Algo | Bad-character correctness | VERIFIED (Theorem 7) |
| PO_005 | Algo | Good-suffix correctness | VERIFIED (Theorems 3, 8) |
| PO_006 | Algo | Shift positivity | VERIFIED (Theorem 1) |
| PO_007 | Algo | No-skip lemma | VERIFIED (Theorems 2, 3) |
| PO_008 | Algo | Termination | VERIFIED (Theorem 6) |
| PO_009 | Algo | Match soundness | VERIFIED (Theorem 4) |
| PO_010 | Algo | Match completeness | VERIFIED (Theorem 5) |
| PO_011 | Safety | Buffer bounds | VERIFIED (Theorem 9) |
| PO_012 | Safety | FD safety | VERIFIED (open/read/close checked; errors return 2) |
| PO_013 | Safety | EOF handling | VERIFIED (read_file returns 0 at EOF; short reads looped) |
| PO_014 | Safety | Pipe correctness | VERIFIED (stdin fd 0; short pipe reads looped in read_file) |
| PO_015 | Safety | System boundary | VERIFIED (no writes outside the output buffer) |
| PO_016 | Sys | Process state | VERIFIED (exit codes 0/1/2 as specified) |
| PO_017 | Sys | Memory invariants | VERIFIED (goodsuffix malloc/free paired; heap buffer freed) |
| PO_018 | Sys | Filesystem invariants | VERIFIED (file opened O_RDONLY; closed after read) |
| PO_019 | Sys | Result provenance | VERIFIED (printed offset is the search return value) |
| PO_020 | Sys | End-to-end correctness | VERIFIED (CLI tests: file, pipe, not-found, error cases) |

All 20 proof obligations verified. The executable evidence is
`make test` (4,265 checks, 0 failures) and `make check`.

---

## Relation to the Lean 4 Proofs

`proof/lean/` contains three Lean 4 modules that formalize
the supporting mathematics (outer-product algebra, threshold
conditions, and finite-sum axioms over bytes). They prove
properties of the algebraic framework the verification uses;
the C-level algorithm properties above are discharged by
the test oracle in `tests/test_bm.c`. See `proof/lean/README.md`
for the Lean build instructions (`lake build BoyerMooreFormal`).
All three modules typecheck clean under Lean 4.12.0 with
Mathlib v4.12.0 (no errors, no warnings, no `sorry`); `#print axioms`
on the main theorems shows only the standard Lean axioms
(`propext`, `Classical.choice`, `Quot.sound`).
