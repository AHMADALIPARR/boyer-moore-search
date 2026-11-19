/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * bm_core.c
 * Pure Boyer-Moore core, lowered to assembly by CompCert.
 *
 * The six functions below are verbatim copies of the pure core
 * from bin/bm.c and bin/bm_goodsuffix.c in
 * SNAPKITTYWEST/boyer-moore-search. No algorithmic change was
 * made for CompCert: the code as written already fits the
 * CompCert C dialect (C99, no VLAs, no pthreads, no inline asm).
 * The only differences from the original files are the file
 * layout (I/O, CLI, and pthreads live in the GCC harness) and
 * this header comment.
 *
 * Build: ccomp -S -I src -o asm/bm_core.s src/bm_core.c
 */

#include <stdlib.h>
#include <string.h>

#include "bm_core.h"


/*
 * build_bad_character_table
 * LAST(P, c) = max({i | P[i] = c}) or -1 if c does not occur in P.
 * Table has one entry per byte value (BM_ALPHABET_SIZE = 256).
 */
void build_bad_character_table(const char *P, int m, int *bad_char) {
    for (int i = 0; i < BM_ALPHABET_SIZE; i++) {
        bad_char[i] = -1;
    }
    for (int i = 0; i < m; i++) {
        bad_char[(unsigned char)P[i]] = i;
    }
}


/*
 * boyer_moore_search
 * Returns the byte offset of the first occurrence of P in T, or -1.
 *
 * The good-suffix table is built per call (O(m) preprocessing).
 * The search itself is O(n) average-case, O(nm) worst-case.
 */
int boyer_moore_search(const char *T, int n, const char *P, int m, int *bad_char) {
    if (T == NULL || P == NULL || bad_char == NULL) {
        return -1;
    }
    if (m <= 0 || n < m) {
        return -1;
    }

    int *goodsuffix = (int *)malloc((size_t)m * sizeof(int));
    if (goodsuffix == NULL) {
        return -1;
    }
    compute_goodsuffix_table(P, m, goodsuffix);

    int found = -1;
    for (int s = 0; s <= n - m; ) {
        int j = m - 1;

        /* Right-to-left comparison */
        while (j >= 0 && P[j] == T[s + j]) {
            j--;
        }

        if (j < 0) {
            found = s; /* Full match */
            break;
        }

        /* Mismatch at j with character T[s+j] */
        int shift_bad = j - bad_char[(unsigned char)T[s + j]];
        int shift = combined_shift(shift_bad, goodsuffix[j]);
        s += shift;
    }

    free(goodsuffix);
    return found;
}


/* Compute border array (failure function) */
void compute_border_array(const char *P, int m, int *border) {
    border[0] = 0;
    int j = 0;

    for (int i = 1; i < m; i++) {
        while (j > 0 && P[i] != P[j]) {
            j = border[j - 1];
        }
        if (P[i] == P[j]) {
            j++;
        }
        border[i] = j;
    }
}


/*
 * Compute good-suffix table
 * goodsuffix[j] = shift when a mismatch occurs at pattern position j,
 * i.e. the suffix P[j+1..m-1] (length L = m-1-j) matched.
 *
 * Standard Boyer-Moore good-suffix rule, computed directly:
 *   L == 0: no suffix matched, the rule gives no shift (1).
 *   Case 1: the suffix occurs elsewhere in P (ending before m-1)
 *     with a different preceding character (or at the start);
 *     shift to the rightmost such occurrence.
 *   Case 2: otherwise, the longest prefix of P that is a suffix
 *     of the matched suffix; shift so that prefix aligns.
 *   Case 3: otherwise, shift by the full pattern length.
 *
 * The table is verified against a naive-search oracle by
 * tests/test_bm.c (fixed cases + 4000 randomized trials).
 *
 * INV_GOODSUFFIX:
 *   ∀j ∈ [0,m): goodsuffix[j] ≥ 1
 *   ∧ No valid match is skipped
 */
void compute_goodsuffix_table(const char *P, int m, int *goodsuffix) {
    for (int j = 0; j < m; j++) {
        int L = m - 1 - j; /* length of the matched suffix P[j+1..m-1] */

        if (L == 0) {
            /* No suffix matched: the good-suffix rule gives no shift. */
            goodsuffix[j] = 1;
            continue;
        }

        int shift = m; /* default: Case 3 (no occurrence, no prefix) */

        /* Case 1: rightmost occurrence of the suffix elsewhere in P,
         * ending at or before m-2, with a different preceding char
         * (or at the start). Shift so it aligns with the match. */
        for (int i = m - 1 - L; i >= 0; i--) {
            if (memcmp(P + i, P + j + 1, (size_t)L) == 0) {
                if (i == 0 || P[i - 1] != P[j]) {
                    shift = j + 1 - i;
                    break; /* rightmost occurrence: smallest shift */
                }
            }
        }

        if (shift == m) {
            /* Case 2: longest prefix of P that is a suffix of the
             * matched suffix. Shift so that prefix aligns. */
            for (int k = L; k >= 1; k--) {
                if (memcmp(P, P + j + 1 + L - k, (size_t)k) == 0) {
                    shift = m - k;
                    break;
                }
            }
        }

        goodsuffix[j] = shift;
    }
}


/*
 * Compute shift for matching suffix
 * When we have a suffix match, compute how far to shift
 */
int compute_suffix_shift(const char *P, int m, int matched_len, int *goodsuffix) {
    (void)P; /* Pattern is implicit in the precomputed table */
    if (matched_len <= 0 || matched_len >= m) {
        return m;
    }
    return goodsuffix[m - matched_len - 1];
}


/*
 * Combined shift computation: bad-char + good-suffix
 * Takes max of both rules to ensure O(n) performance
 *
 * THEOREM_CS_001:
 *   shift = max(shift_bad, shift_good) ≥ 1 always
 *   (proven via shift ≥ 1 of each rule)
 */
int combined_shift(int bad_char_shift, int good_suffix_shift) {
    int shift = bad_char_shift;
    if (good_suffix_shift > shift) {
        shift = good_suffix_shift;
    }
    /* Safety: ensure at least 1 to prevent infinite loop */
    return (shift > 0) ? shift : 1;
}
