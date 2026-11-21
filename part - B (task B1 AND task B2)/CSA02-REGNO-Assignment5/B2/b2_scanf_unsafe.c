/* B2 (part 1) - DELIBERATELY UNSAFE: scanf("%s") has no length limit.
 * The name array holds 9 characters + the terminating NUL.  The age field
 * sits right after it in the same structure, so an over-long name silently
 * overwrites the age; a much longer one wrecks the stack.
 * Build: gcc -std=c11 -Wall -Wextra -O0 -g -o b2_scanf_unsafe b2_scanf_unsafe.c
 */
#include <stdio.h>
#include <string.h>

typedef struct {
    char name[10];
    int  age;
} Donor;

int main(void)
{
    Donor d;
    d.age = 25;

    printf("Enter donor name (array holds 9 characters + NUL): ");
    if (scanf("%s", d.name) != 1) {              /* NO width limit: unsafe */
        fprintf(stderr, "no input\n");
        return 1;
    }
    printf("stored name = \"%s\"  (strlen = %zu, array size = %zu)\n",
           d.name, strlen(d.name), sizeof d.name);
    printf("age         = %d   (it was set to 25 before the read)\n", d.age);
    return 0;
}
