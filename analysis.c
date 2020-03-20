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

#define MAX_LENGTH 150

struct BigN {
    unsigned long long lower;
    unsigned long long upper;
//    unsigned long long top;
};

static inline void addBigN(struct BigN *r, struct BigN *x, struct BigN *y)
{
    // struct BigN *r = (struct BigN *) malloc(sizeof(struct BigN));
    r->upper = x->upper + y->upper;
    if (y->lower > ~x->lower)
        r->upper++;
    r->lower = x->lower + y->lower;
//    r->top = x->top + y->top;
//    if (y->upper > ~x->upper)
//	r->top++;
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

//    f[0].top = 0;
    f[0].upper = 0;
    f[0].lower = 0;

//    f[1].top = 0;
    f[1].upper = 0;
    f[1].lower = 1;

    for (int i = 2; i <= k; i++) {
        addBigN(&f[i], &f[i - 1], &f[i - 2]);
    }

    return f[k];
}

static struct BigN fib_3(int k)
{
    struct BigN t1,t2,t3;
//    t1.top = 0;
    t1.upper = 0;
    t1.lower = 1;
//    t2.top = 0;
    t2.upper = 0;
    t2.lower = 0;

    for(int i=2;i<=k;i++)
    {
        addBigN(&t3,&t1,&t2);
        t2 = t1;
        t1 = t3;
    }

    return t3;
}

/*static struct BigN fib_3(int k)
{
    struct uint128_t t1,t2,t3;
    t1.upper = 0;
    t1.lower = 1;
    t2.upper = 0;
    t2.lower = 0;

    for(int i=2;i<=k;i++)
    {
        add128(&t3,t1,t2);
        t2 = t1;
        t1 = t3;
        //printf("%d : %llx%llx\n",i , t3.upper, t3.lower);
    }

    struct BigN rs;
    rs.upper = t3.upper;
    rs.lower = t3.lower;
    return rs;
}*/


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
    char buf[1000] = {0};
    snprintf(buf, sizeof(buf), "%llu", res.lower);
    get_big_fibnum(buf, res.upper);
    printf("%d = %s\n", index, buf);
}

static uint128_t fib_ctz(int k)
{
    uint128_t a = {.lower = 0, .upper = 0};
    uint128_t b = {.lower = 1, .upper = 0};
    if (k <= 1) {
        if(k == 0)
	    a = a;
	else
	    a = b;
        return a;
    }

    int clz = __builtin_clzll(k);

    for (int i = (64 - clz); i > 0; i--) {
        uint128_t t1, t2, tmp;
        ls_t(&tmp, b, 1);   // tmp = 2b
        sub_t(&tmp, tmp, a);  // tmp = tmp -a
        mul_t(&t1, a, tmp);   // t1 = a*tmp

        mul_t(&a, a, a);   // a = a^2
        mul_t(&b, b, b);   // b = b^2
        add_t(&t2, a, b);  // t2 = a^2 + b^2
        a = t1;
        b = t2;
        if (k & (1ull << (i - 1))) {  // current bit == 1
            add_t(&t1, a, b);
            a = b;
            b = t1;
        }
    }

    return a;
}

void fib_fast_logn(int k, uint128_t *ans0, uint128_t *ans1)
{
    if (k == 0) {
        ans0->upper = 0llu;
	ans0->lower = 0llu;
	ans1->upper = 0llu;
	ans1->lower = 1llu;
    	return;
    }

    fib_fast_logn((k/2), ans0, ans1);
    uint128_t a = *ans0;
    uint128_t b = *ans1;
//    printf("*********%d************ %llu %llu\n",k ,a.upper, a.lower);

    uint128_t b2; 
    uint128_t c = {.upper = 0llu, .lower = 0llu};
    uint128_t d = {.upper = 0llu, .lower = 0llu};
    add_t(&b2,b,b);
    sub_t(&c,b2,a);

    //printf("*********%d************ %llu %llu\n",k ,b2.upper, b2.lower); 
    uint128_t cpa = {.upper = 0llu, .lower = 0llu};
    mul_t(&cpa,a,c);
    mul_t(&a,a,a);
    mul_t(&b,b,b);
    add_t(&d,a,b);

    if(k%2) {
        *ans0 = d;
	add_t(ans1,cpa,d);
    } else {
	*ans0 = cpa;
	*ans1 = d;
    }
    //printf("*********%d************ %llu %llu\n",k ,c.upper, c.lower);
}
    
#define count 10000

int main()
{
    FILE *fp = fopen("./performance", "w");
    char time_buf[500] = {0};

    double t1_rs, t2_rs, t3_rs;

    for (int i = 1; i <= MAX_LENGTH; i++) {
        struct timespec start, stop;

	t1_rs = 0;
	t2_rs = 0;
	t3_rs = 0;

        double t1, t2, t3;

        /* fib_sequence */
        for(int j = 0; j < count; j++) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            struct BigN res1 = fib_sequence(i);
            if(j == 0)
	    	display_big_fibnum(i, res1);
            clock_gettime(CLOCK_MONOTONIC, &stop);
            t1 = diff_in_ns(start, stop);
	    t1_rs += t1;
        }

        /* fib_ctz */
	for(int j = 0; j < count; j++) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            uint128_t x = fib_ctz(i);
            clock_gettime(CLOCK_MONOTONIC, &stop);

	    struct BigN res2;
            res2.upper = x.upper;
	    res2.lower = x.lower;

	    if(j == 0)
	        display_big_fibnum(i, res2);
            t2 = diff_in_ns(start, stop);
	    t2_rs += t2;
	}

	/* fast_logn */
        for(int j = 0; j < count; j++) {
            clock_gettime(CLOCK_MONOTONIC, &start);
	    uint128_t ans0;
            ans0.upper = 0llu;
            ans0.lower = 0llu;
	    uint128_t ans1;
            ans0.upper = 0llu;
            ans0.lower = 0llu;
            fib_fast_logn(i, &ans0, &ans1);
            clock_gettime(CLOCK_MONOTONIC, &stop);

	    struct BigN res3;
	    res3.upper = ans0.upper;
	    res3.lower = ans0.lower;

            if(j == 0)
                display_big_fibnum(i, res3);
            t3 = diff_in_ns(start, stop);
            t3_rs += t3;
            //printf("%d : %llu %llu\n", i, ans0.upper, ans0.lower);  
        }

        /* fib_3  */
        /*for(int j = 0; j < count; j++) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            struct BigN res3 = fib_3(i);
            clock_gettime(CLOCK_MONOTONIC, &stop);
            if(j == 0)
                display_big_fibnum(i, res3);
            t3 = diff_in_ns(start, stop);
            t3_rs += t3;
        }*/

//        t1_rs /= count;
//	t2_rs /= count;
        t3_rs /= count;
	snprintf(time_buf, sizeof(time_buf), "%d %.10lf %.10lf %.10lf\n", i, t1, t2, t3);
        fputs(time_buf, fp);
    }

    return 0;
}
