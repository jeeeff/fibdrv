#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "uint128.h"

#define FIB_DEV "/dev/fibonacci"

#define MAX_LENGTH 100

struct BigN {
    unsigned long long lower;
    unsigned long long upper;
};

static inline void addBigN(struct BigN *r, struct BigN *x, struct BigN *y)
{
    // struct BigN *r = (struct BigN *) malloc(sizeof(struct BigN));
    r->upper = x->upper + y->upper;
    if (y->lower > ~x->lower)
        r->upper++;
    r->lower = x->lower + y->lower;
}

static struct BigN *subtractBigN(struct BigN *r, struct BigN *x, struct BigN *y)
{
    // struct BigN *r = (struct BigN *) malloc(sizeof(struct BigN));

    if (x->lower < y->lower) {
        unsigned long long mycarry = ULLONG_MAX;
        r->lower = mycarry + x->lower - y->lower + 1;
        r->upper = x->upper - y->upper - 1;
    } else {
        r->lower = x->lower - y->lower;
        r->upper = x->upper - y->upper;
    }
    return r;
}

static inline struct BigN *lsftBigN(struct BigN *r,
                                    struct BigN *x,
                                    unsigned char shift)
{
    if (shift == 0) {
        r = x;
        return r;
    } else if (shift >= 64) {
        r->upper = x->lower << (shift - 64);
        r->lower = 0ull;
        return r;
    }
    r->upper = x->upper << shift;
    r->lower = x->lower << shift;
    r->upper |= x->lower >> (64 - shift);
    return r;
}

static struct BigN *multiplieBigN(struct BigN *x, struct BigN *y)
{
    struct BigN *r = (struct BigN *) malloc(sizeof(struct BigN));
    r->lower = 0;
    r->upper = 0;

    size_t w = 8 * sizeof(unsigned long long);

    for (size_t i = 0; i < w; i++) {
        if ((y->lower >> i) & 0x1) {
            struct BigN tmp;

            r->upper += x->upper << i;

            tmp.lower = (x->lower << i);
            tmp.upper = i == 0 ? 0 : (x->lower >> (w - i));
            addBigN(r, r, &tmp);
        }
    }

    for (size_t i = 0; i < w; i++) {
        if ((y->upper >> i) & 0x1) {
            r->upper += (x->lower << i);
        }
    }
    return r;
}


static struct BigN fib_sequence(long long k)
{
    /* FIXME: use clz/ctz and fast algorithms to speed up */
    struct BigN f[k + 2];

    f[0].upper = 0;
    f[0].lower = 0;

    f[1].upper = 0;
    f[1].lower = 1;

    for (int i = 2; i <= k; i++) {
        addBigN(&f[i], &f[i - 1], &f[i - 2]);
    }

    return f[k];
}

static struct BigN fast_fib_sequence_wo_clz(long long k)
{
    int bs = 0;
    long long t = k;
    while (t) {
        bs++;
        t >>= 1;
    }

    struct BigN a, b;
    a.upper = 0;
    a.lower = 0;

    b.upper = 0;
    b.lower = 1;

    struct BigN tmp;
    struct BigN t1;
    struct BigN t2;

    for (int i = bs; i >= 1; i--) {
        addBigN(&tmp, &b, &b);
        t1 = *(multiplieBigN(&a, subtractBigN(&tmp, &tmp, &a)));
        // printf("test1 %llu\n", a.lower);
        // struct BigN *a2 = lsftBigN(&tmp, &a, 1);
        // struct BigN *b2 = lsftBigN(&tmp, &b, 1);
        // printf("test2 %llu\n", a2->lower);
        struct BigN *a2 = multiplieBigN(&a, &a);
        struct BigN *b2 = multiplieBigN(&b, &b);
        addBigN(&t2, a2, b2);
        a = t1;
        b = t2;
        if (k & (1 << (i - 1)) && k > 0) {
            addBigN(&t1, &a, &b);
            a = b;
            b = t1;
            k &= ~(1 << (i - 1));
        }
    }
    return a;
}

static long diff_in_ns(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec - t1.tv_nsec < 0) {
        diff.tv_sec = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec * 1000000000.0 + diff.tv_nsec);
}


void get_big_fibnum(char *buf, long long sz)
{
    char lower[MAX_LENGTH] = {0};
    snprintf(lower, sizeof(lower), "%s", buf);

    long long upper = sz;
    int carry = 0;

    char scale[MAX_LENGTH] = {0};
    snprintf(scale, sizeof(scale), "%s", "18446744073709551616");

    for (int i = 0; i < strlen(lower) / 2; i++) {
        char tmp = lower[i];
        lower[i] = lower[strlen(lower) - 1 - i];
        lower[strlen(lower) - 1 - i] = tmp;
    }

    for (int i = 0; i < strlen(scale) / 2; i++) {
        char tmp = scale[i];
        scale[i] = scale[strlen(scale) - 1 - i];
        scale[strlen(scale) - 1 - i] = tmp;
    }

    for (int i = 0; i < MAX_LENGTH; i++) {
        if ((upper == 0 || scale[i] == '\0') && lower[i] == '\0' &&
            carry == 0) {
            break;
        }
        int x = 0, y = 0;
        if (scale[i] != '\0')
            x = upper * (scale[i] - '0');
        if (lower[i] != '\0')
            y = (lower[i] - '0');
        int tmp = x + y + carry;
        buf[i] = (char) (tmp % 10 + '0');
        carry = tmp / 10;
    }

    for (int i = 0; i < strlen(buf) / 2; i++) {
        char tmp = buf[i];
        buf[i] = buf[strlen(buf) - 1 - i];
        buf[strlen(buf) - 1 - i] = tmp;
    }
    return;
}

void display_big_fibnum(int index, struct BigN res)
{
    // printf("%d = %llu  %llu\n", index, res.upper, res.lower);
    char buf[100] = {0};
    snprintf(buf, sizeof(buf), "%llu", res.lower);
    get_big_fibnum(buf, res.upper);
    printf("%d = %s\n", index, buf);
}

static struct BigN fib_ctz(int k)
{
    uint128_t a = {.lower = 0, .upper = 0};
    uint128_t b = {.lower = 1, .upper = 0};
    if (k <= 1) {
        if(k == 0)
	    a = a;
	else
	    a = b;
	struct BigN rs;
        rs.upper = a.upper;
        rs.lower = a.lower;
        return rs;
    }

    int clz = __builtin_clzll(k);

    for (int i = (64 - clz); i > 0; i--) {
        uint128_t t1, t2, tmp;
        lsft128(&tmp, b, 1);   // tmp = 2b
        sub128(&tmp, tmp, a);  // tmp = tmp -a
        mul128(&t1, a, tmp);   // t1 = a*tmp

        mul128(&a, a, a);   // a = a^2
        mul128(&b, b, b);   // b = b^2
        add128(&t2, a, b);  // t2 = a^2 + b^2
        a = t1;
        b = t2;
        if (k & (1ull << (i - 1))) {  // current bit == 1
            add128(&t1, a, b);
            a = b;
            b = t1;
        }
    }

    struct BigN rs;
    rs.upper = a.upper;
    rs.lower = a.lower;
    return rs;
}


int main()
{
    FILE *fp = fopen("./performance", "w");
    char time_buf[100] = {0};

    for (int i = 0; i <= MAX_LENGTH; i++) {
        struct timespec start, stop;

        /* fib_sequence */
        clock_gettime(CLOCK_MONOTONIC, &start);
        struct BigN res1 = fib_sequence(i);
        // display_big_fibnum(i, res1);
        clock_gettime(CLOCK_MONOTONIC, &stop);
        double t1 = diff_in_ns(start, stop);

        /* fast_fib_sequence_wo_clz */
        clock_gettime(CLOCK_MONOTONIC, &start);
        struct BigN res2 = fast_fib_sequence_wo_clz(i);
        clock_gettime(CLOCK_MONOTONIC, &stop);
        display_big_fibnum(i, res2);
        double t2 = diff_in_ns(start, stop);

	/* fib_ctz */
        clock_gettime(CLOCK_MONOTONIC, &start);
        struct BigN res3 = fib_ctz(i);
        clock_gettime(CLOCK_MONOTONIC, &stop);
        display_big_fibnum(i, res3);
        double t3 = diff_in_ns(start, stop);

        // snprintf(time_buf, sizeof(time_buf), "%d %.10lf\n", i, t1);
        snprintf(time_buf, sizeof(time_buf), "%d %.10lf %.10lf %.10lf\n", i, t1, t2, t3);
        fputs(time_buf, fp);
    }
    return 0;
}
