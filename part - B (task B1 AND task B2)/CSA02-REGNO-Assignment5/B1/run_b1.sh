#!/bin/bash
# Compiles and runs every Task B1 program and stores the verbatim transcripts
# in evidence/*.txt (these are what the PNG screenshots are drawn from).
#   cd B1 && ./run_b1.sh
cd "$(dirname "$0")" || exit 1
mkdir -p evidence
FLAGS="-std=c11 -Wall -Wextra -O0"

cap() {   # cap <file> <command...> : transcript of command + output + exit status
    local out=$1; shift
    { echo "\$ $*"; "$@" 2>&1; local rc=$?
      [ $rc -eq 139 ] && echo "Segmentation fault (core dumped)   <-- stack overflow"
      echo "[exit status $rc]"; } >> "$out"
}
fresh() { : > "$1"; }

# 01 compiler + compile commands
f=evidence/01_compile.txt; fresh $f
cap $f gcc --version
for p in b1_complete_code b1_complete_code_twocalls b1_validate b1_count_calls b1_limit; do
    cap $f gcc $FLAGS -o $p $p.c
done

# 02 Complete the Code - TODO 1
f=evidence/02_complete_code.txt; fresh $f
echo "Expected: linear: 1073741824 in 31 calls / fast : 1073741824 in 6 calls" >> $f
cap $f ./b1_complete_code

# 03 half*half replaced by two calls
f=evidence/03_two_calls_experiment.txt; fresh $f
cap $f ./b1_complete_code_twocalls

# 04 B1.1 validation
f=evidence/04_validate.txt; fresh $f
cap $f ./b1_validate

# 05 B1.3 call counts
f=evidence/05_call_counts.txt; fresh $f
cap $f ./b1_count_calls

# 06 B1.4 find the limit (default 8 MB stack plus other sizes)
f=evidence/06_find_limit.txt; fresh $f
cap $f ./find_limit.sh ./b1_limit

# 07 crash demo, iterative versions, optimiser effect, ASan report
f=evidence/07_limit_details.txt; fresh $f
cap $f ./b1_limit linear 100000
cap $f ./b1_limit linear 1000000
cap $f ./b1_limit iter 2147483647
cap $f ./b1_limit fast 2147483647
cap $f ./b1_limit iterfast 2147483647
gcc -std=c11 -O2 -o /tmp/b1_limit_O2 b1_limit.c
echo "# same source built with -O2 (gcc turns the recursion into a loop)" >> $f
cap $f /tmp/b1_limit_O2 linear 100000000
gcc -std=c11 -O0 -g -fsanitize=address -o /tmp/b1_limit_asan b1_limit.c
{ echo "\$ /tmp/b1_limit_asan linear 1000000     # built with -fsanitize=address"
  /tmp/b1_limit_asan linear 1000000 2>&1 | head -9
  echo "    ... (remaining stack frames omitted) ..."; } >> $f
rm -f /tmp/b1_limit_O2 /tmp/b1_limit_asan
echo "B1 evidence written to $(pwd)/evidence"
