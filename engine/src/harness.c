/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * harness.c
 * GCC-compiled harness for the CompCert-lowered Boyer-Moore core.
 *
 * NOT part of the verified fragment: file I/O (read_file,
 * read_all), the CLI main(), and the system calls they use are
 * compiled with GCC and link against the CompCert core. The
 * function bodies below are verbatim copies from bin/bm.c in
 * SNAPKITTYWEST/boyer-moore-search, with only the BM_NO_MAIN
 * guards removed (this file always provides main()).
 *
 * The harness is deliberately thin: all searching goes through
 * boyer_moore_search() in the CompCert core, so the observable
 * behaviour of the binary is the behaviour of verified code
 * plus plain byte transport.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bm_core.h"

/* Initial read buffer size (64 KiB); grows as needed for large files */
#define BM_BUFFER_SIZE 65536


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
