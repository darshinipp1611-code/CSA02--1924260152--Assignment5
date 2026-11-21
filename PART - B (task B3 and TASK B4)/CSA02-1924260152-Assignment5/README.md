# CSA02 – C Programming – Assignment 5 (CO5) – Part B: Tasks B3 and B4

| | |
|---|---|
| **Student name** | Priyadarshini Parthiban |
| **Register number** | 1924260152 |
| **Repository name** | CSA02-1924260152-Assignment5 |
| **GitHub link** | `<paste your repository URL here>` |
| **Compiler** | GCC 13.3.0 (Ubuntu 24.04), Linux x86-64, glibc 2.39 |
| **Sanitizer** | AddressSanitizer + LeakSanitizer + UBSan (Valgrind is not installed on the test machine; the assignment allows either tool) |

## Folder layout
```
B3/  b3_complete_code.c        Complete-the-Code (update_units)
     b3_1_correctness.c        B3.1  10 records, text vs binary, read-back check
     b3_2_3_benchmark.c        B3.2 + B3.3  size/time at 1k, 10k, 100k + update of 50,000th record
     b3_alt_bulk_fwrite.c      Alternative concept: one fwrite per record vs one fwrite for the array
     donor_common.h            shared Donor record + deterministic data generator
     data/  results/  screenshots/
B4/  b4_complete_code.c        Complete-the-Code (add_donor with realloc doubling)
     b4_failure_demo.c         realloc failure demo (why a temporary pointer is used)
     b4_gen.c                  test-data generator (N donors, text file)
     b4_report.c               nightly eligibility report: memory | batch | buildindex | index
     b4_common.h               date helpers + record
     run_b4_experiments.sh     generates 10k / 100k / 1M donors and runs every strategy 5x
     data/  results/  screenshots/
tools/  shot.py (output -> PNG screenshot), make_graphs.py (graphs)
REPORT.md   explanations, tables, graphs, decisions
```

## Compile and run (exact commands)
All commands are run inside the task folder.

**B3**
```
cd B3
gcc -Wall -Wextra -O2 -o b3_complete_code    b3_complete_code.c    && ./b3_complete_code
gcc -Wall -Wextra -O2 -o b3_1_correctness    b3_1_correctness.c    && ./b3_1_correctness
gcc -Wall -Wextra -O2 -o b3_2_3_benchmark    b3_2_3_benchmark.c    && ./b3_2_3_benchmark
gcc -Wall -Wextra -O2 -o b3_alt_bulk_fwrite  b3_alt_bulk_fwrite.c  && ./b3_alt_bulk_fwrite
```
**B4**
```
cd B4
gcc -Wall -Wextra -O2 -o b4_complete_code b4_complete_code.c && ./b4_complete_code
gcc -Wall -Wextra -O2 -o b4_failure_demo  b4_failure_demo.c  && (ulimit -v 200000; ./b4_failure_demo)
./run_b4_experiments.sh          # builds b4_gen + b4_report, creates data, runs 3 strategies x 5 runs
./b4_report memory data/donors_100000.txt out.csv all      # usage: b4_report <mode> <file> <out> [all|new]
./b4_report buildindex data/donors_100000.txt              # creates data/donors_100000.txt.idx
```
**Memory safety (B4.2)**
```
gcc -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -o b4_report_asan b4_report.c
ASAN_OPTIONS=detect_leaks=1 ./b4_report_asan memory data/donors_100000.txt out.csv all
```
Graphs: `python3 tools/make_graphs.py` (needs matplotlib).

The 100k / 1M donor files (up to 47 MB) are **not** in the zip; `run_b4_experiments.sh` regenerates them in a few seconds.

## Result of each task
* **B3 Complete code** – output matches the expected output exactly (`3 -> 20`, record 9 `failed`, file intact).
* **B3.1** – 10 donors written/read in text and binary: all fields identical (PASS).
* **B3.2** – binary read is ~27x faster than text at 100,000 donors; binary write ~1.9x faster; the binary file is *larger* (64 B fixed records vs ~44 B text lines).
* **B3.3** – updating the 50,000th record: 0.0047 ms in place (binary) vs 13.6 ms rewriting the text file (~2,900x).
* **B4 Complete code** – output matches expected (capacity 2,2,4,4,8).
* **B4.1/B4.2** – report works for 10k, 100k and 1,000,000 donors; ASan/LSan report no errors and no leaks.
* **B4.3** – decision: read the file in batches (≈2 MB at any size, speed comparable to loading everything).

## Suggested commits (one per task – do these in your own repository)
```
git add B3/b3_complete_code.c B3/screenshots/B3_complete_code_output.png && git commit -m "B3 complete code: update_units"
git add B3/b3_1_correctness.c B3/donor_common.h B3/screenshots/B3_1* && git commit -m "B3.1 correctness text vs binary"
git add B3/b3_2_3_benchmark.c B3/results B3/screenshots && git commit -m "B3.2/B3.3 benchmark and update timing"
git add B4/b4_complete_code.c B4/b4_failure_demo.c && git commit -m "B4 complete code: growable list"
git add B4/b4_gen.c B4/b4_common.h B4/b4_report.c && git commit -m "B4.1 nightly report, three strategies"
git add B4/run_b4_experiments.sh B4/results B4/screenshots && git commit -m "B4.2/B4.3 memory safety and production decision"
```
