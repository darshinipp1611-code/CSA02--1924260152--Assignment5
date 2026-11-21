#!/bin/bash
# B1.4 driver: find the largest n for which power_linear survives, by doubling
# then bisection, for several stack sizes.  Each trial is a separate process,
# so a segmentation fault only kills that trial.
#   ./find_limit.sh [binary]            (default ./b1_limit)
BIN=${1:-./b1_limit}

survives() {            # $1 = mode, $2 = n  -> exit 0 if the run survived
    "$BIN" "$1" "$2" >/dev/null 2>&1
}

find_limit() {          # $1 = mode ; uses the current ulimit -s
    local lo=1000 hi
    while survives "$1" "$lo"; do
        lo=$((lo * 2))
        [ "$lo" -gt 2000000000 ] && { echo "none (survived 2e9)"; return; }
    done
    hi=$lo; lo=$((lo / 2))                 # lo survives, hi crashes
    while [ $((hi - lo)) -gt 1 ]; do
        mid=$(( (lo + hi) / 2 ))
        if survives "$1" "$mid"; then lo=$mid; else hi=$mid; fi
    done
    echo "$lo"
}

echo "Largest n for which power_linear (recursive) still works:"
printf "%-16s %-20s %-22s\n" "stack size" "max n (survives)" "first n that crashes"
for kb in 1024 2048 4096 8192 16384; do
    ( ulimit -s $kb
      m=$(find_limit linear)
      printf "%-16s %-20s %-22s\n" "${kb} KB" "$m" "$((m + 1))"
      printf "      bytes of stack per call ~ %s\n" "$(( kb * 1024 / m ))" )
done
