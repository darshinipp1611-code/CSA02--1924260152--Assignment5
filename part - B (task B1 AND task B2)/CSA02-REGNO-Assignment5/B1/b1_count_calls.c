/* B1.3 - Count the recursive calls of linear and halving power.
 * Unsigned arithmetic is used so the huge powers wrap around (well defined)
 * instead of overflowing signed long long; the call counts do not depend
 * on the value computed.
 * Build: gcc -std=c11 -Wall -Wextra -O0 -o b1_count_calls b1_count_calls.c
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>

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

static u64 power_fast_twocalls(u64 base, int exp)   /* the "half*half -> two calls" variant */
{
    calls++;
    if (exp == 0) return 1;
    u64 r = power_fast_twocalls(base, exp / 2) * power_fast_twocalls(base, exp / 2);
    return (exp % 2 == 0) ? r : r * base;
}

/* closed forms obtained from the recurrences
 *   linear : T(0)=1, T(n)=1+T(n-1)            -> n+1
 *   halving: T(0)=1, T(n)=1+T(n/2)            -> floor(log2 n)+2   (n>=1)
 *   2 calls: T(0)=1, T(n)=1+2T(n/2)           -> 2^(floor(log2 n)+2) - 1 ... checked below
 */
static long long t_linear(int n) { return n + 1; }
static long long t_fast(int n)   { long long k = 0; for (int m = n; m > 0; m /= 2) k++; return k + 1; }
static long long t_two(int n)    { return n == 0 ? 1 : 1 + 2 * t_two(n / 2); }

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main(void)
{
    const int ns[] = {10, 100, 1000, 10000};
    const int reps = 20000;
    volatile u64 sink = 0;

    printf("%-7s | %-8s %-8s %-3s | %-5s %-5s %-3s | %-8s %-8s | %-10s %-10s\n",
           "n", "linear", "T=n+1", "ok?", "fast", "T=lg", "ok?", "2-calls", "T_two(n)",
           "lin ns", "fast ns");
    printf("--------+----------------------------+-----------------------+-------------------+----------------------\n");
    for (int i = 0; i < 4; i++) {
        int n = ns[i];
        calls = 0; sink = power_linear(3, n);          long long cl = calls;
        calls = 0; sink = power_fast(3, n);            long long cf = calls;
        calls = 0; sink = power_fast_twocalls(3, n);   long long c2 = calls;

        double t0 = now_ns();
        for (int r = 0; r < reps; r++) sink = power_linear(3, n);
        double tl = (now_ns() - t0) / reps;
        t0 = now_ns();
        for (int r = 0; r < reps; r++) sink = power_fast(3, n);
        double tf = (now_ns() - t0) / reps;

        printf("%-7d | %-8lld %-8lld %-3s | %-5lld %-5lld %-3s | %-8lld %-8lld | %-10.1f %-10.1f\n",
               n, cl, t_linear(n), cl == t_linear(n) ? "yes" : "NO",
               cf, t_fast(n), cf == t_fast(n) ? "yes" : "NO",
               c2, t_two(n), tl, tf);
    }
    (void)sink;
    return 0;
}
