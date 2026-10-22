#include "../snrt.c"

#include <stdio.h>
#include <string.h>

static int fails = 0;

static void
bad(const char *what, u64 bits) {
    if (fails < 5) {
        printf("FAIL %s at %016llx\n", what, (unsigned long long)bits);
    }
    fails++;
}

static f32
bits_f32(u32 b) {
    f32 v;
    memcpy(&v, &b, sizeof b);
    return v;
}

static u32
f32_bits(f32 v) {
    u32 b;
    memcpy(&b, &v, sizeof v);
    return b;
}

static f64
bits_f64(u64 b) {
    f64 v;
    memcpy(&v, &b, sizeof b);
    return v;
}

static u64
f64_bits(f64 v) {
    u64 b;
    memcpy(&b, &v, sizeof v);
    return b;
}

static void
rt_e2m1(void) {
    for (u32 x = 0; x < 16; x++) {
        e2m1 v = (e2m1)x;
        e2m1b t = from_e2m1(v);
        e2m1 w = to_e2m1(t);
        if (w != v) {
            bad("e2m1 to(from(x)) != x", x);
        }
        e2m1b t2 = from_e2m1(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("e2m1 from(to(x)) != x", x);
        }
    }
    printf("e2m1 16 values\n");
}

static void
rt_e4m3(void) {
    for (u32 x = 0; x < 256; x++) {
        if ((x & 0x7F) == 0x7F && x != 0x7F) {
            continue;
        }
        e4m3 v = (e4m3)x;
        e4m3b t = from_e4m3(v);
        e4m3 w = to_e4m3(t);
        if (w != v) {
            bad("e4m3 to(from(x)) != x", x);
        }
        e4m3b t2 = from_e4m3(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("e4m3 from(to(x)) != x", x);
        }
    }
    printf("e4m3 256 values\n");
}

static void
rt_e5m2(void) {
    for (u32 x = 0; x < 256; x++) {
        if ((x & 0x7C) == 0x7C && (x & 0x3) && x != 0x7E) {
            continue;
        }
        e5m2 v = (e5m2)x;
        e5m2b t = from_e5m2(v);
        e5m2 w = to_e5m2(t);
        if (w != v) {
            bad("e5m2 to(from(x)) != x", x);
        }
        e5m2b t2 = from_e5m2(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("e5m2 from(to(x)) != x", x);
        }
    }
    printf("e5m2 256 values\n");
}

static void
rt_f16(void) {
    for (u32 x = 0; x <= 0xFFFF; x++) {
        if ((x & 0x7C00) == 0x7C00 && (x & 0x3FF) && x != 0x7E00) {
            continue;
        }
        f16 v = (f16)x;
        f16b t = from_f16(v);
        f16 w = to_f16(t);
        if (w != v) {
            bad("f16 to(from(x)) != x", x);
        }
        f16b t2 = from_f16(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("f16 from(to(x)) != x", x);
        }
    }
    printf("f16 65536 values\n");
}

static void
rt_bf16(void) {
    for (u32 x = 0; x <= 0xFFFF; x++) {
        if ((x & 0x7F80) == 0x7F80 && (x & 0x7F) && x != 0x7FC0) {
            continue;
        }
        bf16 v = (bf16)x;
        bf16b t = from_bf16(v);
        bf16 w = to_bf16(t);
        if (w != v) {
            bad("bf16 to(from(x)) != x", x);
        }
        bf16b t2 = from_bf16(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("bf16 from(to(x)) != x", x);
        }
    }
    printf("bf16 65536 values\n");
}

static void
rt_u8(void) {
    for (u32 x = 0; x <= UINT8_MAX; x++) {
        u8 v = (u8)x;
        u8b t = from_u8(v);
        u8 w = to_u8(t);
        if (w != v) {
            bad("u8 to(from(x)) != x", x);
        }
        u8b t2 = from_u8(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("u8 from(to(x)) != x", x);
        }
    }
    printf("u8 256 values\n");
}

static void
rt_i8(void) {
    for (i32 x = INT8_MIN; x <= INT8_MAX; x++) {
        i8 v = (i8)x;
        i8b t = from_i8(v);
        i8 w = to_i8(t);
        if (w != v) {
            bad("i8 to(from(x)) != x", (u64)x);
        }
        i8b t2 = from_i8(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("i8 from(to(x)) != x", (u64)x);
        }
    }
    printf("i8 256 values\n");
}

static void
rt_u16(void) {
    for (u32 x = 0; x <= UINT16_MAX; x++) {
        u16 v = (u16)x;
        u16b t = from_u16(v);
        u16 w = to_u16(t);
        if (w != v) {
            bad("u16 to(from(x)) != x", x);
        }
        u16b t2 = from_u16(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("u16 from(to(x)) != x", x);
        }
    }
    printf("u16 65536 values\n");
}

static void
rt_i16(void) {
    for (i32 x = INT16_MIN; x <= INT16_MAX; x++) {
        i16 v = (i16)x;
        i16b t = from_i16(v);
        i16 w = to_i16(t);
        if (w != v) {
            bad("i16 to(from(x)) != x", (u64)x);
        }
        i16b t2 = from_i16(w);
        if (strcmp(t2.b, t.b) != 0) {
            bad("i16 from(to(x)) != x", (u64)x);
        }
    }
    printf("i16 65536 values\n");
}

#define SAMPLES 10000000ull

static u64 nchk = 0;
static u64 rng = 0x243F6A8885A308D3ull;

static u64
rnd(void) {
    u64 z = (rng += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static void
chk_u32(u32 v) {
    nchk++;
    u32b t = from_u32(v);
    u32 w = to_u32(t);
    if (w != v) {
        bad("u32 to(from(x)) != x", v);
    }
    u32b t2 = from_u32(w);
    if (strcmp(t2.b, t.b) != 0) {
        bad("u32 from(to(x)) != x", v);
    }
}

static void
rt_u32(void) {
    u64 start = nchk;
    u32 p = 1;
    for (;;) {
        chk_u32(p);
        chk_u32(p - 1);
        chk_u32(p + 1);
        if (p > UINT32_MAX / 10) {
            break;
        }
        p *= 10;
    }
    for (u32 k = 0; k < 32; k++) {
        u32 b = 1u << k;
        chk_u32(b - 1);
        chk_u32(b);
        chk_u32(b + 1);
    }
    chk_u32(UINT32_MAX);
    for (u64 n = 0; n < SAMPLES; n++) {
        chk_u32(((u32)rnd()) >> (n % 32));
    }
    printf("u32 %llu values\n", (unsigned long long)(nchk - start));
}

static void
chk_i32(i32 v) {
    nchk++;
    i32b t = from_i32(v);
    i32 w = to_i32(t);
    if (w != v) {
        bad("i32 to(from(x)) != x", (u64)(u32)v);
    }
    i32b t2 = from_i32(w);
    if (strcmp(t2.b, t.b) != 0) {
        bad("i32 from(to(x)) != x", (u64)(u32)v);
    }
}

static void
rt_i32(void) {
    u64 start = nchk;
    i64 p = 1;
    for (;;) {
        chk_i32((i32)p);
        chk_i32((i32)(p - 1));
        chk_i32((i32)(p + 1));
        chk_i32((i32)-p);
        chk_i32((i32)(-p + 1));
        chk_i32((i32)(-p - 1));
        if (p > INT32_MAX / 10) {
            break;
        }
        p *= 10;
    }
    for (u32 k = 0; k < 31; k++) {
        i32 b = (i32)1 << k;
        chk_i32(b - 1);
        chk_i32(b);
        chk_i32(b + 1);
        chk_i32(-b - 1);
        chk_i32(-b);
        chk_i32(-b + 1);
    }
    chk_i32(INT32_MIN);
    chk_i32(INT32_MAX);
    for (u64 n = 0; n < SAMPLES; n++) {
        u32 r = (u32)rnd();
        i32 v = (i32)(r >> (1 + n % 31));
        chk_i32(r & 1 ? -v : v);
    }
    printf("i32 %llu values\n", (unsigned long long)(nchk - start));
}

static void
chk_f32(u32 bits) {
    nchk++;
    f32 v = bits_f32(bits);
    f32b t = from_f32(v);
    f32 w = to_f32(t);
    if (f32_bits(w) != bits) {
        bad("f32 to(from(x)) != x", bits);
    }
    f32b t2 = from_f32(w);
    if (strcmp(t2.b, t.b) != 0) {
        bad("f32 from(to(x)) != x", bits);
    }
}

static void
rt_f32(void) {
    u64 start = nchk;
    static const u32 edges[] = {
        0x00000000u, 0x80000000u,                // +-0
        0x3F800000u, 0xBF800000u,                // +-1
        0x7F800000u, 0xFF800000u,                // +-inf
        0x7FC00000u,                             // nan
        0x00000001u, 0x80000001u,                // min subnormal
        0x007FFFFFu, 0x807FFFFFu,                // max subnormal
        0x00800000u, 0x80800000u,                // min normal
        0x7F7FFFFFu, 0xFF7FFFFFu,                // max finite
        0x3F800001u, 0x3F7FFFFFu,                // 1 + ulp, nextbelow(1)
        0x4B800000u, 0x4B800001u, 0x4B7FFFFFu,   // 2^24, 2^24 + 2, nextbelow
    };
    for (u32 i = 0; i < COUNT(edges); i++) {
        chk_f32(edges[i]);
    }
    for (u32 s = 0; s < 2; s++) {
        u32 sign = s << 31;
        for (u32 e = 0; e <= 0xFE; e++) {
            u32 base = sign | (e << 23);
            chk_f32(base);                       // exact power of two
            chk_f32(base | 1);
            chk_f32(base | 0x00400000u);         // half ulp
            chk_f32(base | 0x007FFFFFu);         // next power - 1
        }
    }
    for (u32 k = 0; k <= 1000; k++) {
        chk_f32(f32_bits((f32)k));
    }
    static const struct {
        const char *s;
        u32 even, odd;
    } ties[] = {
        { "3e10",      0x50DF8476u, 0x50DF8475u },
        { "9e9",       0x50061C46u, 0x50061C47u },
        { "1.1e10",    0x5023E9ACu, 0x5023E9ABu },
        { "1.5e10",    0x505F8476u, 0x505F8475u },
        { "1.7e10",    0x507D51DAu, 0x507D51DBu },
        { "4.5e9",     0x4F861C46u, 0x4F861C47u },
        { "4.3e9",     0x4F802666u, 0x4F802665u },
        { "16777217",  0x4B800000u, 0x4B800001u },
        { "-3e10",     0xD0DF8476u, 0xD0DF8475u },
        { "-9e9",      0xD0061C46u, 0xD0061C47u },
        { "-16777217", 0xCB800000u, 0xCB800001u },
    };
    for (u32 i = 0; i < COUNT(ties); i++) {
        f32b t;
        strcpy(t.b, ties[i].s);
        if (f32_bits(to_f32(t)) != ties[i].even) {
            bad("f32 tie must round to even", ties[i].even);
        }
        if (strlen(from_f32(bits_f32(ties[i].even)).b) > strlen(ties[i].s)) {
            bad("f32 tie neighbour text too long", ties[i].even);
        }
        chk_f32(ties[i].even);
        chk_f32(ties[i].odd);
    }
    for (u32 len = 1; len <= 9; len++) {
        char buf[24];
        for (u32 d = 0; d < len; d++) {
            buf[d] = (char)('1' + (d + len) % 9);
        }
        for (i32 e = -30; e <= 30; e += 30) {
            snprintf(buf + len, sizeof buf - len, "e%d", (int)e);
            f32b t;
            strcpy(t.b, buf);
            chk_f32(f32_bits(to_f32(t)));
        }
    }
    for (u64 n = 0; n < SAMPLES; n++) {
        u32 bits = (u32)rnd();
        if ((bits & 0x7F800000u) == 0x7F800000u &&
            (bits & 0x007FFFFFu) && bits != 0x7FC00000u) {
            continue;
        }
        chk_f32(bits);
    }
    printf("f32 %llu values\n", (unsigned long long)(nchk - start));
}

static void
chk_u64(u64 v) {
    nchk++;
    u64b t = from_u64(v);
    u64 w = to_u64(t);
    if (w != v) {
        bad("u64 to(from(x)) != x", v);
    }
    u64b t2 = from_u64(w);
    if (strcmp(t2.b, t.b) != 0) {
        bad("u64 from(to(x)) != x", v);
    }
}

static void
rt_u64(void) {
    u64 start = nchk;
    u64 p = 1;
    for (;;) {
        chk_u64(p);
        chk_u64(p - 1);
        chk_u64(p + 1);
        if (p > UINT64_MAX / 10) {
            break;
        }
        p *= 10;
    }
    for (u32 k = 0; k < 64; k++) {
        u64 b = 1ull << k;
        chk_u64(b - 1);
        chk_u64(b);
        chk_u64(b + 1);
    }
    chk_u64(UINT64_MAX);
    for (u64 n = 0; n < SAMPLES; n++) {
        chk_u64(rnd() >> (n % 64));
    }
    printf("u64 %llu values\n", (unsigned long long)(nchk - start));
}

static void
chk_i64(i64 v) {
    nchk++;
    i64b t = from_i64(v);
    i64 w = to_i64(t);
    if (w != v) {
        bad("i64 to(from(x)) != x", (u64)v);
    }
    i64b t2 = from_i64(w);
    if (strcmp(t2.b, t.b) != 0) {
        bad("i64 from(to(x)) != x", (u64)v);
    }
}

static void
rt_i64(void) {
    u64 start = nchk;
    u64 p = 1;
    for (;;) {
        chk_i64((i64)p);
        chk_i64((i64)(p - 1));
        chk_i64((i64)(p + 1));
        chk_i64(-(i64)p);
        chk_i64(-(i64)p + 1);
        chk_i64(-(i64)p - 1);
        if (p > (u64)INT64_MAX / 10) {
            break;
        }
        p *= 10;
    }
    for (u32 k = 0; k < 63; k++) {
        i64 b = (i64)1 << k;
        chk_i64(b - 1);
        chk_i64(b);
        chk_i64(b + 1);
        chk_i64(-b - 1);
        chk_i64(-b);
        chk_i64(-b + 1);
    }
    chk_i64(INT64_MIN);
    chk_i64(INT64_MAX);
    for (u64 n = 0; n < SAMPLES; n++) {
        u64 r = rnd();
        i64 v = (i64)(r >> (1 + n % 63));
        chk_i64(r & 1 ? -v : v);
    }
    printf("i64 %llu values\n", (unsigned long long)(nchk - start));
}

static void
chk_f64(u64 bits) {
    nchk++;
    f64 v = bits_f64(bits);
    f64b t = from_f64(v);
    f64 w = to_f64(t);
    if (f64_bits(w) != bits) {
        bad("f64 to(from(x)) != x", bits);
    }
    f64b t2 = from_f64(w);
    if (strcmp(t2.b, t.b) != 0) {
        bad("f64 from(to(x)) != x", bits);
    }
}

static void
rt_f64(void) {
    u64 start = nchk;
    static const u64 edges[] = {
        0x0000000000000000ull, 0x8000000000000000ull,   // +-0
        0x3FF0000000000000ull, 0xBFF0000000000000ull,   // +-1
        0x7FF0000000000000ull, 0xFFF0000000000000ull,   // +-inf
        0x7FF8000000000000ull,                          // nan
        0x0000000000000001ull, 0x8000000000000001ull,   // min subnormal
        0x000FFFFFFFFFFFFFull, 0x800FFFFFFFFFFFFFull,   // max subnormal
        0x0010000000000000ull, 0x8010000000000000ull,   // min normal
        0x7FEFFFFFFFFFFFFFull, 0xFFEFFFFFFFFFFFFFull,   // max finite
        0x3FF0000000000001ull, 0x3FEFFFFFFFFFFFFFull,   // 1 + ulp, nextbelow(1)
        0x4340000000000000ull, 0x4340000000000001ull,   // 2^53, 2^53 + 2
        0x433FFFFFFFFFFFFFull,                          // nextbelow(2^53)
    };
    for (u32 i = 0; i < COUNT(edges); i++) {
        chk_f64(edges[i]);
    }
    for (u32 s = 0; s < 2; s++) {
        u64 sign = (u64)s << 63;
        for (u32 e = 0; e <= 0x7FE; e++) {
            u64 base = sign | ((u64)e << 52);
            chk_f64(base);                           // exact power of two
            chk_f64(base | 1);
            chk_f64(base | 0x0008000000000000ull);   // half ulp
            chk_f64(base | 0x000FFFFFFFFFFFFFull);   // next power - 1
        }
    }
    for (u32 k = 0; k <= 1000; k++) {
        chk_f64(f64_bits((f64)k));
    }
    static const struct {
        const char *s;
        u64 even, odd;
    } ties[] = {
        { "5e22",              0x44A52D02C7E14AF6ull, 0x44A52D02C7E14AF7ull },
        { "7e22",              0x44ADA56A4B0835C0ull, 0x44ADA56A4B0835BFull },
        { "1e23",              0x44B52D02C7E14AF6ull, 0x44B52D02C7E14AF7ull },
        { "2.1e22",            0x4491C9A62D04ED0Cull, 0x4491C9A62D04ED0Dull },
        { "1.25e22",           0x44852D02C7E14AF6ull, 0x44852D02C7E14AF7ull },
        { "9.5e21",            0x448017F7DF96BE18ull, 0x448017F7DF96BE17ull },
        { "9007199254740993",  0x4340000000000000ull, 0x4340000000000001ull },
        { "-5e22",             0xC4A52D02C7E14AF6ull, 0xC4A52D02C7E14AF7ull },
        { "-1e23",             0xC4B52D02C7E14AF6ull, 0xC4B52D02C7E14AF7ull },
        { "-9007199254740993", 0xC340000000000000ull, 0xC340000000000001ull },
    };
    for (u32 i = 0; i < COUNT(ties); i++) {
        f64b t;
        strcpy(t.b, ties[i].s);
        if (f64_bits(to_f64(t)) != ties[i].even) {
            bad("f64 tie must round to even", ties[i].even);
        }
        if (strlen(from_f64(bits_f64(ties[i].even)).b) > strlen(ties[i].s)) {
            bad("f64 tie neighbour text too long", ties[i].even);
        }
        chk_f64(ties[i].even);
        chk_f64(ties[i].odd);
    }
    for (u32 len = 1; len <= 17; len++) {
        char buf[32];
        for (u32 d = 0; d < len; d++) {
            buf[d] = (char)('1' + (d + len) % 9);
        }
        for (i32 e = -300; e <= 300; e += 300) {
            snprintf(buf + len, sizeof buf - len, "e%d", (int)e);
            f64b t;
            strcpy(t.b, buf);
            chk_f64(f64_bits(to_f64(t)));
        }
    }
    for (u64 n = 0; n < SAMPLES; n++) {
        u64 bits = rnd();
        if ((bits & 0x7FF0000000000000ull) == 0x7FF0000000000000ull &&
            (bits & 0x000FFFFFFFFFFFFFull) && bits != 0x7FF8000000000000ull) {
            continue;
        }
        chk_f64(bits);
    }
    printf("f64 %llu values\n", (unsigned long long)(nchk - start));
}

int
main(void) {
    printf("snrt round-trip tests\n");
    rt_e2m1();
    rt_e4m3();
    rt_e5m2();
    rt_f16();
    rt_bf16();
    rt_u8();
    rt_i8();
    rt_u16();
    rt_i16();
    rt_u32();
    rt_i32();
    rt_f32();
    rt_u64();
    rt_i64();
    rt_f64();
    printf("%d failures: %s\n", fails, fails ? "FAIL" : "PASS");
    return fails;
}
