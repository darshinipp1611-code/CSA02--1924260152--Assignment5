/* B2 - Alternative concept: getline() allocates a buffer of the size needed.
 * The caller owns the buffer and must free() it.  The buffer is reused for
 * every line and grows (realloc) only when a longer line arrives.
 * Build: gcc -std=c11 -Wall -Wextra -O0 -g -o b2_getline_alt b2_getline_alt.c
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(void)
{
    char   *line = NULL;          /* getline allocates / grows this */
    size_t  cap  = 0;
    ssize_t len;
    int     n = 0;

    while (1) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        len = getline(&line, &cap, stdin);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (len < 0) break;                       /* EOF or error */
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        double us = (t1.tv_sec - t0.tv_sec) * 1e6 + (t1.tv_nsec - t0.tv_nsec) / 1e3;
        printf("line %d: length = %zd chars, buffer capacity now = %zu bytes, "
               "getline took %.1f us\n", ++n, len, cap, us);
        if (len <= 20) printf("        text   = \"%s\"\n", line);
    }
    free(line);                   /* the CALLER must free the buffer, even after EOF */
    line = NULL;
    return 0;
}
