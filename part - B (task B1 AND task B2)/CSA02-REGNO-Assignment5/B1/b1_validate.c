/* B1.1 - Validate the implementation
 * Digital root (recursive), power_linear, power_fast (halving) and an
 * iterative square-and-multiply power, tested against known answers.
 * Build: gcc -std=c11 -Wall -Wextra -O0 -o b1_validate b1_validate.c
 */
#include <stdio.h>

long long calls = 0;

/* ---------- digital root ---------- */
static int digit_sum(long long n)          /* sum of decimal digits, n >= 0 */
{
    int s = 0;
    while (n > 0) { s += (int)(n % 10); n /= 10; }
    return s;
}

/* Recursive: base case = a single digit; recursive case = root of digit sum. */
int digital_root(long long n)
{
    calls++;
    if (n < 10) return (int)n;             /* includes 0 */
    return digital_root(digit_sum(n));
}

/* closed form used only as an independent oracle for the tests */
static int digital_root_formula(long long n) { return n == 0 ? 0 : (int)(1 + (n - 1) % 9); }

/* ---------- powers ---------- */
long long power_linear(long long base, int exp)
{
    calls++;
    if (exp == 0) return 1;
    return base * power_linear(base, exp - 1);
}

long long power_fast(long long base, int exp)
{
    calls++;
    if (exp == 0) return 1;
    long long half = power_fast(base, exp / 2);
    if (exp % 2 == 0) return half * half;
    return half * half * base;
}

long long power_iter(long long base, int exp)   /* square-and-multiply loop */
{
    long long result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

static int failures = 0;

static void check_root(const char *what, long long n, int expected)
{
    calls = 0;
    int got = digital_root(n);
    int ok = (got == expected) && (got == digital_root_formula(n));
    printf("%-34s digital_root(%lld) = %d (expected %d, %lld calls)  %s\n",
           what, n, got, expected, calls, ok ? "PASS" : "FAIL");
    if (!ok) failures++;
}

static void check_pow(const char *what, long long b, int e, long long expected)
{
    long long l = power_linear(b, e), f = power_fast(b, e), i = power_iter(b, e);
    int ok = (l == expected) && (f == expected) && (i == expected);
    printf("%-34s %lld^%d = %lld | lin %lld fast %lld iter %lld  %s\n",
           what, b, e, expected, l, f, i, ok ? "PASS" : "FAIL");
    if (!ok) failures++;
}

int main(void)
{
    printf("=== Digital root tests ===\n");
    check_root("zero (base case, smallest input)", 0, 0);
    check_root("one (smallest positive)",          1, 1);
    check_root("9 (largest single digit)",         9, 9);
    check_root("10 (first multi-digit)",          10, 1);
    check_root("ordinary value",                  38, 2);   /* 3+8=11 -> 1+1=2 */
    check_root("multiple of 9",                99999, 9);
    check_root("long value, 3 levels",     987654321LL, 9);
    check_root("my own value (98765432)",   98765432LL, 8); /* 44 -> 8 */
    check_root("largest int",             2147483647LL, 1); /* 46 -> 10 -> 1 */

    printf("\n=== Power tests (linear, halving, iterative must all agree) ===\n");
    check_pow("exponent 0 (base case)",        2,  0, 1);
    check_pow("exponent 1",                    2,  1, 2);
    check_pow("ordinary even exponent",        2, 10, 1024);
    check_pow("ordinary odd exponent",         3,  5, 243);
    check_pow("0^0 convention",                0,  0, 1);
    check_pow("0^n",                           0,  5, 0);
    check_pow("1^large",                       1, 1000, 1);
    check_pow("(-2)^3 negative base",         -2,  3, -8);
    check_pow("my own value 7^13",             7, 13, 96889010407LL);
    check_pow("near the long long limit",      2, 62, 4611686018427387904LL);

    printf("\nSummary: %s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
