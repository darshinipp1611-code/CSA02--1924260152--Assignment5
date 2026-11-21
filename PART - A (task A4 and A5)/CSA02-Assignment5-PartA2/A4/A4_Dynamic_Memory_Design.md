# TASK A4 – Design Dynamic Memory Use (4 Marks)

The number of donors is unknown (50 in a small hospital, 50,000 in the district), so a fixed array is either wasteful or too small.

## A4.1 Growable Donor List (2 Marks)

### Data design
```
Donor     : record { id, name[50], group[4], age, phone[16], last : Date }
DonorList : record { data : pointer to Donor array on the heap,
                     size : number of donors stored,
                     capacity : number of donors the block can hold }
Initial state: data = NULL, size = 0, capacity = 0
```
Memory picture (capacity 4, size 3):
```
list ──► data ──► [ donor0 | donor1 | donor2 | (free) ]
         size = 3, capacity = 4
```

### Pseudocode: add a donor
```
ALGORITHM add_donor(list, donor)
INPUT  : list (pointer to DonorList), donor (record to append)
OUTPUT : 1 on success, 0 if memory could not be allocated (list unchanged and still valid)
BEGIN
    IF list->size = list->capacity THEN                  // list is full
        IF list->capacity = 0 THEN
            newcap ← 2
        ELSE
            newcap ← list->capacity * 2                  // doubling
        END IF
        p ← realloc(list->data, newcap * size of Donor)   // result goes into a TEMPORARY pointer
        IF p = NULL THEN                                  // allocation failed
            PRINT error message TO stderr
            RETURN 0                                      // list->data still points to the OLD block, nothing lost
        END IF
        list->data ← p                                    // only now accept the new block
        list->capacity ← newcap
    END IF
    list->data[list->size] ← donor
    list->size ← list->size + 1
    RETURN 1
END

ALGORITHM free_list(list)
BEGIN
    free(list->data)
    list->data ← NULL ;  list->size ← 0 ;  list->capacity ← 0
END
```
Failure handling: if realloc fails it returns NULL but the OLD block is still allocated and valid. Writing `list->data ← realloc(...)` directly would overwrite the only
pointer to that block with NULL, causing a memory leak and a later crash. With the temporary pointer the caller gets 0, can still save the donors already in memory,
and then free the list normally. Callers must check the return value (e.g. load_donors stops loading and reports the error).

Growth trace (initial capacity 0): add #1 → capacity 2; #3 → 4; #5 → 8; #9 → 16 …

## A4.2 Memory Errors (2 Marks)

| Error | Meaning | How it could occur in the blood bank | How the design prevents it |
|-------|---------|--------------------------------------|----------------------------|
| **Memory leak** | Heap memory is allocated but never freed and the program loses the pointer to it | (1) `list->data = realloc(list->data, n)` fails and overwrites the only pointer; (2) `load_donors` returns early on a file error without calling `free_list`; (3) program ends without freeing the list | Temporary pointer for realloc; one cleanup path (`free_list`) called on every exit path; verify with valgrind: "All heap blocks were freed" |
| **Dangling pointer** | A pointer that still holds the address of memory that has been freed or moved | (1) `Donor *d = &list.data[2];` then `add_donor` triggers realloc, the block moves, and `d` now points to freed memory; (2) a function returns the address of a local `Donor` | Do not keep pointers into the list across calls to add_donor – keep an INDEX instead (A3 alternative) or re-fetch the pointer after each add; set `list->data = NULL` after free; never return addresses of locals |
| **Double free** | `free()` called twice on the same block (undefined behaviour, heap corruption, crash) | (1) `free_list(&list)` called in an error handler and again at the end of main; (2) two copies of the DonorList struct both calling free on the same `data` | `free_list` sets `data = NULL` afterwards (free(NULL) is harmless); only ONE owner of the block (main owns the list; other functions get pointers but never free); never copy the DonorList by value and free both |

## Alternative Concept – Fixed Growth of 10 Records

```
ALGORITHM add_donor_fixed(list, donor)
BEGIN
    IF list->size = list->capacity THEN
        newcap ← list->capacity + 10
        p ← realloc(list->data, newcap * size of Donor)
        IF p = NULL THEN RETURN 0
        list->data ← p ;  list->capacity ← newcap
    END IF
    list->data[list->size] ← donor ;  list->size ← list->size + 1
    RETURN 1
END
```
(Only the line `newcap` changes; the temporary-pointer rule is kept.)

### Elements copied by realloc (worst case: the block moves every time)

- **Doubling:** growth happens when size = 2, 4, 8, …, 2^k, copying 2 + 4 + 8 + … + 2^k < 2n elements ⇒ **O(n) total**, i.e. O(1) amortised per add.
- **Fixed +10:** growth at size 10, 20, …, 10m with m ≈ n/10, copying 10 + 20 + … + 10m = 10·m(m+1)/2 ≈ n²/20 ⇒ **O(n²) total**.

| n donors | Doubling: elements copied | Fixed +10: elements copied |
|----------|---------------------------|-----------------------------|
| 50 | 62 (2+4+8+16+32) | 100 (10+20+30+40) |
| 50,000 | 65,534 | 124,975,000 |

For 50,000 donors the fixed strategy copies about 1,900 times more elements (and makes 4,999 realloc calls instead of 15).

### Unused space

- Doubling: capacity is less than 2n, so up to almost n slots (about 50% of the block) can be unused just after a growth.
- Fixed +10: at most 9 unused slots (O(1)) at any time.

### Conclusion
Doubling trades extra memory (up to 2×) for O(n) total copying and far fewer realloc calls; fixed growth wastes little memory but becomes quadratic.
For up to 50,000 donors (each about 100 bytes, at most ~10 MB of slack) the memory cost is acceptable, so **doubling is recommended**. A common refinement is to `realloc` down to size (shrink-to-fit) after loading is finished.
