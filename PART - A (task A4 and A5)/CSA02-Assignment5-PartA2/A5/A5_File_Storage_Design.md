# TASK A5 – Design the File Storage (4 Marks)

## A5.1 Text or Binary? (2 Marks)

### Donor file – text file (donors.txt), one donor per line, written with fprintf
Format (fields separated by `|` because names may contain spaces):
```
id|name|group|age|phone|dd-mm-yyyy
7001|Ravi Kumar|O+|29|9876543210|14-03-2026
7002|Anita Devi|A+|34|9123456780|02-01-2026
```
```
ALGORITHM save_donors(filename, list, n)
BEGIN
    fp ← fopen(filename, "w")
    IF fp = NULL THEN perror("donors file") ; RETURN 0
    FOR i ← 0 TO n-1 DO
        fprintf(fp, "%d|%s|%s|%d|%s|%02d-%02d-%04d\n", id, name, group, age, phone, day, month, year)
    END FOR
    fclose(fp) ; RETURN 1
END
```
Justification for text:
- Human-readable and editable with any editor (the registrar can check or correct a record; easy to debug).
- Portable between computers, compilers and operating systems (no byte order or struct padding problems).
- Records are naturally variable length (names differ), so no space is wasted on padding.
- Donor data changes rarely (registration, occasional correction) and is always processed as a whole list (find eligible donors scans everyone), so sequential reading is fine.
- Easy to import/export (CSV-like) for other hospital systems.

### Stock file – binary file (stock.dat), fixed-size records, written with fwrite
```
record Stock { int group_code ; int units }         // 8 bytes (two 4-byte ints)
File layout: 8 groups × 8 bytes = 64 bytes
offset  0: {0,units} A+   offset  8: {1,units} A-  ...  offset 56: {7,units} O-
```
```
ALGORITHM save_stock(filename, stock, n)
BEGIN
    fp ← fopen(filename, "wb")
    IF fp = NULL THEN perror("stock file") ; RETURN 0
    written ← fwrite(stock, size of Stock, n, fp)
    fclose(fp)
    RETURN (written = n)
END
```
Justification for binary:
- All records have the SAME size, so record i is at byte offset i × sizeof(Stock): direct access in O(1) using fseek.
- Stock changes after every donation and issue, so fast in-place updates matter.
- Compact (64 bytes for all groups) and fast to read/write (no number-to-text conversion).
- Not meant to be edited by hand. Limitation: not portable between computers with different int size, byte order (endianness) or padding; it must be read on the same type of system (or use fixed-width types and a documented byte order).

## A5.2 Update in Place (2 Marks)

Change the units of blood group index g in the binary stock file without rewriting the file.
```
ALGORITHM update_units_in_file(filename, g, new_units, n)
INPUT  : filename, g = record index (0..n-1), new_units, n = number of records in the file
OUTPUT : 1 on success, 0 on failure (file unchanged on failure)
BEGIN
    IF g < 0 OR g >= n OR new_units < 0 THEN RETURN 0
    fp ← fopen(filename, "r+b")                          // read AND write, binary, file must exist
    IF fp = NULL THEN perror("stock file") ; RETURN 0
    offset ← g * size of Stock
    IF fseek(fp, offset, SEEK_SET) ≠ 0 THEN fclose(fp) ; RETURN 0
    IF fread(ADDRESS_OF rec, size of Stock, 1, fp) ≠ 1 THEN fclose(fp) ; RETURN 0
    rec.units ← new_units
    IF fseek(fp, -(size of Stock), SEEK_CUR) ≠ 0 THEN fclose(fp) ; RETURN 0    // go back one record
    IF fwrite(ADDRESS_OF rec, size of Stock, 1, fp) ≠ 1 THEN fclose(fp) ; RETURN 0
    fclose(fp)                                           // flushes the data to the file
    RETURN 1
END
```
Notes: the fseek between reading and writing is required by the C standard when switching from read to write on an update-mode stream.
Only 8 bytes are touched, so the cost does not depend on how many records the file has.

**Why this is difficult for the text file:**
- Lines have variable length, so the position of line g is not known without reading and counting the newlines of all earlier lines (O(n) to find it).
- Overwriting in place is unsafe: if the new text is longer or shorter (e.g. "9" → "10", or a longer name), the following bytes are overwritten or left behind and the next lines are corrupted.
- Text files cannot insert or delete bytes in the middle; the usual solution is to copy the whole file to a temporary file with the change and then replace the original (remove/rename), which is O(n) and risks losing data if the program stops half way.

## Alternative Concept – Stock in a Text File, Rewritten After Every Update

```
ALGORITHM update_stock_text(filename, stock, n, g, new_units)
BEGIN
    stock[g].units ← new_units
    fp ← fopen("stock.tmp", "w")
    IF fp = NULL THEN perror(...) ; RETURN 0
    FOR i ← 0 TO n-1 DO
        fprintf(fp, "%d %d\n", stock[i].group_code, stock[i].units)
    END FOR
    fclose(fp)
    rename("stock.tmp", filename)                          // replace the old file
    RETURN 1
END
```
(The in-memory array is loaded once at start; every update rewrites the whole file.)

| Aspect | Binary, update in place | Text, rewrite whole file |
|--------|-------------------------|--------------------------|
| Time per update (n records) | O(1): one fseek, one fread, one fwrite of 8 bytes | O(n): n fprintf calls with number-to-text conversion, plus the rename |
| Extra space | O(1) (one record in memory) | O(n) if loaded as array, plus a temporary file the size of the data on disk |
| File size | Exactly 8 × n bytes (64 for 8 groups; 800,000 for 100,000 records) | Variable, about 5–12 bytes per record (the sum of the digits plus separator and newline), e.g. a few digits for the code and units; for 100,000 records roughly 1 MB |
| Crash safety | An interrupted write could damage one record | Temp-file + rename keeps the old file intact until the new one is complete |
| Readability / portability | Not human-readable; depends on system (size of int, byte order) | Human-readable, portable |

Conclusion: for 8 records the difference is unnoticeable, but the O(n) rewrite grows with the number of records while the in-place update stays O(1), so the binary
in-place design is better for frequently updated fixed-size stock records, and the text format remains better for the rarely updated, variable-length donor data.
(These predictions will be measured in Task B3.)
