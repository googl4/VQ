#ifndef _VQ_H
#define _VQ_H

#include "types.h"

int encodeFrame( u16* vf, u8x4* img, int imgW, int imgH, int tileW, int tileH, u8x4* cb, int cbSize, u32x4 region );
void renderFrame( u8x4* win, u8x4* ref, int tileW, int tileH, u16* vf, int imgW, int imgH, u8x4* cb, s16x2* mv, int blockW, int blockH, int accumulate, u32x4 region );
void refineCodebook( int maxErrCb, u8x4* cb, int cbSize, u16* vf, int tileW, int tileH, u8x4* img, int imgW, int imgH, int cbStart, int cbNum );
void frameDelta( u8x4* dst, u8x4* a, u8x4* b, int imgW, int imgH, s16x2* mv, int blockW, int blockH, u32x4 region );
void motionVectors( s16x2* mv, u8x4* a, u8x4* b, int blockW, int blockH, int imgW, int imgH, u32x4 region );
void resetCodebook( u8* cb, int cbSize, int tileW, int tileH, int cbStart, int cbNum );

#endif
