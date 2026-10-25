#include "../snrt.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *s;
    u64 want;
} row;

static int fails = 0;
static u64 nchk = 0;

static void
chk(const char *what, const char *s, u64 got, u64 want) {
    nchk++;
    if (got != want) {
        if (fails < 5) {
            printf("FAIL %s to(\"%s\") = %llx want %llx\n", what, s,
                   (unsigned long long)got, (unsigned long long)want);
        }
        fails++;
    }
}

static u64
f64_bits(f64 v) {
    u64 b;
    memcpy(&b, &v, sizeof v);
    return b;
}

static u64
f32_bits(f32 v) {
    u32 b;
    memcpy(&b, &v, sizeof v);
    return b;
}

static void
p_f64(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",                       0x0000000000000000ull },
        { "0.0",                     0x0000000000000000ull },
        { "-0",                      0x8000000000000000ull },
        { "-0.0",                    0x8000000000000000ull },
        { "+0",                      0x0000000000000000ull },
        { "1",                       0x3FF0000000000000ull },
        { "42",                      0x4045000000000000ull },
        { "-42",                     0xC045000000000000ull },
        { "+42",                     0x4045000000000000ull },
        { "007",                     0x401C000000000000ull },
        { "0.1",                     0x3FB999999999999Aull },
        { "0.25",                    0x3FD0000000000000ull },
        { "5.",                      0x4014000000000000ull },
        { ".5",                      0x3FE0000000000000ull },
        { ".25",                     0x3FD0000000000000ull },
        { "1e21",                    0x444B1AE4D6E2EF50ull },
        { "1E21",                    0x444B1AE4D6E2EF50ull },
        { "1e+21",                   0x444B1AE4D6E2EF50ull },
        { "1e-21",                   0x3B92E3B40A0E9B4Full },
        { "5.96046e-8",              0x3E6FFFFE6CB2FA82ull },
        { "1e0",                     0x3FF0000000000000ull },
        { "1e+0",                    0x3FF0000000000000ull },
        { "1e-0",                    0x3FF0000000000000ull },
        { "5.e3",                    0x40B3880000000000ull },
        { ".5e3",                    0x407F400000000000ull },
        { "1.7976931348623157e308",  0x7FEFFFFFFFFFFFFFull },
        { "2.2250738585072014e-308", 0x0010000000000000ull },
        { "4.9e-324",                0x0000000000000001ull },
        { "5e-324",                  0x0000000000000001ull },
        { "2.4e-324",                0x0000000000000000ull },
        { "1e309",                   0x7FF0000000000000ull },
        { "-1e309",                  0xFFF0000000000000ull },
        { "1e-400",                  0x0000000000000000ull },
        { "-1e-400",                 0x8000000000000000ull },
        { "1e99999999999999999999",  0x7FF0000000000000ull },
        { "inf",                     0x7FF0000000000000ull },
        { "Inf",                     0x7FF0000000000000ull },
        { "INF",                     0x7FF0000000000000ull },
        { "infinity",                0x7FF0000000000000ull },
        { "Infinity",                0x7FF0000000000000ull },
        { "INFINITY",                0x7FF0000000000000ull },
        { "-inf",                    0xFFF0000000000000ull },
        { "-infinity",               0xFFF0000000000000ull },
        { "+inf",                    0x7FF0000000000000ull },
        { "nan",                     0x7FF8000000000000ull },
        { "NaN",                     0x7FF8000000000000ull },
        { "NAN",                     0x7FF8000000000000ull },
        { "-nan",                    0xFFF8000000000000ull },
        { "+nan",                    0x7FF8000000000000ull },
    };
    static const char *no[] = {
        "", " ", "+", "-", ".",
        "abc", "nanx", "infx", "infinityx",
        "in", "infi", "na", "nannan",
        "1e", "1e+", "1e-", "1efoo", "1e5e5",
        "1.2.3", "0..5", "..5", ".-5", "5.-", "1,5",
        "--1", "+-5", "1-", "0x10", ".e5", "e5",
        "42 ", " 42", "1 2",
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("f64", ok[i].s, f64_bits(to_f64(ok[i].s)), ok[i].want);
    }
    for (u32 i = 0; i < COUNT(no); i++) {
        chk("f64", no[i], f64_bits(to_f64(no[i])), 0);
    }
    printf("f64 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_f32(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0.1",                 0x3DCCCCCDu },
        { "0.5",                 0x3F000000u },
        { "-0",                  0x80000000u },
        { "16777217",            0x4B800000u },
        { "16777217.0000000005", 0x4B800001u },
        { "3.4028235e38",        0x7F7FFFFFu },
        { "1e39",                0x7F800000u },
        { "-1e39",               0xFF800000u },
        { "1e-50",               0x00000000u },
        { "1e-45",               0x00000001u },
        { "7e-46",               0x00000000u },
        { "7.1e-46",             0x00000001u },
        { "1.5e-45",             0x00000001u },
        { "inf",                 0x7F800000u },
        { "-inf",                0xFF800000u },
        { "nan",                 0x7FC00000u },
        { "-nan",                0xFFC00000u },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("f32", ok[i].s, f32_bits(to_f32(ok[i].s)), ok[i].want);
    }
    static const row slow[] = {
        { "12345678901234567890",    0x5F2B54AAu },
        { "123456789012345.67890",   0x56E0910Cu },
        { "12345678901234567890123", 0x642750AEu },
        { "18446744073709551616",    0x5F800000u },
        { "99999999999999999999999", 0x65A96816u },
    };
    for (u32 i = 0; i < COUNT(slow); i++) {
        chk("f32", slow[i].s, f32_bits(to_f32(slow[i].s)), slow[i].want);
    }
    printf("f32 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_f16(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0.1",            0x2E66u },
        { "1",              0x3C00u },
        { "-2.5",           0xC100u },
        { "65504",          0x7BFFu },
        { "65520",          0x7C00u },
        { "5.96046e-8",     0x0001u },
        { "6.103515625e-5", 0x0400u },
        { "1e-8",           0x0000u },
        { "3e-8",           0x0001u },
        { "7e-8",           0x0001u },
        { "1e-9",           0x0000u },
        { "inf",            0x7C00u },
        { "-inf",           0xFC00u },
        { "nan",            0x7E00u },
        { "-nan",           0xFE00u },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("f16", ok[i].s, to_f16(ok[i].s), ok[i].want);
    }
    printf("f16 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_bf16(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0.1",          0x3DCDu },
        { "1",            0x3F80u },
        { "-2.5",         0xC020u },
        { "3.3895314e38", 0x7F7Fu },
        { "3.4e38",       0x7F80u },
        { "1e39",         0x7F80u },
        { "1e-45",        0x0000u },
        { "1e-46",        0x0000u },
        { "inf",          0x7F80u },
        { "-inf",         0xFF80u },
        { "nan",          0x7FC0u },
        { "-nan",         0xFFC0u },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("bf16", ok[i].s, to_bf16(ok[i].s), ok[i].want);
    }
    printf("bf16 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_e5m2(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",     0x00u },
        { "1",     0x3Cu },
        { "-2.5",  0xC1u },
        { "0.2",   0x32u },
        { "0.3",   0x35u },
        { "57344", 0x7Bu },
        { "57600", 0x7Bu },
        { "61439", 0x7Bu },
        { "61440", 0x7Cu },
        { "61441", 0x7Cu },
        { "1e30",  0x7Cu },
        { "-1e30", 0xFCu },
        { "inf",   0x7Cu },
        { "-inf",  0xFCu },
        { "nan",   0x7Eu },
        { "-nan",  0xFEu },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("e5m2", ok[i].s, to_e5m2(ok[i].s), ok[i].want);
    }
    printf("e5m2 %llu spellings\n", (unsigned long long)(nchk - start));
}

static u32
e4m3_int(u64 n) {
    if (!n) {
        return 0;
    }
    u32 e = 0;
    while (((u64)1 << (e + 1)) <= n) {
        e++;
    }
    u64 den = (u64)1 << e;
    u64 q = (n << 3) / den;
    u64 r = (n << 3) - q * den;
    if (r * 2 > den || (r * 2 == den && (q & 1))) {
        if (++q == 16) {
            q = 8;
            e++;
        }
    }
    if (e + 7 > 15 || (e + 7 == 15 && q == 15)) {
        return 0x7E;
    }
    return (u32)((e + 7) << 3) | (u32)(q - 8);
}

static void
p_e4m3(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",     0x00u },
        { "1",     0x38u },
        { "-2.5",  0xC2u },
        { "0.2",   0x25u },
        { "0.3",   0x2Au },
        { "448",   0x7Eu },
        { "447",   0x7Eu },
        { "500",   0x7Eu },
        { "1e30",  0x7Eu },
        { "-1e30", 0xFEu },
        { "inf",   0x7Eu },
        { "-inf",  0xFEu },
        { "nan",   0x7Fu },
        { "-nan",  0xFFu },
        { "8.5",   0x50u },
        { "-0",    0x80u },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("e4m3", ok[i].s, to_e4m3(ok[i].s), ok[i].want);
    }
    for (i64 v = -600; v <= 600; v++) {
        char buf[8];
        snprintf(buf, sizeof buf, "%lld", (long long)v);
        chk("e4m3", buf, to_e4m3(buf),
            e4m3_int((u64)(v < 0 ? -v : v)) | (v < 0 ? 0x80u : 0));
    }
    printf("e4m3 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_e2m1(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",     0x00u },
        { "0.1",   0x00u },
        { "0.3",   0x01u },
        { "-0.3",  0x09u },
        { "3",     0x05u },
        { "4",     0x06u },
        { "2.6",   0x05u },
        { "6",     0x07u },
        { "7",     0x07u },
        { "1e30",  0x07u },
        { "-1e30", 0x0Fu },
        { "inf",   0x07u },
        { "-inf",  0x0Fu },
        { "nan",   0x00u },
        { "-nan",  0x00u },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("e2m1", ok[i].s, to_e2m1(ok[i].s), ok[i].want);
    }
    printf("e2m1 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_u8(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",     0 },
        { "007",   7 },
        { "255", 255 },
        { "256", 255 },
        { "999", 255 },
        { "-0",    0 },
        { "-1",    0 },
        { "1.5",   0 },
        { "1e5",   0 },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("u8", ok[i].s, to_u8(ok[i].s), ok[i].want);
    }
    printf("u8 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_i8(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",    0x0000000000000000ull },
        { "127",  0x000000000000007Full },
        { "128",  0x000000000000007Full },
        { "-128", 0xFFFFFFFFFFFFFF80ull },
        { "-129", 0xFFFFFFFFFFFFFF80ull },
        { "-1",   0xFFFFFFFFFFFFFFFFull },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("i8", ok[i].s, (u64)(i64)to_i8(ok[i].s), ok[i].want);
    }
    printf("i8 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_u16(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",         0 },
        { "65535", 65535 },
        { "65536", 65535 },
        { "-1",        0 },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("u16", ok[i].s, to_u16(ok[i].s), ok[i].want);
    }
    printf("u16 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_i16(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",      0x0000000000000000ull },
        { "32767",  0x0000000000007FFFull },
        { "32768",  0x0000000000007FFFull },
        { "-32768", 0xFFFFFFFFFFFF8000ull },
        { "-32769", 0xFFFFFFFFFFFF8000ull },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("i16", ok[i].s, (u64)(i64)to_i16(ok[i].s), ok[i].want);
    }
    printf("i16 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_u32(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",                       0 },
        { "42",                     42 },
        { "+42",                    42 },
        { "007",                     7 },
        { "4294967295",  0xFFFFFFFFull },
        { "4294967296",  0xFFFFFFFFull },
        { "42949672950", 0xFFFFFFFFull },
        { "-0",                      0 },
        { "-1",                      0 },
    };
    static const char *no[] = {
        "", "+", "-", "abc", "1.5", "1e5", "12 3", "0x10",
        "1-2", "--1", " 1", "1 ",
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("u32", ok[i].s, to_u32(ok[i].s), ok[i].want);
    }
    for (u32 i = 0; i < COUNT(no); i++) {
        chk("u32", no[i], to_u32(no[i]), 0);
    }
    printf("u32 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_i32(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",           0x0000000000000000ull },
        { "42",          0x000000000000002Aull },
        { "-42",         0xFFFFFFFFFFFFFFD6ull },
        { "+7",          0x0000000000000007ull },
        { "2147483647",  0x000000007FFFFFFFull },
        { "2147483648",  0x000000007FFFFFFFull },
        { "-2147483648", 0xFFFFFFFF80000000ull },
        { "-2147483649", 0xFFFFFFFF80000000ull },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("i32", ok[i].s, (u64)(i64)to_i32(ok[i].s), ok[i].want);
    }
    printf("i32 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_u64(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",                                           0 },
        { "42",                                         42 },
        { "18446744073709551615",    0xFFFFFFFFFFFFFFFFull },
        { "18446744073709551616",    0xFFFFFFFFFFFFFFFFull },
        { "99999999999999999999999", 0xFFFFFFFFFFFFFFFFull },
        { "-0",                                          0 },
        { "-1",                                          0 },
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("u64", ok[i].s, to_u64(ok[i].s), ok[i].want);
    }
    printf("u64 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
p_i64(void) {
    u64 start = nchk;
    static const row ok[] = {
        { "0",                       0x0000000000000000ull },
        { "-42",                     0xFFFFFFFFFFFFFFD6ull },
        { "9223372036854775807",     0x7FFFFFFFFFFFFFFFull },
        { "9223372036854775808",     0x7FFFFFFFFFFFFFFFull },
        { "99999999999999999999999", 0x7FFFFFFFFFFFFFFFull },
        { "-9223372036854775808",    0x8000000000000000ull },
        { "-9223372036854775809",    0x8000000000000000ull },
        { "-9999999999999999999999", 0x8000000000000000ull },
    };
    static const char *no[] = {
        "", "+", "-", "abc", "1.5", "1e5", "12 3", "0x10",
        "1-2", "--1", " 1", "1 ",
    };
    for (u32 i = 0; i < COUNT(ok); i++) {
        chk("i64", ok[i].s, (u64)to_i64(ok[i].s), ok[i].want);
    }
    for (u32 i = 0; i < COUNT(no); i++) {
        chk("i64", no[i], (u64)to_i64(no[i]), 0);
    }
    printf("i64 %llu spellings\n", (unsigned long long)(nchk - start));
}

static void
pchk(int ok, const char *what, const char *s) {
    nchk++;
    if (!ok) {
        if (fails < 5) {
            printf("FAIL %s \"%s\"\n", what, s);
        }
        fails++;
    }
}

static u64 rnd_state = 0x9E3779B97F4A7C15ull;

static u64
rnd(void) {
    rnd_state ^= rnd_state << 13;
    rnd_state ^= rnd_state >> 7;
    rnd_state ^= rnd_state << 17;
    return rnd_state;
}

typedef struct {
    const char *s;
    u64 len;
} lrow;

static void
s_f64(const char *s, u64 want) {
    char pre[64];
    size_t c;
    u64 got = f64_bits(scan_f64(s, strlen(s), &c));
    memcpy(pre, s, want);
    pre[want] = '\0';
    chk("scan_f64.len", s, c, want);
    chk("scan_f64.val", s, got, f64_bits(to_f64(pre)));
}

static void
s_u64(const char *s, u64 want) {
    char pre[64];
    size_t c;
    u64 got = scan_u64(s, strlen(s), &c);
    memcpy(pre, s, want);
    pre[want] = '\0';
    chk("scan_u64.len", s, c, want);
    chk("scan_u64.val", s, got, to_u64(pre));
}

static void
p_scan_table(void) {
    u64 start = nchk;
    static const lrow fl[] = {
        { "1.5x",      3 }, { "1.5e+",    3 }, { "1.5e-",     3 }, { "1.5e",  3 },
        { "1.5e+x",    3 }, { "1.5e5x",   5 }, { "1e",        1 }, { "1e+",   1 },
        { "1e-",       1 }, { "1ee5",     1 }, { "1e5x",      3 }, { "1e5e5", 3 },
        { "1.2.3",     3 }, { "1.5.6",    3 }, { "0..5",      2 }, { "..5",   0 },
        { ".-5",       0 }, { ".e5",      0 }, { "5.-",       2 }, { "5.",    2 },
        { ".5",        2 }, { ".25",      3 }, { "-.5",       3 }, { "-.5x",  3 },
        { "5.e3",      4 }, { ".5e3",     4 }, { "007",       3 }, { "0x10",  1 },
        { "--1",       0 }, { "+-5",      0 }, { "1-",        1 }, { "1,5",   1 },
        { "+5",        2 }, { "123x5678", 3 }, { "inf",       3 }, { "infx",  3 },
        { "Infx",      3 }, { "infinity", 8 }, { "INFINITY",  8 },
        { "infinityx", 8 }, { "infinit",  3 }, { "infin",     3 }, { "infi",  3 },
        { "in",        0 }, { "na",       0 }, { "nan",       3 }, { "nanx",  3 },
        { "nannan",    3 }, { "-inf",     4 }, { "-infinity", 9 }, { "+nan",  4 },
        { "-nanx",     4 }, { "e5",       0 }, { "x",         0 }, { "",      0 },
        { "+",         0 }, { "-",        0 }, { ".",         0 }, { "42 ",   2 },
        { "1 2",       1 }, { "1.5 ",     3 },
        { "1e99999999999999999999x", 22 },
    };
    for (u32 i = 0; i < COUNT(fl); i++) {
        s_f64(fl[i].s, fl[i].len);
    }
    static const lrow ul[] = {
        { "123x", 3 }, { "1.5", 1 }, { "1e5", 1 }, { "0x10", 1 },
        { "12 3", 2 }, { "1-2", 1 }, { "--1", 0 }, { "+-5",  0 },
        { " 1",   0 }, { "1 ",  1 }, { "",    0 }, { "+",    0 },
        { "-",    0 }, { "5.",  1 }, { "007", 3 }, { "007x", 3 },
        { "18446744073709551615", 20 }, { "18446744073709551616", 20 },
        { "99999999999999999999", 20 }, { "-1x", 2 }, { "15,", 2 },
    };
    for (u32 i = 0; i < COUNT(ul); i++) {
        s_u64(ul[i].s, ul[i].len);
    }
    printf("scan grammar %llu\n", (unsigned long long)(nchk - start));
}

static void
p_scan_prop(void) {
    u64 start = nchk;
    static const char *fc[] = {
        "", "-", "+", ".", ".e5", "e5", "0", "-0", "1", "-1", "+1", "007",
        "5.", ".5", ".25", "1.5", "-.5", "1e5", "1E5", "1e+5", "1e-5",
        "5.e3", ".5e3", "1.5e+5", "1e", "1e+", "1e-", "1ee5", "1.2.3",
        "inf", "-inf", "infinity", "-infinity", "nan", "-nan", "nannan",
        "123456789012345678901234567890", "1e99999999999999999999",
        "4.9e-324", "1e309", "0x10", "x", "123x5678",
    };
    static const char *uc[] = {
        "", "+", "-", "0", "-0", "007", "1", "-1", "123", "123x", "1.5",
        "1e5", "0x10", "12 3", "18446744073709551615",
        "18446744073709551616", "99999999999999999999", " 1", "x", "5.",
    };
    static const char *sx[] = {
        "", "x", " ", ".", "e", "e+", "e5", ",", "0", "inf", "nan", "5", "x5",
    };
    char buf[128], zbuf[128];
    for (int f = 0; f < 2; f++) {
        const char **corpus = f ? uc : fc;
        u32 ncorpus = f ? COUNT(uc) : COUNT(fc);
        for (u32 a = 0; a < ncorpus; a++) {
            for (u32 b = 0; b < COUNT(sx); b++) {
                size_t cl = strlen(corpus[a]);
                size_t sl = strlen(sx[b]);
                memcpy(buf, corpus[a], cl);
                memcpy(buf + cl, sx[b], sl);
                size_t n = cl + sl;
                buf[n] = '\0';
                size_t cf, cr, c, c2, c3, c4;
                u64 vf, vr, v, v2, v3, v4;
                if (f) {
                    vf = scan_u64(buf, n, &cf);
                    vr = scan_u64(buf, cf, &cr);
                } else {
                    vf = f64_bits(scan_f64(buf, n, &cf));
                    vr = f64_bits(scan_f64(buf, cf, &cr));
                }
                pchk(cf <= n, "prop: c <= n", buf);
                if (!cf) {
                    pchk(vf == 0, "prop: no numeral reads 0", buf);
                }
                pchk(cr == cf && vr == vf, "prop: prefix rescan", buf);
                if (f) {
                    pchk(to_u64(buf) == (cf == n ? vf : 0), "prop: to_u64 agrees", buf);
                } else {
                    pchk(f64_bits(to_f64(buf)) == (cf == n ? vf : 0), "prop: to_f64 agrees", buf);
                }
                for (size_t l = 0; l <= n; l++) {
                    if (f) {
                        v = scan_u64(buf, l, &c);
                        v2 = scan_u64(buf, c, &c2);
                    } else {
                        v = f64_bits(scan_f64(buf, l, &c));
                        v2 = f64_bits(scan_f64(buf, c, &c2));
                    }
                    pchk(c <= l, "prop: c <= l", buf);
                    if (l >= cf) {
                        pchk(c == cf && v == vf, "prop: stable", buf);
                    }
                    pchk(c2 == c && v2 == v, "prop: rescan at l", buf);
                }
                for (size_t k = 0; k <= n; k++) {
                    memcpy(zbuf, buf, n);
                    zbuf[k] = '\0';
                    if (f) {
                        v3 = scan_u64(zbuf, n, &c3);
                        v4 = scan_u64(buf, k, &c4);
                    } else {
                        v3 = f64_bits(scan_f64(zbuf, n, &c3));
                        v4 = f64_bits(scan_f64(buf, k, &c4));
                    }
                    pchk(c3 == c4 && v3 == v4, "prop: \\0 equals truncation", buf);
                }
            }
        }
    }
    printf("scan properties %llu\n", (unsigned long long)(nchk - start));
}

static void
p_scan_buf(void) {
    u64 start = nchk;
    static const char *ts[] = {
        "1.5", "1.5xy", "inf", "nan", "", "x", "1e+", "123", "1.5.5", "-1",
    };
    for (u32 i = 0; i < COUNT(ts); i++) {
        size_t n = strlen(ts[i]);
        char *m = (char *)malloc(n ? n : 1);
        size_t c, c2;
        u64 v, v2;
        memcpy(m, ts[i], n);
        v = f64_bits(scan_f64(m, n, &c));
        v2 = f64_bits(scan_f64(ts[i], n, &c2));
        pchk(c == c2 && v == v2, "buf: sized equals cstr", ts[i]);
        pchk(f64_bits(scan_f64(m, n, NULL)) == v, "buf: null consumed", ts[i]);
        v = scan_u64(m, n, &c);
        v2 = scan_u64(ts[i], n, &c2);
        pchk(c == c2 && v == v2, "buf: u64 sized equals cstr", ts[i]);
        pchk(scan_u64(m, n, NULL) == v, "buf: u64 null consumed", ts[i]);
        free(m);
    }
    printf("scan buffers %llu\n", (unsigned long long)(nchk - start));
}

static void
p_scan_rt(void) {
    u64 start = nchk;
    for (u32 x = 0; x < 16; x++) {
        b32 t = from_e2m1((e2m1)x);
        size_t n = strlen(t.b), c;
        pchk(scan_e2m1(t.b, n, &c) == (e2m1)x && c == n, "rt e2m1", t.b);
    }
    for (u32 x = 0; x < 256; x++) {
        b32 t = from_e4m3((e4m3)x);
        size_t n = strlen(t.b), c;
        pchk(scan_e4m3(t.b, n, &c) == (e4m3)x && c == n, "rt e4m3", t.b);
    }
    for (u32 x = 0; x < 256; x++) {
        if ((x & 0x7C) == 0x7C && (x & 0x3) && (x & 0x3) != 2) {
            continue;
        }
        b32 t = from_e5m2((e5m2)x);
        size_t n = strlen(t.b), c;
        pchk(scan_e5m2(t.b, n, &c) == (e5m2)x && c == n, "rt e5m2", t.b);
    }
    for (u32 x = 0; x <= 0xFFFF; x++) {
        if ((x & 0x7C00) == 0x7C00 && (x & 0x3FF) && (x & 0x3FF) != 0x200) {
            continue;
        }
        b32 t = from_f16((f16)x);
        size_t n = strlen(t.b), c;
        pchk(scan_f16(t.b, n, &c) == (f16)x && c == n, "rt f16", t.b);
    }
    for (u32 x = 0; x <= 0xFFFF; x++) {
        if ((x & 0x7F80) == 0x7F80 && (x & 0x7F) && (x & 0x7F) != 0x40) {
            continue;
        }
        b32 t = from_bf16((bf16)x);
        size_t n = strlen(t.b), c;
        pchk(scan_bf16(t.b, n, &c) == (bf16)x && c == n, "rt bf16", t.b);
    }
    for (u32 x = 0; x <= UINT8_MAX; x++) {
        b32 t = from_u8((u8)x);
        size_t n = strlen(t.b), c;
        pchk(scan_u8(t.b, n, &c) == (u8)x && c == n, "rt u8", t.b);
        b32 q = from_i8((i8)x);
        size_t m = strlen(q.b), d;
        pchk(scan_i8(q.b, m, &d) == (i8)x && d == m, "rt i8", q.b);
    }
    for (u32 x = 0; x <= 0xFFFF; x++) {
        b32 t = from_u16((u16)x);
        size_t n = strlen(t.b), c;
        pchk(scan_u16(t.b, n, &c) == (u16)x && c == n, "rt u16", t.b);
        b32 q = from_i16((i16)x);
        size_t m = strlen(q.b), d;
        pchk(scan_i16(q.b, m, &d) == (i16)x && d == m, "rt i16", q.b);
    }
    printf("scan roundtrip small %llu\n", (unsigned long long)(nchk - start));
}

static void
p_scan_wide(void) {
    u64 start = nchk;
    static const u32 e32[] = {
        0u, 0x80000000u, 1u, 0x80000001u, 0x007FFFFFu, 0x00800000u,
        0x7F7FFFFFu, 0xFF7FFFFFu, 0x7F800000u, 0xFF800000u,
        0x7FC00000u, 0xFFC00000u,
        0x3F800000u, 0x3F000000u, 0x4B800000u, 0x4B800001u, 0x33800000u,
        0x3FC00000u, 0x00000001u, 0x40490FDBu,
    };
    for (u32 i = 0; i < COUNT(e32); i++) {
        f32 v;
        u32 bits = e32[i];
        memcpy(&v, &bits, sizeof v);
        b32 t = from_f32(v);
        size_t n = strlen(t.b), c;
        pchk(f32_bits(scan_f32(t.b, n, &c)) == bits && c == n, "rt f32 edge", t.b);
    }
    for (u64 k = 0; k < 100000; k++) {
        u32 bits = (u32)rnd();
        if ((bits & 0x7F800000u) == 0x7F800000u && (bits & 0x7FFFFFu)) {
            bits = 0x7FC00000u | (bits & 0x80000000u);
        }
        f32 v;
        memcpy(&v, &bits, sizeof v);
        b32 t = from_f32(v);
        size_t n = strlen(t.b), c;
        pchk(f32_bits(scan_f32(t.b, n, &c)) == bits && c == n, "rt f32 rnd", t.b);
    }
    static const u64 e64[] = {
        0u, 0x8000000000000000ull, 1u, 0x8000000000000001ull,
        0x000FFFFFFFFFFFFFull, 0x0010000000000000ull,
        0x7FEFFFFFFFFFFFFFull, 0xFFEFFFFFFFFFFFFFull,
        0x7FF0000000000000ull, 0xFFF0000000000000ull,
        0x7FF8000000000000ull, 0xFFF8000000000000ull,
        0x3FF0000000000000ull,
        0x4000000000000000ull, 0x4330000000000000ull,
        0x4330000000000001ull, 0x3CA0000000000000ull,
        0x3FF921FB54442D18ull,
    };
    for (u32 i = 0; i < COUNT(e64); i++) {
        f64 v;
        u64 bits = e64[i];
        memcpy(&v, &bits, sizeof v);
        b32 t = from_f64(v);
        size_t n = strlen(t.b), c;
        pchk(f64_bits(scan_f64(t.b, n, &c)) == bits && c == n, "rt f64 edge", t.b);
    }
    for (u64 k = 0; k < 100000; k++) {
        u64 bits = rnd();
        if ((bits & 0x7FF0000000000000ull) == 0x7FF0000000000000ull &&
            (bits & 0xFFFFFFFFFFFFFull)) {
            bits = 0x7FF8000000000000ull | (bits & 0x8000000000000000ull);
        }
        f64 v;
        memcpy(&v, &bits, sizeof v);
        b32 t = from_f64(v);
        size_t n = strlen(t.b), c;
        pchk(f64_bits(scan_f64(t.b, n, &c)) == bits && c == n, "rt f64 rnd", t.b);
    }
    static const u64 edges[] = {
        0u, 1u, 9u, 10u, 99u, 100u, 999u, 1000u, 99999u, 100000u,
        4294967295u, 4294967296u, 9999999999999999999u,
        10000000000000000000u, 18446744073709551614u,
        18446744073709551615u, 0x8000000000000000ull,
    };
    for (u32 i = 0; i < COUNT(edges); i++) {
        u64 x = edges[i];
        b32 t = from_u64(x);
        size_t n = strlen(t.b), c;
        pchk(scan_u64(t.b, n, &c) == x && c == n, "rt u64 edge", t.b);
        i64 y = (i64)x;
        b32 q = from_i64(y);
        size_t m = strlen(q.b), d;
        pchk(scan_i64(q.b, m, &d) == y && d == m, "rt i64 edge", q.b);
    }
    for (u64 k = 0; k < 100000; k++) {
        u64 x = rnd();
        b32 t = from_u64(x);
        size_t n = strlen(t.b), c;
        pchk(scan_u64(t.b, n, &c) == x && c == n, "rt u64 rnd", t.b);
        u32 u = (u32)(x >> (k % 33));
        b32 p = from_u32(u);
        size_t l = strlen(p.b), e;
        pchk(scan_u32(p.b, l, &e) == u && e == l, "rt u32 rnd", p.b);
        i32 w = (i32)(x >> (1 + k % 31));
        b32 q = from_i32(w);
        size_t m = strlen(q.b), d;
        pchk(scan_i32(q.b, m, &d) == w && d == m, "rt i32 rnd", q.b);
    }
    printf("scan roundtrip wide %llu\n", (unsigned long long)(nchk - start));
}

int
main(void) {
    printf("snrt parse tests\n");
    p_e2m1();
    p_e4m3();
    p_e5m2();
    p_f16();
    p_bf16();
    p_u8();
    p_i8();
    p_u16();
    p_i16();
    p_u32();
    p_i32();
    p_f32();
    p_u64();
    p_i64();
    p_f64();
    p_scan_table();
    p_scan_prop();
    p_scan_buf();
    p_scan_rt();
    p_scan_wide();
    printf("%d failures: %s\n", fails, fails ? "FAIL" : "PASS");
    return fails;
}
