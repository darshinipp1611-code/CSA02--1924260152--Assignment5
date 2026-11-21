# TASK A2 – Design with Recursion (5 Marks)

## A2.1 Recursive Design (2 Marks)

### (a) Digital root (check digit)

Helper: sum of the digits of n.
```
ALGORITHM digit_sum(n)
INPUT  : n >= 0
OUTPUT : sum of decimal digits of n
BEGIN
    IF n = 0 THEN RETURN 0                          // base case
    RETURN (n MOD 10) + digit_sum(n DIV 10)         // recursive case
END

ALGORITHM digital_root(n)
INPUT  : n >= 0
OUTPUT : single digit obtained by repeatedly summing digits
BEGIN
    IF n < 10 THEN RETURN n                         // base case: already one digit
    RETURN digital_root(digit_sum(n))               // recursive case: smaller number
END
```
- Base case: `n < 10` (one digit remains). Recursive case: `digital_root(digit_sum(n))`. digit_sum(n) < n for n ≥ 10, so the argument always shrinks and the base case is reached.
- The check digit of a donor ID is `digital_root(ID without its check digit)`.

### (b) Power by halving: base^n = (base^(n/2))²

Because base^n = base^(n/2) × base^(n/2) when n is even, and base × (base^(n/2))² when n is odd (integer division).
```
ALGORITHM power_fast(base, n)
INPUT  : base, n >= 0
OUTPUT : base^n
BEGIN
    IF n = 0 THEN RETURN 1                          // base case
    half ← power_fast(base, n DIV 2)                // recursive case: ONE call, stored
    IF n MOD 2 = 0 THEN
        RETURN half * half
    ELSE
        RETURN half * half * base
    END IF
END
```
For comparison, linear version:
```
ALGORITHM power_linear(base, n)
BEGIN
    IF n = 0 THEN RETURN 1                          // base case
    RETURN base * power_linear(base, n - 1)         // recursive case
END
```
| Function | Base case | Recursive case |
|----------|-----------|----------------|
| digit_sum | n = 0 → 0 | n mod 10 + digit_sum(n div 10) |
| digital_root | n < 10 → n | digital_root(digit_sum(n)) |
| power_linear | n = 0 → 1 | base × power_linear(base, n−1) |
| power_fast | n = 0 → 1 | half = power_fast(base, n div 2); square (× base if n odd) |

## A2.2 Trace the Calls (2 Marks)

### Trace 1 – digital_root(98765)

```
digital_root(98765)  → 98765 ≥ 10 → digit_sum = 9+8+7+6+5 = 35
 └ digital_root(35)  → 35 ≥ 10    → digit_sum = 3+5 = 8
    └ digital_root(8) → 8 < 10 → return 8
returns 8 → 8 → 8        Result = 8   (3 calls)
```
Call stack at its deepest point (top = most recent):
```
| digital_root(8)      |  ← top (base case, about to return 8)
| digital_root(35)     |  waiting for result
| digital_root(98765)  |  waiting for result
| main                 |
```
Depth = 3 (digit_sum(98765) adds its own 6 frames temporarily: digit_sum(98765)…digit_sum(0), depth 6 while computing, then they are popped).

### Trace 2 – power_fast(2, 10)

```
power_fast(2,10) → half = power_fast(2,5)
  power_fast(2,5) → half = power_fast(2,2)
    power_fast(2,2) → half = power_fast(2,1)
      power_fast(2,1) → half = power_fast(2,0)
        power_fast(2,0) → returns 1                    (base case)
      returns 1*1*2 = 2                                (n=1 odd)
    returns 2*2 = 4                                    (n=2 even)
  returns 4*4*2 = 32                                   (n=5 odd)
returns 32*32 = 1024                                   (n=10 even)
```
Call stack at the deepest point:
```
| power_fast(2,0)  |  ← top, returns 1
| power_fast(2,1)  |
| power_fast(2,2)  |
| power_fast(2,5)  |
| power_fast(2,10) |
| main             |
```
5 calls, depth 5 (n = 10, 5, 2, 1, 0).

For comparison power_linear(2,5) makes 6 calls (n = 5,4,3,2,1,0) and has stack depth 6.

### Recurrences for the number of calls T(n)

- Linear: T(0) = 1, T(n) = 1 + T(n−1)  ⇒ **T(n) = n + 1**, O(n).
- Halving: T(0) = 1, T(n) = 1 + T(⌊n/2⌋)  ⇒ **T(n) = ⌊log₂ n⌋ + 2** for n ≥ 1, O(log n).
- Check: n = 10 → 3 + 2 = 5 ✓ (matches trace); n = 30 → 4 + 2 = 6 ✓.
- Bad variant (two recursive calls instead of storing `half`): T(n) = 2T(⌊n/2⌋) + 1 ⇒ about 2n − 1 calls, O(n); the halving idea is wasted.

Predicted counts (to compare with measurements in B1):

| n | Linear (n+1) | Halving (⌊log₂n⌋+2) |
|---|--------------|----------------------|
| 10 | 11 | 5 |
| 100 | 101 | 8 |
| 1,000 | 1,001 | 11 |
| 10,000 | 10,001 | 15 |

## A2.3 Recursion Risks (1 Mark)

- **Missing base case:** the function calls itself forever (e.g. power_linear never reaches n = 0). Each call adds a stack frame, so the stack fills up and the program crashes with a **stack overflow** (segmentation fault). The same happens if the recursive step does not move towards the base case (e.g. n+1 instead of n−1), or if negative n is passed to power_linear.
- **Replace recursion with a loop when:** depth can be large or is unbounded (power_linear with n = 100000, digit_sum of long numbers); the function is tail-recursive/simple repetition so a loop is equally clear; the program runs on small-stack systems; or speed matters (no call overhead). Keep recursion when it matches the problem naturally (trees, divide-and-conquer) and the depth is small (log n).

## Alternative Concept – Loop or Direct Formula for the Digital Root

### Loop version
```
ALGORITHM digital_root_loop(n)
BEGIN
    WHILE n >= 10 DO
        s ← 0
        WHILE n > 0 DO
            s ← s + (n MOD 10)
            n ← n DIV 10
        END WHILE
        n ← s
    END WHILE
    RETURN n
END
```
### Formula version
```
ALGORITHM digital_root_formula(n)
BEGIN
    IF n = 0 THEN RETURN 0
    RETURN 1 + ((n - 1) MOD 9)
END
```
Check: n = 98765 → 98764 mod 9 = 7 (since 98765 mod 9 = 8) → 1 + 7 = 8 ✓ matches the trace.

### Analysis

| Version | Time | Extra stack/space |
|---------|------|--------------------|
| Recursive (with recursive digit_sum) | O(d), d = number of digits (first pass d digits; later passes work on a number of at most about 9d, so they are much shorter) | Stack O(d) because digit_sum recurses once per digit; digital_root alone only nests a few levels |
| Loop | O(d) | O(1) |
| Formula 1 + (n−1) % 9 | O(1) for fixed-width integers (one division) | O(1) |

The formula works because a number is congruent to its digit sum modulo 9. The loop and formula give identical results to the recursion for n ≥ 0.
Recommendation: use the formula in production (fastest, no stack); keep the recursive version for teaching/validating the formula; use the loop if the formula is not allowed.

Similarly for power: an iterative square-and-multiply loop is O(log n) time and O(1) stack, versus O(log n) stack for power_fast and O(n) stack for power_linear.
