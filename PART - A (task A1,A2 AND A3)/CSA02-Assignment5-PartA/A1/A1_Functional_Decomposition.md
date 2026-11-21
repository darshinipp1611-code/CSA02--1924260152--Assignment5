# TASK A1 – Decompose the Program into Functions (7 Marks)

## A1.1 Identify the Functions (2 Marks)

| # | Function | Responsibility |
|---|----------|----------------|
| 1 | main | Entry point: parse arguments, load files, dispatch command, save, clean up |
| 2 | parse_command | Validate argc/argv, identify the command (find / issue / stock / lowest / register) |
| 3 | load_donors | Read donor text file into the donor array |
| 4 | save_donors | Write donor array to the donor text file |
| 5 | load_stock | Read binary stock file into the stock array |
| 6 | save_stock | Write stock array to the binary stock file |
| 7 | register_donor | Read and validate one donor, append to the donor list |
| 8 | validate_donor | Check id, age range, group, phone format |
| 9 | group_to_index | Convert "O+" to index 6 (-1 if invalid) |
| 10 | check_eligibility | Decide if ONE donor may donate on a given date |
| 11 | days_between | Number of days between two dates |
| 12 | find_eligible_donors | Scan list: count eligible donors of a group and find the youngest |
| 13 | update_stock | Add units after a donation / subtract units after an issue |
| 14 | issue_blood | Check availability, call update_stock, report shortage |
| 15 | lowest_stock | Return pointer to element with the least units (see A3) |
| 16 | print_donor / print_stock | Display records |

### Structure chart (caller → callee)

```
                                   main
     ┌────────────┬───────────────┬────┴──────────┬────────────────┬──────────────┐
parse_command  load_donors    load_stock      (command handler)  save_donors   save_stock
     │                                            │
group_to_index        ┌───────────────┬───────────┼──────────────┬──────────────┐
                register_donor  find_eligible_donors  issue_blood  lowest_stock  print_stock
                      │                │                  │
               validate_donor    check_eligibility    update_stock
                      │                │                  │
               group_to_index     days_between      group_to_index
```
(The growable list function add_donor, which calls realloc, is designed in Task A4 and is called by register_donor and load_donors.)

Data flow: arguments → parse_command → handler. The donor and stock arrays are owned by main
and passed DOWN to the functions by pointer (nothing global).

Design principles: one job per function (high cohesion), no shared global state (low coupling),
I/O (load/save/print) kept separate from logic (check_eligibility, update_stock) so the logic can be tested alone.

## A1.2 Design the Interfaces (3 Marks)

Types:
```
Date            : record { day, month, year }
Donor           : record { id, name[50], group[4], age, phone[16], last : Date }
Stock           : record { group_code, units }
EligibleSummary : record { count, youngest_id }      (used in A1.3)
```

Prototypes (signatures only, the algorithms are in pseudocode):

| Prototype | Parameter decisions and justification |
|-----------|----------------------------------------|
| `int parse_command(int argc, char *argv[], char *cmd, char *grp, int *units)` | `argc` by value (small int). `argv` is already an array of pointers. `cmd`, `grp` are output buffers → pointer, non-const since written. `units` is an output → pointer. Returns 0 = ok / error code. |
| `int group_to_index(const char *group)` | Only reads the string. Arrays cannot be passed by value, so pointer; `const` so it cannot modify and string literals can be passed. Index returned by value. |
| `int load_donors(const char *filename, Donor *list, int max)` | filename read-only → `const char *`. `list` is written → non-const pointer (avoids copying a big array). `max` by value prevents buffer overflow. Returns count loaded or -1 on failure. |
| `int save_donors(const char *filename, const Donor *list, int n)` | Only reads donors → `const Donor *`; n by value. |
| `int load_stock(const char *filename, Stock *stock, int n)` | Writes into the caller's array → non-const pointer. |
| `int save_stock(const char *filename, const Stock *stock, int n)` | Read-only data → const. |
| `int register_donor(Donor *list, int *n, int max)` | Adds to the array → `list` pointer. The count must be CHANGED, so `n` is by pointer (by value would lose the update). |
| `int validate_donor(const Donor *d)` | Donor is large (~100 bytes): pointer avoids a copy; `const` because validation must not modify. |
| `int check_eligibility(const Donor *d, Date today)` | `d` by const pointer (large, read-only). `Date` is 12 bytes: by value is cheap and gives the function its own copy, keeping it pure and testable. |
| `int days_between(Date a, Date b)` | Two tiny read-only structs, by value, no side effects. |
| `int find_eligible_donors(const Donor *list, int n, const char *group, Date today, int *youngest_id)` | list/group read-only → const pointers; n, today by value; `youngest_id` is a second OUTPUT → pointer. Returns the count. |
| `EligibleSummary summarize_eligible(const Donor *list, int n, const char *group, Date today)` | Alternative 2 of A1.3: all inputs read-only; results returned in a struct. |
| `int update_stock(Stock *stock, int n, const char *group, int delta)` | Stock modified → non-const pointer; group read-only; delta by value (+ donation, − issue). Returns 1/0. |
| `int issue_blood(Stock *stock, int n, const char *group, int units)` | Modifies stock → pointer; the rest by value / const. |
| `Stock *lowest_stock(Stock *stock, int n)` | Returns a pointer INTO the caller's array so the caller can update the element directly; non-const because the returned pointer is used to modify. |
| `void print_stock(const Stock *stock, int n)` | Display only → const. |

Rules applied: (1) by value for small read-only data; (2) by pointer when the function must modify the caller's variable or the object is an
array / large struct; (3) `const` on every pointer the function only reads, so the compiler enforces the promise.

## A1.3 Return More Than One Value (2 Marks)

Required results: the number of eligible donors AND the ID of the youngest eligible donor.

### Alternative 1 – Pointer (output) parameter

```
ALGORITHM find_eligible_donors(list, n, group, today, youngest_id)
INPUT  : list (read-only array of donors), n, group, today
OUTPUT : returns count of eligible donors; *youngest_id = ID of the youngest eligible donor (-1 if none)
BEGIN
    count ← 0
    youngest_age ← 999
    *youngest_id ← -1
    FOR i ← 0 TO n-1 DO
        IF list[i].group = group AND check_eligibility(list[i], today) THEN
            count ← count + 1
            IF list[i].age < youngest_age THEN
                youngest_age ← list[i].age
                *youngest_id ← list[i].id
            END IF
        END IF
    END FOR
    RETURN count
END
```
Call: `c ← find_eligible_donors(list, n, "O+", today, ADDRESS_OF yid)`

### Alternative 2 – Return a structure

```
ALGORITHM summarize_eligible(list, n, group, today)
OUTPUT : EligibleSummary {count, youngest_id}
BEGIN
    result.count ← 0 ;  result.youngest_id ← -1 ;  youngest_age ← 999
    FOR i ← 0 TO n-1 DO
        IF list[i].group = group AND check_eligibility(list[i], today) THEN
            result.count ← result.count + 1
            IF list[i].age < youngest_age THEN
                youngest_age ← list[i].age
                result.youngest_id ← list[i].id
            END IF
        END IF
    END FOR
    RETURN result
END
```
Call: `s ← summarize_eligible(list, n, "O+", today)` then use `s.count`, `s.youngest_id`.

### Comparison

| Criterion | Pointer parameter | Return structure |
|-----------|-------------------|------------------|
| Readability at call site | Output hidden among inputs; reader must know `yid` is written | Both results named in the return type |
| Safety | Caller may pass NULL / uninitialised pointer → must be checked | No pointers to misuse |
| Efficiency | No copy of the results | 8-byte struct copy (negligible) |
| Scalability | Every extra result adds a parameter | Add a field to the struct |
| Testing | Needs variables to receive outputs | One returned value to compare/print |
| Return value use | Return value can still be a status code | Status needs an extra field |

Both are O(n) time and O(1) extra space.
**Recommendation:** return the struct (only two small values; clearer and safer). Use pointer parameters when results are large or the return value must be a status code.

## Alternative Concept – Global Variables Instead of Parameters

Solution: declare the donor array, `donor_count`, the stock array, `eligible_count` and `youngest_id` outside all functions.
`find_eligible_donors(group, today)` then reads the donor array directly and writes `eligible_count` and `youngest_id` directly.
The multiple-return problem disappears because the results are also globals.

Complexity: time stays O(n) (same loop); memory stays O(n) for the data. Only differences: no parameters are pushed (negligible gain),
the globals live for the whole run, and a fixed-size global array cannot grow (A4) unless a global pointer is used.

| Aspect | Parameters / pointers (chosen) | Globals |
|--------|--------------------------------|---------|
| Coupling | Low: dependencies visible in the prototype | High: hidden dependency on shared variables; any function can change them |
| Testing | Pass a small custom array; tests are independent | Global state must be set up and reset before every test; test order can matter |
| Reuse | Same function works on any list (hospital 1, hospital 2) | Tied to one list; cannot be reused for another dataset |
| Debugging | Changes happen only through known pointers | Any function can corrupt the data; hard to trace |
| Recursion / threads | Safe | Unsafe: shared mutable state |

Conclusion: globals change neither time nor memory significantly, but they damage modularity, so the parameter-passing design is preferred.
