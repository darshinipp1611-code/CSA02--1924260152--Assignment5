/* B3.2 and B3.3 - text file vs binary file benchmark.
 *   B3.2: file size, write time and read time for 1,000 / 10,000 / 100,000 donors
 *   B3.3: time to update the 50,000th record
 *         (binary: fseek + fread + fwrite in place;  text: rewrite whole file)
 * Every timing is the MEDIAN of several repetitions.  Results go to the screen
 * and to results/b3_results.csv.                                              */
#include <stdio.h>
#include <stdlib.h>
#include "donor_common.h"

#define REPS 7

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static double median(double *v, int n)
{
    qsort(v, (size_t)n, sizeof(double), cmp_double);
    return v[n / 2];
}

/* ---------- text format ---------- */
static int write_text_file(const char *path, const Donor *a, int n)
{
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return 0; }
    for (int i = 0; i < n; i++) if (!write_text_record(f, &a[i])) { perror("fprintf"); fclose(f); return 0; }
    return fclose(f) == 0;
}
static int read_text_file(const char *path, Donor *a, int n)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 0; }
    int k = 0;
    while (k < n && read_text_record(f, &a[k])) k++;
    fclose(f);
    return k == n;
}

/* ---------- binary format ---------- */
static int write_bin_file(const char *path, const Donor *a, int n)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return 0; }
    for (int i = 0; i < n; i++)                         /* one fwrite per record */
        if (fwrite(&a[i], sizeof(Donor), 1, f) != 1) { perror("fwrite"); fclose(f); return 0; }
    return fclose(f) == 0;
}
static int read_bin_file(const char *path, Donor *a, int n)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 0; }
    size_t k = fread(a, sizeof(Donor), (size_t)n, f);   /* one fread for all */
    fclose(f);
    return (int)k == n;
}

/* ---------- B3.3 update routines (change last_date of record idx) ---------- */
static int update_binary_in_place(const char *path, int idx, int new_date)
{
    FILE *f = fopen(path, "rb+");
    if (!f) { perror(path); return 0; }
    Donor d;
    int ok = fseek(f, (long)idx * (long)sizeof(Donor), SEEK_SET) == 0 &&
             fread(&d, sizeof d, 1, f) == 1;
    if (ok) {
        d.last_date = new_date;
        ok = fseek(f, -(long)sizeof(Donor), SEEK_CUR) == 0 && fwrite(&d, sizeof d, 1, f) == 1;
    }
    if (fclose(f) != 0) ok = 0;
    return ok;
}

static int update_text_by_rewrite(const char *path, int idx, int new_date)
{
    char tmp[256];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *in = fopen(path, "r");
    if (!in) { perror(path); return 0; }
    FILE *out = fopen(tmp, "w");
    if (!out) { perror(tmp); fclose(in); return 0; }
    char line[256];
    int ln = 0, ok = 1, found = 0;
    while (fgets(line, sizeof line, in)) {
        if (ln == idx) {
            Donor d;
            if (sscanf(line, "%d %31s %3s %d %15s %d", &d.id, d.name, d.group, &d.age, d.phone, &d.last_date) != 6)
                { ok = 0; break; }
            d.last_date = new_date;
            write_text_record(out, &d);
            found = 1;
        } else fputs(line, out);
        ln++;
    }
    fclose(in);
    if (fclose(out) != 0) ok = 0;
    if (!ok || !found) { remove(tmp); return 0; }
    return rename(tmp, path) == 0;                      /* atomic replace */
}

int main(void)
{
    const int sizes[] = {1000, 10000, 100000};
    FILE *csv = fopen("results/b3_results.csv", "w");
    if (!csv) { perror("results/b3_results.csv"); return 1; }
    fprintf(csv, "kind,n,text,binary\n");

    printf("B3.2  File size and I/O time  (median of %d runs, sizeof(Donor)=%zu)\n", REPS, sizeof(Donor));
    printf("%-9s | %-26s | %-26s | %-26s\n", "donors", "file size (bytes)", "write all (ms)", "read all (ms)");
    printf("%-9s | %-12s %-13s | %-12s %-13s | %-12s %-13s\n", "", "text", "binary", "text", "binary", "text", "binary");
    printf("----------+----------------------------+----------------------------+---------------------------\n");

    for (int s = 0; s < 3; s++) {
        int n = sizes[s];
        Donor *src = malloc((size_t)n * sizeof(Donor));
        Donor *dst = malloc((size_t)n * sizeof(Donor));
        if (!src || !dst) { perror("malloc"); return 1; }
        for (int i = 0; i < n; i++) make_donor(&src[i], i);

        char tpath[64], bpath[64];
        snprintf(tpath, sizeof tpath, "data/donors_%d.txt", n);
        snprintf(bpath, sizeof bpath, "data/donors_%d.bin", n);

        double tw[REPS], bw[REPS], tr[REPS], br[REPS];
        for (int r = 0; r < REPS; r++) {
            double t0 = now_ms();
            if (!write_text_file(tpath, src, n)) return 1;
            tw[r] = now_ms() - t0;
            t0 = now_ms();
            if (!write_bin_file(bpath, src, n)) return 1;
            bw[r] = now_ms() - t0;
            t0 = now_ms();
            if (!read_text_file(tpath, dst, n)) return 1;
            tr[r] = now_ms() - t0;
            t0 = now_ms();
            if (!read_bin_file(bpath, dst, n)) return 1;
            br[r] = now_ms() - t0;
        }
        /* correctness check of the last text and binary read-back */
        int bad = 0;
        read_text_file(tpath, dst, n);
        for (int i = 0; i < n; i++) if (!donor_equal(&src[i], &dst[i])) bad++;
        read_bin_file(bpath, dst, n);
        for (int i = 0; i < n; i++) if (memcmp(&src[i], &dst[i], sizeof(Donor)) != 0) bad++;

        long ts = file_size(tpath), bs = file_size(bpath);
        double mtw = median(tw, REPS), mbw = median(bw, REPS), mtr = median(tr, REPS), mbr = median(br, REPS);
        printf("%-9d | %-12ld %-13ld | %-12.2f %-13.2f | %-12.2f %-13.2f %s\n",
               n, ts, bs, mtw, mbw, mtr, mbr, bad ? "  <-- DATA MISMATCH" : "");
        fprintf(csv, "size_bytes,%d,%ld,%ld\nwrite_ms,%d,%.3f,%.3f\nread_ms,%d,%.3f,%.3f\n",
                n, ts, bs, n, mtw, mbw, n, mtr, mbr);
        free(src); free(dst);
    }

    /* ---------------- B3.3 ---------------- */
    const int n = 100000, idx = 49999;                  /* 50,000th record = index 49999 */
    const char *tpath = "data/donors_100000.txt", *bpath = "data/donors_100000.bin";
    printf("\nB3.3  Update the 50,000th record of %d  (set last_date)\n", n);

    double tb[50], tt[REPS];
    for (int r = 0; r < 50; r++) {
        double t0 = now_ms();
        if (!update_binary_in_place(bpath, idx, 20261100 + r % 28 + 1)) return 1;
        tb[r] = now_ms() - t0;
    }
    for (int r = 0; r < REPS; r++) {
        double t0 = now_ms();
        if (!update_text_by_rewrite(tpath, idx, 20261100 + r % 28 + 1)) return 1;
        tt[r] = now_ms() - t0;
    }
    double mb = median(tb, 50), mt = median(tt, REPS);

    /* verify the last update landed in both files and neighbours are untouched */
    Donor chk, want; int ok_b = 0, ok_t = 0;
    int last_b = 20261100 + 49 % 28 + 1, last_t = 20261100 + (REPS - 1) % 28 + 1;
    FILE *f = fopen(bpath, "rb");
    if (f) { fseek(f, (long)idx * (long)sizeof(Donor), SEEK_SET);
             make_donor(&want, idx); want.last_date = last_b;
             ok_b = fread(&chk, sizeof chk, 1, f) == 1 && memcmp(&chk, &want, sizeof chk) == 0;
             fseek(f, (long)(idx + 1) * (long)sizeof(Donor), SEEK_SET);
             make_donor(&want, idx + 1);
             ok_b = ok_b && fread(&chk, sizeof chk, 1, f) == 1 && memcmp(&chk, &want, sizeof chk) == 0;
             fclose(f); }
    f = fopen(tpath, "r");
    if (f) { Donor d; int k = 0, hit = 0;
             while (read_text_record(f, &d)) { if (k == idx) { make_donor(&want, idx); want.last_date = last_t; hit = donor_equal(&d, &want); }
                                               else if (k == idx + 1) { make_donor(&want, idx + 1); ok_t = hit && donor_equal(&d, &want); break; }
                                               k++; }
             fclose(f); }

    printf("  binary, in place  (fseek+fread+fwrite) : %10.4f ms  (median of 50)  verified: %s\n", mb, ok_b ? "yes" : "NO");
    printf("  text,   full rewrite (read+write+rename): %10.4f ms  (median of %d)   verified: %s\n", mt, REPS, ok_t ? "yes" : "NO");
    printf("  speed-up of in-place binary update      : %10.0f x\n", mt / mb);
    fprintf(csv, "update_ms,%d,%.4f,%.4f\n", n, mt, mb);
    fclose(csv);
    return 0;
}
