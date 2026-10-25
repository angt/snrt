#undef SNRT_NO_SHORT_NAMES
#include "snrt.h"

#include "consts.h"

#include <limits.h>
#include <string.h>

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if __has_builtin(__builtin_expect)
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(always_inline)
#define force_inline inline __attribute__((always_inline))
#else
#define force_inline inline
#endif

#if __has_attribute(noinline)
#define no_inline __attribute__((noinline))
#else
#define no_inline
#endif

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

#if defined(__SIZEOF_INT128__) && !defined(SNRT_NO_INT128)
#define SNRT_HAS_U128 1
#else
#define SNRT_HAS_U128 0
#endif

#if __has_builtin(__builtin_clzll) && !defined(SNRT_NO_CLZLL)
#define SNRT_HAS_CLZLL 1
#else
#define SNRT_HAS_CLZLL 0
#endif

#if __has_builtin(__builtin_ctzll) && !defined(SNRT_NO_CLZLL)
#define SNRT_HAS_CTZLL 1
#else
#define SNRT_HAS_CTZLL 0
#endif

typedef struct {
    u64 hi;
    u64 lo;
} wide;

typedef struct {
    u64 q;
    u64 r;
} qr;

#if SNRT_HAS_U128

typedef __uint128_t u128;

static wide
mul64(u64 a, u64 b) {
    u128 p = (u128)a * b;
    wide w = { (u64)(p >> 64), (u64)p };
    return w;
}

static u64
div128(u64 hi, u64 lo, u64 den) {
    u128 n = ((u128)hi << 64) | lo;
    return (u64)(n / den);
}

#else

static u64
lo32(u64 x) {
    return x & 0xFFFFFFFFull;
}

static wide
mul64(u64 a, u64 b) {
    u64 a0 = lo32(a), a1 = a >> 32;
    u64 b0 = lo32(b), b1 = b >> 32;
    u64 t = a0 * b0;
    u64 u = a1 * b0 + (t >> 32);
    u64 v = a0 * b1 + lo32(u);
    wide w = { a1 * b1 + (u >> 32) + (v >> 32), (v << 32) | lo32(t) };
    return w;
}

static u64
div128(u64 hi, u64 lo, u64 den) {
    u64 r = hi, q = 0;
    for (i32 i = 0; i < 64; i++) {
        u64 top = r >> 63;
        r = (r << 1) | (lo >> 63);
        lo <<= 1;
        q <<= 1;
        u64 bit = top | (r >= den);
        r = bit ? r - den : r;
        q |= bit;
    }
    return q;
}

#endif

static i32
u64_bitlen(u64 v) {
    if (!v) {
        return 0;
    }
#if SNRT_HAS_CLZLL && ULLONG_MAX == UINT64_MAX
    return 64 - __builtin_clzll(v);
#else
    i32 n = 0;
    while (v) {
        n++;
        v >>= 1;
    }
    return n;
#endif
}

static i32
pow5bits(i32 e) {
    return (i32)((u64)e * LOG2_5_Q19 >> 19) + 1;
}

static inline qr
div5(u64 hi, u64 lo, i32 c) {
    u64 vl = pow5_recip[c][0], vh = pow5_recip[c][1];
    wide p0 = mul64(lo, vl);
    wide p1 = mul64(lo, vh);
    wide p2 = mul64(hi, vl);
    wide p3 = mul64(hi, vh);
    u64 T[4];
    T[0] = p0.lo;
    u64 s = p0.hi + p1.lo;
    u64 cy = s < p0.hi;
    s += p2.lo;
    cy += s < p2.lo;
    T[1] = s;
    s = p1.hi + p2.hi;
    u64 cy2 = s < p1.hi;
    s += p3.lo;
    cy2 += s < p3.lo;
    s += cy;
    cy2 += s < cy;
    T[2] = s;
    T[3] = p3.hi + cy2;
    i32 kt = pow5bits(c) + 124;
    i32 w = kt / 64, f = kt % 64;
    u64 q = T[w] >> f;
    u64 over;
    if (likely(f)) {
        q |= T[w + 1] << (64 - f);
        over = T[w + 1] >> f;
    } else {
        over = T[w + 1];
    }
    for (i32 i = w + 2; i < (i32)COUNT(T); i++) {
        over |= T[i];
    }
    if (unlikely(over)) {
        q = UINT64_MAX;
    }
    u64 d = pow5_u64[c];
    wide prod = mul64(q, d);
    u64 rl = lo - prod.lo;
    u64 rh = hi - prod.hi - (lo < prod.lo);
    if (unlikely(rh)) {
        q--;
        rl += d;
    }
    qr out = { q, rl };
    return out;
}

enum { BN_LIMBS = 64 };

typedef struct {
    u64 d[BN_LIMBS];
    i32 n;   // used limbs
} bn;

static void
bn_zero(bn *b) {
    b->n = 0;
}

static void
bn_u64(bn *b, u64 v) {
    b->d[0] = v;
    b->n = v ? 1 : 0;
}

static void
bn_norm(bn *b) {
    while (b->n && !b->d[b->n - 1]) {
        b->n--;
    }
}

static i32
bn_is_zero(const bn *b) {
    return !b->n;
}

static u64
bn_to_u64(const bn *b) {
    if (b->n > 1) {
        return UINT64_MAX;
    }
    return b->n == 1 ? b->d[0] : 0;
}

static void
bn_mul_u64(bn *b, u64 v) {
    if (unlikely(!v || !b->n)) {
        bn_zero(b);
        return;
    }
    u64 carry = 0;
    for (i32 i = 0; i < b->n; i++) {
        wide p = mul64(b->d[i], v);
        u64 lo = p.lo + carry;
        u64 hi = p.hi + (lo < carry);
        b->d[i] = lo;
        carry = hi;
    }
    if (carry) {
        b->d[b->n++] = carry;
    }
}

static u64
bn_ripple(bn *a, i32 i, u64 carry) {
    for (; carry && i < a->n; i++) {
        u64 s = a->d[i] + carry;
        carry = s < carry;
        a->d[i] = s;
    }
    return carry;
}

static void
bn_add_u64(bn *a, u64 v) {
    u64 carry = bn_ripple(a, 0, v);
    if (carry) {
        a->d[a->n++] = carry;
    }
}

static void
bn_add(bn *a, const bn *b) {
    i32 m = a->n < b->n ? a->n : b->n;
    u64 carry = 0;
    for (i32 i = 0; i < m; i++) {
        u64 s = a->d[i] + b->d[i];
        u64 c = s < b->d[i];
        s += carry;
        c += s < carry;
        a->d[i] = s;
        carry = c;
    }
    for (i32 i = m; i < b->n; i++) {
        a->d[i] = b->d[i];
    }
    if (a->n < b->n) {
        a->n = b->n;
    }
    carry = bn_ripple(a, m, carry);
    if (carry) {
        a->d[a->n++] = carry;
    }
}

static void
bn_mul_add(bn *m, u64 p, u64 v) {
    bn_mul_u64(m, p);
    bn_add_u64(m, v);
}

static void
bn_sub(bn *a, const bn *b) {
    u64 borrow = 0;
    for (i32 i = 0; i < a->n; i++) {
        u64 x = i < b->n ? b->d[i] : 0;
        u64 d0 = a->d[i] - x;
        u64 b1 = a->d[i] < x;
        u64 d1 = d0 - borrow;
        u64 b2 = d0 < borrow;
        a->d[i] = d1;
        borrow = b1 | b2;
    }
    bn_norm(a);
}

static void
bn_shl(bn *b, i32 s) {
    if (unlikely(!b->n || !s)) {
        return;
    }
    i32 w = s / 64, f = s % 64;
    if (w) {
        for (i32 i = b->n + w; i-- > w;) {
            b->d[i] = b->d[i - w];
        }
        for (i32 i = 0; i < w; i++) {
            b->d[i] = 0;
        }
        b->n += w;
    }
    if (likely(f)) {
        u64 carry = 0;
        for (i32 i = 0; i < b->n; i++) {
            u64 x = b->d[i];
            b->d[i] = (x << f) | carry;
            carry = x >> (64 - f);
        }
        if (carry) {
            b->d[b->n++] = carry;
        }
    }
}

static void
bn_shr(bn *b, i32 s) {
    if (unlikely(!b->n || !s)) {
        return;
    }
    i32 w = s / 64, f = s % 64;
    if (w >= b->n) {
        bn_zero(b);
        return;
    }
    i32 n = b->n - w;
    for (i32 i = 0; i < n; i++) {
        u64 x = b->d[i + w];
        if (likely(f)) {
            x >>= f;
            if (likely(i + w + 1 < b->n)) {
                x |= b->d[i + w + 1] << (64 - f);
            }
        }
        b->d[i] = x;
    }
    for (i32 i = n; i < b->n; i++) {
        b->d[i] = 0;
    }
    b->n = n;
    bn_norm(b);
}

static i32
bn_cmp(const bn *a, const bn *b) {
    if (a->n != b->n) {
        return a->n < b->n ? -1 : 1;
    }
    for (i32 i = a->n - 1; i >= 0; i--) {
        if (a->d[i] != b->d[i]) {
            return a->d[i] < b->d[i] ? -1 : 1;
        }
    }
    return 0;
}

static i32
bn_bitlen(const bn *b) {
    if (!b->n) {
        return 0;
    }
    return (b->n - 1) * 64 + u64_bitlen(b->d[b->n - 1]);
}

static void
bn_mul_limbs(bn *b, const u64 *m, i32 mn) {
    if (unlikely(!mn || !b->n)) {
        bn_zero(b);
        return;
    }

    if (b->n == 1) {
        u64 x = b->d[0], carry = 0;
        i32 i = 0;
        for (; i < mn; i++) {
            wide p = mul64(m[i], x);
            u64 lo = p.lo + carry;
            u64 hi = p.hi + (lo < carry);
            b->d[i] = lo;
            carry = hi;
        }
        if (carry) {
            b->d[i++] = carry;
        }
        b->n = i;
        bn_norm(b);
        return;
    }

    bn r = {0};
    for (i32 i = 0; i < mn; i++) {
        if (!m[i]) {
            continue;
        }
        u64 carry = 0;
        for (i32 j = 0; j < b->n; j++) {
            wide p = mul64(m[i], b->d[j]);
            u64 t = r.d[i + j] + p.lo;
            u64 c = t < p.lo;
            t += carry;
            if (t < carry) {
                c++;
            }
            r.d[i + j] = t;
            carry = p.hi + c;
        }
        i32 k = i + b->n;
        while (carry) {
            u64 t = r.d[k] + carry;
            carry = t < carry;
            r.d[k] = t;
            k++;
        }
    }
    r.n = b->n + mn + 1;
    bn_norm(&r);
    *b = r;
}

typedef struct {
    bn p;
    i32 q;  // exponent of p, -1 while empty
} pow5_cache;

static const bn *
pow5_at(pow5_cache *c, i32 q) {
    if (c->q != q) {
        i32 top = q > POW5_ROW_MAX ? POW5_ROW_MAX : q;
        const u64 *m = pow5_limbs + pow5_off[top];
        i32 n = (i32)(pow5_off[top + 1] - pow5_off[top]);
        for (i32 i = 0; i < n; i++) {
            c->p.d[i] = m[i];
        }
        c->p.n = n;
        for (i32 t = q - top; t > 0;) {
            i32 k = t > POW5_U64_MAX ? POW5_U64_MAX : t;
            bn_mul_u64(&c->p, pow5_u64[k]);
            t -= k;
        }
        c->q = q;
    }
    return &c->p;
}

enum { DIVMOD_CHUNKS = (POW5_T_MAX + POW5_U64_MAX - 1) / POW5_U64_MAX };

static void
divmod_pow5_chunks(const bn *n, i32 t, bn *q, bn *r) {
    u64 rem[DIVMOD_CHUNKS];
    i32 cs[DIVMOD_CHUNKS];
    i32 k = 0;
    *q = *n;
    while (t > 0) {
        i32 c = t > POW5_U64_MAX ? POW5_U64_MAX : t;
        u64 rr = 0;
        for (i32 i = q->n - 1; i >= 0; i--) {
            qr d = div5(rr, q->d[i], c);
            q->d[i] = d.q;
            rr = d.r;
        }
        bn_norm(q);
        rem[k] = rr;
        cs[k] = c;
        k++;
        t -= c;
    }
    bn_zero(r);
    for (i32 i = k - 1; i >= 0; i--) {
        bn_mul_u64(r, pow5_u64[cs[i]]);
        bn_add_u64(r, rem[i]);
    }
}

static void
divmod_pow5(const bn *n, i32 t, const bn *d, bn *q, bn *r) {
    if (unlikely(!t)) {
        *q = *n;
        bn_zero(r);
        return;
    }
    if (bn_cmp(n, d) < 0) {
        bn_u64(q, 0);
        *r = *n;
        return;
    }
    bn d64 = *d;
    bn_shl(&d64, 64);
    if (unlikely(bn_cmp(n, &d64) >= 0)) {
        divmod_pow5_chunks(n, t, q, r);
        return;
    }
    i32 lz = 64 - u64_bitlen(d->d[d->n - 1]);
    bn dn = *d, nn = *n;
    bn_shl(&dn, lz);
    bn_shl(&nn, lz);
    u64 hi = nn.n > dn.n ? nn.d[dn.n] : 0;
    u64 lo = nn.d[dn.n - 1];
    u64 qhat;
    if (hi >= dn.d[dn.n - 1]) {
        qhat = UINT64_MAX;
    } else {
        qhat = div128(hi, lo, dn.d[dn.n - 1]);
    }
    bn prod = dn;
    bn_mul_u64(&prod, qhat);
    while (bn_cmp(&prod, &nn) > 0) {
        bn_sub(&prod, &dn);
        qhat--;
    }
    bn_sub(&nn, &prod);
    bn_u64(q, qhat);
    if (likely(lz)) {
        bn_shr(&nn, lz);
    }
    *r = nn;
}

typedef enum {
    FRAC_ZERO,      // 0
    FRAC_UNDER_Q,   // (0, 1/4)
    FRAC_Q,         // 1/4
    FRAC_OVER_Q,    // (1/4, 1/2)
    FRAC_HALF,      // 1/2
    FRAC_UNDER_TQ,  // (1/2, 3/4)
    FRAC_TQ,        // 3/4
    FRAC_OVER_TQ    // (3/4, 1)
} frac_kind;

typedef struct {
    u64 v;
    frac_kind frac;
} scaled;

static scaled
bn_shr_frac(const bn *b, i32 rsh) {
    i32 b1 = 0, b2 = 0, rest = 0;
    if (rsh >= 2) {
        i32 w2 = (rsh - 2) / 64, f2 = (rsh - 2) % 64;
        for (i32 i = 0; i < w2 && i < b->n; i++) {
            if (b->d[i]) {
                rest = 1;
            }
        }
        if (w2 < b->n) {
            u64 x = b->d[w2];
            b2 = (i32)((x >> f2) & 1);
            if (f2 && (x & ((1ull << f2) - 1))) {
                rest = 1;
            }
        }
    }
    i32 w1 = (rsh - 1) / 64, f1 = (rsh - 1) % 64;
    if (w1 < b->n) {
        b1 = (i32)((b->d[w1] >> f1) & 1);
    }
    frac_kind frac = (frac_kind)((b1 << 2) | (b2 << 1) | rest);
    u64 v = 0;
    i32 w = rsh / 64, f = rsh % 64;
    if (w < b->n) {
        u64 hi = w + 1 < b->n ? b->d[w + 1] : 0;
        v = b->d[w] >> f;
        if (f) {
            v |= hi << (64 - f);
        }
        u64 over = f ? hi >> f : hi;
        for (i32 i = w + 2; i < b->n; i++) {
            over |= b->d[i];
        }
        if (over) {
            v = UINT64_MAX;
        }
    }
    scaled out = { v, frac };
    return out;
}

static i32
cmp_wide(wide a, wide b) {
    if (a.hi != b.hi) {
        return a.hi < b.hi ? -1 : 1;
    }
    return a.lo < b.lo ? -1 : (a.lo > b.lo ? 1 : 0);
}

static frac_kind
frac_of_ratio(const bn *r, const bn *den) {
    if (unlikely(bn_is_zero(r))) {
        return FRAC_ZERO;
    }
    bn two = *r;
    bn_shl(&two, 1);
    i32 c = bn_cmp(&two, den);
    if (c == 0) {
        return FRAC_HALF;
    }
    bn four = *r;
    bn_shl(&four, 2);
    if (c < 0) {
        i32 q = bn_cmp(&four, den);
        return q < 0 ? FRAC_UNDER_Q : (q == 0 ? FRAC_Q : FRAC_OVER_Q);
    }
    bn d3 = *den;
    bn_mul_u64(&d3, 3);
    i32 t = bn_cmp(&four, &d3);
    return t < 0 ? FRAC_UNDER_TQ : (t == 0 ? FRAC_TQ : FRAC_OVER_TQ);
}

static frac_kind
frac_of_u64(u64 rem, u64 div) {
    if (unlikely(!rem)) {
        return FRAC_ZERO;
    }
    u64 r2 = rem << 1;
    if (r2 == div) {
        return FRAC_HALF;
    }
    wide r4 = mul64(rem, 4);
    if (r2 < div) {
        i32 q = cmp_wide(r4, (wide) { 0, div });
        return q < 0 ? FRAC_UNDER_Q : (q == 0 ? FRAC_Q : FRAC_OVER_Q);
    }
    i32 t = cmp_wide(r4, mul64(div, 3));
    return t < 0 ? FRAC_UNDER_TQ : (t == 0 ? FRAC_TQ : FRAC_OVER_TQ);
}

static scaled
shift2(u64 hi, u64 lo, i32 sh) {
    if (sh >= 0) {
        scaled out = { lo << sh, FRAC_ZERO };
        return out;
    }
    i32 rsh = -sh;
    u64 b1, b2, rest, v;
    if (rsh >= 64) {
        i32 f = rsh - 64;
        v = f ? hi >> f : hi;
        b1 = f ? (hi >> (f - 1)) & 1 : lo >> 63;
        if (f >= 2) {
            b2 = (hi >> (f - 2)) & 1;
            rest = lo || (hi & (((u64)1 << (f - 2)) - 1));
        } else {
            b2 = (lo >> (f ? 63 : 62)) & 1;
            rest = lo & ((f ? (u64)1 << 63 : (u64)1 << 62) - 1);
        }
    } else {
        v = (lo >> rsh) | (hi << (64 - rsh));
        b1 = (lo >> (rsh - 1)) & 1;
        b2 = rsh > 1 ? (lo >> (rsh - 2)) & 1 : 0;
        rest = rsh > 2 ? lo & (((u64)1 << (rsh - 2)) - 1) : 0;
    }
    frac_kind frac = (frac_kind)((b1 << 2) | (b2 << 1) | (rest != 0));
    scaled out = { v, frac };
    return out;
}

static i32
log10pow2(i32 e) {
    return (i32)((u64)e * LOG10_2_Q18 >> 18);
}

static i32
log10pow5(i32 e) {
    return (i32)((u64)e * LOG10_5_Q20 >> 20);
}

static i32
log2pow10(i32 e) {
    return (i32)((i64)e * LOG2_10_Q29 >> 29);
}

static i32
pow5_factor(u64 value) {
    i32 count = 0;
    for (;;) {
        value *= INV5_MOD64;
        if (value > U64_MAX_DIV5) {
            break;
        }
        count++;
    }
    return count;
}

static u64
mul_shift(u64 m, const u64 *mul, i32 j) {
    wide b0 = mul64(m, mul[0]);
    wide b2 = mul64(m, mul[1]);
    u64 s = b0.hi + b2.lo;
    u64 t = b2.hi + (s < b0.hi);
    i32 sh = j - 64;
    return sh ? ((s >> sh) | (t << (64 - sh))) : s;
}

static i32
scale_fast(u64 m, i32 k, i32 s, scaled *out) {
    const u64 *mul;
    i32 j;
    if (s >= 0) {
        mul = pow5_split[s];
        j = 125 - pow5bits(s) - k - s;
    } else {
        i32 t = -s;
        mul = pow5_recip[t];
        j = pow5bits(t) + 124 - k + t;
    }
    if (j < 65 || j > 190) {
        return 0;
    }
    wide p0 = mul64(m, mul[0]);
    wide p1 = mul64(m, mul[1]);
    u64 mid = p0.hi + p1.lo;
    u64 hi = p1.hi + (mid < p0.hi);
    u64 v, f, st;
    if (j >= 128) {
        i32 r = j - 128;
        v = hi >> r;
        f = r ? (mid >> r) | (hi << (64 - r)) : mid;
        st = p0.lo || (r && (mid & (((u64)1 << r) - 1)));
    } else {
        i32 r = j - 64;
        v = r ? (mid >> r) | (hi << (64 - r)) : mid;
        f = r ? (p0.lo >> r) | (mid << (64 - r)) : p0.lo;
        st = r && (p0.lo & (((u64)1 << r) - 1));
    }
    if (f > UINT64_MAX - 3) {
        return 0;
    }
    u64 flo = f > 2 ? f - 2 : 0;
    u64 fhi = f + (st ? 1 : 0) + 2;
    u64 q = flo >> 62;
    if (unlikely((fhi >> 62) != q || !(flo << 2))) {
        return 0;
    }
    out->frac = (frac_kind)(1 + 2 * q);
    out->v = v;
    return 1;
}

static scaled
scale_bn(const bn *M, i32 k, i32 s, pow5_cache *pc) {
    if (s >= 0 && s <= POW5_U64_MAX && M->n == 1) {
        wide p = mul64(M->d[0], pow5_u64[s]);
        return shift2(p.hi, p.lo, k + s);
    }
    scaled fast;
    if (M->n == 1 && scale_fast(M->d[0], k, s, &fast)) {
        return fast;
    }
    if (s >= 0) {
        const bn *p5 = pow5_at(pc, s);
        bn a = *M;
        bn_mul_limbs(&a, p5->d, p5->n);
        i32 sh = k + s;
        if (sh >= 0) {
            bn_shl(&a, sh);
            scaled out = { bn_to_u64(&a), FRAC_ZERO };
            return out;
        }
        return bn_shr_frac(&a, -sh);
    }
    i32 t = -s;
    i32 adj = k - t;
    if (likely(t <= POW5_U64_MAX && adj >= 0 && adj < 128 && M->n <= 2)) {
        u64 w[3] = { M->d[0], M->n > 1 ? M->d[1] : 0, 0 };
        i32 sh = adj;
        if (unlikely(sh >= 64)) {
            w[2] = w[1];
            w[1] = w[0];
            w[0] = 0;
            sh -= 64;
        }
        if (sh) {
            w[2] = (w[2] << sh) | (w[1] >> (64 - sh));
            w[1] = (w[1] << sh) | (w[0] >> (64 - sh));
            w[0] <<= sh;
        }
        u64 div = pow5_u64[t];
        u64 rem = 0;
        for (i32 i = (i32)COUNT(w) - 1; i >= 0; i--) {
            qr d = div5(rem, w[i], t);
            w[i] = d.q;
            rem = d.r;
        }
        if (likely(!w[2])) {
            scaled out = { w[1] ? UINT64_MAX : w[0], frac_of_u64(rem, div) };
            return out;
        }
    }
    const bn *den = pow5_at(pc, t);
    if (adj >= 0) {
        bn num = *M;
        bn_shl(&num, adj);
        bn q, rem;
        divmod_pow5(&num, t, den, &q, &rem);
        scaled out = { bn_to_u64(&q), frac_of_ratio(&rem, den) };
        return out;
    }
    i32 z = -adj;
    bn q5, r5;
    divmod_pow5(M, t, den, &q5, &r5);
    u64 qz = bn_shr_frac(&q5, z).v;
    bn rz = q5;
    {
        i32 w = z / 64, f = z % 64;
        if (w < rz.n) {
            if (f) {
                rz.d[w] &= (1ull << f) - 1;
            }
            for (i32 i = w + (f ? 1 : 0); i < rz.n; i++) {
                rz.d[i] = 0;
            }
            bn_norm(&rz);
        }
    }
    bn num = rz;
    bn_mul_limbs(&num, den->d, den->n);
    bn_add(&num, &r5);
    bn dz = *den;
    bn_shl(&dz, z);
    scaled out = { qz, frac_of_ratio(&num, &dz) };
    return out;
}

static i32
dec_len(u64 v) {
    i32 t = u64_bitlen(v | 1) - 1;
    i32 n = (i32)((u64)t * LOG10_2_Q18 >> 18) + 1;
    return n + (v >= pow10_u64[n]);
}

#if __has_attribute(may_alias)

typedef u16 u16a __attribute__((may_alias, aligned(1)));

static void
put_pair(char *w, const u16 *tbl, u64 i) {
    *(u16a *)w = tbl[i];
}

#else

static void
put_pair(char *w, const u16 *tbl, u64 i) {
    memcpy(w, &tbl[i], 2);
}

#endif

static u64
fmt_n(u64 v, i32 k, char *w) {
    while (k >= 4) {
        u64 q = v / 10000;
        u32 r = (u32)(v - q * 10000);
        u32 rp = r / 100;
        v = q;
        k -= 4;
        w -= 4;
        put_pair(w, dig2, rp);
        put_pair(w + 2, dig2, r - 100 * rp);
    }
    while (k >= 2) {
        u64 p = v % 100;
        v /= 100;
        w -= 2;
        put_pair(w, dig2, p);
        k -= 2;
    }
    if (k) {
        *--w = (char)('0' + v % 10);
        v /= 10;
    }
    return v;
}

static void
fmt_dec(u64 d, i32 e, char *out) {
    while (d >= 10 && d % 10 == 0) {
        d /= 10;
        e++;
    }
    i32 n = dec_len(d);
    i32 exp10 = e + n - 1;
    i32 a = exp10 < 0 ? -exp10 : exp10;
    i32 ed = dec_len((u64)a);
    i32 sci_len = n + (n > 1 ? 1 : 0) + 1 + (exp10 < 0 ? 1 : 0) + ed;
    i32 fixed_len;
    if (exp10 >= 0) {
        fixed_len = n <= exp10 + 1 ? exp10 + 1 : n + 1;
    } else {
        fixed_len = 2 + (-exp10 - 1) + n;
    }
    char *stop;
    if (sci_len < fixed_len) {
        if (likely(n > 1)) {
            out[1] = '.';
        }
        out[0] = (char)('0' + fmt_n(d, n - 1, out + n + 1));
        char *end = out + n + (n > 1 ? 1 : 0);
        end[0] = 'e';
        if (exp10 < 0) {
            end[1] = '-';
        }
        stop = end + 1 + (exp10 < 0 ? 1 : 0) + ed;
        fmt_n((u64)a, ed, stop);
    } else if (exp10 >= 0) {
        if (n <= exp10 + 1) {
            fmt_n(d, n, out + n);
            for (i32 i = n; i <= exp10; i++) {
                out[i] = '0';
            }
            stop = out + exp10 + 1;
        } else {
            i32 k = n - exp10 - 1;
            u64 h = fmt_n(d, k, out + n + 1);
            fmt_n(h, exp10 + 1, out + exp10 + 1);
            out[exp10 + 1] = '.';
            stop = out + n + 1;
        }
    } else {
        out[0] = '0';
        out[1] = '.';
        for (i32 i = 0; i < -exp10 - 1; i++) {
            out[2 + i] = '0';
        }
        stop = out + 2 + (-exp10 - 1) + n;
        fmt_n(d, n, stop);
    }
    stop[0] = '\0';
}

static void
dtoa_short(u64 m, i32 k, i32 p, i32 k_sub, char *out) {
    u64 mv = 4 * m;
    i32 accept = !(m & 1);
    u64 mm_shift = (m == (1ull << (p - 1)) && k > k_sub) ? 0 : 1;
    i32 e2 = k - 2;
    u64 vr, vp, vm;
    i32 vrz = 0, vmz = 0;
    i32 q, e10;
    if (e2 >= 0) {
        q = log10pow2(e2) - (e2 > 3);
        e10 = q;
        i32 j = -e2 + q + 125 + pow5bits(q) - 1;
        vr = mul_shift(mv, pow5_recip[q], j);
        vp = mul_shift(mv + 2, pow5_recip[q], j);
        vm = mul_shift(mv - 1 - mm_shift, pow5_recip[q], j);
        if (q <= 21) {
            if (unlikely(mv % 5 == 0)) {
                vrz = pow5_factor(mv) >= q;
            } else if (accept) {
                vmz = pow5_factor(mv - 1 - mm_shift) >= q;
            } else if (pow5_factor(mv + 2) >= q) {
                vp--;
            }
        }
    } else {
        q = log10pow5(-e2) - (-e2 > 1);
        e10 = q + e2;
        i32 i = -e2 - q;
        i32 j = q - (pow5bits(i) - 125);
        vr = mul_shift(mv, pow5_split[i], j);
        vp = mul_shift(mv + 2, pow5_split[i], j);
        vm = mul_shift(mv - 1 - mm_shift, pow5_split[i], j);
        if (q <= 1) {
            vrz = 1;
            if (accept) {
                vmz = mm_shift == 1;
            } else {
                vp -= 1;
            }
        } else if (q < 63) {
            vrz = (mv & ((1ull << q) - 1)) == 0;
        }
    }
    i32 removed = 0;
    u64 last = 0;
    u64 mant;
    if (unlikely(vmz || vrz)) {
        for (;;) {
            u64 vp10 = vp / 10;
            u64 vm10 = vm / 10;
            if (vp10 <= vm10) {
                break;
            }
            u64 vr10 = vr / 10;
            vmz = vmz && vm == 10 * vm10;
            vrz = vrz && last == 0;
            last = vr - 10 * vr10;
            vr = vr10;
            vp = vp10;
            vm = vm10;
            removed++;
        }
        if (unlikely(vmz)) {
            while (vm % 10 == 0) {
                u64 vr10 = vr / 10;
                vrz = vrz && last == 0;
                last = vr - 10 * vr10;
                vr = vr10;
                vp /= 10;
                vm /= 10;
                removed++;
            }
        }
        if (unlikely(vrz && last == 5 && !(vr & 1))) {
            last = 4;
        }
        mant = vr + ((vr == vm && (!accept || !vmz)) || last >= 5);
    } else {
        i32 up = 0;
        u64 vp100 = vp / 100;
        u64 vm100 = vm / 100;
        if (likely(vp100 > vm100)) {
            u64 vr100 = vr / 100;
            up = vr - 100 * vr100 >= 50;
            vr = vr100;
            vp = vp100;
            vm = vm100;
            removed += 2;
        }
        for (;;) {
            u64 vp10 = vp / 10;
            u64 vm10 = vm / 10;
            if (vp10 <= vm10) {
                break;
            }
            u64 vr10 = vr / 10;
            up = vr - 10 * vr10 >= 5;
            vr = vr10;
            vp = vp10;
            vm = vm10;
            removed++;
        }
        mant = vr + (vr == vm || up);
    }
    fmt_dec(mant, e10 + removed, out);
}

typedef enum { SCAN_BAD, SCAN_FINITE, SCAN_INF, SCAN_NAN } scan_kind;

static i32
word_eq(const char *s, size_t cap, size_t *i, const char *w) {
    size_t j = *i;
    for (size_t k = 0; w[k]; k++) {
        if (unlikely(j >= cap || !s[j])) {
            return 0;
        }
        char c = s[j];
        if (unlikely(c >= 'A' && c <= 'Z')) {
            c = (char)(c - 'A' + 'a');
        }
        if (likely(c != w[k])) {
            return 0;
        }
        j++;
    }
    *i = j;
    return 1;
}

static no_inline scan_kind
scan_words(const char *s, size_t cap, size_t i, size_t *end) {
    size_t j = i;
    if (word_eq(s, cap, &j, "infinity") || word_eq(s, cap, &j, "inf")) {
        *end = j;
        return SCAN_INF;
    }
    j = i;
    if (word_eq(s, cap, &j, "nan")) {
        *end = j;
        return SCAN_NAN;
    }
    return SCAN_BAD;
}

enum { EXP_Q_LIM = 1 << 20 };

static const u64 EXP_ACC_SAT = (u64)1 << 40;

typedef struct {
    scan_kind spec;
    u64 sign;
    i32 ndigits;
    i32 q;
    i32 sticky;
} numeral;

static i32
digits8(const char *s, u64 *out) {
    u64 v;
    memcpy(&v, s, sizeof v);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    v = __builtin_bswap64(v);
#endif
    u64 x = v ^ 0x3030303030303030ull;
    u64 bad = (x & 0xF0F0F0F0F0F0F0F0ull)
            | (((x & 0x0F0F0F0F0F0F0F0Full) + 0x0606060606060606ull)
               & 0x1010101010101010ull);
    i32 p = 8;
    if (bad) {
#if SNRT_HAS_CTZLL
        p = (i32)(__builtin_ctzll(bad) >> 3);
#else
        p = 0;
        for (u64 b = bad; !(b & 0xFF); b >>= 8) {
            p++;
        }
#endif
        if (!p) {
            *out = 0;
            return 0;
        }
        v = (v << (8 * (8 - p))) | (0x3030303030303030ull >> (8 * p));
    }
    x = (v - 0x3030303030303030ull) & 0x0F0F0F0F0F0F0F0Full;
    u64 q = ((x * 0x0A01ull) >> 8) & 0x00FF00FF00FF00FFull;
    u64 r = ((q * 0x00640001ull) >> 16) & 0x0000FFFF0000FFFFull;
    *out = (r * 0x271000000001ull) >> 32;
    return p;
}

static no_inline numeral
scan_numeral(const char *s, size_t cap, bn *m, size_t *end) {
    numeral out = {0};
    bn_zero(m);
    *end = 0;
    size_t i = 0;
    if (i < cap && (s[i] == '+' || s[i] == '-')) {
        out.sign = s[i] == '-';
        i++;
    }
    if (unlikely(i >= cap || (s[i] != '.' && (s[i] < '0' || s[i] > '9')))) {
        out.spec = scan_words(s, cap, i, end);
        return out;
    }
    u64 acc = 0;
    i32 held = 0;
    i32 nd = 0;
    i32 sig = 0;
    i32 started = 0;
    i32 dropped = 0;
    i32 dot_at = -1;
    u64 exp = 0;
    i32 expneg = 0;
    for (;;) {
        if (i >= cap || !s[i]) {
            break;
        }
        char c = s[i];
        if (likely(c >= '0' && c <= '9')) {
            u64 v8;
            i32 p = 0;
            if (likely(i + 8 < cap && s[i + 1] >= '0' && s[i + 1] <= '9')) {
                p = digits8(s + i, &v8);
            }
            if (likely(p && sig + p <= DIGITS_KEPT)) {
                if (unlikely(held + p > POW10_U64_MAX)) {
                    bn_mul_add(m, pow10_u64[held], acc);
                    acc = 0;
                    held = 0;
                }
                acc = acc * pow10_u64[p] + v8;
                held += p;
                nd += p;
                if (likely(started)) {
                    sig += p;
                } else if (v8) {
                    started = 1;
                    sig = dec_len(v8);
                }
                i += (size_t)p;
                continue;
            }
            u64 d = (u64)(c - '0');
            nd++;
            if (unlikely(sig == DIGITS_KEPT)) {
                dropped++;
                out.sticky |= d != 0;
            } else {
                if (unlikely(held == POW10_U64_MAX)) {
                    bn_mul_add(m, pow10_u64[held], acc);
                    acc = 0;
                    held = 0;
                }
                acc = acc * 10 + d;
                held++;
                if (likely(started)) {
                    sig++;
                } else if (d) {
                    started = 1;
                    sig = 1;
                }
            }
            i++;
        } else if (c == '.' && dot_at < 0) {
            dot_at = nd;
            i++;
        } else if ((c == 'e' || c == 'E') && nd > 0) {
            size_t ie = i;
            i++;
            if (i < cap && (s[i] == '+' || s[i] == '-')) {
                expneg = s[i] == '-';
                i++;
            }
            size_t e0 = i;
            u64 e10 = 0;
            for (; i < cap && s[i] >= '0' && s[i] <= '9'; i++) {
                u64 d10 = e10 * 10 + (u64)(s[i] - '0');
                e10 = d10 > EXP_ACC_SAT ? EXP_ACC_SAT : d10;
            }
            if (unlikely(i == e0)) {
                i = ie;
                break;
            }
            exp = e10;
            break;
        } else {
            break;
        }
    }
    if (unlikely(!nd)) {
        return out;
    }
    bn_mul_add(m, pow10_u64[held], acc);
    out.ndigits = sig;
    i64 q = (dot_at < 0 ? 0 : (i64)dot_at - nd)
          + (expneg ? -(i64)exp : (i64)exp) + dropped;
    out.q = q > EXP_Q_LIM ? EXP_Q_LIM : q < -EXP_Q_LIM ? -EXP_Q_LIM : (i32)q;
    out.spec = SCAN_FINITE;
    *end = i;
    return out;
}

typedef enum {
    FMT_IEEE,   // inf and nan at the all-ones exponent
    FMT_FN,     // e4m3fn saturates and has one nan pattern
    FMT_FINITE  // e2m1 is finite only and saturates
} fmt_kind;

static u64
round_even(scaled s, i32 sticky) {
    if (sticky && s.frac == FRAC_HALF) {
        s.frac = FRAC_UNDER_TQ;
    }
    return s.v + (s.frac > FRAC_HALF || (s.frac == FRAC_HALF && (s.v & 1)));
}

static scaled
scaled_halve(scaled s) {
    static const frac_kind even[8] = {
        FRAC_ZERO, FRAC_UNDER_Q, FRAC_UNDER_Q, FRAC_UNDER_Q,
        FRAC_Q, FRAC_OVER_Q, FRAC_OVER_Q, FRAC_OVER_Q
    };
    static const frac_kind odd[8] = {
        FRAC_HALF, FRAC_UNDER_TQ, FRAC_UNDER_TQ, FRAC_UNDER_TQ,
        FRAC_TQ, FRAC_OVER_TQ, FRAC_OVER_TQ, FRAC_OVER_TQ
    };
    u64 o = s.v & 1;
    s.v >>= 1;
    s.frac = o ? odd[s.frac] : even[s.frac];
    return s;
}

static u64
max_finite_bits(u64 sign, i32 ebits, i32 sbits, fmt_kind kind) {
    u64 signb = sign << (ebits + sbits);
    u64 smask = (1ull << sbits) - 1;
    u64 emaxf = (1ull << ebits) - 1;
    if (kind == FMT_FN) {
        return signb | (emaxf << sbits) | (smask - 1);
    }
    return signb | (emaxf << sbits) | smask;
}

static u64
overflow_bits(u64 sign, i32 ebits, i32 sbits, fmt_kind kind) {
    if (kind == FMT_IEEE) {
        return (sign << (ebits + sbits)) | (((1ull << ebits) - 1) << sbits);
    }
    return max_finite_bits(sign, ebits, sbits, kind);
}

static no_inline u64
special_bits(scan_kind spec, u64 sign, i32 ebits, i32 sbits, fmt_kind kind) {
    if (spec == SCAN_BAD) {
        return 0;
    }
    if (spec == SCAN_INF) {
        return overflow_bits(sign, ebits, sbits, kind);
    }
    u64 signb = sign << (ebits + sbits);
    u64 emaxf = (1ull << ebits) - 1;
    if (kind == FMT_IEEE) {
        return signb | (emaxf << sbits) | (1ull << (sbits - 1));
    }
    if (kind == FMT_FN) {
        return signb | (emaxf << sbits) | ((1ull << sbits) - 1);
    }
    return 0;
}

static force_inline u64
compose_float(u64 sign, u64 m, i32 k, i32 ebits, i32 sbits, i32 bias, fmt_kind kind) {
    i32 k_sub = 1 - bias - sbits;
    u64 signb = sign << (ebits + sbits);
    u64 smask = (1ull << sbits) - 1;
    u64 emaxf = (1ull << ebits) - 1;
    u64 max_e_f = kind == FMT_IEEE ? emaxf - 1 : emaxf;
    if (likely(m >= (1ull << sbits))) {
        u64 e_f = (u64)(k - k_sub + 1);
        u64 frac = m - (1ull << sbits);
        i32 over = e_f > max_e_f;
        if (unlikely(kind == FMT_FN && e_f == max_e_f && frac == smask)) {
            over = 1;
        }
        if (unlikely(over)) {
            return overflow_bits(sign, ebits, sbits, kind);
        }
        return signb | (e_f << sbits) | frac;
    }
    return signb | m;
}

static force_inline u64
parse_float(const char *s, size_t cap, size_t *end, i32 strict, i32 ebits,
            i32 sbits, i32 bias, fmt_kind kind) {
    bn M;
    size_t e;
    numeral num = scan_numeral(s, cap, &M, &e);
    if (end) {
        *end = e;
    }
    if (strict && e != cap) {
        return 0;
    }
    u64 signb = num.sign << (ebits + sbits);
    if (unlikely(num.spec != SCAN_FINITE)) {
        return special_bits(num.spec, num.sign, ebits, sbits, kind);
    }
    if (unlikely(bn_is_zero(&M))) {
        return signb;
    }
    if (unlikely(num.q > EXP_OVERFLOW)) {
        return overflow_bits(num.sign, ebits, sbits, kind);
    }
    if (unlikely(num.q + num.ndigits <= EXP_UNDERFLOW)) {
        return signb;
    }
    i32 p = sbits + 1;
    i32 k_sub = 1 - bias - sbits;
    i32 emin_val = 1 - bias;
    i32 E = bn_bitlen(&M) - 1 + log2pow10(num.q);
    pow5_cache pc;
    pc.q = -1;
    if (E >= emin_val) {
        scaled sc = scale_bn(&M, p - 1 - E, num.q, &pc);
        u64 Rhi = round_even(scaled_halve(sc), num.sticky);
        u64 Rlo = round_even(sc, num.sticky);
        i32 up = sc.v >= (1ull << p);
        u64 R = up ? Rhi : Rlo;
        E += up;
        if (unlikely(R >= (1ull << p))) {
            return compose_float(num.sign, 1ull << (p - 1),
                                 E + 1 - (p - 1), ebits, sbits, bias, kind);
        }
        return compose_float(num.sign, R, E - (p - 1), ebits, sbits,
                             bias, kind);
    }
    scaled sc = scale_bn(&M, -k_sub, num.q, &pc);
    u64 R = round_even(sc, num.sticky);
    if (unlikely(!R)) {
        return signb;
    }
    return compose_float(num.sign, R, k_sub, ebits, sbits, bias, kind);
}

static no_inline u64
parse_any(const char *s, size_t cap, size_t *end, i32 ebits, i32 sbits,
          i32 bias, fmt_kind kind) {
    return parse_float(s, cap, end, 0, ebits, sbits, bias, kind);
}

static force_inline void
emit_float(u64 bits, i32 ebits, i32 sbits, i32 bias, char *out) {
    if (ebits + sbits + 1 < 64) {
        bits &= (1ull << (ebits + sbits + 1)) - 1;
    }
    u64 sign = (bits >> (ebits + sbits)) & 1;
    u64 smask = (1ull << sbits) - 1;
    u64 emaxf = (1ull << ebits) - 1;
    u64 expf = (bits >> sbits) & emaxf;
    u64 frac = bits & smask;
    out[0] = '-';
    char *o = out + sign;
    if (unlikely(expf == emaxf)) {
        memcpy(o, frac ? "nan" : "inf", sizeof "nan");
        return;
    }
    u64 m;
    i32 k;
    if (unlikely(!expf)) {
        m = frac;
        k = 1 - bias - sbits;
    } else {
        m = frac | (1ull << sbits);
        k = (i32)expf - bias - sbits;
    }
    if (unlikely(!m)) {
        memcpy(o, "0", sizeof "0");
        return;
    }
    dtoa_short(m, k, sbits + 1, 1 - bias - sbits, o);
}

b32
from_f64(f64 v) {
    b32 r;
    u64 bits;
    memcpy(&bits, &v, sizeof bits);
    emit_float(bits, F64_EBITS, F64_SBITS, F64_BIAS, r.b);
    return r;
}

f64
scan_f64(const char *s, size_t n, size_t *consumed) {
    u64 bits = parse_any(s, n, consumed,
                         F64_EBITS, F64_SBITS, F64_BIAS, FMT_IEEE);
    f64 v;
    memcpy(&v, &bits, sizeof v);
    return v;
}

f64
to_f64(const char *s) {
    u64 bits = parse_float(s, strlen(s), NULL, 1,
                           F64_EBITS, F64_SBITS, F64_BIAS, FMT_IEEE);
    f64 v;
    memcpy(&v, &bits, sizeof v);
    return v;
}

b32
from_f32(f32 v) {
    b32 r;
    u32 bits;
    memcpy(&bits, &v, sizeof bits);
    emit_float(bits, F32_EBITS, F32_SBITS, F32_BIAS, r.b);
    return r;
}

f32
scan_f32(const char *s, size_t n, size_t *consumed) {
    u32 bits = (u32)parse_any(s, n, consumed,
                              F32_EBITS, F32_SBITS, F32_BIAS, FMT_IEEE);
    f32 v;
    memcpy(&v, &bits, sizeof v);
    return v;
}

f32
to_f32(const char *s) {
    u32 bits = (u32)parse_float(s, strlen(s), NULL, 1,
                                F32_EBITS, F32_SBITS, F32_BIAS, FMT_IEEE);
    f32 v;
    memcpy(&v, &bits, sizeof v);
    return v;
}

b32
from_f16(f16 bits) {
    b32 r;
    emit_float(bits, F16_EBITS, F16_SBITS, F16_BIAS, r.b);
    return r;
}

f16
scan_f16(const char *s, size_t n, size_t *consumed) {
    return (f16)parse_any(s, n, consumed, F16_EBITS, F16_SBITS,
                          F16_BIAS, FMT_IEEE);
}

f16
to_f16(const char *s) {
    return (f16)parse_float(s, strlen(s), NULL, 1, F16_EBITS, F16_SBITS,
                            F16_BIAS, FMT_IEEE);
}

b32
from_bf16(bf16 bits) {
    b32 r;
    emit_float(bits, BF16_EBITS, BF16_SBITS, BF16_BIAS, r.b);
    return r;
}

bf16
scan_bf16(const char *s, size_t n, size_t *consumed) {
    return (bf16)parse_any(s, n, consumed, BF16_EBITS, BF16_SBITS,
                           BF16_BIAS, FMT_IEEE);
}

bf16
to_bf16(const char *s) {
    return (bf16)parse_float(s, strlen(s), NULL, 1, BF16_EBITS, BF16_SBITS,
                             BF16_BIAS, FMT_IEEE);
}

b32
from_e5m2(e5m2 bits) {
    b32 r;
    memcpy(r.b, &e5m2_word[bits], sizeof e5m2_word[0]);
    return r;
}

e5m2
scan_e5m2(const char *s, size_t n, size_t *consumed) {
    return (e5m2)parse_any(s, n, consumed, E5M2_EBITS, E5M2_SBITS,
                           E5M2_BIAS, FMT_IEEE);
}

e5m2
to_e5m2(const char *s) {
    return (e5m2)parse_float(s, strlen(s), NULL, 1, E5M2_EBITS, E5M2_SBITS,
                             E5M2_BIAS, FMT_IEEE);
}

b32
from_e4m3(e4m3 bits) {
    b32 r;
    u64 i = bits & 0x7f;
    u64 neg = bits >> 7;
    memcpy(r.b, e4m3_row + 1 + E4M3_ROW * i - neg, 8);
    return r;
}

e4m3
scan_e4m3(const char *s, size_t n, size_t *consumed) {
    return (e4m3)parse_any(s, n, consumed, E4M3_EBITS, E4M3_SBITS,
                           E4M3_BIAS, FMT_FN);
}

e4m3
to_e4m3(const char *s) {
    return (e4m3)parse_float(s, strlen(s), NULL, 1, E4M3_EBITS, E4M3_SBITS,
                             E4M3_BIAS, FMT_FN);
}

b32
from_e2m1(e2m1 bits) {
    b32 r;
    u64 x = bits & ((1 << (E2M1_EBITS + E2M1_SBITS + 1)) - 1);
    u64 expf = (x >> E2M1_SBITS) & ((1 << E2M1_EBITS) - 1);
    u64 h = expf ? (2 + (x & 1)) << (expf - 1) : x & 1;
    u64 w = '0' + (h >> 1);
    if (h & 1) {
        w |= (u64)'.' << 8 | (u64)'5' << 16;
    }
    if (x >> (E2M1_EBITS + E2M1_SBITS)) {
        w = (w << 8) | '-';
    }
    memcpy(r.b, &w, sizeof w);
    return r;
}

e2m1
scan_e2m1(const char *s, size_t n, size_t *consumed) {
    return (e2m1)parse_any(s, n, consumed, E2M1_EBITS, E2M1_SBITS,
                           E2M1_BIAS, FMT_FINITE);
}

e2m1
to_e2m1(const char *s) {
    return (e2m1)parse_float(s, strlen(s), NULL, 1, E2M1_EBITS, E2M1_SBITS,
                             E2M1_BIAS, FMT_FINITE);
}

static void
fmt_u64(u64 v, char *out) {
    i32 n = dec_len(v);
    fmt_n(v, n, out + n);
    out[n] = '\0';
}

static force_inline u64
mag_step(u64 v, u64 d) {
    u64 nv = v * 10 + d;
    if (unlikely(v > (UINT64_MAX - 9) / 10)) {
        nv = v == UINT64_MAX / 10 && d <= 5 ? nv : UINT64_MAX;
    }
    return nv;
}

typedef struct {
    i32 ok;
    i32 neg;
    u64 mag;
} signmag;

static force_inline signmag
scan_mag(const char *s, size_t cap, size_t *end) {
    signmag out = {0};
    *end = 0;
    size_t i = 0;
    i32 sign = 0;
    if (i < cap) {
        char c = s[i];
        sign = (c == '+') | (c == '-');
        out.neg = c == '-';
    }
    i += (size_t)sign;
    u64 v = 0;
    i32 nd = 0;
    for (; i < cap && s[i]; ) {
        u64 v8;
        i32 p = 0;
        if (likely(i + 8 < cap && s[i + 1] >= '0' && s[i + 1] <= '9')) {
            p = digits8(s + i, &v8);
        }
        if (likely(p == 8)) {
            wide t = mul64(v, pow10_u64[8]);
            t.lo += v8;
            t.hi += t.lo < v8;
            v = t.hi ? UINT64_MAX : t.lo;
            nd += 8;
            i += 8;
        } else if (p) {
            for (i32 k = 0; k < p; k++) {
                v = mag_step(v, (u64)(s[i] - '0'));
                nd++;
                i++;
            }
        } else {
            u64 d = (u64)(s[i] - '0');
            if (unlikely(d > 9)) {
                break;
            }
            v = mag_step(v, d);
            nd++;
            i++;
        }
    }
    if (unlikely(!nd)) {
        return out;
    }
    out.mag = v;
    *end = i;
    out.ok = 1;
    return out;
}

static u64
mag_u64(i64 v) {
    return v < 0 ? (u64)0 - (u64)v : (u64)v;
}

static void
fmt_i64(i64 v, char *out) {
    i32 neg = v < 0;
    out[0] = '-';
    fmt_u64(mag_u64(v), out + neg);
}

static force_inline i64
scan_int(const char *s, size_t cap, size_t *end, i32 strict, i64 lo, i64 hi) {
    size_t e;
    signmag sm = scan_mag(s, cap, &e);
    if (end) {
        *end = e;
    }
    if (strict && e != cap) {
        return 0;
    }
    if (!sm.ok) {
        return 0;
    }
    u64 lim = sm.neg ? mag_u64(lo) : (u64)hi;
    u64 v = sm.mag > lim ? lim : sm.mag;
    return sm.neg ? (i64)((u64)0 - v) : (i64)v;
}

static force_inline u64
scan_uint(const char *s, size_t cap, size_t *end, i32 strict, u64 hi) {
    size_t e;
    signmag sm = scan_mag(s, cap, &e);
    if (end) {
        *end = e;
    }
    if (strict && e != cap) {
        return 0;
    }
    if (!sm.ok || (sm.neg && sm.mag)) {
        return 0;
    }
    return sm.mag > hi ? hi : sm.mag;
}

static no_inline u64
scan_uint_any(const char *s, size_t cap, size_t *end, u64 hi) {
    return scan_uint(s, cap, end, 0, hi);
}

static no_inline i64
scan_int_any(const char *s, size_t cap, size_t *end, i64 lo, i64 hi) {
    return scan_int(s, cap, end, 0, lo, hi);
}

b32
from_u8(u8 v) {
    b32 r;
    fmt_u64(v, r.b);
    return r;
}

u8
scan_u8(const char *s, size_t n, size_t *consumed) {
    return (u8)scan_uint_any(s, n, consumed, UINT8_MAX);
}

u8
to_u8(const char *s) {
    return (u8)scan_uint(s, strlen(s), NULL, 1, UINT8_MAX);
}

b32
from_i8(i8 v) {
    b32 r;
    fmt_i64(v, r.b);
    return r;
}

i8
scan_i8(const char *s, size_t n, size_t *consumed) {
    return (i8)scan_int_any(s, n, consumed, INT8_MIN, INT8_MAX);
}

i8
to_i8(const char *s) {
    return (i8)scan_int(s, strlen(s), NULL, 1, INT8_MIN, INT8_MAX);
}

b32
from_u16(u16 v) {
    b32 r;
    fmt_u64(v, r.b);
    return r;
}

u16
scan_u16(const char *s, size_t n, size_t *consumed) {
    return (u16)scan_uint_any(s, n, consumed, UINT16_MAX);
}

u16
to_u16(const char *s) {
    return (u16)scan_uint(s, strlen(s), NULL, 1, UINT16_MAX);
}

b32
from_i16(i16 v) {
    b32 r;
    fmt_i64(v, r.b);
    return r;
}

i16
scan_i16(const char *s, size_t n, size_t *consumed) {
    return (i16)scan_int_any(s, n, consumed, INT16_MIN, INT16_MAX);
}

i16
to_i16(const char *s) {
    return (i16)scan_int(s, strlen(s), NULL, 1, INT16_MIN, INT16_MAX);
}

b32
from_u32(u32 v) {
    b32 r;
    fmt_u64(v, r.b);
    return r;
}

u32
scan_u32(const char *s, size_t n, size_t *consumed) {
    return (u32)scan_uint_any(s, n, consumed, UINT32_MAX);
}

u32
to_u32(const char *s) {
    return (u32)scan_uint(s, strlen(s), NULL, 1, UINT32_MAX);
}

b32
from_i32(i32 v) {
    b32 r;
    fmt_i64(v, r.b);
    return r;
}

i32
scan_i32(const char *s, size_t n, size_t *consumed) {
    return (i32)scan_int_any(s, n, consumed, INT32_MIN, INT32_MAX);
}

i32
to_i32(const char *s) {
    return (i32)scan_int(s, strlen(s), NULL, 1, INT32_MIN, INT32_MAX);
}

b32
from_u64(u64 v) {
    b32 r;
    fmt_u64(v, r.b);
    return r;
}

u64
scan_u64(const char *s, size_t n, size_t *consumed) {
    return scan_uint_any(s, n, consumed, UINT64_MAX);
}

u64
to_u64(const char *s) {
    return scan_uint(s, strlen(s), NULL, 1, UINT64_MAX);
}

b32
from_i64(i64 v) {
    b32 r;
    fmt_i64(v, r.b);
    return r;
}

i64
scan_i64(const char *s, size_t n, size_t *consumed) {
    return scan_int_any(s, n, consumed, INT64_MIN, INT64_MAX);
}

i64
to_i64(const char *s) {
    return scan_int(s, strlen(s), NULL, 1, INT64_MIN, INT64_MAX);
}
