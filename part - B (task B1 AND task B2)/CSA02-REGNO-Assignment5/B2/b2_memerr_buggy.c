/* B2 - memory errors found by Valgrind / AddressSanitizer (BUGGY on purpose).
 * Usage: ./b2_memerr_buggy overflow | uaf | doublefree | leak
 * Build: gcc -std=c11 -Wall -Wextra -O0 -g -o b2_memerr_buggy b2_memerr_buggy.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int id; char group[4]; } Donor;

static void heap_overflow(void)
{
    char *name = malloc(8);                       /* room for 7 chars + NUL */
    strcpy(name, "Ramanathan Iyer");              /* BUG: writes 16 bytes */
    printf("name = %s\n", name);
    free(name);
}

static void use_after_free(void)
{
    Donor *d = malloc(sizeof *d);
    d->id = 7001;
    free(d);
    printf("id = %d\n", d->id);                   /* BUG: dangling pointer read */
}

static void double_free(void)
{
    Donor *d = malloc(sizeof *d);
    free(d);
    free(d);                                      /* BUG: freed twice */
}

static void leak(void)
{
    Donor *list = malloc(100 * sizeof *list);
    list->id = 1;
    list = NULL;                                  /* BUG: last pointer lost, block never freed */
}

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s overflow|uaf|doublefree|leak\n", argv[0]); return 2; }
    if      (strcmp(argv[1], "overflow")   == 0) heap_overflow();
    else if (strcmp(argv[1], "uaf")        == 0) use_after_free();
    else if (strcmp(argv[1], "doublefree") == 0) double_free();
    else if (strcmp(argv[1], "leak")       == 0) leak();
    else { fprintf(stderr, "unknown mode %s\n", argv[1]); return 2; }
    return 0;
}
