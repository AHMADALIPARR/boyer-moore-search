/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * bm.c
 * Core Boyer-Moore string search: bad-character rule, good-suffix
 * rule, right-to-left comparison, Unix file I/O and pipe support.
 *
 * Algorithm (Boyer & Moore, 1977):
 *   Input:  T (text, length n), P (pattern, length m)
 *   Output: smallest k with T[k:k+m] = P, or NOT_FOUND (-1)
 *
 * Invariant INV_SEARCH(s), maintained at every shift position s:
 *   0 <= s <= n - m
 *   and every k in [0, s) has been proven not to match P.
 *
 * Each iteration compares P right-to-left against T[s:s+m]. On a
 * mismatch at position j with character c = T[s+j]:
 *   shift_bad  = j - LAST(P, c)          (bad-character rule)
 *   shift_good = goodsuffix[j]           (good-suffix rule)
 *   shift      = max(shift_bad, shift_good, 1)
 * Both rules are sound: neither skips a position that could match,
 * so the first match found is the leftmost match.
 *
 * CLI: ./bm pattern [file]
 *   Reads from file, or from stdin (fd 0) when no file is given.
 *   Prints the byte offset of the first match on stdout.
 *   Exit codes: 0 = found, 1 = not found, 2 = error.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bm.h"

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

/*
 * read_file
 * Read up to `size` bytes from fd into buffer, looping over short
 * reads (pipes) and EINTR. Returns bytes read (0 at EOF),
 * or -1 on error.
 */
ssize_t read_file(int fd, char *buffer, size_t size) {
    size_t total = 0;

    while (total < size) {
        ssize_t r = read(fd, buffer + total, size - total);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (r == 0) {
            break; /* EOF */
        }
        total += (size_t)r;
    }

    return (ssize_t)total;
}

/*
 * read_all
 * Read an entire fd into a heap buffer that grows as needed.
 * Initial capacity is BM_BUFFER_SIZE (64 KiB). Returns the buffer
 * (caller frees it) and sets *out_len, or NULL on error.
 * Only used by main(); excluded from the test build (BM_NO_MAIN).
 */
#ifndef BM_NO_MAIN
static char *read_all(int fd, size_t *out_len) {
    size_t cap = BM_BUFFER_SIZE;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (buf == NULL) {
        return NULL;
    }

    for (;;) {
        if (len == cap) {
            size_t ncap = cap * 2;
            char *nbuf = (char *)realloc(buf, ncap);
            if (nbuf == NULL) {
                free(buf);
                return NULL;
            }
            buf = nbuf;
            cap = ncap;
        }

        ssize_t r = read_file(fd, buf + len, cap - len);
        if (r < 0) {
            free(buf);
            return NULL;
        }
        if (r == 0) {
            break; /* EOF */
        }
        len += (size_t)r;
    }

    *out_len = len;
    return buf;
}
#endif /* BM_NO_MAIN */

#ifndef BM_NO_MAIN
int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s pattern [file]\n", argv[0]);
        return 2;
    }

    const char *pattern = argv[1];
    int m = (int)strlen(pattern);
    if (m == 0) {
        fprintf(stderr, "error: empty pattern\n");
        return 2;
    }

    int fd;
    if (argc == 3) {
        fd = open(argv[2], O_RDONLY);
        if (fd < 0) {
            perror("open");
            return 2;
        }
    } else {
        fd = STDIN_FILENO;
    }

    size_t n = 0;
    char *text = read_all(fd, &n);
    if (text == NULL) {
        perror("read");
        if (argc == 3) {
            close(fd);
        }
        return 2;
    }
    if (argc == 3) {
        close(fd);
    }

    if (n > (size_t)INT_MAX) {
        fprintf(stderr, "error: input too large (%zu bytes)\n", n);
        free(text);
        return 2;
    }

    int bad_char[BM_ALPHABET_SIZE];
    build_bad_character_table(pattern, m, bad_char);

    int off = boyer_moore_search(text, (int)n, pattern, m, bad_char);
    free(text);

    if (off >= 0) {
        printf("%d\n", off);
        return 0;
    }
    return 1;
}
#endif /* BM_NO_MAIN */
