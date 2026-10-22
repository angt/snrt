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
    f32b s = from_f32(x);
    printf("%s\n", s.b);
    float y = to_f32(s);
    return y != x;
}
```

The exit status is 0 only when the round trip held:

    gcc -O2 -o demo demo.c && ./demo

Every type T gets one text buffer type `Tb` and two functions that
are exact inverses of each other:

```c
typedef float snrt_f32;                              // the value
typedef struct snrt_f32b { char b[24]; } snrt_f32b;  // the text

snrt_f32b snrt_from_f32(snrt_f32 v);   // value -> text
snrt_f32 snrt_to_f32(snrt_f32b s);     // text  -> value
```

The buffer is a plain char array with room for the worst case plus a
`\0`, so `.b` is a C string and a function can take the struct and
return it.

By default, short names are used (`f32`, `f32b`, `to_f32`, `from_f32`)
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

Every row has a text buffer type `Tb` (`f32b`, `u8b`, ...) and the
pair `from_T` / `to_T`. The 16-bit, 8-bit and 4-bit floats take and
return raw bits: that is the form memory stores them in.

## Guarantees

- **Print is total and lossless.** Every bit pattern has a text form
  (`0`, `-0`, `inf`, `-inf`, `nan`, or a decimal), the sign of zero
  survives, and the text is the shortest decimal that reads back as
  the same value at that type's own precision. A NaN payload does not
  survive: `nan` is one value in text.

- **Parse is the exact inverse.** `to_T(from_T(v)) == v` for every
  value, and `from_T(to_T(s)) == s` for every text snrt emits. Parse
  accepts the printed forms plus ordinary decimal spelling, rounds once
  at the target type (ties to even), and never detours through double:
  widening to double and rounding that can be a different value (double
  rounding). Text that is not a numeral reads as zero; overflow
  saturates exactly at the type bounds (or becomes infinity where the
  format has one).

- **The sizes are exact.** Worst case text plus `\0`:

  | type             | max text   | buffer       |
  |------------------|------------|--------------|
  | f64              | 24         | `char b[32]` |
  | f32              | 15         | `char b[24]` |
  | f16, bf16        | 10         | `char b[16]` |
  | e5m2             | 7          | `char b[16]` |
  | e4m3             | 6          | `char b[16]` |
  | e2m1             | 4          | `char b[8]`  |
  | u64, i64         | 20         | `char b[24]` |
  | u32, i32         | 10, 11     | `char b[16]` |
  | u8, i8, u16, i16 | 3, 4, 5, 6 | `char b[8]`  |

  The buffer is sized in whole 8-byte registers.

- **Spelling.** Shortest digits, then the shorter of fixed and
  scientific notation, ties to fixed. The scientific form keeps one
  digit before the point and the exponent sign only when negative;
  shorter unnormalized spellings like `1234e7` are refused: `0.1`,
  `100`, `-0`, `65504`, `1e21`, `5.96046e-8`.
