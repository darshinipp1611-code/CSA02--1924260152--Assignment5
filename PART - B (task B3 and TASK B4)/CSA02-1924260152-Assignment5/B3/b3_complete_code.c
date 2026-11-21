#include <stdio.h>

typedef struct { int group_code; int units; } Stock; /* fixed-size binary record */

/* Overwrites the units of record number index directly in the binary file. */
int update_units(FILE *fp, int index, int units)
{
    Stock s;
    if (index < 0 || fseek(fp, (long)index * (long)sizeof(Stock), SEEK_SET) != 0)
        return 0;                                   /* cannot position           */
    if (fread(&s, sizeof s, 1, fp) != 1)
        return 0;                                   /* record does not exist     */
    s.units = units;                                /* change the units          */
    if (fseek(fp, -(long)sizeof(Stock), SEEK_CUR) != 0)
        return 0;                                   /* step back one record      */
    if (fwrite(&s, sizeof s, 1, fp) != 1)
        return 0;                                   /* write the record again    */
    return 1;
}

int main(void)
{
    FILE *fp = tmpfile();
    if (fp == NULL) { perror("tmpfile"); return 1; }
    Stock initial[5] = {{1, 40}, {2, 25}, {3, 12}, {4, 8}, {5, 30}};
    fwrite(initial, sizeof(Stock), 5, fp);
    printf("update record 3: %s\n", update_units(fp, 3, 20) ? "done" : "failed");
    printf("update record 9: %s\n", update_units(fp, 9, 20) ? "done" : "failed");
    rewind(fp);
    Stock s;
    while (fread(&s, sizeof s, 1, fp) == 1) printf("%d:%d ", s.group_code, s.units);
    printf("\n");
    fclose(fp);
    return 0;
}
