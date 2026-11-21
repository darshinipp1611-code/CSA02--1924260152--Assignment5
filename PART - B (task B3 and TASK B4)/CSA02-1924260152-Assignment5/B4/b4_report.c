/* b4_report.c - nightly report of donors eligible to donate within the next week.
 *
 * A donor is listed when   age is 18..65   AND   last_donation + 90 days <= report_date + 7.
 * With the optional argument "new" only donors who become eligible from report_date onward
 * (not those already eligible) are listed - a SELECTIVE query for which an index can help.
 *
 * usage: b4_report <mode> <donors.txt> <report.csv> [all|new]
 *   mode = memory      load every donor into the growable DonorList, then filter
 *          batch       read the file in batches of 4096 donors, filter each batch
 *          buildindex  create <donors.txt>.idx : (next_eligible_day, file offset) sorted
 *          index       use the sorted index file (binary search with fseek) + fetch records
 * Output: one summary line  key=value ...  with time, heap use and peak memory.            */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <sys/resource.h>
#include "b4_common.h"

#define BATCH 4096

typedef struct { Donor *data; int size, capacity; } DonorList;
typedef struct { int next; long offset; } IdxEntry;                 /* 16 bytes */
typedef struct { IdxEntry *data; int size, capacity; } IdxList;

static int TODAY, CUTOFF, LOWER;                   /* LOWER = INT_MIN for "all" */
static long g_lines = 0, g_bad = 0, g_eligible = 0;

/* ---------------- growable donor list (same design as the Complete-the-Code list) ------ */
static int add_donor(DonorList *list, const Donor *d)
{
    if (list->size == list->capacity) {
        int new_capacity = (list->capacity == 0) ? 2 : list->capacity * 2;
        Donor *p = realloc(list->data, (size_t)new_capacity * sizeof(Donor));
        if (p == NULL) return 0;                   /* old block still valid */
        list->data = p;
        list->capacity = new_capacity;
    }
    list->data[list->size++] = *d;
    return 1;
}
static int add_idx(IdxList *list, IdxEntry e)
{
    if (list->size == list->capacity) {
        int new_capacity = (list->capacity == 0) ? 1024 : list->capacity * 2;
        IdxEntry *p = realloc(list->data, (size_t)new_capacity * sizeof(IdxEntry));
        if (p == NULL) return 0;
        list->data = p;
        list->capacity = new_capacity;
    }
    list->data[list->size++] = e;
    return 1;
}

/* Parses  id,name,group,age,phone,yyyy-mm-dd  - returns 1 if the line is valid. */
static int parse_line(const char *line, Donor *d)
{
    int y, m, dd;
    if (sscanf(line, "%d,%31[^,],%3[^,],%d,%15[^,],%d-%d-%d",
               &d->id, d->name, d->group, &d->age, d->phone, &y, &m, &dd) != 8) return 0;
    if (m < 1 || m > 12 || dd < 1 || dd > 31 || d->age < 0 || d->age > 120) return 0;
    d->last_donation = days_from_civil(y, m, dd);
    return 1;
}

static int is_listed(const Donor *d)
{
    int next = d->last_donation + GAP_DAYS;
    return d->age >= MIN_AGE && d->age <= MAX_AGE && next <= CUTOFF && next >= LOWER;
}

static int write_report_line(FILE *out, const Donor *d)
{
    int y, m, dd;
    civil_from_days(d->last_donation + GAP_DAYS, &y, &m, &dd);
    g_eligible++;
    return fprintf(out, "%d,%s,%s,%s,%04d-%02d-%02d\n", d->id, d->name, d->group, d->phone, y, m, dd) > 0;
}

/* bytes currently allocated from the heap (small blocks + mmap'd large blocks) */
static size_t heap_bytes(void)
{
    struct mallinfo2 mi = mallinfo2();
    return mi.uordblks + mi.hblkhd;
}

static void summary(const char *mode, long n, double ms, size_t heap, long extra_reads)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    printf("mode=%-10s records=%ld invalid_lines=%ld listed=%ld time_ms=%.1f heap_in_use_bytes=%zu peak_rss_kb=%ld index_reads=%ld\n",
           mode, n, g_bad, g_eligible, ms, heap, ru.ru_maxrss, extra_reads);
}

/* ---------------- strategy 1: everything in memory ---------------- */
static int run_memory(const char *in, const char *outp)
{
    double t0 = now_ms();
    FILE *f = fopen(in, "r");
    if (!f) { perror(in); return 1; }
    FILE *out = fopen(outp, "w");
    if (!out) { perror(outp); fclose(f); return 1; }
    DonorList list = {NULL, 0, 0};
    char line[256];
    Donor d;
    while (fgets(line, sizeof line, f)) {
        g_lines++;
        if (!parse_line(line, &d)) { g_bad++; continue; }
        if (!add_donor(&list, &d)) {
            fprintf(stderr, "out of memory after %d donors - report aborted\n", list.size);
            free(list.data); fclose(f); fclose(out); return 1;
        }
    }
    fclose(f);
    size_t heap = heap_bytes();            /* heap in use at the peak */
    for (int i = 0; i < list.size; i++)
        if (is_listed(&list.data[i]) && !write_report_line(out, &list.data[i])) { perror("write"); break; }
    if (fclose(out) != 0) perror(outp);
    int n = list.size;
    free(list.data);                                /* released exactly once */
    list.data = NULL; list.size = list.capacity = 0;
    summary("memory", n, now_ms() - t0, heap, 0);
    return 0;
}

/* ---------------- strategy 2: batches ---------------- */
static int run_batch(const char *in, const char *outp)
{
    double t0 = now_ms();
    FILE *f = fopen(in, "r");
    if (!f) { perror(in); return 1; }
    FILE *out = fopen(outp, "w");
    if (!out) { perror(outp); fclose(f); return 1; }
    Donor *batch = malloc(BATCH * sizeof(Donor));
    if (!batch) { perror("malloc"); fclose(f); fclose(out); return 1; }
    char line[256];
    size_t heap = 0;
    long total = 0;
    int k = 0;
    for (;;) {
        char *got = fgets(line, sizeof line, f);
        if (got) {
            g_lines++;
            if (!parse_line(line, &batch[k])) g_bad++; else { k++; total++; }
        }
        if (k == BATCH || (!got && k > 0)) {        /* batch full, or last partial batch */
            if (heap == 0) heap = heap_bytes();
            for (int i = 0; i < k; i++)
                if (is_listed(&batch[i]) && !write_report_line(out, &batch[i])) { perror("write"); break; }
            k = 0;
        }
        if (!got) break;
    }
    free(batch);
    fclose(f);
    if (fclose(out) != 0) perror(outp);
    summary("batch", total, now_ms() - t0, heap, 0);
    return 0;
}

/* ---------------- strategy 3: sorted index file ---------------- */
static int cmp_idx(const void *a, const void *b)
{
    const IdxEntry *x = a, *y = b;
    return (x->next > y->next) - (x->next < y->next);
}
static int run_buildindex(const char *in)
{
    double t0 = now_ms();
    char ipath[512];
    snprintf(ipath, sizeof ipath, "%s.idx", in);
    FILE *f = fopen(in, "r");
    if (!f) { perror(in); return 1; }
    IdxList list = {NULL, 0, 0};
    char line[256];
    Donor d;
    long off = ftell(f);
    while (fgets(line, sizeof line, f)) {
        g_lines++;
        if (parse_line(line, &d)) {
            IdxEntry e = { d.last_donation + GAP_DAYS, off };
            if (!add_idx(&list, e)) { fprintf(stderr, "out of memory while building index\n"); free(list.data); fclose(f); return 1; }
        } else g_bad++;
        off = ftell(f);
    }
    fclose(f);
    size_t heap = heap_bytes();
    qsort(list.data, (size_t)list.size, sizeof(IdxEntry), cmp_idx);
    FILE *o = fopen(ipath, "wb");
    if (!o) { perror(ipath); free(list.data); return 1; }
    if (fwrite(list.data, sizeof(IdxEntry), (size_t)list.size, o) != (size_t)list.size) perror("fwrite");
    if (fclose(o) != 0) perror(ipath);
    int n = list.size;
    free(list.data);
    summary("buildindex", n, now_ms() - t0, heap, 0);
    return 0;
}

/* first position whose next >= key (lower bound) using fseek+fread; counts reads */
static long idx_lower_bound(FILE *fi, long n, int key, long *reads)
{
    long lo = 0, hi = n;
    IdxEntry e;
    while (lo < hi) {
        long mid = lo + (hi - lo) / 2;
        if (fseek(fi, mid * (long)sizeof(IdxEntry), SEEK_SET) != 0 || fread(&e, sizeof e, 1, fi) != 1) return -1;
        (*reads)++;
        if (e.next < key) lo = mid + 1; else hi = mid;
    }
    return lo;
}
static int run_index(const char *in, const char *outp)
{
    double t0 = now_ms();
    char ipath[512];
    snprintf(ipath, sizeof ipath, "%s.idx", in);
    FILE *fi = fopen(ipath, "rb");
    if (!fi) { perror(ipath); return 1; }
    FILE *f = fopen(in, "r");
    if (!f) { perror(in); fclose(fi); return 1; }
    FILE *out = fopen(outp, "w");
    if (!out) { perror(outp); fclose(fi); fclose(f); return 1; }
    fseek(fi, 0, SEEK_END);
    long n = ftell(fi) / (long)sizeof(IdxEntry);
    long reads = 0;
    long first = (LOWER == INT_MIN) ? 0 : idx_lower_bound(fi, n, LOWER, &reads);
    long last  = idx_lower_bound(fi, n, CUTOFF + 1, &reads);          /* first entry > CUTOFF */
    if (first < 0 || last < 0) { perror("index read"); return 1; }
    fseek(fi, first * (long)sizeof(IdxEntry), SEEK_SET);
    IdxEntry e;
    char line[256];
    Donor d;
    for (long i = first; i < last && fread(&e, sizeof e, 1, fi) == 1; i++) {
        reads++;
        if (fseek(f, e.offset, SEEK_SET) != 0 || !fgets(line, sizeof line, f)) { g_bad++; continue; }
        g_lines++;
        if (!parse_line(line, &d)) { g_bad++; continue; }
        if (is_listed(&d) && !write_report_line(out, &d)) { perror("write"); break; }
    }
    fclose(fi); fclose(f);
    if (fclose(out) != 0) perror(outp);
    summary("index", n, now_ms() - t0, heap_bytes(), reads);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 5) {
        fprintf(stderr, "usage: %s <memory|batch|buildindex|index> <donors.txt> [report.csv] [all|new]\n", argv[0]);
        return 1;
    }
    TODAY  = days_from_civil(2026, 11, 21);       /* fixed report date => reproducible results */
    CUTOFF = TODAY + REPORT_WINDOW;
    LOWER  = INT_MIN;
    if (argc == 5) {
        if (strcmp(argv[4], "new") == 0) LOWER = TODAY;
        else if (strcmp(argv[4], "all") != 0) { fprintf(stderr, "invalid option '%s' (use all or new)\n", argv[4]); return 1; }
    }
    const char *mode = argv[1];
    if (strcmp(mode, "buildindex") == 0) return run_buildindex(argv[2]);
    if (argc < 4) { fprintf(stderr, "report file name missing\n"); return 1; }
    if (strcmp(mode, "memory") == 0) return run_memory(argv[2], argv[3]);
    if (strcmp(mode, "batch")  == 0) return run_batch(argv[2], argv[3]);
    if (strcmp(mode, "index")  == 0) return run_index(argv[2], argv[3]);
    fprintf(stderr, "unknown mode '%s'\n", mode);
    return 1;
}
