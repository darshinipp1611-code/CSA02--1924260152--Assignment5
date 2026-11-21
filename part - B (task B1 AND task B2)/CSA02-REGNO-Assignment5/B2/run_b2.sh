#!/bin/bash
# Builds and runs every Task B2 experiment; verbatim transcripts go to evidence/*.txt
# (the PNG screenshots are drawn from these files).      cd B2 && ./run_b2.sh
# Needs: gcc, valgrind.  When run as root the read-only-file tests are run as
# user "nobody" (root ignores file permissions); as a normal user nothing special happens.
cd "$(dirname "$0")" || exit 1
HERE=$(pwd)
mkdir -p evidence
FLAGS="-std=c11 -Wall -Wextra -O0 -g"
ASAN="-std=c11 -O0 -g -fsanitize=address -fno-omit-frame-pointer"

# T <file> <displayed command> [actual command]   -> appends "$ cmd", output, exit status
T() {
    local f=$1 shown=$2 actual=${3:-$2}
    { echo "\$ $shown"
      eval "$actual" 2>&1 | grep -v -E 'run_b2\.sh: line|^ +[0-9]+ +(Aborted|Done)'
      local rc=${PIPESTATUS[0]}
      [ $rc -eq 134 ] && echo "Aborted (core dumped)"
      [ $rc -eq 139 ] && echo "Segmentation fault (core dumped)"
      echo "[exit status $rc]"; } >> "$f"
}
note()  { echo "# $2" >> "$1"; }
fresh() { : > "$1"; }

as_user() {                       # run a command as an unprivileged user
    if [ "$(id -u)" = 0 ]; then setpriv --reuid=65534 --regid=65534 --clear-groups "$@"
    else "$@"; fi
}

vg() {                            # valgrind with the noise removed
    local tmp; tmp=$(mktemp)
    valgrind --leak-check=full --error-exitcode=9 "$@" > "$tmp" 2>&1
    local rc=$?
    sed -E 's/^==[0-9]+== //' "$tmp" \
      | grep -v -E '^(Memcheck,|Copyright|Using Valgrind|Command:|Rerun with|For lists|$|Use --track|To see)' \
      | head -n ${VGLINES:-26}
    rm -f "$tmp"
    return $rc
}
asan() {                          # run, show the first lines, return the program's real status
    local tmp; tmp=$(mktemp)
    "$@" > "$tmp" 2>&1
    local rc=$?
    head -n ${ASANLINES:-9} "$tmp"
    rm -f "$tmp"
    return $rc
}

# scratch copy of the project so mutating commands never touch the repo's data
W=$(mktemp -d); chmod 755 "$W"
trap 'rm -rf "$W"' EXIT

############################################################ 01 compile
f=evidence/01_compile.txt; fresh $f
T $f "gcc --version | head -1"
for p in b2_scanf_unsafe b2_fgets_safe b2_getline_alt b2_memerr_fixed b2_bloodbank; do
    T $f "gcc $FLAGS -o $p $p.c"
done
note $f "gcc itself warns about the deliberately buggy program (this is expected):"
T $f "gcc $FLAGS -o b2_memerr_buggy b2_memerr_buggy.c 2>&1 | grep -E 'warning' "
gcc $FLAGS -o b2_memerr_buggy b2_memerr_buggy.c 2>/dev/null
cp b2_bloodbank "$W"/; cp -r data "$W"/

############################################################ 02 scanf %s (unsafe)
f=evidence/02_scanf_overflow.txt; fresh $f
note $f "name[10] and age (set to 25) live in the same structure"
T $f "echo Anil | ./b2_scanf_unsafe"
T $f "echo AAAAAAAAAAAAAAA | ./b2_scanf_unsafe          # 15 characters"
T $f "python3 -c \"print('B'*60)\" | ./b2_scanf_unsafe   # 60 characters"
gcc $ASAN -o /tmp/b2_scanf_asan b2_scanf_unsafe.c
T $f "python3 -c \"print('B'*60)\" | ./b2_scanf_unsafe   # rebuilt with -fsanitize=address" \
       "python3 -c \"print('B'*60)\" | ASANLINES=12 asan /tmp/b2_scanf_asan"
rm -f /tmp/b2_scanf_asan

############################################################ 03 fgets (safe)
f=evidence/03_fgets_safe.txt; fresh $f
note $f "same inputs, safe version: lines 1,3,4 fit; line 2 has 60 characters"
T $f "(echo Anil; python3 -c \"print('C'*60)\"; echo Ravi; echo 123456789) | ./b2_fgets_safe"

############################################################ 04 file errors
f=evidence/04_file_errors.txt; fresh $f
T $f "BB_DONORS=/nonexistent/donors.txt ./b2_bloodbank find O+     # missing file"
T $f "BB_STOCK=missing_stock.txt ./b2_bloodbank issue O+          # missing file"
cp "$W"/data/donors.txt "$W"/ro_donors.txt; cp "$W"/data/stock.txt "$W"/ro_stock.txt
chmod 444 "$W"/ro_donors.txt "$W"/ro_stock.txt
cp "$W"/data/donors.txt "$W"/noread.txt; chmod 000 "$W"/noread.txt
T $f "ls -l ro_donors.txt ro_stock.txt noread.txt        # in a scratch copy" "(cd $W && ls -l ro_donors.txt ro_stock.txt noread.txt | awk '{print \$1, \$9}')"
T $f "BB_DONORS=noread.txt ./b2_bloodbank find O+                # file without read permission" \
       "(cd $W && BB_DONORS=noread.txt as_user ./b2_bloodbank find O+)"
T $f "printf 'Test Donor\\n30\\n9876543210\\n' | BB_DONORS=ro_donors.txt ./b2_bloodbank register 7999 O+    # read-only file" \
       "(cd $W && printf 'Test Donor\\n30\\n9876543210\\n' | BB_DONORS=ro_donors.txt as_user ./b2_bloodbank register 7999 O+)"
T $f "BB_STOCK=ro_stock.txt ./b2_bloodbank issue O+              # read-only stock file" \
       "(cd $W && BB_STOCK=ro_stock.txt as_user ./b2_bloodbank issue O+)"
T $f "BB_DONORS=data/donors_bad.txt ./b2_bloodbank find O+     # damaged data file: bad lines reported, program continues" \
       "BB_DONORS=data/donors_bad.txt ./b2_bloodbank find O+"

############################################################ 05 command-line arguments
f=evidence/05_cli_arguments.txt; fresh $f
BB="cd $W && ./b2_bloodbank"
T $f "./b2_bloodbank                         # no arguments"           "($BB)"
T $f "./b2_bloodbank find O+ extra           # too many arguments"      "($BB find O+ extra)"
T $f "./b2_bloodbank find                    # too few arguments"       "($BB find)"
T $f "./b2_bloodbank issue A2                # invalid blood group"     "($BB issue A2)"
T $f "./b2_bloodbank find o+                 # lower case is not valid" "($BB find o+)"
T $f "./b2_bloodbank donate O+               # unknown command"         "($BB donate O+)"
T $f "./b2_bloodbank register abc O+         # invalid donor ID"        "($BB register abc O+)"
note $f "valid commands, run in a scratch copy of the data:"
T $f "./b2_bloodbank find O+"       "($BB find O+)"
T $f "./b2_bloodbank issue O-"      "($BB issue O-)"
T $f "cat data/stock.txt | tail -3" "(cd $W && tail -3 data/stock.txt)"

############################################################ 06 adversarial register input
f=evidence/06_register_inputs.txt; fresh $f
R="cd $W && ./b2_bloodbank register 7500 B+"
T $f "name of 30 characters"       "(printf '%s\\n30\\n9876543210\\n' ABCDEFGHIJKLMNOPQRSTUVWXYZabcd | ($R))"
T $f "empty name"                  "(printf '\\n30\\n9876543210\\n' | ($R))"
T $f "name containing a comma"     "(printf 'Smith, John\\n30\\n9876543210\\n' | ($R))"
T $f "age = abc"                   "(printf 'Test Donor\\nabc\\n9876543210\\n' | ($R))"
T $f "age = 17 (below limit)"      "(printf 'Test Donor\\n17\\n9876543210\\n' | ($R))"
T $f "phone = 12ab"                "(printf 'Test Donor\\n30\\n12ab\\n' | ($R))"
T $f "input ends after the name (EOF)" "(printf 'Test Donor\\n' | ($R))"
note $f "valid registration, then look the donor up:"
T $f "printf 'Latha Krishnan\\n33\\n9876543210\\n' | ./b2_bloodbank register 7500 B+" \
       "(printf 'Latha Krishnan\\n33\\n9876543210\\n' | ($R))"
T $f "./b2_bloodbank find B+" "($BB find B+)"

############################################################ 07 Valgrind / AddressSanitizer
f=evidence/07a_buggy_asan.txt; fresh $f
gcc $ASAN -o /tmp/b2_buggy_asan b2_memerr_buggy.c 2>/dev/null
note $f "b2_memerr_buggy built with -fsanitize=address (AddressSanitizer + LeakSanitizer)"
for m in overflow uaf doublefree leak; do
    ASANLINES=8 T $f "./b2_memerr_buggy $m" "ASANLINES=8 asan /tmp/b2_buggy_asan $m"
done
rm -f /tmp/b2_buggy_asan

f=evidence/07b_buggy_valgrind_1.txt; fresh $f
for m in overflow uaf; do
    T $f "valgrind --leak-check=full ./b2_memerr_buggy $m" "VGLINES=26 vg ./b2_memerr_buggy $m"
done
f=evidence/07b_buggy_valgrind_2.txt; fresh $f
for m in doublefree leak; do
    T $f "valgrind --leak-check=full ./b2_memerr_buggy $m" "VGLINES=26 vg ./b2_memerr_buggy $m"
done

f=evidence/07c_fixed_clean.txt; fresh $f
gcc $ASAN -o /tmp/b2_fixed_asan b2_memerr_fixed.c
T $f "./b2_memerr_fixed all             # built with -fsanitize=address" "asan /tmp/b2_fixed_asan all"
T $f "valgrind --leak-check=full ./b2_memerr_fixed all" "VGLINES=14 vg ./b2_memerr_fixed all"
rm -f /tmp/b2_fixed_asan

f=evidence/07d_bloodbank_clean.txt; fresh $f
gcc $ASAN -o "$W"/b2_bloodbank_asan b2_bloodbank.c
SCEN='./PROG find O+ >/dev/null; BB_DONORS=data/donors_bad.txt ./PROG find O+ >/dev/null 2>&1; ./PROG issue A+ >/dev/null; printf "Ravi Shankar\n41\n9876543210\n" | ./PROG register 7600 AB- >/dev/null; ./PROG find Z+ >/dev/null 2>&1; BB_DONORS=/nonexistent ./PROG find O+ >/dev/null 2>&1; echo scenarios finished'
T $f "(AddressSanitizer build) find, damaged file, issue, register, bad args, missing file" \
       "(cd $W && ${SCEN//PROG/./b2_bloodbank_asan}) 2>&1 | head -5; echo \"ASan: no error report\""
rm -f "$W"/data/*.tmp
cp data/donors.txt data/stock.txt "$W"/data/
T $f "valgrind --leak-check=full ./b2_bloodbank  (same six scenarios)" \
       "(cd $W && valgrind --leak-check=full --error-exitcode=9 -q ./b2_bloodbank find O+ >/dev/null; echo \"find O+: valgrind exit \$?\"; BB_DONORS=data/donors_bad.txt valgrind --leak-check=full --error-exitcode=9 -q ./b2_bloodbank find O+ >/dev/null 2>&1; echo \"damaged file: valgrind exit \$?\"; valgrind --leak-check=full --error-exitcode=9 -q ./b2_bloodbank issue A+ >/dev/null; echo \"issue A+: valgrind exit \$?\"; printf 'Ravi Shankar\\n41\\n9876543210\\n' | valgrind --leak-check=full --error-exitcode=9 -q ./b2_bloodbank register 7600 AB- >/dev/null; echo \"register: valgrind exit \$?\"; valgrind --leak-check=full --error-exitcode=9 -q ./b2_bloodbank find Z+ >/dev/null 2>&1; echo \"bad group (usage error 2): valgrind exit \$?\"; BB_DONORS=/nonexistent valgrind --leak-check=full --error-exitcode=9 -q ./b2_bloodbank find O+ >/dev/null 2>&1; echo \"missing file: valgrind exit \$?\")"
note $f "valgrind exit 0 = no memory errors / leaks; 2 and 1 are the program's own error codes, 9 would mean valgrind found an error"
T $f "valgrind --leak-check=full ./b2_bloodbank find O+     # full summary" \
       "(cd $W && VGLINES=12 vg ./b2_bloodbank find O+)"

############################################################ 08 getline alternative
f=evidence/08_getline.txt; fresh $f
cp b2_getline_alt "$W"/
{ echo Anil; python3 -c "print('D'*60)"; python3 -c "print('E'*100000)"; echo Ravi; } > "$W"/names.txt
note $f "names.txt = 4 lines: 4 chars, 60 chars, 100000 chars, 4 chars"
T $f "./b2_getline_alt < names.txt" "(cd $W && ./b2_getline_alt < names.txt)"
note $f "time to read ONE line of length L, and heap used (= buffer capacity):"
for L in 1000 10000 100000 1000000 10000000; do
    python3 -c "print('x'*$L)" > "$W"/line_$L.txt
    T $f "./b2_getline_alt < line_of_${L}_chars.txt" "(cd $W && ./b2_getline_alt < line_$L.txt)"
done
T $f "valgrind --leak-check=full ./b2_getline_alt < names.txt   # who frees the buffer? main() does" \
       "(cd $W && VGLINES=24 vg ./b2_getline_alt < names.txt | grep -v '^line\\|^        text')"

echo "B2 evidence written to $HERE/evidence"
