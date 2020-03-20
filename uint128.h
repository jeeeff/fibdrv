typedef struct __attribute__((packed)) uint128 {
unsigned long long lower;    
unsigned long long upper;
} uint128_t;

static inline void add_t(uint128_t *out, uint128_t op1, uint128_t op2)
{
    out->upper = op1.upper + op2.upper;
    if (op1.lower > ~op2.lower) {
        out->upper++;
    }
    out->lower = op1.lower + op2.lower;
}

static inline void sub_t(uint128_t *out, uint128_t op1, uint128_t op2)
{
    out->upper = op1.upper - op2.upper;
    if(op1.lower < op2.lower) {
        out->upper--;
    }
    out->lower = op1.lower - op2.lower;
}

static inline void ls_t(uint128_t *out, uint128_t op, unsigned char shift)
{
    if (!shift) {
        *out = op;
        return;
    } else if (shift >= 64) {
        out->upper = op.lower << (shift - 64);
        out->lower = 0ull;
        return;
    }
    out->upper = op.upper << shift;
    out->lower = op.lower << shift;
    out->upper |= op.lower >> (64 - shift);
}

static inline void mul_t(uint128_t *out, uint128_t lo, uint128_t hi)
{
    out->lower = 0ull;
    out->upper = 0ull;
    uint128_t prod;
    unsigned char shft;
    while (lo.lower) {
        shft = __builtin_ctzll(lo.lower);
        ls_t(&prod, hi, shft);
        add_t(out, prod, *out);
        lo.lower &= ~(1ull << shft);
    }
    while (lo.upper) {
        shft = __builtin_ctzll(lo.upper);
        ls_t(&prod, hi, shft + 64);
        add_t(out, prod, *out);
        lo.upper &= ~(1ull << shft);
    }
}

