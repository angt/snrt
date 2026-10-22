#ifndef SNRT_H
#define SNRT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double   snrt_f64;
typedef float    snrt_f32;
typedef uint16_t snrt_f16;
typedef uint16_t snrt_bf16;
typedef uint8_t  snrt_e5m2;
typedef uint8_t  snrt_e4m3;
typedef uint8_t  snrt_e2m1;

typedef uint8_t  snrt_u8;
typedef int8_t   snrt_i8;
typedef uint16_t snrt_u16;
typedef int16_t  snrt_i16;
typedef uint32_t snrt_u32;
typedef int32_t  snrt_i32;
typedef uint64_t snrt_u64;
typedef int64_t  snrt_i64;

typedef struct snrt_f64b  { char b[32]; } snrt_f64b;
typedef struct snrt_f32b  { char b[24]; } snrt_f32b;
typedef struct snrt_f16b  { char b[16]; } snrt_f16b;
typedef struct snrt_bf16b { char b[16]; } snrt_bf16b;
typedef struct snrt_e5m2b { char b[16]; } snrt_e5m2b;
typedef struct snrt_e4m3b { char b[16]; } snrt_e4m3b;
typedef struct snrt_e2m1b { char b[ 8]; } snrt_e2m1b;

typedef struct snrt_u8b   { char b[ 8]; } snrt_u8b;
typedef struct snrt_i8b   { char b[ 8]; } snrt_i8b;
typedef struct snrt_u16b  { char b[ 8]; } snrt_u16b;
typedef struct snrt_i16b  { char b[ 8]; } snrt_i16b;
typedef struct snrt_u32b  { char b[16]; } snrt_u32b;
typedef struct snrt_i32b  { char b[16]; } snrt_i32b;
typedef struct snrt_u64b  { char b[24]; } snrt_u64b;
typedef struct snrt_i64b  { char b[24]; } snrt_i64b;

snrt_f64b  snrt_from_f64(snrt_f64 v);
snrt_f32b  snrt_from_f32(snrt_f32 v);
snrt_f16b  snrt_from_f16(snrt_f16 v);
snrt_bf16b snrt_from_bf16(snrt_bf16 v);
snrt_e5m2b snrt_from_e5m2(snrt_e5m2 v);
snrt_e4m3b snrt_from_e4m3(snrt_e4m3 v);
snrt_e2m1b snrt_from_e2m1(snrt_e2m1 v);

snrt_f64   snrt_to_f64(snrt_f64b s);
snrt_f32   snrt_to_f32(snrt_f32b s);
snrt_f16   snrt_to_f16(snrt_f16b s);
snrt_bf16  snrt_to_bf16(snrt_bf16b s);
snrt_e5m2  snrt_to_e5m2(snrt_e5m2b s);
snrt_e4m3  snrt_to_e4m3(snrt_e4m3b s);
snrt_e2m1  snrt_to_e2m1(snrt_e2m1b s);

snrt_u8b   snrt_from_u8(snrt_u8 v);
snrt_i8b   snrt_from_i8(snrt_i8 v);
snrt_u16b  snrt_from_u16(snrt_u16 v);
snrt_i16b  snrt_from_i16(snrt_i16 v);
snrt_u32b  snrt_from_u32(snrt_u32 v);
snrt_i32b  snrt_from_i32(snrt_i32 v);
snrt_u64b  snrt_from_u64(snrt_u64 v);
snrt_i64b  snrt_from_i64(snrt_i64 v);

snrt_u8    snrt_to_u8(snrt_u8b s);
snrt_i8    snrt_to_i8(snrt_i8b s);
snrt_u16   snrt_to_u16(snrt_u16b s);
snrt_i16   snrt_to_i16(snrt_i16b s);
snrt_u32   snrt_to_u32(snrt_u32b s);
snrt_i32   snrt_to_i32(snrt_i32b s);
snrt_u64   snrt_to_u64(snrt_u64b s);
snrt_i64   snrt_to_i64(snrt_i64b s);

#ifndef SNRT_NO_SHORT_NAMES

typedef snrt_f64  f64;
typedef snrt_f32  f32;
typedef snrt_f16  f16;
typedef snrt_bf16 bf16;
typedef snrt_e5m2 e5m2;
typedef snrt_e4m3 e4m3;
typedef snrt_e2m1 e2m1;

typedef snrt_u8   u8;
typedef snrt_i8   i8;
typedef snrt_u16  u16;
typedef snrt_i16  i16;
typedef snrt_u32  u32;
typedef snrt_i32  i32;
typedef snrt_u64  u64;
typedef snrt_i64  i64;

typedef snrt_f64b  f64b;
typedef snrt_f32b  f32b;
typedef snrt_f16b  f16b;
typedef snrt_bf16b bf16b;
typedef snrt_e5m2b e5m2b;
typedef snrt_e4m3b e4m3b;
typedef snrt_e2m1b e2m1b;

typedef snrt_u8b   u8b;
typedef snrt_i8b   i8b;
typedef snrt_u16b  u16b;
typedef snrt_i16b  i16b;
typedef snrt_u32b  u32b;
typedef snrt_i32b  i32b;
typedef snrt_u64b  u64b;
typedef snrt_i64b  i64b;

#define to_f64   snrt_to_f64
#define to_f32   snrt_to_f32
#define to_f16   snrt_to_f16
#define to_bf16  snrt_to_bf16
#define to_e5m2  snrt_to_e5m2
#define to_e4m3  snrt_to_e4m3
#define to_e2m1  snrt_to_e2m1

#define to_u8    snrt_to_u8
#define to_i8    snrt_to_i8
#define to_u16   snrt_to_u16
#define to_i16   snrt_to_i16
#define to_u32   snrt_to_u32
#define to_i32   snrt_to_i32
#define to_u64   snrt_to_u64
#define to_i64   snrt_to_i64

#define from_f64   snrt_from_f64
#define from_f32   snrt_from_f32
#define from_f16   snrt_from_f16
#define from_bf16  snrt_from_bf16
#define from_e5m2  snrt_from_e5m2
#define from_e4m3  snrt_from_e4m3
#define from_e2m1  snrt_from_e2m1

#define from_u8    snrt_from_u8
#define from_i8    snrt_from_i8
#define from_u16   snrt_from_u16
#define from_i16   snrt_from_i16
#define from_u32   snrt_from_u32
#define from_i32   snrt_from_i32
#define from_u64   snrt_from_u64
#define from_i64   snrt_from_i64

#endif

#ifdef __cplusplus
}
#endif

#endif
