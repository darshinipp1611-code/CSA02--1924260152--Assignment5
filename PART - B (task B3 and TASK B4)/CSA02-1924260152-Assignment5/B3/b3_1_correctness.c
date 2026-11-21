/* B3.1 - write 10 donors to a text file and a binary file, read both back and
 * verify that every field is identical to the original data.               */
#include <stdio.h>
#include <stdlib.h>
#include "donor_common.h"

#define N 10

int main(void)
{
    Donor orig[N], from_txt[N], from_bin[N];
    for (int i = 0; i < N; i++) make_donor(&orig[i], i);

    /* ---- text file ---- */
    FILE *ft = fopen("data/donors10.txt", "w");
    if (!ft) { perror("data/donors10.txt"); return 1; }
    for (int i = 0; i < N; i++)
        if (!write_text_record(ft, &orig[i])) { perror("fprintf"); fclose(ft); return 1; }
    fclose(ft);

    /* ---- binary file ---- */
    FILE *fb = fopen("data/donors10.bin", "wb");
    if (!fb) { perror("data/donors10.bin"); return 1; }
    if (fwrite(orig, sizeof(Donor), N, fb) != N) { perror("fwrite"); fclose(fb); return 1; }
    fclose(fb);

    /* ---- read back ---- */
    ft = fopen("data/donors10.txt", "r");
    if (!ft) { perror("data/donors10.txt"); return 1; }
    int nt = 0;
    while (nt < N && read_text_record(ft, &from_txt[nt])) nt++;
    fclose(ft);

    fb = fopen("data/donors10.bin", "rb");
    if (!fb) { perror("data/donors10.bin"); return 1; }
    int nb = (int)fread(from_bin, sizeof(Donor), N, fb);
    fclose(fb);

    printf("records written: %d | read from text: %d | read from binary: %d\n", N, nt, nb);
    printf("sizeof(Donor) = %zu bytes\n\n", sizeof(Donor));
    printf("%-3s %-7s %-14s %-5s %-4s %-11s %-9s | txt  bin\n",
           "#", "ID", "Name", "Group", "Age", "Phone", "LastDon.");
    int bad = 0;
    for (int i = 0; i < N; i++) {
        int okt = donor_equal(&orig[i], &from_txt[i]);
        int okb = donor_equal(&orig[i], &from_bin[i]);
        int okbm = memcmp(&orig[i], &from_bin[i], sizeof(Donor)) == 0; /* byte-for-byte */
        printf("%-3d %-7d %-14s %-5s %-4d %-11s %-9d | %-4s %-4s\n", i + 1,
               from_bin[i].id, from_bin[i].name, from_bin[i].group, from_bin[i].age,
               from_bin[i].phone, from_bin[i].last_date,
               okt ? "OK" : "DIFF", (okb && okbm) ? "OK" : "DIFF");
        if (!okt || !okb || !okbm) bad++;
    }
    printf("\nfile size  text   : %ld bytes (variable-length lines)\n", file_size("data/donors10.txt"));
    printf("file size  binary : %ld bytes (10 x %zu)\n", file_size("data/donors10.bin"), sizeof(Donor));
    if (nt == N && nb == N && bad == 0)
        printf("\nRESULT: PASS - data read back is identical in both formats.\n");
    else
        printf("\nRESULT: FAIL - %d mismatching record(s).\n", bad);
    return bad != 0;
}
