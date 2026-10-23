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

typedef struct snrt_b32  { char b[32]; } snrt_b32;

snrt_b32   snrt_from_f64(snrt_f64 v);
snrt_b32   snrt_from_f32(snrt_f32 v);
snrt_b32   snrt_from_f16(snrt_f16 v);
snrt_b32   snrt_from_bf16(snrt_bf16 v);
snrt_b32   snrt_from_e5m2(snrt_e5m2 v);
snrt_b32   snrt_from_e4m3(snrt_e4m3 v);
snrt_b32   snrt_from_e2m1(snrt_e2m1 v);

snrt_f64   snrt_to_f64(const char *s);
snrt_f32   snrt_to_f32(const char *s);
snrt_f16   snrt_to_f16(const char *s);
snrt_bf16  snrt_to_bf16(const char *s);
snrt_e5m2  snrt_to_e5m2(const char *s);
snrt_e4m3  snrt_to_e4m3(const char *s);
snrt_e2m1  snrt_to_e2m1(const char *s);

snrt_b32   snrt_from_u8(snrt_u8 v);
snrt_b32   snrt_from_i8(snrt_i8 v);
snrt_b32   snrt_from_u16(snrt_u16 v);
snrt_b32   snrt_from_i16(snrt_i16 v);
snrt_b32   snrt_from_u32(snrt_u32 v);
snrt_b32   snrt_from_i32(snrt_i32 v);
snrt_b32   snrt_from_u64(snrt_u64 v);
snrt_b32   snrt_from_i64(snrt_i64 v);

snrt_u8    snrt_to_u8(const char *s);
snrt_i8    snrt_to_i8(const char *s);
snrt_u16   snrt_to_u16(const char *s);
snrt_i16   snrt_to_i16(const char *s);
snrt_u32   snrt_to_u32(const char *s);
snrt_i32   snrt_to_i32(const char *s);
snrt_u64   snrt_to_u64(const char *s);
snrt_i64   snrt_to_i64(const char *s);

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

typedef snrt_b32  b32;

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
