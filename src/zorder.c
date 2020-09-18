#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "types.h"

u32 interleavebits( u32 a, u32 b ) {
	u32 x = ( a | ( a << 8 ) ) & 0x00FF00FF;
	x = ( x | ( x << 4 ) ) & 0x0F0F0F0F;
}
