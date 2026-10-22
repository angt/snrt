#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
must(int cond, const char *what) {
    if (!cond) {
        fprintf(stderr, "gen_consts: FAILED: %s\n", what);
        exit(1);
    }
}

typedef enum {
    KIND_IEEE,     // inf and nan at the all-ones exponent
    KIND_FN,       // e4m3fn saturates and has one nan pattern
    KIND_FINITE    // e2m1 is finite only and saturates
} fmt_kind;

static const struct {
    const char *name;
    int ebits, sbits;
    fmt_kind kind;
} formats[] = {
    { "F64",  11, 52, KIND_IEEE },     // IEEE binary64
    { "F32",   8, 23, KIND_IEEE },     // IEEE binary32
    { "F16",   5, 10, KIND_IEEE },     // IEEE binary16
    { "BF16",  8,  7, KIND_IEEE },     // bfloat16
    { "E5M2",  5,  2, KIND_IEEE },     // OCP float8 e5m2
    { "E4M3",  4,  3, KIND_FN },       // OCP float8 e4m3fn
    { "E2M1",  2,  1, KIND_FINITE },   // OCP float4 e2m1
};

enum { NFORMATS = (int)(sizeof formats / sizeof *formats) };

enum {
    POW5_MAX = 399,
    MAX_STRUCT_CHARS = 32,
    GUARD_SLACK = 8
};

enum { BIG_MAX = 48 }; // 5^399 is 29 digits, with room to spare

typedef struct {
    uint32_t d[BIG_MAX];
    int n;
} big;

static void
big_mul_small(big *b, unsigned m) {
    uint64_t carry = 0;
    for (int i = 0; i < b->n; i++) {
        uint64_t t = (uint64_t)b->d[i] * m + carry;
        b->d[i] = (uint32_t)t;
        carry = t >> 32;
    }
    while (carry > 0) {
        must(b->n < BIG_MAX, "base 2^32 number out of room");
        b->d[b->n++] = (uint32_t)carry;
        carry >>= 32;
    }
}

static int
big_cmp(const big *a, const big *b) {
    int n = a->n > b->n ? a->n : b->n;
    for (int i = n - 1; i >= 0; i--) {
        uint32_t x = i < a->n ? a->d[i] : 0;
        uint32_t y = i < b->n ? b->d[i] : 0;
        if (x != y) {
            return x < y ? -1 : 1;
        }
    }
    return 0;
}

static void
big_sub(big *a, const big *b) {
    uint64_t borrow = 0;
    for (int i = 0; i < a->n; i++) {
        uint64_t y = (uint64_t)(i < b->n ? b->d[i] : 0) + borrow;
        borrow = a->d[i] < y;
        a->d[i] = (uint32_t)(a->d[i] - y);
    }
}

static void
big_shl1(big *b) {
    uint64_t carry = 0;
    for (int i = 0; i < b->n; i++) {
        uint64_t t = (uint64_t)b->d[i] << 1 | carry;
        b->d[i] = (uint32_t)t;
        carry = t >> 32;
    }
    if (carry) {
        must(b->n < BIG_MAX, "base 2^32 number out of room");
        b->d[b->n++] = (uint32_t)carry;
    }
}

static int
big_bitlen(const big *b) {
    int n = b->n;
    while (n > 0 && b->d[n - 1] == 0) {
        n--;
    }
    if (n == 0) {
        return 0;
    }
    uint32_t top = b->d[n - 1];
    int bits = 32 * (n - 1);
    while (top) {
        top >>= 1;
        bits++;
    }
    return bits;
}

static void
floor_pow2_div(const big *v, int bits, uint64_t *hi, uint64_t *lo) {
    big r = { .n = 1 };
    uint64_t qh = 0, ql = 0;
    for (int i = bits; i >= 0; i--) {
        big_shl1(&r);
        if (i == bits) {
            r.d[0] |= 1;
        }
        qh = (qh << 1) | (ql >> 63);
        ql <<= 1;
        if (big_cmp(&r, v) >= 0) {
            big_sub(&r, v);
            ql |= 1;
        }
    }
    *hi = qh;
    *lo = ql;
}

static void
big_bits_at(const big *b, int s, uint64_t *hi, uint64_t *lo) {
    uint64_t l = 0, h = 0;
    for (int i = 0; i < 128; i++) {
        int pos = s + i;
        if (pos >= 0 && pos / 32 < b->n && ((b->d[pos / 32] >> (pos % 32)) & 1)) {
            if (i < 64) {
                l |= (uint64_t)1 << i;
            } else {
                h |= (uint64_t)1 << (i - 64);
            }
        }
    }
    *hi = h;
    *lo = l;
}

static int
pow5_limbs(int rows) {
    big p = { .d = { 1 }, .n = 1 };
    for (int q = 0; q < rows; q++) {
        big_mul_small(&p, 5);
    }
    return (p.n + 1) / 2;
}

static unsigned
max_pow_u64(unsigned base) {
    uint64_t v = 1;
    unsigned q = 0;
    while (v <= UINT64_MAX / base) {
        v *= base;
        q++;
    }
    return q;
}

static void
downcase_name(const char *in, char *out) {
    int i = 0;
    for (; in[i]; i++) {
        char c = in[i];
        out[i] = c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
    }
    out[i] = '\0';
}

static unsigned
floor_pos(double x) {
    return (unsigned)x;
}

static unsigned
ceil_pos(double x) {
    unsigned f = floor_pos(x);
    return (double)f == x ? f : f + 1;
}

static int
dec_digits(uint64_t v) {
    int n = 1;
    while (v >= 10) {
        v /= 10;
        n++;
    }
    return n;
}

static int
cmp_x_edge(uint64_t c, int t, uint64_t odd, int sh) {
    big a = { .d = { (uint32_t)c }, .n = 1 };
    big b = { .d = { (uint32_t)odd }, .n = 1 };
    if (t > 0) {
        for (int i = 0; i < t; i++) {
            big_mul_small(&a, 5);
        }
    } else {
        for (int i = 0; i < -t; i++) {
            big_mul_small(&b, 5);
        }
    }
    if (t >= sh) {
        for (int i = 0; i < t - sh; i++) {
            big_shl1(&a);
        }
    } else {
        for (int i = 0; i < sh - t; i++) {
            big_shl1(&b);
        }
    }
    return big_cmp(&a, &b);
}

static int
rounds_to(uint64_t c, int t, uint64_t m, int E, int binade_low) {
    uint64_t lo = binade_low ? 4 * m - 1 : 4 * m - 2;
    int a = cmp_x_edge(c, t, lo, E - 2);
    if (a < 0 || (a == 0 && (m & 1))) {
        return 0;
    }
    int b = cmp_x_edge(c, t, 4 * m + 2, E - 2);
    return b < 0 || (b == 0 && !(m & 1));
}

static void
fmt_text(uint64_t d, int e, char *out) {
    while (d >= 10 && d % 10 == 0) {
        d /= 10;
        e++;
    }
    int n = dec_digits(d);
    int x = e + n - 1;
    int a = x < 0 ? -x : x;
    int ed = dec_digits((uint64_t)a);
    int sci_len = n + (n > 1 ? 1 : 0) + 1 + (x < 0 ? 1 : 0) + ed;
    int fixed_len;
    if (x >= 0) {
        fixed_len = n <= x + 1 ? x + 1 : n + 1;
    } else {
        fixed_len = 2 + (-x - 1) + n;
    }
    char digs[32];
    sprintf(digs, "%llu", (unsigned long long)d);
    char *w = out;
    if (sci_len < fixed_len) {
        *w++ = digs[0];
        if (n > 1) {
            *w++ = '.';
            for (int i = 1; i < n; i++) {
                *w++ = digs[i];
            }
        }
        *w++ = 'e';
        if (x < 0) {
            *w++ = '-';
        }
        sprintf(w, "%d", a);
        return;
    }
    if (x >= 0 && n <= x + 1) {
        for (int i = 0; i < n; i++) {
            *w++ = digs[i];
        }
        for (int i = n; i <= x; i++) {
            *w++ = '0';
        }
    } else if (x >= 0) {
        for (int i = 0; i <= x; i++) {
            *w++ = digs[i];
        }
        *w++ = '.';
        for (int i = x + 1; i < n; i++) {
            *w++ = digs[i];
        }
    } else {
        *w++ = '0';
        *w++ = '.';
        for (int i = 0; i < -x - 1; i++) {
            *w++ = '0';
        }
        for (int i = 0; i < n; i++) {
            *w++ = digs[i];
        }
    }
    *w = '\0';
}

static void
pattern_text(int ebits, int sbits, fmt_kind kind, unsigned u, char *out) {
    int bias = (1 << (ebits - 1)) - 1;
    unsigned sign = (u >> (ebits + sbits)) & 1;
    unsigned expf = (u >> sbits) & ((1u << ebits) - 1);
    unsigned frac = u & ((1u << sbits) - 1);
    unsigned emaxf = (1u << ebits) - 1;
    unsigned smask = (1u << sbits) - 1;
    if (kind == KIND_IEEE && expf == emaxf) {
        if (frac) {
            memcpy(out, "nan", sizeof "nan");
        } else if (sign) {
            memcpy(out, "-inf", sizeof "-inf");
        } else {
            memcpy(out, "inf", sizeof "inf");
        }
        return;
    }
    if (kind == KIND_FN && expf == emaxf && frac == smask) {
        memcpy(out, "nan", sizeof "nan");
        return;
    }
    uint64_t m;
    int E;
    if (!expf) {
        m = frac;
        E = 1 - bias - sbits;
    } else {
        m = frac | (1u << sbits);
        E = (int)expf - bias - sbits;
    }
    if (!m) {
        if (sign) {
            memcpy(out, "-0", sizeof "-0");
        } else {
            memcpy(out, "0", sizeof "0");
        }
        return;
    }
    double v = ldexp((double)m, E);
    int binade_low = expf >= 2 && m == ((uint64_t)1 << sbits);
    char *w = out;
    if (sign) {
        *w++ = '-';
    }
    for (int k = 1; k <= 7; k++) {
        char tmp[64];
        snprintf(tmp, sizeof tmp, "%.*e", k - 1, v);
        uint64_t n0 = 0;
        int q0 = 0;
        const char *p = tmp;
        for (; *p != 'e'; p++) {
            if (*p >= '0' && *p <= '9') {
                n0 = n0 * 10 + (uint64_t)(*p - '0');
            }
        }
        p++;
        int negq = *p == '-';
        if (*p == '-' || *p == '+') {
            p++;
        }
        for (; *p >= '0' && *p <= '9'; p++) {
            q0 = q0 * 10 + (*p - '0');
        }
        if (negq) {
            q0 = -q0;
        }
        int t = q0 - k + 1;
        const uint64_t cs[3] = { n0, n0 - 1, n0 + 1 };
        for (int i = 0; i < 3; i++) {
            if (rounds_to(cs[i], t, m, E, binade_low)) {
                fmt_text(cs[i], t, w);
                return;
            }
        }
    }
    must(0, "a pattern has no round-trip text");
}

int
main(void) {
    double log10_2 = log10(2.0);
    double log2_10 = log2(10.0);

    unsigned log10_2_q18 = floor_pos(262144.0 * log10_2);
    unsigned log10_5_q20 = floor_pos(1048576.0 * log10(5.0));
    unsigned log2_5_q19 = floor_pos(524288.0 * log2(5.0));
    unsigned log2_10_q29 = floor_pos(536870912.0 * log2_10);

    unsigned bias = (unsigned)((1 << (formats[0].ebits - 1)) - 1);
    unsigned sbits = (unsigned)formats[0].sbits;

    unsigned digits_max = ceil_pos((sbits + 1) * log10_2) + 1;
    unsigned exp_overflow = floor_pos((bias + 1) * log10_2);
    int exp_underflow = -(int)ceil_pos((bias + sbits) * log10_2);
    int digits_kept = 0;

    for (int s = exp_underflow + 1; s <= (int)exp_overflow + 1; s++) {
        double e = floor((s - 1) * log2_10);
        if (e < 1 - (double)bias) {
            e = 1 - (double)bias;
        }
        int k = (int)sbits + 1 + s - (int)e;
        if (k > digits_kept) {
            digits_kept = k;
        }
    }
    int pow5_t_max = digits_kept - exp_underflow - 1;

    must(POW5_MAX >= (unsigned)-exp_underflow + MAX_STRUCT_CHARS + GUARD_SLACK,
         "POW5_MAX does not cover the scaling bounds in snrt.c");
    must(exp_overflow < (unsigned)POW5_MAX,
         "POW5_MAX does not cover the s side of scale_bn");

    unsigned pow5_u64_max = max_pow_u64(5);
    unsigned pow10_u64_max = max_pow_u64(10);
    int pow5_row_max = -exp_underflow + (int)pow10_u64_max + 1;
    int limbs = pow5_limbs(pow5_row_max);

    FILE *f = fopen("consts.h", "w");
    must(f != NULL, "cannot create consts.h");

    fprintf(f, "// generated by gen_consts.c -- do not edit" "\n"
                                                             "\n"
               "#ifndef CONSTS_H"                            "\n"
               "#define CONSTS_H"                            "\n"
                                                             "\n"
               "#include <stdint.h>"                         "\n"
                                                             "\n");

    for (int i = 0; i < NFORMATS; i++) {
        fprintf(f, "enum {"                                    "\n"
                   "    %s_EBITS = %d,"                        "\n"
                   "    %s_SBITS = %d,"                        "\n"
                   "    %s_BIAS  = (1 << (%s_EBITS - 1)) - 1," "\n"
                   "};"                                        "\n"
                                                               "\n",
                formats[i].name, formats[i].ebits,
                formats[i].name, formats[i].sbits,
                formats[i].name, formats[i].name);
    }
    fprintf(f, "enum {"                     "\n"
               "    POW5_U64_MAX    = %u,"  "\n"
               "    POW5_ROW_MAX    = %d,"  "\n"
               "    POW5_MAX        = %d,"  "\n"
               "    POW5_T_MAX      = %d,"  "\n"
               "    POW5_LIMBS      = %d,"  "\n"
               "    POW10_U64_MAX   = %u,"  "\n"
               "    DIGITS_KEPT     = %d,"  "\n"
               "    DEC_DIGITS_MAX  = %u,"  "\n"
               "    DEC_DIGITS_WORK = %u,"  "\n"
               "    EXP_OVERFLOW    = %u,"  "\n"
               "    EXP_UNDERFLOW   = %d,"  "\n"
               "    LOG10_2_Q18     = %u,"  "\n"
               "    LOG10_5_Q20     = %u,"  "\n"
               "    LOG2_5_Q19      = %u,"  "\n"
               "    LOG2_10_Q29     = %u,"  "\n"
               "};"                         "\n"
                                            "\n",
            pow5_u64_max, pow5_row_max, POW5_MAX, pow5_t_max, limbs,
            pow10_u64_max,
            digits_kept, digits_max, digits_max + 1, exp_overflow,
            exp_underflow, log10_2_q18, log10_5_q20, log2_5_q19,
            log2_10_q29);

    fprintf(f, "static const double LOG10_2 = %.17g;\n"
               "static const double LOG2_10 = %.17g;\n"
                                                   "\n",
            log10_2, log2_10);

    fprintf(f, "static const uint64_t pow10_u64[POW10_U64_MAX + 1] = {\n");
    for (unsigned k = 0; k <= pow10_u64_max; k++) {
        uint64_t v = 1;
        for (unsigned i = 0; i < k; i++) {
            v *= 10;
        }
        fprintf(f, "    %lluull,\n", (unsigned long long)v);
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const char dig2[] =\n");
    for (int i = 0; i < 100; i += 25) {
        fprintf(f, "    \"");
        for (int j = i; j < i + 25; j++) {
            fprintf(f, "%02d", j);
        }
        fprintf(f, "\"%s\n", i + 25 < 100 ? "" : ";");
    }
    fprintf(f, "\n");

    for (int fi = 0; fi < NFORMATS; fi++) {
        int width = formats[fi].ebits + formats[fi].sbits + 1;
        if (width > 8) {
            continue;
        }
        unsigned npat = 1u << width;
        char name[16];
        downcase_name(formats[fi].name, name);
        char rows[256][40];
        unsigned w = 1;
        for (unsigned u = 0; u < npat; u++) {
            pattern_text(formats[fi].ebits, formats[fi].sbits,
                         formats[fi].kind, u, rows[u]);
            unsigned len = (unsigned)strlen(rows[u]) + 1;
            if (len > w) {
                w = len;
            }
        }
        fprintf(f, "static const char %s_text[%u][%u] = {\n", name, npat, w);
        for (unsigned u = 0; u < npat; u++) {
            fprintf(f, "    \"%s\",\n", rows[u]);
        }
        fprintf(f, "};\n\n");
    }
    big p = { .d = { 1 }, .n = 1 };

    static unsigned off_buf[POW5_MAX + 2];
    int pack_n = 0;
    int col = 0;

    fprintf(f, "static const uint64_t pow5_u64[POW5_U64_MAX + 1] = {\n");
    {
        uint64_t v = 1;
        for (unsigned k = 0; k <= pow5_u64_max; k++) {
            fprintf(f, "    0x%016llxull,\n", (unsigned long long)v);
            v *= 5;
        }
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const uint64_t pow5_limbs[] = {\n");
    for (int q = 0; q <= pow5_row_max; q++) {
        off_buf[q] = (unsigned)pack_n;
        int row = (p.n + 1) / 2;
        must(row <= limbs, "a pow5 row is wider than POW5_LIMBS");
        for (int i = 0; i < row; i++) {
            uint64_t lo = (2 * i < p.n) ? p.d[2 * i] : 0;
            uint64_t hi = (2 * i + 1 < p.n) ? p.d[2 * i + 1] : 0;
            if (col == 0) {
                fprintf(f, "   ");
            }
            fprintf(f, " 0x%016llxull,", (unsigned long long)(lo | hi << 32));
            pack_n++;
            col = (col + 1) % 4;
            if (col == 0) {
                fprintf(f, "\n");
            }
        }
        big_mul_small(&p, 5);
    }
    if (col) {
        fprintf(f, "\n");
    }
    fprintf(f, "};\n\n");
    off_buf[pow5_row_max + 1] = (unsigned)pack_n;
    must(pack_n <= 0xFFFF, "a packed pow5 offset does not fit u16");

    fprintf(f, "static const uint16_t pow5_off[POW5_ROW_MAX + 2] = {\n");
    for (int i = 0; i <= pow5_row_max + 1; i += 12) {
        fprintf(f, "   ");
        for (int j = i; j <= pow5_row_max + 1 && j < i + 12; j++) {
            fprintf(f, " %u,", off_buf[j]);
        }
        fprintf(f, "\n");
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const uint64_t pow5_split[POW5_ROW_MAX + 1][2] = {\n");
    big ps = { .d = { 1 }, .n = 1 };
    for (int i = 0; i <= pow5_row_max; i++) {
        uint64_t hi = 0, lo = 0;
        big_bits_at(&ps, big_bitlen(&ps) - 125, &hi, &lo);
        fprintf(f, "    { 0x%016llxull, 0x%016llxull },\n",
                (unsigned long long)lo, (unsigned long long)hi);
        big_mul_small(&ps, 5);
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const uint64_t pow5_recip[POW5_ROW_MAX + 1][2] = {\n");
    big qr = { .d = { 1 }, .n = 1 };
    for (int t = 0; t <= pow5_row_max; t++) {
        uint64_t hi = 0, lo = 0;
        floor_pow2_div(&qr, big_bitlen(&qr) + 124, &hi, &lo);
        lo += 1;
        hi += (lo == 0);
        fprintf(f, "    { 0x%016llxull, 0x%016llxull },\n",
                (unsigned long long)lo, (unsigned long long)hi);
        big_mul_small(&qr, 5);
    }
    fprintf(f, "};\n\n");

    uint64_t inv5 = 1;
    for (int i = 0; i < 6; i++) {
        inv5 *= 2 - 5 * inv5;
    }
    fprintf(f, "static const uint64_t INV5_MOD64 = 0x%016llxull;" "\n"
               "static const uint64_t U64_MAX_DIV5 = %lluull;"    "\n"
                                                                  "\n",
            (unsigned long long)inv5,
            (unsigned long long)(UINT64_MAX / 5));

    fprintf(f, "#endif\n");
    must(fclose(f) == 0, "writing consts.h");
    return 0;
}
