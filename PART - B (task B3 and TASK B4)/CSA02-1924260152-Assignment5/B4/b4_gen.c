/* b4_gen.c - writes a deterministic donor text file:  id,name,group,age,phone,yyyy-mm-dd
 * usage: b4_gen <number_of_donors> <output_file>                                      */
#include <stdio.h>
#include <stdlib.h>
#include "b4_common.h"

static uint64_t rng = 88172645463325252ULL;
static unsigned next_rand(void)
{
    rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
    return (unsigned)(rng >> 33);
}

int main(int argc, char *argv[])
{
    if (argc != 3) { fprintf(stderr, "usage: %s <number_of_donors> <output_file>\n", argv[0]); return 1; }
    char *end;
    long n = strtol(argv[1], &end, 10);
    if (*end != '\0' || n <= 0 || n > 50000000L) { fprintf(stderr, "invalid donor count '%s'\n", argv[1]); return 1; }
    FILE *f = fopen(argv[2], "w");
    if (!f) { perror(argv[2]); return 1; }

    static const char *FIRST[] = {"Asha","Ravi","Meena","Karthik","Divya","Arun","Nila","Suresh"};
    static const char *LAST[]  = {"Kumar","Devi","Raj","Nair","Iyer","Rao","Das","Pillai"};
    static const char *G[] = {"O+","A+","B+","AB+","O-","A-","B-","AB-"};
    const int today = days_from_civil(2026, 11, 21);

    for (long i = 0; i < n; i++) {
        int age  = 16 + (int)(next_rand() % 55);              /* 16..70 : some are out of range */
        int last = today - (int)(next_rand() % 720);          /* donated within the last 2 years */
        int y, m, d;
        civil_from_days(last, &y, &m, &d);
        if (fprintf(f, "%ld,%s_%s,%s,%d,9%09u,%04d-%02d-%02d\n", 700001 + i,
                    FIRST[next_rand() % 8], LAST[next_rand() % 8], G[next_rand() % 8],
                    age, next_rand() % 1000000000u, y, m, d) < 0) { perror("fprintf"); fclose(f); return 1; }
    }
    if (fclose(f) != 0) { perror("fclose"); return 1; }
    printf("wrote %ld donors to %s\n", n, argv[2]);
    return 0;
}
