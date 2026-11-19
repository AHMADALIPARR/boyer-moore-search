/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * bm.h
 * Public API for the Boyer-Moore string search implementation.
 *
 * Core (bin/bm.c): bad-character table, search loop, file I/O, CLI.
 * Extensions: good-suffix (bin/bm_goodsuffix.c), parallel search
 * (bin/bm_parallel.c), UTF-8 support (bin/bm_unicode.c).
 */

#ifndef BM_H
#define BM_H

#include <stddef.h>
#include <sys/types.h>

/* Initial read buffer size (64 KiB); grows as needed for large files */
#define BM_BUFFER_SIZE 65536

/* Byte alphabet size */
#define BM_ALPHABET_SIZE 256

/* ------------------------------------------------------------------ */
/* Core implementation (bin/bm.c)                                     */
/* ------------------------------------------------------------------ */

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
 * read_file
 * Read up to `size` bytes from fd into buffer, looping over short
 * reads and EINTR. Returns bytes read (0 at EOF), or -1 on error.
 */
ssize_t read_file(int fd, char *buffer, size_t size);

/* ------------------------------------------------------------------ */
/* Good-suffix rule (bin/bm_goodsuffix.c)                             */
/* ------------------------------------------------------------------ */

void compute_border_array(const char *P, int m, int *border);
void compute_goodsuffix_table(const char *P, int m, int *goodsuffix);
int compute_suffix_shift(const char *P, int m, int matched_len, int *goodsuffix);
int combined_shift(int bad_char_shift, int good_suffix_shift);

/* ------------------------------------------------------------------ */
/* Parallel search (bin/bm_parallel.c)                                */
/* ------------------------------------------------------------------ */

int parallel_boyer_moore_search(const char *text, size_t text_len,
                                const char *pattern, int pattern_len,
                                const int *bad_char, int num_threads);
int optimal_thread_count(size_t text_len);

/* ------------------------------------------------------------------ */
/* UTF-8 support (bin/bm_unicode.c)                                   */
/* ------------------------------------------------------------------ */

int utf8_char_length(const unsigned char *str, size_t offset, size_t max_len);
size_t utf8_next(const unsigned char *str, size_t offset, size_t len);
int utf8_strcmp_at(const unsigned char *text, size_t t_off, size_t t_len,
                   const unsigned char *pattern, size_t p_len);
size_t utf8_strlen(const unsigned char *str, size_t byte_len);
int utf8_is_valid(const unsigned char *str, size_t len);

#endif /* BM_H */
