/* b4_common.h - date helpers and the donor record used by the nightly report */
#ifndef B4_COMMON_H
#define B4_COMMON_H
#include <stdio.h>
#include <stdint.h>
#include <time.h>

/* Days since 1970-01-01 for a civil date (Howard Hinnant's algorithm). */
static inline int days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153u * (unsigned)(m + (m > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int)doe - 719468;
}
static inline void civil_from_days(int z, int *y, int *m, int *d)
{
    z += 719468;
    int era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int yy = (int)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned mp = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = yy + (*m <= 2);
}

#define MIN_AGE 18
#define MAX_AGE 65
#define GAP_DAYS 90              /* minimum days between whole-blood donations */
#define REPORT_WINDOW 7          /* "eligible in the next week"                */

typedef struct {
    int  id;
    char name[32];
    char group[4];
    int  age;
    char phone[16];
    int  last_donation;          /* days since 1970-01-01 */
} Donor;                         /* 64 bytes */

static inline double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}
#endif
