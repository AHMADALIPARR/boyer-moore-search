/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * bm_core.h
 * Public API of the pure Boyer-Moore core lowered by CompCert.
 *
 * This is the verified fragment: the six pure functions below are
 * compiled to assembly by CompCert 3.12 (x86-64), whose
 * machine-checked compiler correctness theorem guarantees the
 * generated assembly behaves as the C source does. Everything
 * else in the original repo (file I/O, CLI, pthreads, UTF-8
 * helpers) is outside the verified fragment and is compiled
 * with GCC as a harness that links against this core.
 *
 * Function bodies are verbatim copies of bin/bm.c and
 * bin/bm_goodsuffix.c from SNAPKITTYWEST/boyer-moore-search.
 * Only the file I/O, CLI main(), and pthreads were left out.
 */

#ifndef BM_CORE_H
#define BM_CORE_H

/* Byte alphabet size */
#define BM_ALPHABET_SIZE 256

/*
 * build_bad_character_table
 * LAST(P, c) = rightmost index of byte c in P, or -1 if absent.
 * bad_char must point to an array of BM_ALPHABET_SIZE ints.
 */
void build_bad_character_table(const char *P, int m, int *bad_char);

/*
 * boyer_moore_search
 * Returns the byte offset of the first occurrence of P in T,
 * or -1 if P does not occur in T.
 * bad_char must be a table built by build_bad_character_table for P.
 * Returns -1 on invalid input (NULL pointers, m <= 0, n < m).
 */
int boyer_moore_search(const char *T, int n, const char *P, int m, int *bad_char);

/*
 * compute_border_array
 * KMP prefix function for P.
 */
void compute_border_array(const char *P, int m, int *border);

/*
 * compute_goodsuffix_table
 * goodsuffix[j] = shift when a mismatch occurs at pattern
 * position j. Every entry is >= 1.
 */
void compute_goodsuffix_table(const char *P, int m, int *goodsuffix);

/*
 * compute_suffix_shift
 * Shift for a suffix match of matched_len characters.
 */
int compute_suffix_shift(const char *P, int m, int matched_len, int *goodsuffix);

/*
 * combined_shift
 * max(bad_char_shift, good_suffix_shift), at least 1.
 */
int combined_shift(int bad_char_shift, int good_suffix_shift);

#endif /* BM_CORE_H */
