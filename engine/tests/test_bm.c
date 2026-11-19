/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * test_bm.c
 * Unit and property tests for the Boyer-Moore implementation.
 *
 * Every search test is checked against a naive O(nm) oracle.
 * The randomized property test is the load-bearing check for the
 * good-suffix table: it compares boyer_moore_search against the
 * naive oracle over thousands of patterns drawn from small
 * alphabets (heavy repetition and borders), where an unsound
 * shift would skip a real match.
 *
 * Build: make test   (compiles with -DBM_NO_MAIN so bin/bm.c
 * contributes its search functions but not its main()).
 *
 * Exit code: 0 if all tests pass, 1 otherwise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bm.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

/* ------------------------------------------------------------------ */
/* Naive oracle: O(nm) reference search, first match or -1            */
/* ------------------------------------------------------------------ */

static int naive_search(const char *T, int n, const char *P, int m) {
    if (m <= 0 || n < m) {
        return -1;
    }
    for (int s = 0; s <= n - m; s++) {
        int j = 0;
        while (j < m && T[s + j] == P[j]) {
            j++;
        }
        if (j == m) {
            return s;
        }
    }
    return -1;
}

/* Run boyer_moore_search and compare against the oracle */
static void check_search(const char *text, const char *pattern) {
    int n = (int)strlen(text);
    int m = (int)strlen(pattern);
    int bad_char[BM_ALPHABET_SIZE];
    build_bad_character_table(pattern, m, bad_char);
    int got = boyer_moore_search(text, n, pattern, m, bad_char);
    int want = naive_search(text, n, pattern, m);
    checks++;
    if (got != want) {
        failures++;
        printf("FAIL search: text=\"%s\" pattern=\"%s\" got=%d want=%d\n",
               text, pattern, got, want);
    }
}

/* ------------------------------------------------------------------ */
/* Bad-character table                                                 */
/* ------------------------------------------------------------------ */

static void test_bad_char_table(void) {
    int bc[BM_ALPHABET_SIZE];

    build_bad_character_table("ABCD", 4, bc);
    CHECK(bc[(unsigned char)'A'] == 0);
    CHECK(bc[(unsigned char)'B'] == 1);
    CHECK(bc[(unsigned char)'C'] == 2);
    CHECK(bc[(unsigned char)'D'] == 3);
    CHECK(bc[(unsigned char)'X'] == -1);
    CHECK(bc[(unsigned char)'\0'] == -1);

    /* Rightmost occurrence wins */
    build_bad_character_table("ABCA", 4, bc);
    CHECK(bc[(unsigned char)'A'] == 3);
    CHECK(bc[(unsigned char)'B'] == 1);

    /* Repeated character */
    build_bad_character_table("AAAA", 4, bc);
    CHECK(bc[(unsigned char)'A'] == 3);
}

/* ------------------------------------------------------------------ */
/* Good-suffix table: every shift must be >= 1 (no infinite loop)    */
/* ------------------------------------------------------------------ */

static void test_goodsuffix_table(void) {
    const char *patterns[] = {
        "ABCD", "AAAA", "ABAB", "ABCABC", "A", "AA", "AB",
        "ABCAB", "MISSISSIPPI", "ABACABAD", "AAAAAAAAAB", NULL
    };

    for (int p = 0; patterns[p] != NULL; p++) {
        const char *pat = patterns[p];
        int m = (int)strlen(pat);
        int *gs = (int *)malloc((size_t)m * sizeof(int));
        CHECK(gs != NULL);
        if (gs == NULL) {
            continue;
        }
        compute_goodsuffix_table(pat, m, gs);
        for (int i = 0; i < m; i++) {
            checks++;
            if (gs[i] < 1) {
                failures++;
                printf("FAIL goodsuffix: pattern=\"%s\" i=%d shift=%d (< 1)\n",
                       pat, i, gs[i]);
            }
        }
        /* combined_shift never returns < 1 */
        for (int i = 0; i < m; i++) {
            CHECK(combined_shift(-5, gs[i]) >= 1);
            CHECK(combined_shift(0, gs[i]) >= 1);
        }
        free(gs);
    }
}

/* ------------------------------------------------------------------ */
/* Fixed search cases (README test categories)                        */
/* ------------------------------------------------------------------ */

static void test_fixed_cases(void) {
    /* Matches */
    check_search("XABCABCD", "ABCD");       /* README example: found at 2 */
    check_search("XABCABCD", "ABCABCD");    /* full match at 1 */
    check_search("XABCABCD", "ABC");        /* prefix at 1 */
    check_search("XABCABCD", "BCD");        /* suffix at 5 */
    check_search("XABCABCD", "CAB");        /* middle at 3 */
    check_search("XABCABCD", "AB");         /* multiple matches: first at 1 */
    check_search("A", "A");                 /* single character */
    check_search("ABC", "ABC");             /* pattern == text */
    check_search("hello world", "world");   /* word at end */
    check_search("hello world", "hello");   /* word at start */
    check_search("hello world", "o w");     /* spanning the space */

    /* Not found */
    check_search("XABCABCD", "XYZ");
    check_search("XABCABCD", "DCBA");
    check_search("ABC", "ABCD");            /* pattern longer than text */
    check_search("ABCD", "ABCDE");          /* pattern longer than text */
    check_search("", "ABC");                /* empty text */
    check_search("ABC", "");                /* empty pattern (both return -1) */
    check_search("", "");                   /* both empty */

    /* Repeated characters (stress the good-suffix rule) */
    check_search("AAAAAAAAAA", "AAA");
    check_search("AAAAAAAAAA", "AAAAAB");
    check_search("ABABABABAB", "ABAB");
    check_search("ABABABABAB", "BABA");
    check_search("ABABABABAB", "ABABAB");
    check_search("AAAAABAAAA", "AAB");
    check_search("AAAAABAAAA", "BAAA");

    /* Borders / self-overlap */
    check_search("ABCABCABC", "ABCABC");
    check_search("ABCABCABC", "BCAB");
    check_search("MISSISSIPPI", "ISSI");
    check_search("MISSISSIPPI", "SSI");
    check_search("MISSISSIPPI", "PPI");
    check_search("MISSISSIPPI", "MISSISSIPPI");
    check_search("MISSISSIPPI", "ISSIPP");
    check_search("ABACABADABACABA", "ABACABAD");
    check_search("ABACABADABACABA", "ABAD");

    /* Binary-ish data (embedded NUL is handled by length, not strlen) */
    {
        char text[6] = {'A', '\0', 'B', 'C', 'D', 'E'};
        char pat[3] = {'\0', 'B', 'C'};
        int bc[BM_ALPHABET_SIZE];
        build_bad_character_table(pat, 3, bc);
        int got = boyer_moore_search(text, 6, pat, 3, bc);
        CHECK(got == 1);
    }
}

/* ------------------------------------------------------------------ */
/* Randomized property test vs the naive oracle                       */
/* ------------------------------------------------------------------ */

/* Deterministic xorshift32 PRNG (fixed seed: reproducible runs) */
static unsigned int rng_state = 0x12345678u;

static unsigned int rng_next(void) {
    unsigned int x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static void test_random(void) {
    /* Small alphabets force repetition and borders */
    static const char alphabets[][8] = {
        "AB", "ABC", "ABCD", "ab", "01"
    };
    const int n_alphabets = 5;

    for (int trial = 0; trial < 4000; trial++) {
        const char *alpha = alphabets[trial % n_alphabets];
        int asz = (int)strlen(alpha);

        int n = 1 + (int)(rng_next() % 60);   /* text length 1..60 */
        int m = 1 + (int)(rng_next() % 12);   /* pattern length 1..12 */
        if (m > n) {
            m = n;
        }

        char text[64];
        char pattern[16];
        for (int i = 0; i < n; i++) {
            text[i] = alpha[rng_next() % (unsigned int)asz];
        }
        text[n] = '\0';

        /* Half the time, plant the pattern in the text so a match exists */
        if ((trial % 2) == 0 && m <= n) {
            int pos = (int)(rng_next() % (unsigned int)(n - m + 1));
            for (int i = 0; i < m; i++) {
                pattern[i] = text[pos + i];
            }
        } else {
            for (int i = 0; i < m; i++) {
                pattern[i] = alpha[rng_next() % (unsigned int)asz];
            }
        }
        pattern[m] = '\0';

        int bc[BM_ALPHABET_SIZE];
        build_bad_character_table(pattern, m, bc);
        int got = boyer_moore_search(text, n, pattern, m, bc);
        int want = naive_search(text, n, pattern, m);
        checks++;
        if (got != want) {
            failures++;
            printf("FAIL random trial %d: text=\"%s\" pattern=\"%s\" got=%d want=%d\n",
                   trial, text, pattern, got, want);
            if (failures > 10) {
                printf("...too many failures, stopping random test\n");
                return;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Parallel search                                                     */
/* ------------------------------------------------------------------ */

static void test_parallel(void) {
    const char *text = "The quick brown fox jumps over the lazy dog. "
                       "The quick brown fox jumps over the lazy dog. "
                       "The quick brown fox jumps over the lazy dog.";
    const char *patterns[] = {"fox", "dog", "quick", "cat", "The", NULL};
    int n = (int)strlen(text);

    for (int p = 0; patterns[p] != NULL; p++) {
        const char *pat = patterns[p];
        int m = (int)strlen(pat);
        int bc[BM_ALPHABET_SIZE];
        build_bad_character_table(pat, m, bc);
        int want = naive_search(text, n, pat, m);
        for (int threads = 1; threads <= 4; threads++) {
            int got = parallel_boyer_moore_search(text, (size_t)n, pat, m, bc, threads);
            checks++;
            if (got != want) {
                failures++;
                printf("FAIL parallel: pattern=\"%s\" threads=%d got=%d want=%d\n",
                       pat, threads, got, want);
            }
        }
    }

    CHECK(optimal_thread_count(100) == 1);          /* small text: 1 thread */
    CHECK(optimal_thread_count(2 * 1024 * 1024) >= 1);
}

/* ------------------------------------------------------------------ */
/* UTF-8 helpers                                                       */
/* ------------------------------------------------------------------ */

static void test_utf8(void) {
    /* ASCII */
    const unsigned char ascii[] = "hello";
    CHECK(utf8_char_length(ascii, 0, 5) == 1);
    CHECK(utf8_strlen(ascii, 5) == 5);
    CHECK(utf8_is_valid(ascii, 5) == 1);

    /* 2-byte sequence: é = 0xC3 0xA9 */
    const unsigned char eacute[] = {0xC3, 0xA9, 0x00};
    CHECK(utf8_char_length(eacute, 0, 2) == 2);
    CHECK(utf8_strlen(eacute, 2) == 1);
    CHECK(utf8_is_valid(eacute, 2) == 1);

    /* 3-byte sequence: € = 0xE2 0x82 0xAC */
    const unsigned char euro[] = {0xE2, 0x82, 0xAC, 0x00};
    CHECK(utf8_char_length(euro, 0, 3) == 3);
    CHECK(utf8_strlen(euro, 3) == 1);
    CHECK(utf8_is_valid(euro, 3) == 1);

    /* Invalid: lone continuation byte */
    const unsigned char bad[] = {0x80, 0x00};
    CHECK(utf8_char_length(bad, 0, 1) == 0);
    CHECK(utf8_is_valid(bad, 1) == 0);

    /* Invalid: truncated 2-byte sequence */
    const unsigned char trunc[] = {0xC3, 0x00};
    CHECK(utf8_char_length(trunc, 0, 1) == 0);
    CHECK(utf8_is_valid(trunc, 1) == 0);

    /* Substring compare at a character boundary */
    const unsigned char text[] = "a\xC3\xA9" "b"; /* a, é, b */
    const unsigned char pat[] = {0xC3, 0xA9, 0x00};
    CHECK(utf8_strcmp_at(text, 1, 4, pat, 2) == 1);
    CHECK(utf8_strcmp_at(text, 0, 4, pat, 2) == 0);
    CHECK(utf8_next(text, 0, 4) == 1);
    CHECK(utf8_next(text, 1, 4) == 3);
}

int main(void) {
    test_bad_char_table();
    test_goodsuffix_table();
    test_fixed_cases();
    test_random();
    test_parallel();
    test_utf8();

    printf("%d checks, %d failures\n", checks, failures);
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("TESTS FAILED\n");
    return 1;
}
