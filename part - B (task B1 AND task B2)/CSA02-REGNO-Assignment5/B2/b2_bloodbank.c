/* B2 - Blood bank command-line program with hardened input and error handling.
 *
 *   bloodbank find <GROUP>            list donors of a blood group (text file)
 *   bloodbank register <ID> <GROUP>   add a donor; name/age/phone read from stdin
 *   bloodbank issue <GROUP>           issue one unit (stock text file is updated)
 *
 * Data files: data/donors.txt  (id,name,group,age,phone,last_donation)
 *             data/stock.txt   (group units, one line per group)
 * Override the paths with the environment variables BB_DONORS and BB_STOCK.
 *
 * Exit status: 0 success, 1 file / runtime error, 2 command-line usage error.
 *
 * Build: gcc -std=c11 -Wall -Wextra -O0 -g -o b2_bloodbank b2_bloodbank.c
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define NAME_SIZE 20            /* 19 characters + NUL                    */
#define LINE_SIZE 128           /* longest accepted input line + NUL      */
#define N_GROUPS  8

static const char *GROUPS[N_GROUPS] = {"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"};

typedef struct {
    int  id;
    char name[NAME_SIZE];
    char group[4];
    int  age;
    char phone[16];
    char last[11];
} Donor;

typedef struct { char group[4]; int units; } StockRec;

/* ------------------------------------------------------------------ helpers */

static int group_index(const char *g)
{
    for (int i = 0; i < N_GROUPS; i++)
        if (strcmp(GROUPS[i], g) == 0) return i;
    return -1;
}

/* Reads one line into buf (capacity size).
 * returns 1 = line read, 0 = end of input, -1 = line did not fit (rest discarded). */
static int read_line(FILE *fp, char *buf, size_t size)
{
    if (fgets(buf, (int)size, fp) == NULL) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') { buf[len - 1] = '\0'; return 1; }
    int c = fgetc(fp);
    if (c == '\n' || c == EOF) return 1;                 /* it fitted exactly */
    while (c != '\n' && c != EOF) c = fgetc(fp);         /* discard the remainder */
    return -1;
}

static void usage(FILE *out)
{
    fprintf(out,
            "usage: bloodbank find <GROUP>\n"
            "       bloodbank register <ID> <GROUP>   (name, age, phone are read from standard input)\n"
            "       bloodbank issue <GROUP>\n"
            "valid groups: A+ A- B+ B- AB+ AB- O+ O-\n");
}

/* ------------------------------------------------------------------ find */

static int parse_donor(const char *line, Donor *d)
{
    int used = 0;
    /* every %s / %[ has a width, so sscanf can never overflow a field */
    int n = sscanf(line, "%d,%19[^,],%3[^,],%d,%15[^,],%10[^,]%n",
                   &d->id, d->name, d->group, &d->age, d->phone, d->last, &used);
    if (n != 6 || line[used] != '\0') return 0;
    if (d->id <= 0 || d->age < 18 || d->age > 65 || group_index(d->group) < 0) return 0;
    return 1;
}

static int cmd_find(const char *path, const char *group)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) { perror(path); return 1; }

    char line[LINE_SIZE];
    Donor d;
    int lineno = 0, found = 0, bad = 0, rc;

    printf("Donors with blood group %s:\n", group);
    while ((rc = read_line(fp, line, sizeof line)) != 0) {
        lineno++;
        if (rc < 0) {
            fprintf(stderr, "%s:%d: line longer than %d characters - skipped\n", path, lineno, LINE_SIZE - 1);
            bad++;
            continue;
        }
        if (line[0] == '\0' || line[0] == '#') continue;
        if (!parse_donor(line, &d)) {
            fprintf(stderr, "%s:%d: malformed record - skipped\n", path, lineno);
            bad++;
            continue;
        }
        if (strcmp(d.group, group) == 0) {
            printf("  %-6d %-19s age %-3d phone %-12s last donated %s\n", d.id, d.name, d.age, d.phone, d.last);
            found++;
        }
    }
    if (ferror(fp)) { perror(path); fclose(fp); return 1; }
    fclose(fp);
    printf("%d donor(s) found, %d bad line(s) skipped.\n", found, bad);
    return 0;
}

/* ------------------------------------------------------------------ register */

static int ask(const char *prompt, char *buf, size_t size, const char *what)
{
    printf("%s", prompt);
    fflush(stdout);
    int rc = read_line(stdin, buf, size);
    if (rc == 0)  { fprintf(stderr, "\nerror: no %s supplied (end of input) - nothing was saved\n", what); return 0; }
    if (rc < 0)   { fprintf(stderr, "error: %s is longer than %zu characters - nothing was saved\n", what, size - 1); return 0; }
    if (buf[0] == '\0') { fprintf(stderr, "error: %s is empty - nothing was saved\n", what); return 0; }
    return 1;
}

static int cmd_register(const char *path, int id, const char *group)
{
    char name[NAME_SIZE], agebuf[8], phone[16];
    if (!ask("Donor name  (max 19 characters): ", name, sizeof name, "name")) return 1;
    if (strchr(name, ',')) { fprintf(stderr, "error: name must not contain a comma - nothing was saved\n"); return 1; }
    if (!ask("Donor age   (18-65): ", agebuf, sizeof agebuf, "age")) return 1;
    char *end;
    errno = 0;
    long age = strtol(agebuf, &end, 10);
    if (errno || *end != '\0' || age < 18 || age > 65) {
        fprintf(stderr, "error: age must be a whole number from 18 to 65 - nothing was saved\n");
        return 1;
    }
    if (!ask("Donor phone (10-15 digits): ", phone, sizeof phone, "phone number")) return 1;
    size_t plen = strlen(phone);
    if (plen < 10 || strspn(phone, "0123456789") != plen) {
        fprintf(stderr, "error: phone must be 10-15 digits - nothing was saved\n");
        return 1;
    }

    char date[11];
    time_t now = time(NULL);
    strftime(date, sizeof date, "%Y-%m-%d", localtime(&now));

    FILE *fp = fopen(path, "a");
    if (fp == NULL) { perror(path); return 1; }
    if (fprintf(fp, "%d,%s,%s,%ld,%s,%s\n", id, name, group, age, phone, date) < 0 || fclose(fp) != 0) {
        perror(path);
        return 1;
    }
    printf("Registered donor %d (%s), group %s.\n", id, name, group);
    return 0;
}

/* ------------------------------------------------------------------ issue */

static int load_stock(const char *path, StockRec st[N_GROUPS])
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) { perror(path); return 0; }
    char line[LINE_SIZE];
    int n = 0, rc;
    while (n < N_GROUPS && (rc = read_line(fp, line, sizeof line)) != 0) {
        if (rc < 0 || sscanf(line, "%3s %d", st[n].group, &st[n].units) != 2 ||
            group_index(st[n].group) < 0 || st[n].units < 0) {
            fprintf(stderr, "%s: bad stock record on line %d\n", path, n + 1);
            fclose(fp);
            return 0;
        }
        n++;
    }
    fclose(fp);
    if (n != N_GROUPS) { fprintf(stderr, "%s: expected %d stock records, found %d\n", path, N_GROUPS, n); return 0; }
    return 1;
}

static int cmd_issue(const char *path, const char *group)
{
    StockRec st[N_GROUPS];
    if (!load_stock(path, st)) return 1;

    StockRec *rec = NULL;
    for (int i = 0; i < N_GROUPS; i++)
        if (strcmp(st[i].group, group) == 0) rec = &st[i];
    if (rec == NULL || rec->units == 0) {
        fprintf(stderr, "error: no units of %s available\n", group);
        return 1;
    }
    rec->units--;

    FILE *fp = fopen(path, "w");                 /* fails on a read-only file */
    if (fp == NULL) { perror(path); return 1; }
    for (int i = 0; i < N_GROUPS; i++)
        if (fprintf(fp, "%s %d\n", st[i].group, st[i].units) < 0) { perror(path); fclose(fp); return 1; }
    if (fclose(fp) != 0) { perror(path); return 1; }
    printf("Issued 1 unit of %s; %d unit(s) left.\n", group, rec->units);
    return 0;
}

/* ------------------------------------------------------------------ main */

int main(int argc, char **argv)
{
    const char *donors = getenv("BB_DONORS");
    const char *stock  = getenv("BB_STOCK");
    if (donors == NULL) donors = "data/donors.txt";
    if (stock  == NULL) stock  = "data/stock.txt";

    if (argc < 2) {
        fprintf(stderr, "error: no command given\n");
        usage(stderr);
        return 2;
    }
    const char *cmd = argv[1];
    int want;                                    /* required argc for the command */
    if      (strcmp(cmd, "find") == 0 || strcmp(cmd, "issue") == 0) want = 3;
    else if (strcmp(cmd, "register") == 0)                          want = 4;
    else {
        fprintf(stderr, "error: unknown command '%s'\n", cmd);
        usage(stderr);
        return 2;
    }
    if (argc != want) {
        fprintf(stderr, "error: '%s' needs %d argument(s) but %d given\n", cmd, want - 2, argc - 2);
        usage(stderr);
        return 2;
    }
    const char *group = argv[want - 1];
    if (group_index(group) < 0) {
        fprintf(stderr, "error: '%s' is not a valid blood group\n", group);
        usage(stderr);
        return 2;
    }

    if (strcmp(cmd, "find") == 0)  return cmd_find(donors, group);
    if (strcmp(cmd, "issue") == 0) return cmd_issue(stock, group);

    char *end;
    errno = 0;
    long id = strtol(argv[2], &end, 10);
    if (errno || *end != '\0' || id < 1 || id > 999999) {
        fprintf(stderr, "error: '%s' is not a valid donor ID (1-999999)\n", argv[2]);
        usage(stderr);
        return 2;
    }
    return cmd_register(donors, (int)id, group);
}
