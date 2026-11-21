/* B2 (part 1) - SAFE version: fgets() with the real array size.
 * read_name() never writes past the array, detects an over-long line
 * (no '\n' in the buffer), discards the rest of that line so the next read
 * starts cleanly, and reports the problem to the caller.
 * Build: gcc -std=c11 -Wall -Wextra -O0 -g -o b2_fgets_safe b2_fgets_safe.c
 */
#include <stdio.h>
#include <string.h>

typedef struct {
    char name[10];
    int  age;
} Donor;

/* returns  1 = name read,  0 = end of input,  -1 = line too long (rest discarded) */
static int read_name(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {       /* whole line fitted */
        buf[len - 1] = '\0';
        return 1;
    }
    int c = fgetc(stdin);                        /* line exactly filled the buffer? */
    if (c == '\n' || c == EOF) return 1;
    while (c != '\n' && c != EOF) c = fgetc(stdin);   /* discard the remainder */
    return -1;
}

int main(void)
{
    Donor d;
    d.age = 25;
    int n = 0, rc;

    for (;;) {
        printf("Enter donor name %d (max %zu characters): ", ++n, sizeof d.name - 1);
        fflush(stdout);
        rc = read_name(d.name, sizeof d.name);
        if (rc == 0) { printf("\n(end of input)\n"); break; }
        if (rc < 0) {
            printf("REJECTED: name is longer than %zu characters, rest of line discarded\n",
                   sizeof d.name - 1);
            continue;
        }
        printf("accepted : name = \"%s\" (strlen = %zu), age = %d (unchanged)\n",
               d.name, strlen(d.name), d.age);
    }
    return 0;
}
