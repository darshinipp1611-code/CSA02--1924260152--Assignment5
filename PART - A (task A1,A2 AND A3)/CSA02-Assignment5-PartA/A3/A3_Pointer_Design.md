# TASK A3 – Design with Pointers (5 Marks)

## A3.1 Pointer Operations (2 Marks)

Declaration: `int stock[8];  int *p = stock;`  (p points to stock[0])
Index order: 0=A+, 1=A-, 2=B+, 3=B-, 4=AB+, 5=AB-, 6=O+, 7=O-.
Example values and assumed addresses (sizeof(int) = 4 bytes, array at 0x1000):

```
address :  0x1000  0x1004  0x1008  0x100C  0x1010  0x1014  0x1018  0x101C
index   :  [0]     [1]     [2]     [3]     [4]     [5]     [6]     [7]
group   :  A+      A-      B+      B-      AB+     AB-     O+      O-
value   :  40      12      25      8       15      5       60      20
            ▲                      ▲
            p (= stock)            p+3
```

Each expression is evaluated separately starting from `p = stock`:

| Expression | Meaning | Result in diagram |
|------------|---------|-------------------|
| `p + 3` | Pointer arithmetic: address of the element 3 positions after p, i.e. `&stock[3]`. Adds 3 × sizeof(int) = 12 bytes. p itself does not change. | 0x100C |
| `*(p + 3)` | Dereference: the VALUE stored at that address = `stock[3]` | 8 (B- units) |
| `p[3]` | Subscript is defined as `*(p + 3)`; identical to the previous expression | 8 |
| `*p++` | Postfix `++` has higher precedence than `*`, so it is `*(p++)`: use the current p to read `stock[0]` (value 40), THEN advance p by one element to `&stock[1]` (0x1004). The array contents are not changed (to increment the value use `(*p)++`). | value 40; p becomes 0x1004 |

Note: `p+3` is a pointer (address); `*(p+3)` and `p[3]` are int values.

## A3.2 Pointers and Functions (2 Marks)

Function returning a pointer to the element with the lowest stock:

```
ALGORITHM lowest_stock(stock, n)
INPUT  : stock (pointer to first element of caller's array of Stock), n = number of elements
OUTPUT : pointer to the element with the smallest units, or NULL if stock is NULL or n <= 0
BEGIN
    IF stock = NULL OR n <= 0 THEN RETURN NULL
    low ← stock                                  // pointer to first element
    FOR i ← 1 TO n-1 DO
        IF (stock + i)->units < low->units THEN
            low ← stock + i
        END IF
    END FOR
    RETURN low                                   // address inside the caller's array
END
```
Caller:
```
q ← lowest_stock(stock, 8)
IF q ≠ NULL THEN
    q->units ← q->units + 10                      // updates the real array element directly
END IF
```
Prototype: `Stock *lowest_stock(Stock *stock, int n)`. Time O(n), space O(1).

**Why returning a pointer to a local variable is wrong:** a local variable lives in the function's stack frame, which is destroyed when the function returns.
The returned address then points to memory that may be reused by the next call, so using it is a **dangling pointer** (undefined behaviour: garbage values, overwriting
other data, or crash). A pointer returned from lowest_stock is safe because it points into the caller's array, which still exists after the function returns
(a static variable or heap block would also be safe).

## A3.3 Command-Line Interface (1 Mark)

Usage:
```
bloodbank find <group>            list eligible donors of the group       e.g. bloodbank find O+
bloodbank issue <group> <units>   issue units from stock                   e.g. bloodbank issue A+ 2
bloodbank stock                   show the stock of all groups
bloodbank lowest                  show the group with the lowest stock
```
(`argc` includes the program name: find → argc = 3, issue → argc = 4, stock / lowest → argc = 2.)

```
ALGORITHM main(argc, argv)
BEGIN
    IF argc < 2 THEN
        PRINT usage TO stderr ; RETURN 1                         // no arguments
    END IF
    cmd ← argv[1]
    IF cmd = "find" THEN
        IF argc ≠ 3 THEN PRINT "find needs exactly one group" + usage TO stderr ; RETURN 1
        g ← group_to_index(argv[2])
        IF g = -1 THEN PRINT "invalid blood group: " + argv[2] TO stderr ; RETURN 2
        run find for argv[2]
    ELSE IF cmd = "issue" THEN
        IF argc ≠ 4 THEN PRINT usage TO stderr ; RETURN 1
        g ← group_to_index(argv[2])
        IF g = -1 THEN PRINT "invalid blood group" TO stderr ; RETURN 2
        units ← convert argv[3] to integer using strtol with end-pointer check
        IF conversion failed OR units <= 0 THEN PRINT "units must be a positive number" TO stderr ; RETURN 3
        run issue
    ELSE IF cmd = "stock" OR cmd = "lowest" THEN
        IF argc ≠ 2 THEN PRINT "too many arguments" + usage TO stderr ; RETURN 1
        run command
    ELSE
        PRINT "unknown command: " + cmd + usage TO stderr ; RETURN 1
    END IF
    RETURN 0
END
```
Invalid-argument handling: (1) check `argc` BEFORE touching `argv[i]` (avoids reading past the array); (2) validate the blood group against the table of 8 valid names (A+, A-, B+, B-, AB+, AB-, O+, O-);
(3) convert numbers with strtol and check the end pointer and range (atoi cannot detect errors); (4) print a clear message and the usage line to stderr;
(5) return a non-zero exit status (1 = wrong usage, 2 = bad group, 3 = bad number) so scripts can detect the failure; no data file is modified when the arguments are invalid.

## Alternative Concept – Return the Index Instead of a Pointer

```
ALGORITHM lowest_stock_index(stock, n)
BEGIN
    IF stock = NULL OR n <= 0 THEN RETURN -1
    low ← 0
    FOR i ← 1 TO n-1 DO
        IF stock[i].units < stock[low].units THEN low ← i
    RETURN low
END
```
Calling code:
```
i ← lowest_stock_index(stock, 8)
IF i ≠ -1 THEN
    stock[i].units ← stock[i].units + 10          // caller indexes the array itself
END IF
```

| Aspect | Returns pointer | Returns index |
|--------|-----------------|---------------|
| Time | O(n) | O(n) |
| Space | O(1) | O(1) |
| Calling code | `q->units += 10` (direct) | `stock[i].units += 10` (needs array name and index) |
| Error value | NULL | -1 (must be checked) |
| Safety when the array is later **reallocated** | **Unsafe:** realloc may move the block, so the saved pointer becomes dangling | **Safe:** an index stays valid because it is relative to whatever the array's current address is |
| Safety for stored/long-lived references | Must be refreshed after every possible realloc | Can be stored |

Recommendation: return a pointer for short-lived, immediate use on a fixed-size array (8 groups); return an index when the array may grow (such as the donor list in A4) or the result must be kept.
