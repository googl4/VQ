#ifndef _VQ_AVX_H
#define _VQ_AVX_H

#include "types.h"

int encodeFrameAVX( u16* vf, u8x4* img, int imgW, int imgH, u8x4* cb, int cbSize, u32x4 region );
void refineCodebookAVX2( int maxErrCb, u8x4* cb, int cbSize, u16* vf, u8x4* img, int imgW, int imgH, int cbStart, int cbNum );
void motionVectorsAVX2( s16x2* mv, u8x4* a, u8x4* b, int imgW, int imgH, u32x4 region );

#endif
