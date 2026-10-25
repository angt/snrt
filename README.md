# snrt

snrt is **S**hortest **N**umber **R**ound **T**rip: the shortest text
that comes back as the same number.

## Why

Printing a number and reading it back should give the same number.
Usually it does not: `%g` prints six digits and loses the value, a
fixed-precision `%.17g` keeps it but is long and noisy. snrt prints the
SHORTEST decimal that reads back as the same value, and parses that text
back to the same bits. Print and parse are exact inverses.

It covers the types an LLM actually carries: f64, f32, f16, bf16, the
fp8 formats e5m2 and e4m3, the fp4 format e2m1, and the plain integers.
libc cannot print f16, bf16 or fp8 at all. The usual workaround (widen
to double and print that) is both slow and wrong: the widened double
rounds twice and prints its own digits (`0.0999755859375` where the
half's own shortest text is `0.1`).

## Use

A simple example, showing how to convert a float to string and back:

```c
#include "snrt.c"
#include <stdio.h>

int main(void) {
    float x = 0.1f;
    b32 s = from_f32(x);
    printf("%s\n", s.b);
    float y = to_f32(s.b);
    return y != x;
}
```

The exit status is 0 only when the round trip held:

    gcc -O2 -o demo demo.c && ./demo

Every type T has two functions that are exact inverses of each other,
sharing one text buffer type `b32`:

```c
typedef float snrt_f32;                            // the value
typedef struct snrt_b32 { char b[32]; } snrt_b32;  // the text

snrt_b32  snrt_from_f32(snrt_f32 v);               // value -> text
snrt_f32  snrt_to_f32(const char *s);              // text  -> value

// generic call for parsers
snrt_f32  snrt_scan_f32(const char *s, size_t n, size_t *consumed);
```

The buffer is a plain char array with room for the worst case of any
type plus a `\0`, so `.b` is a C string and a function can take the
struct and return it. `to_T` takes any `\0`-terminated string.

`scan_T` is the generic form, for parsers and streams. It takes the
buffer and its length, reads no byte past `n`, and reports how many
bytes the numeral occupied. It needs no `\0`: a `\0` inside the buffer
simply ends the numeral. `consumed == 0` says there was no numeral at
all, so a real `0` is never confused with garbage. `to_T` is `scan_T`
over a whole string: it reads zero unless the string is exactly one
numeral.

By default, short names are used (`f32`, `b32`, `to_f32`, `from_f32`)
with a longer version prefixed by `snrt_`. Define `SNRT_NO_SHORT_NAMES`
before the include to keep only the `snrt_` names.

## Types

| T      | value      | format            |
|--------|------------|-------------------|
| `f64`  | `double`   | IEEE binary64     |
| `f32`  | `float`    | IEEE binary32     |
| `f16`  | `uint16_t` | IEEE binary16     |
| `bf16` | `uint16_t` | bfloat16          |
| `e5m2` | `uint8_t`  | OCP float8 e5m2   |
| `e4m3` | `uint8_t`  | OCP float8 e4m3fn |
| `e2m1` | `uint8_t`  | OCP float4 e2m1   |
| `u8`…  | `uintT_t`  | decimal           |
| `i8`…  | `intT_t`   | decimal           |

Every row has `from_T`, `to_T` and `scan_T`. The 16-bit, 8-bit and 4-bit
floats take and return raw bits.

## Guarantees

- **Print is total.** Every bit pattern has a text form (`0`, `-0`,
  `inf`, `-inf`, `nan`, `-nan`, or a decimal), and the text is the
  shortest that reads back as the same value at that type's own
  precision. The sign of zero and the sign of a NaN survive; only a
  NaN's payload does not: `nan` and `-nan` are the two NaN texts, and
  each reads back as the canonical quiet NaN of that sign.

- **Parse is the exact inverse.** `to_T(from_T(v).b) == v` for every
  value except a NaN payload, which folds as above, and
  `from_T(to_T(s)).b == s` for every text snrt emits. `scan_T` reports
  the exact extent of that text in a larger buffer. Parse accepts the
  printed forms plus ordinary decimal spelling, rounds once at the
  target type (ties to even), and never detours through double: widening
  to double and rounding that can be a different value (double
  rounding). Text that is not a numeral reads as zero; overflow
  saturates exactly at the type bounds (or becomes infinity where the
  format has one).

- **The size is exact.** One buffer, `char b[32]`, holds the worst
  case text of every type plus a `\0`.

- **Spelling.** Shortest digits, then the shorter of fixed and
  scientific notation, ties to fixed. The scientific form keeps one
  digit before the point and the exponent sign only when negative;
  shorter unnormalized spellings like `1234e7` are refused: `0.1`,
  `100`, `-0`, `65504`, `1e21`, `5.96046e-8`.
