/* B3 Alternative concept - one fwrite per record vs one fwrite for the whole
 * array of stock records.  Counts library calls, measures time and memory.   */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct { int group_code; int units; } Stock;

static double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

int main(void)
{
    const int sizes[] = {1000, 100000, 5000000};
    printf("%-10s | %-26s | %-26s | %s\n", "n records",
           "per-record fwrite (calls/ms)", "single fwrite (calls/ms)", "array memory");
    for (int s = 0; s < 3; s++) {
        int n = sizes[s];
        Stock *arr = malloc((size_t)n * sizeof(Stock));
        if (!arr) { perror("malloc"); return 1; }
        for (int i = 0; i < n; i++) { arr[i].group_code = i % 8; arr[i].units = i % 50; }

        long calls1 = 0, calls2 = 0;
        double best1 = 1e18, best2 = 1e18;
        for (int rep = 0; rep < 5; rep++) {
            FILE *f = fopen("data/stock_per_record.bin", "wb");
            if (!f) { perror("fopen"); return 1; }
            double t0 = now_ms();
            calls1 = 0;
            for (int i = 0; i < n; i++) { if (fwrite(&arr[i], sizeof(Stock), 1, f) != 1) { perror("fwrite"); return 1; } calls1++; }
            fclose(f);
            double t = now_ms() - t0;
            if (t < best1) best1 = t;

            f = fopen("data/stock_bulk.bin", "wb");
            if (!f) { perror("fopen"); return 1; }
            t0 = now_ms();
            calls2 = 0;
            if (fwrite(arr, sizeof(Stock), (size_t)n, f) != (size_t)n) { perror("fwrite"); return 1; }
            calls2++;
            fclose(f);
            t = now_ms() - t0;
            if (t < best2) best2 = t;
        }
        /* verify both files are byte-identical in size */
        FILE *a = fopen("data/stock_per_record.bin", "rb"), *b = fopen("data/stock_bulk.bin", "rb");
        long same = 1, ca = 0, cb = 0;
        while (a && b && (ca = fgetc(a)) == (cb = fgetc(b))) { if (ca == EOF) break; }
        same = (ca == cb);
        if (a) fclose(a);
        if (b) fclose(b);
        printf("%-10d | %10ld calls %8.3f ms | %10ld call  %8.3f ms | %.1f KB  (files identical: %s)\n",
               n, calls1, best1, calls2, best2, n * (double)sizeof(Stock) / 1024.0, same ? "yes" : "NO");
        free(arr);
    }
    return 0;
}
