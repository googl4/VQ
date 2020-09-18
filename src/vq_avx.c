#include <string.h>

#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

#include "types.h"

int encodeFrameAVX( u16* vf, u8x4* img, int imgW, int imgH, u8x4* cb, int cbSize, u32x4 region ) {
	int maxErr = 0;
	int maxErrCb = -1;
	
	u8x4 (*cb2)[4][4] = (u8x4(*)[4][4])cb;
	
	for( int by = region.y / 4; by < ( region.y + region.h ) / 4; by++ ) {
		for( int bx = region.x / 4; bx < ( region.x + region.w ) / 4; bx++ ) {
			int minErr = INT_MAX;
			int minErrCb = -1;
			
			u32* ptr = (u32*)( img + by * 4 * imgW + bx * 4 );
			
			__m256i shfRG = _mm256_setr_epi8(
				0x00, 0x04, 0x08, 0x0C,
				0x10, 0x14, 0x18, 0x1C,
				0x01, 0x05, 0x09, 0x0D,
				0x11, 0x15, 0x19, 0x1D,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80
			);
			
			__m256i shfBX = _mm256_setr_epi8(
				0x02, 0x06, 0x0A, 0x0E,
				0x12, 0x16, 0x1A, 0x1E,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80
			);
			
			__m128i r[4];
			r[0] = _mm_load_si128( (__m128i*)ptr ); ptr += imgW;
			r[1] = _mm_load_si128( (__m128i*)ptr ); ptr += imgW;
			r[2] = _mm_load_si128( (__m128i*)ptr ); ptr += imgW;
			r[3] = _mm_load_si128( (__m128i*)ptr );
			
			__m256i rx[2];
			//r[0] = _mm256_loadu2_m128i( (__m128i*)ptr, (__m128i*)( ptr + imgW ) ); ptr += imgW * 2;
			//r[1] = _mm256_loadu2_m128i( (__m128i*)ptr, (__m128i*)( ptr + imgW ) );
			rx[0] = _mm256_set_m128i( r[1], r[0] );
			rx[1] = _mm256_set_m128i( r[3], r[2] );
			
			__m128i rs[4];
			rs[0] = _mm256_castsi256_si128( _mm256_shuffle_epi8( rx[0], shfRG ) );
			rs[1] = _mm256_castsi256_si128( _mm256_shuffle_epi8( rx[0], shfBX ) );
			rs[2] = _mm256_castsi256_si128( _mm256_shuffle_epi8( rx[1], shfRG ) );
			rs[3] = _mm256_castsi256_si128( _mm256_shuffle_epi8( rx[1], shfBX ) );
			
			for( int i = 0; i < cbSize; i++ ) {
				int mse = 0;
				
				u32* ptr2 = (u32*)( cb2 + i );
				
				/*
				__m128i c[4];
				c[0] = _mm_load_si128( (__m128i*)ptr2 ); ptr2 += 4;
				c[1] = _mm_load_si128( (__m128i*)ptr2 ); ptr2 += 4;
				c[2] = _mm_load_si128( (__m128i*)ptr2 ); ptr2 += 4;
				c[3] = _mm_load_si128( (__m128i*)ptr2 );
				*/
				__m256i c[2];
				c[0] = _mm256_load_si256( (__m256i*)ptr2 ); ptr2 += 8;
				c[1] = _mm256_load_si256( (__m256i*)ptr2 );
				
				__m128i cs[4];
				cs[0] = _mm256_castsi256_si128( _mm256_shuffle_epi8( c[0], shfRG ) );
				cs[1] = _mm256_castsi256_si128( _mm256_shuffle_epi8( c[0], shfBX ) );
				cs[2] = _mm256_castsi256_si128( _mm256_shuffle_epi8( c[1], shfRG ) );
				cs[3] = _mm256_castsi256_si128( _mm256_shuffle_epi8( c[1], shfBX ) );
				
				/*
				__m128i d[4];
				d[0] = _mm_sad_epu8( c[0], r[0] );
				d[1] = _mm_sad_epu8( c[1], r[1] );
				d[2] = _mm_sad_epu8( c[2], r[2] );
				d[3] = _mm_sad_epu8( c[3], r[3] );
				
				d[0] = _mm_add_epi64( d[0], d[1] );
				d[2] = _mm_add_epi64( d[2], d[3] );
				
				d[0] = _mm_add_epi64( d[0], d[2] );
				
				__int64 r = _mm_cvtsi128_si64( d[0] ) + _mm_extract_epi64( d[0], 1 );
				
				mse = r;
				*/
				__m128i d[4];
				d[0] = _mm_sad_epu8( cs[0], rs[0] );
				d[1] = _mm_sad_epu8( cs[1], rs[1] );
				d[2] = _mm_sad_epu8( cs[2], rs[2] );
				d[3] = _mm_sad_epu8( cs[3], rs[3] );
				
				d[0] = _mm_add_epi64( d[0], d[2] );
				d[1] = _mm_add_epi64( d[1], d[3] );
				
				__int64 r = _mm_cvtsi128_si64( d[0] ) + _mm_extract_epi64( d[0], 1 ) * 2 + _mm_cvtsi128_si64( d[1] );
				
				mse = r;
				
				if( mse < minErr ) {
					minErr = mse;
					minErrCb = i;
				}
			}
			
			vf[by*(imgW/4)+bx] = minErrCb;
			
			if( maxErr < minErr ) {
				maxErr = minErr;
				maxErrCb = minErrCb;
			}
		}
	}
	
	return maxErrCb;
}

void refineCodebookAVX2( int maxErrCb, u8x4* cb, int cbSize, u16* vf, u8x4* img, int imgW, int imgH, int cbStart, int cbNum ) {
	u8x4 (*cb2)[4][4] = (u8x4(*)[4][4])cb;
	
	static u32 n = 0;
	
	int splitMax = FALSE;
	
	int maxTiles = 0;
	int maxTilesCb = maxErrCb;
	
	u32x4* avg = _mm_malloc( cbNum * 4 * 4 * 4 * 4, 32 );
	int* nTiles = malloc( cbNum * 4 );
	
	memset( avg, 0, cbNum * 4 * 4 * 4 * 4 );
	memset( nTiles, 0, cbNum * 4 );
	
	u32x4 (*avg2)[4][4] = (u32x4(*)[4][4])avg;
	
	for( int by = 0; by < imgH / 4; by++ ) {
		for( int bx = 0; bx < imgW / 4; bx++ ) {
			int v = vf[by*(imgW/4)+bx];
			
			if( v < cbStart || v >= cbStart + cbNum ) {
				continue;
			}
			
			for( int y = 0; y < 4; y++ ) {
				for( int x = 0; x < 4; x++ ) {
					avg2[v-cbStart][y][x].r += img[(by*4+y)*imgW+bx*4+x].r;
					avg2[v-cbStart][y][x].g += img[(by*4+y)*imgW+bx*4+x].g;
					avg2[v-cbStart][y][x].b += img[(by*4+y)*imgW+bx*4+x].b;
				}
			}
			
			/*
			u32* ptr = (u32*)( img + ( by * 4 ) * imgW + bx * 4 );
			
			__m128i r[4];
			r[0] = _mm_load_si128( (__m128i*)ptr ); ptr += imgW;
			r[1] = _mm_load_si128( (__m128i*)ptr ); ptr += imgW;
			r[2] = _mm_load_si128( (__m128i*)ptr ); ptr += imgW;
			r[3] = _mm_load_si128( (__m128i*)ptr );
			
			__m256i rx[8];
			rx[0] = _mm256_cvtepu8_epi32( r[0] );
			rx[1] = _mm256_cvtepu8_epi32( _mm_bsrli_si128( r[0], 64 ) );
			rx[2] = _mm256_cvtepu8_epi32( r[1] );
			rx[3] = _mm256_cvtepu8_epi32( _mm_bsrli_si128( r[1], 64 ) );
			rx[4] = _mm256_cvtepu8_epi32( r[2] );
			rx[5] = _mm256_cvtepu8_epi32( _mm_bsrli_si128( r[2], 64 ) );
			rx[6] = _mm256_cvtepu8_epi32( r[3] );
			rx[7] = _mm256_cvtepu8_epi32( _mm_bsrli_si128( r[3], 64 ) );
			
			u32* ptr2 = (u32*)( avg + ( v - cbStart ) * 4 * 4 );
			u32* ptr3 = ptr2;
			
			__m256i c[8];
			c[0] = _mm256_load_si256( (__m256i*)ptr2 ); ptr2 += 8;
			c[1] = _mm256_load_si256( (__m256i*)ptr2 ); ptr2 += 8;
			c[2] = _mm256_load_si256( (__m256i*)ptr2 ); ptr2 += 8;
			c[3] = _mm256_load_si256( (__m256i*)ptr2 ); ptr2 += 8;
			c[4] = _mm256_load_si256( (__m256i*)ptr2 ); ptr2 += 8;
			c[5] = _mm256_load_si256( (__m256i*)ptr2 ); ptr2 += 8;
			c[6] = _mm256_load_si256( (__m256i*)ptr2 ); ptr2 += 8;
			c[7] = _mm256_load_si256( (__m256i*)ptr2 );
			
			__m256i a[8];
			a[0] = _mm256_add_epi32( c[0], rx[0] );
			a[1] = _mm256_add_epi32( c[1], rx[1] );
			a[2] = _mm256_add_epi32( c[2], rx[2] );
			a[3] = _mm256_add_epi32( c[3], rx[3] );
			a[4] = _mm256_add_epi32( c[4], rx[4] );
			a[5] = _mm256_add_epi32( c[5], rx[5] );
			a[6] = _mm256_add_epi32( c[6], rx[6] );
			a[7] = _mm256_add_epi32( c[7], rx[7] );
			
			_mm256_store_si256( (__m256i*)ptr3, a[0] ); ptr3 += 8;
			_mm256_store_si256( (__m256i*)ptr3, a[1] ); ptr3 += 8;
			_mm256_store_si256( (__m256i*)ptr3, a[2] ); ptr3 += 8;
			_mm256_store_si256( (__m256i*)ptr3, a[3] ); ptr3 += 8;
			_mm256_store_si256( (__m256i*)ptr3, a[4] ); ptr3 += 8;
			_mm256_store_si256( (__m256i*)ptr3, a[5] ); ptr3 += 8;
			_mm256_store_si256( (__m256i*)ptr3, a[6] ); ptr3 += 8;
			_mm256_store_si256( (__m256i*)ptr3, a[7] );
			*/
			
			nTiles[v-cbStart] += 1;
			
			if( nTiles[v-cbStart] > maxTiles ) {
				maxTiles = nTiles[v-cbStart];
				maxTilesCb = v - cbStart;
			}
		}
	}
	
	for( int i = 0; i < cbNum; i++ ) {
		if( nTiles[i] == 0 ) {
			if( !splitMax ) {
				for( int y = 0; y < 4; y++ ) {
					for( int x = 0; x < 4; x++ ) {
						cb2[i][y][x].r = cb2[maxErrCb][y][x].r ^ 1;
						cb2[i][y][x].g = cb2[maxErrCb][y][x].g ^ 1;
						cb2[i][y][x].b = cb2[maxErrCb][y][x].b ^ 1;
					}
				}
				splitMax = TRUE;
				
			} else {
				if( ( i + maxTiles ) & 1 ) {
					for( int y = 0; y < 4; y++ ) {
						for( int x = 0; x < 4; x++ ) {
							cb2[i][y][x].r = cb2[maxTilesCb][y][x].r ^ 1;
							cb2[i][y][x].g = cb2[maxTilesCb][y][x].g ^ 1;
							cb2[i][y][x].b = cb2[maxTilesCb][y][x].b ^ 1;
						}
					}
					
				} else {
					for( int j = 0; j < 4; j++ ) {
						for( int k = 0; k < 4; k++ ) {
							cb2[i][j][k].r = n >> 24;
							n = n * 1664525 + 1013904223;
							cb2[i][j][k].g = n >> 24;
							n = n * 1664525 + 1013904223;
							cb2[i][j][k].b = n >> 24;
							n = n * 1664525 + 1013904223;
						}
					}
				}
			}
			
		} else {
			/*
			u32* ptr = (u32*)( avg + i * 4 * 4 );
			
			__m256i a[8];
			a[0] = _mm256_load_si256( (__m256i*)ptr ); ptr += 8;
			a[1] = _mm256_load_si256( (__m256i*)ptr ); ptr += 8;
			a[2] = _mm256_load_si256( (__m256i*)ptr ); ptr += 8;
			a[3] = _mm256_load_si256( (__m256i*)ptr ); ptr += 8;
			a[4] = _mm256_load_si256( (__m256i*)ptr ); ptr += 8;
			a[5] = _mm256_load_si256( (__m256i*)ptr ); ptr += 8;
			a[6] = _mm256_load_si256( (__m256i*)ptr ); ptr += 8;
			a[7] = _mm256_load_si256( (__m256i*)ptr );
			
			// divide through packed single
			__m256 f[8];
			f[0] = _mm256_cvtepi32_ps( a[0] );
			f[1] = _mm256_cvtepi32_ps( a[1] );
			f[2] = _mm256_cvtepi32_ps( a[2] );
			f[3] = _mm256_cvtepi32_ps( a[3] );
			f[4] = _mm256_cvtepi32_ps( a[4] );
			f[5] = _mm256_cvtepi32_ps( a[5] );
			f[6] = _mm256_cvtepi32_ps( a[6] );
			f[7] = _mm256_cvtepi32_ps( a[7] );
			
			__m256 d = _mm256_set1_ps( 1.0 / nTiles[i] );
			
			f[0] = _mm256_mul_ps( f[0], d );
			f[1] = _mm256_mul_ps( f[1], d );
			f[2] = _mm256_mul_ps( f[2], d );
			f[3] = _mm256_mul_ps( f[3], d );
			f[4] = _mm256_mul_ps( f[4], d );
			f[5] = _mm256_mul_ps( f[5], d );
			f[6] = _mm256_mul_ps( f[6], d );
			f[7] = _mm256_mul_ps( f[7], d );
			
			__m256i c[8];
			c[0] = _mm256_cvtps_epi32( f[0] );
			c[1] = _mm256_cvtps_epi32( f[1] );
			c[2] = _mm256_cvtps_epi32( f[2] );
			c[3] = _mm256_cvtps_epi32( f[3] );
			c[4] = _mm256_cvtps_epi32( f[4] );
			c[5] = _mm256_cvtps_epi32( f[5] );
			c[6] = _mm256_cvtps_epi32( f[6] );
			c[7] = _mm256_cvtps_epi32( f[7] );
			
			__m256i shf = _mm256_setr_epi8(
				0x00, 0x04, 0x08, 0x0C,
				0x10, 0x14, 0x18, 0x1C,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80
			);
			
			__m256i s[8];
			s[0] = _mm256_shuffle_epi8( c[0], shf );
			s[1] = _mm256_shuffle_epi8( c[1], shf );
			s[2] = _mm256_shuffle_epi8( c[2], shf );
			s[3] = _mm256_shuffle_epi8( c[3], shf );
			s[4] = _mm256_shuffle_epi8( c[4], shf );
			s[5] = _mm256_shuffle_epi8( c[5], shf );
			s[6] = _mm256_shuffle_epi8( c[6], shf );
			s[7] = _mm256_shuffle_epi8( c[7], shf );
			
			u32* ptr2 = (u32*)( cb + ( cbStart + i ) * 4 * 4 );
			
			_mm_storeu_si64( ptr2, _mm256_castsi256_si128( s[0] ) ); ptr2 += 2;
			_mm_storeu_si64( ptr2, _mm256_castsi256_si128( s[1] ) ); ptr2 += 2;
			_mm_storeu_si64( ptr2, _mm256_castsi256_si128( s[2] ) ); ptr2 += 2;
			_mm_storeu_si64( ptr2, _mm256_castsi256_si128( s[3] ) ); ptr2 += 2;
			_mm_storeu_si64( ptr2, _mm256_castsi256_si128( s[4] ) ); ptr2 += 2;
			_mm_storeu_si64( ptr2, _mm256_castsi256_si128( s[5] ) ); ptr2 += 2;
			_mm_storeu_si64( ptr2, _mm256_castsi256_si128( s[6] ) ); ptr2 += 2;
			_mm_storeu_si64( ptr2, _mm256_castsi256_si128( s[7] ) );
			*/
			
			for( int y = 0; y < 4; y++ ) {
				for( int x = 0; x < 4; x++ ) {
					cb2[i][y][x].r = avg2[i][y][x].r / nTiles[i];
					cb2[i][y][x].g = avg2[i][y][x].g / nTiles[i];
					cb2[i][y][x].b = avg2[i][y][x].b / nTiles[i];
				}
			}
			
		}
	}
	
	_mm_free( avg );
	free( nTiles );
}

void motionVectorsAVX2( s16x2* mv, u8x4* a, u8x4* b, int imgW, int imgH, u32x4 region ) {
	for( int by = region.y / 8; by < ( region.y + region.h ) / 8; by++ ) {
		for( int bx = region.x / 8; bx < ( region.x + region.w ) / 8; bx++ ) {
			int leastErrX = 0;
			int leastErrY = 0;
			int leastErr = INT_MAX;
			
			u32* ptr = (u32*)( b + by * 8 * imgW + bx * 8 );
			
			__m256i rb[8];
			rb[0] = _mm256_load_si256( (__m256i*)ptr ); ptr += imgW;
			rb[1] = _mm256_load_si256( (__m256i*)ptr ); ptr += imgW;
			rb[2] = _mm256_load_si256( (__m256i*)ptr ); ptr += imgW;
			rb[3] = _mm256_load_si256( (__m256i*)ptr ); ptr += imgW;
			rb[4] = _mm256_load_si256( (__m256i*)ptr ); ptr += imgW;
			rb[5] = _mm256_load_si256( (__m256i*)ptr ); ptr += imgW;
			rb[6] = _mm256_load_si256( (__m256i*)ptr ); ptr += imgW;
			rb[7] = _mm256_load_si256( (__m256i*)ptr );
			
			for( int ofy = -16; ofy < 16; ofy++ ) {
				for( int ofx = -16; ofx < 16; ofx++ ) {
					//int x1 = clamp( bx * 8 + ofx, 0, imgW - 1 );
					//int y1 = clamp( by * 8 + ofy, 0, imgH - 1 );
					int x = bx * 8 + ofx;
					int y = by * 8 + ofy;
					
					if( x < 0 || y < 0 || x >= imgW - 8 || y >= imgH - 8 ) {
						continue;
					}
					
					u32* ptr2 = (u32*)( a + y * imgW + x );
					
					__m256i ra[8];
					ra[0] = _mm256_loadu_si256( (__m256i*)ptr2 ); ptr2 += imgW;
					ra[1] = _mm256_loadu_si256( (__m256i*)ptr2 ); ptr2 += imgW;
					ra[2] = _mm256_loadu_si256( (__m256i*)ptr2 ); ptr2 += imgW;
					ra[3] = _mm256_loadu_si256( (__m256i*)ptr2 ); ptr2 += imgW;
					ra[4] = _mm256_loadu_si256( (__m256i*)ptr2 ); ptr2 += imgW;
					ra[5] = _mm256_loadu_si256( (__m256i*)ptr2 ); ptr2 += imgW;
					ra[6] = _mm256_loadu_si256( (__m256i*)ptr2 ); ptr2 += imgW;
					ra[7] = _mm256_loadu_si256( (__m256i*)ptr2 );
					
					__m256i d[8];
					d[0] = _mm256_sad_epu8( ra[0], rb[0] );
					d[1] = _mm256_sad_epu8( ra[1], rb[1] );
					d[2] = _mm256_sad_epu8( ra[2], rb[2] );
					d[3] = _mm256_sad_epu8( ra[3], rb[3] );
					d[4] = _mm256_sad_epu8( ra[4], rb[4] );
					d[5] = _mm256_sad_epu8( ra[5], rb[5] );
					d[6] = _mm256_sad_epu8( ra[6], rb[6] );
					d[7] = _mm256_sad_epu8( ra[7], rb[7] );
					
					d[0] = _mm256_add_epi64( d[0], d[1] );
					d[2] = _mm256_add_epi64( d[2], d[3] );
					d[4] = _mm256_add_epi64( d[4], d[5] );
					d[6] = _mm256_add_epi64( d[6], d[7] );
					
					d[0] = _mm256_add_epi64( d[0], d[2] );
					d[4] = _mm256_add_epi64( d[4], d[6] );
					
					d[0] = _mm256_add_epi64( d[0], d[4] );
					
					__m128i s1 = _mm256_castsi256_si128( d[0] );
					__m128i s2 = _mm256_extracti128_si256( d[0], 1 );
					__m128i s = _mm_add_epi64( s1, s2 );
					
					__int64 r = _mm_cvtsi128_si64( s ) + _mm_extract_epi64( s, 1 );
					
					if( r < leastErr ) {
						leastErr = r;
						leastErrX = ofx;
						leastErrY = ofy;
					}
				}
			}
			
			mv[by*(imgW/8)+bx] = (s16x2){ leastErrX, leastErrY };
		}
	}
}
