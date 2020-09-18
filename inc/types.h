#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

#define TRUE 1
#define FALSE 0

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef struct {
	u8 r, g, b, a;
} u8x4;

typedef union {
	struct { u32 r, g, b, a; };
	struct { u32 x, y, w, h; };
} u32x4;

typedef struct {
	s16 x, y;
} s16x2;

#endif
