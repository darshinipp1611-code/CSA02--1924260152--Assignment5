/* B1.4 - Find the limit.
 * Usage: ./b1_limit <mode> <n>
 *   mode = linear | fast | iter | iterfast
 * Runs base^n once (base 1, unsigned, so there is no arithmetic overflow)
 * and prints the call/iteration count.  A crash shows up as a non-zero exit
 * status (139 = SIGSEGV) which find_limit.sh uses for bisection.
 * Build (no optimisation, so recursion is really recursion):
 *   gcc -std=c11 -Wall -Wextra -O0 -o b1_limit b1_limit.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef unsigned long long u64;
static long long calls = 0;

static u64 power_linear(u64 base, int exp)
{
    calls++;
    if (exp == 0) return 1;
    return base * power_linear(base, exp - 1);
}

static u64 power_fast(u64 base, int exp)
{
    calls++;
    if (exp == 0) return 1;
    u64 half = power_fast(base, exp / 2);
    return (exp % 2 == 0) ? half * half : half * half * base;
}

static u64 power_iter(u64 base, int exp)            /* plain loop, n iterations */
{
    u64 r = 1;
    for (int i = 0; i < exp; i++) { r *= base; calls++; }
    return r;
}

static u64 power_iterfast(u64 base, int exp)        /* square-and-multiply, log n iterations */
{
    u64 r = 1;
    while (exp > 0) {
        if (exp & 1) r *= base;
        base *= base;
        exp >>= 1;
        calls++;
    }
    return r;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s linear|fast|iter|iterfast <n>\n", argv[0]);
        return 2;
    }
    char *end;
    errno = 0;
    long n = strtol(argv[2], &end, 10);
    if (errno || *end != '\0' || n < 0 || n > 2147483647L) {
        fprintf(stderr, "%s: n must be an integer in 0..2147483647\n", argv[0]);
        return 2;
    }
    u64 r;
    if      (strcmp(argv[1], "linear")   == 0) r = power_linear(1, (int)n);
    else if (strcmp(argv[1], "fast")     == 0) r = power_fast(1, (int)n);
    else if (strcmp(argv[1], "iter")     == 0) r = power_iter(1, (int)n);
    else if (strcmp(argv[1], "iterfast") == 0) r = power_iterfast(1, (int)n);
    else { fprintf(stderr, "unknown mode '%s'\n", argv[1]); return 2; }
    printf("%-8s n=%-11ld result=%llu  calls/iterations=%lld  -> survived\n",
           argv[1], n, r, calls);
    return 0;
}
