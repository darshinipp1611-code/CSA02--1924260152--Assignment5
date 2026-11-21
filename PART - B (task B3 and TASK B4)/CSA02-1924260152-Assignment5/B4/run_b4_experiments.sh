#!/bin/bash
# B4 experiments: generate data, run the three strategies 5x each, collect results.
# usage: ./run_b4_experiments.sh        (run from the B4 folder)
set -e
gcc -Wall -Wextra -O2 -o b4_gen b4_gen.c
gcc -Wall -Wextra -O2 -o b4_report b4_report.c
OUT=results/b4_results.csv
echo "records,query,mode,run,time_ms,heap_bytes,peak_rss_kb,listed,index_reads" > $OUT
for N in 10000 100000 1000000; do
  F=data/donors_$N.txt
  ./b4_gen $N $F > /dev/null
  ./b4_report buildindex $F | tee results/buildindex_$N.txt | sed "s/^/[$N] /"
  for Q in all new; do
    for M in memory batch index; do
      for R in 1 2 3 4 5; do
        L=$(./b4_report $M $F results/report_${M}_${Q}_$N.csv $Q)
        t=$(echo "$L" | sed -n 's/.*time_ms=\([0-9.]*\).*/\1/p')
        h=$(echo "$L" | sed -n 's/.*heap_in_use_bytes=\([0-9]*\).*/\1/p')
        k=$(echo "$L" | sed -n 's/.*peak_rss_kb=\([0-9]*\).*/\1/p')
        l=$(echo "$L" | sed -n 's/.*listed=\([0-9]*\).*/\1/p')
        x=$(echo "$L" | sed -n 's/.*index_reads=\([0-9]*\).*/\1/p')
        echo "$N,$Q,$M,$R,$t,$h,$k,$l,$x" >> $OUT
      done
    done
    # all three strategies must produce the same set of donors
    a=$(sort results/report_memory_${Q}_$N.csv | md5sum | cut -d' ' -f1)
    b=$(sort results/report_batch_${Q}_$N.csv  | md5sum | cut -d' ' -f1)
    c=$(sort results/report_index_${Q}_$N.csv  | md5sum | cut -d' ' -f1)
    if [ "$a" = "$b" ] && [ "$b" = "$c" ]; then echo "[$N][$Q] identical report from all 3 strategies"; else echo "[$N][$Q] MISMATCH"; fi
  done
done
