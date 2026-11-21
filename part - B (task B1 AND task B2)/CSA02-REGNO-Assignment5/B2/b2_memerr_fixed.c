/* B2 - the four memory bugs of b2_memerr_buggy.c, FIXED.
 * Usage: ./b2_memerr_fixed overflow | uaf | doublefree | leak | all
 * Build: gcc -std=c11 -Wall -Wextra -O0 -g -o b2_memerr_fixed b2_memerr_fixed.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int id; char group[4]; } Donor;

static void heap_overflow(void)
{
    const char *src = "Ramanathan Iyer";
    char *name = malloc(strlen(src) + 1);         /* FIX: size from the data (+1 for NUL) */
    if (name == NULL) { perror("malloc"); return; }
    memcpy(name, src, strlen(src) + 1);
    printf("name = %s\n", name);
    free(name);
}

static void use_after_free(void)
{
    Donor *d = malloc(sizeof *d);
    if (d == NULL) { perror("malloc"); return; }
    d->id = 7001;
    printf("id = %d\n", d->id);                   /* FIX: use the data before freeing */
    free(d);
    d = NULL;                                     /* FIX: no dangling pointer is left */
}

static void double_free(void)
{
    Donor *d = malloc(sizeof *d);
    if (d == NULL) { perror("malloc"); return; }
    free(d);
    d = NULL;                                     /* FIX: a second free(d) is now free(NULL), a no-op */
    free(d);
    printf("freed once, second free(NULL) harmless\n");
}

static void leak(void)
{
    Donor *list = malloc(100 * sizeof *list);
    if (list == NULL) { perror("malloc"); return; }
    list->id = 1;
    free(list);                                   /* FIX: release before the pointer goes away */
    list = NULL;
    printf("list released\n");
}

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s overflow|uaf|doublefree|leak|all\n", argv[0]); return 2; }
    int all = strcmp(argv[1], "all") == 0, ran = 0;
    if (all || strcmp(argv[1], "overflow")   == 0) { heap_overflow();  ran = 1; }
    if (all || strcmp(argv[1], "uaf")        == 0) { use_after_free(); ran = 1; }
    if (all || strcmp(argv[1], "doublefree") == 0) { double_free();    ran = 1; }
    if (all || strcmp(argv[1], "leak")       == 0) { leak();           ran = 1; }
    if (!ran) { fprintf(stderr, "unknown mode %s\n", argv[1]); return 2; }
    return 0;
}
