# Desk Check Example: "ABCD" in "XABCABCD"

Manual hand-trace of the Boyer-Moore search in this repository,
verified by running the built binary and by the test suite
(`tests/test_bm.c`, which checks this exact case against a
naive oracle).

## Setup

```
Text:    X A B C A B C D
Index:   0 1 2 3 4 5 6 7
         n = 8

Pattern: A B C D
Index:   0 1 2 3
         m = 4
```

Bad-character table `LAST(P, c)` (rightmost index, -1 if absent):

| c | A | B | C | D | X | (other) |
|---|---|---|---|---|---|---------|
| LAST | 0 | 1 | 2 | 3 | -1 | -1 |

Good-suffix table `goodsuffix[j]` (shift on mismatch at `j`):

| j | 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| goodsuffix[j] | 4 | 4 | 4 | 1 |

Why: the pattern has no repeated suffix, so for `j = 0, 1, 2`
no occurrence and no prefix match exists (shift = m = 4).
For `j = 3` no suffix matched (`L = 0`), so the good-suffix
rule gives no shift (1).

## Iteration-by-Iteration Trace

### Iteration 1: s = 0

Compare right-to-left against `T[0..3] = "XABC"`:

| j | P[j] | T[s+j] | Result |
|---|------|--------|--------|
| 3 | D | C (T[3]) | mismatch, stop |

Mismatch at `j = 3` with character `c = 'C'`.

- `shift_bad = j - LAST(P, 'C') = 3 - 2 = 1`
- `shift_good = goodsuffix[3] = 1`
- `shift = max(1, 1) = 1`
- `s = 0 + 1 = 1`

Invariant check: position 0 is ruled out (`T[0..3] = "XABC" ≠ "ABCD"`).
INV_SEARCH(1) holds.

### Iteration 2: s = 1

Compare right-to-left against `T[1..4] = "ABCA"`:

| j | P[j] | T[s+j] | Result |
|---|------|--------|--------|
| 3 | D | A (T[4]) | mismatch, stop |

Mismatch at `j = 3` with character `c = 'A'`.

- `shift_bad = j - LAST(P, 'A') = 3 - 0 = 3`
- `shift_good = goodsuffix[3] = 1`
- `shift = max(3, 1) = 3`
- `s = 1 + 3 = 4`

Invariant check: positions 1, 2, 3 are ruled out. Position 1
fails at `j = 3` (`D ≠ A`). Positions 2 and 3 cannot match
either: shifting by 3 is justified by the bad-character rule —
aligning the rightmost `A` in the pattern (index 0) with the
`A` at `T[4]` — and any match starting at 2 or 3 would need
`P[3] = T[s+3]`, but `T[5] = 'B'` and `T[6] = 'C'`, neither
of which is `D`. INV_SEARCH(4) holds.

### Iteration 3: s = 4

Compare right-to-left against `T[4..7] = "ABCD"`:

| j | P[j] | T[s+j] | Result |
|---|------|--------|--------|
| 3 | D | D (T[7]) | match |
| 2 | C | C (T[6]) | match |
| 1 | B | B (T[5]) | match |
| 0 | A | A (T[4]) | match |

`j` reaches -1: full match.

**FOUND at position 4.**

## Verification

```
$ echo "XABCABCD" | ./bin/bm "ABCD"
4
$ echo $?
0
```

The binary prints `4` and exits 0. The test suite checks this
case (`check_search("XABCABCD", "ABCD")`) against the naive
oracle and passes.

Note: an earlier draft of the README traced this example to a
match at position 2. That is incorrect: `T[2..5] = "BCAB"`,
not `"ABCD"`. The correct leftmost match is at position 4
(`T[4..7] = "ABCD"`). This document records the verified trace.

## Corner Cases

### Empty pattern

`./bm "" file` → error, exit 2. An empty pattern has no
meaningful search; the CLI rejects it rather than returning
a spurious offset.

### Empty text

`./bm "ABC" /dev/null` → not found, exit 1. `n = 0 < m`,
so `boyer_moore_search` returns -1 immediately.

### Pattern longer than text

`./bm "ABCDEFGH" file` (4-byte file) → not found, exit 1.
`n < m` returns -1 without entering the loop.

### Pattern equals text

`./bm "ABC" file` where the file contains `ABC` → `0`, exit 0.
`s = 0` matches fully on the first iteration.

### Pattern not found

`./bm "XYZ" file` → not found, exit 1. The loop exhausts
all shift positions (`s` passes `n - m`) without a full match.

### Repeated characters

Text `"AAAAAAAAAA"`, pattern `"AAA"` → `0`, exit 0.
The good-suffix table for `"AAA"` is `[1, 1, 1]` (the suffix
re-occurs immediately), so the search advances by 1 and
finds the match at the first position. The bad-character
rule alone would also find it; both rules agree.

### Pattern with a border

Pattern `"ABCABC"` has border `"ABC"`. Its good-suffix table
is `[3, 3, 3, 6, 6, 1]`: for `j = 2` the suffix `"ABC"`
re-occurs at position 0, giving shift 3; for `j = 1` and
`j = 0` a prefix of length 3 matches a suffix of the matched
suffix, also giving shift 3. This is the case where the
good-suffix rule does real work beyond the bad-character rule,
and it is covered by the randomized oracle test.
