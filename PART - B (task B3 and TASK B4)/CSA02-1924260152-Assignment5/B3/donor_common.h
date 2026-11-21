/* donor_common.h - shared donor record, deterministic data generator and
 * helpers used by the Task B3 programs.                                    */
#ifndef DONOR_COMMON_H
#define DONOR_COMMON_H
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    int  id;
    char name[32];
    char group[4];
    int  age;
    char phone[16];
    int  last_date;          /* yyyymmdd, e.g. 20260815 */
} Donor;                      /* sizeof(Donor) == 64, no padding */

static const char *FIRST[] = {"Asha","Ravi","Meena","Karthik","Divya","Arun","Nila","Suresh"};
static const char *LAST[]  = {"Kumar","Devi","Raj","Nair","Iyer","Rao","Das","Pillai"};
static const char *GROUPS[] = {"O+","A+","B+","AB+","O-","A-","B-","AB-"};

/* Deterministic record number i -> Donor (same data on every run). */
static void make_donor(Donor *d, int i)
{
    memset(d, 0, sizeof *d);                 /* zero padding / unused bytes */
    d->id = 100000 + i;
    snprintf(d->name, sizeof d->name, "%s_%s", FIRST[i % 8], LAST[(i / 8) % 8]);
    snprintf(d->group, sizeof d->group, "%s", GROUPS[(i * 7 + 3) % 8]);
    d->age = 18 + (i * 13) % 48;
    snprintf(d->phone, sizeof d->phone, "9%09d", (int)(((long long)i * 7919) % 1000000000LL));
    d->last_date = 20260000 + (1 + (i * 5) % 11) * 100 + 1 + (i * 11) % 28;
}

static int write_text_record(FILE *fp, const Donor *d)
{
    return fprintf(fp, "%d %s %s %d %s %d\n",
                   d->id, d->name, d->group, d->age, d->phone, d->last_date) > 0;
}

static int read_text_record(FILE *fp, Donor *d)
{
    memset(d, 0, sizeof *d);
    return fscanf(fp, "%d %31s %3s %d %15s %d",
                  &d->id, d->name, d->group, &d->age, d->phone, &d->last_date) == 6;
}

static int donor_equal(const Donor *a, const Donor *b)
{
    return a->id == b->id && strcmp(a->name, b->name) == 0 &&
           strcmp(a->group, b->group) == 0 && a->age == b->age &&
           strcmp(a->phone, b->phone) == 0 && a->last_date == b->last_date;
}

static long file_size(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fclose(f);
    return n;
}

static inline __attribute__((unused)) double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}
#endif
