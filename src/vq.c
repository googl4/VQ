#include <limits.h>
#include <string.h>
#include <math.h>

#include "vq.h"
#include "types.h"

int clamp( int n, int l, int u ) {
	if( n < l ) {
		return l;
	}
	
	if( n > u ) {
		return u;
	}
	
	return n;
}

int saturate( int n ) {
	return clamp( n, 0, 255 );
}

int encodeFrame( u16* vf, u8x4* img, int imgW, int imgH, int tileW, int tileH, u8x4* cb, int cbSize, u32x4 region ) {
	int maxErr = 0;
	int maxErrCb = -1;
	
	u8x4 (*cb2)[tileH][tileW] = (u8x4(*)[tileH][tileW])cb;
	
	for( int by = region.y / tileH; by < ( region.y + region.h ) / tileH; by++ ) {
		for( int bx = region.x / tileW; bx < ( region.x + region.w ) / tileW; bx++ ) {
			int minErr = INT_MAX;
			int minErrCb = -1;
			
			for( int i = 0; i < cbSize; i++ ) {
				int mse = 0;
				
				for( int y = 0; y < tileH; y++ ) {
					for( int x = 0; x < tileW; x++ ) {
						int r = img[(by*tileH+y)*imgW+bx*tileW+x].r;
						int g = img[(by*tileH+y)*imgW+bx*tileW+x].g;
						int b = img[(by*tileH+y)*imgW+bx*tileW+x].b;
						int e = abs( r - cb2[i][y][x].r ) + abs( g - cb2[i][y][x].g ) * 2 + abs( b - cb2[i][y][x].b );
						//mse += ( e * e ) >> 8;
						mse += e * e;
					}
				}
				
				if( mse < minErr ) {
					minErr = mse;
					minErrCb = i;
				}
			}
			
			vf[by*(imgW/tileW)+bx] = minErrCb;
			
			if( maxErr < minErr ) {
				maxErr = minErr;
				maxErrCb = minErrCb;
			}
		}
	}
	
	return maxErrCb;
}

void renderFrame( u8x4* win, u8x4* ref, int tileW, int tileH, u16* vf, int imgW, int imgH, u8x4* cb, s16x2* mv, int blockW, int blockH, int accumulate, u32x4 region ) {
	u8x4 (*cb2)[tileH][tileW] = (u8x4(*)[tileH][tileW])cb;
	
	for( int by = region.y / tileH; by < ( region.y + region.h ) / tileH; by++ ) {
		for( int bx = region.x / tileW; bx < ( region.x + region.w ) / tileW; bx++ ) {
			u16 v = vf[by*(imgW/tileW)+bx];
			
			for( int y = 0; y < tileH; y++ ) {
				for( int x = 0; x < tileW; x++ ) {
					int x1 = bx * tileW + x;
					int y1 = by * tileH + y;
					
					u8x4* p = &win[y1*imgW+x1];
					//int d = 128 + img->pix[(by*tileH+y)*img->w+bx*tileW+x].g - cb[v][y][x];
					//int d = abs( (int)img->pix[(by*tileH+y)*img->w+bx*tileW+x].g - cb[v][y][x] );
					
					int bx = x1 / blockW;
					int by = y1 / blockH;
					
					s16x2 m = mv[by*(imgW/blockW)+bx];
					
					int x2 = clamp( x1 + m.x, 0, imgW - 1 );
					int y2 = clamp( y1 + m.y, 0, imgH - 1 );
					
					u8x4 p2 = ref[y2*imgW+x2];
					
					if( accumulate ) {
						//p->r = saturate( p2.r + ( (int)cb2[v][y][x].r - 128 ) * 2 );
						//p->g = saturate( p2.g + ( (int)cb2[v][y][x].g - 128 ) * 2 );
						//p->b = saturate( p2.b + ( (int)cb2[v][y][x].b - 128 ) * 2 );
						p->r = saturate( p2.r + ( (int)cb2[v][y][x].r - 128 ) );
						p->g = saturate( p2.g + ( (int)cb2[v][y][x].g - 128 ) );
						p->b = saturate( p2.b + ( (int)cb2[v][y][x].b - 128 ) );
						
					} else {
						p->r = cb2[v][y][x].r;
						p->g = cb2[v][y][x].g;
						p->b = cb2[v][y][x].b;
					}
				}
			}
		}
	}
}

void refineCodebook( int maxErrCb, u8x4* cb, int cbSize, u16* vf, int tileW, int tileH, u8x4* img, int imgW, int imgH, int cbStart, int cbNum ) {
	u8x4 (*cb2)[tileH][tileW] = (u8x4(*)[tileH][tileW])cb;
	
	static u32 n = 0;
	
	int splitMax = FALSE;
	
	int maxTiles = 0;
	int maxTilesCb = maxErrCb;
	
	for( int i = cbStart; i < cbStart + cbNum; i++ ) {
		u32x4 avg[tileH][tileW];
		memset( avg, 0, tileW * tileH * 4 * 4 );
		
		int numTiles = 0;
		for( int j = 0; j < ( imgW / tileW ) * ( imgH / tileH ); j++ ) {
			if( vf[j] == i ) {
				int bx = j % ( imgW / tileW );
				int by = j / ( imgW / tileW );
				
				for( int y = 0; y < tileH; y++ ) {
					for( int x = 0; x < tileW; x++ ) {
						avg[y][x].r += img[(by*tileH+y)*imgW+bx*tileW+x].r;
						avg[y][x].g += img[(by*tileH+y)*imgW+bx*tileW+x].g;
						avg[y][x].b += img[(by*tileH+y)*imgW+bx*tileW+x].b;
					}
				}
				
				numTiles++;
			}
		}
		
		if( numTiles == 0 ) {
			if( !splitMax ) {
				for( int y = 0; y < tileH; y++ ) {
					for( int x = 0; x < tileW; x++ ) {
						cb2[i][y][x].r = cb2[maxErrCb][y][x].r ^ 1;
						cb2[i][y][x].g = cb2[maxErrCb][y][x].g ^ 1;
						cb2[i][y][x].b = cb2[maxErrCb][y][x].b ^ 1;
					}
				}
				splitMax = TRUE;
				
			} else {
				if( ( i + maxTiles ) & 1 ) {
					for( int y = 0; y < tileH; y++ ) {
						for( int x = 0; x < tileW; x++ ) {
							cb2[i][y][x].r = cb2[maxTilesCb][y][x].r ^ 1;
							cb2[i][y][x].g = cb2[maxTilesCb][y][x].g ^ 1;
							cb2[i][y][x].b = cb2[maxTilesCb][y][x].b ^ 1;
						}
					}
					
				} else {
					for( int j = 0; j < tileH; j++ ) {
						for( int k = 0; k < tileW; k++ ) {
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
			for( int y = 0; y < tileH; y++ ) {
				for( int x = 0; x < tileW; x++ ) {
					cb2[i][y][x].r = avg[y][x].r / numTiles;
					cb2[i][y][x].g = avg[y][x].g / numTiles;
					cb2[i][y][x].b = avg[y][x].b / numTiles;
				}
			}
		}
		
		if( numTiles > maxTiles ) {
			maxTiles = numTiles;
			maxTilesCb = i;
		}
	}
}

void frameDelta( u8x4* dst, u8x4* a, u8x4* b, int imgW, int imgH, s16x2* mv, int blockW, int blockH, u32x4 region ) {
	for( int y = region.y; y < region.y + region.h; y++) {
		for( int x = region.x; x < region.x + region.w; x++ ) {
			int bx = x / blockW;
			int by = y / blockH;
			
			s16x2 m = mv[by*(imgW/blockW)+bx];
			
			int x2 = clamp( x + m.x, 0, imgW - 1 );
			int y2 = clamp( y + m.y, 0, imgH - 1 );
			
			//dst[y*imgW+x].r = saturate( ( (int)a[y*imgW+x].r - b[y2*imgW+x2].r ) / 2 + 128 );
			//dst[y*imgW+x].g = saturate( ( (int)a[y*imgW+x].g - b[y2*imgW+x2].g ) / 2 + 128 );
			//dst[y*imgW+x].b = saturate( ( (int)a[y*imgW+x].b - b[y2*imgW+x2].b ) / 2 + 128 );
			dst[y*imgW+x].r = saturate( ( (int)a[y*imgW+x].r - b[y2*imgW+x2].r ) + 128 );
			dst[y*imgW+x].g = saturate( ( (int)a[y*imgW+x].g - b[y2*imgW+x2].g ) + 128 );
			dst[y*imgW+x].b = saturate( ( (int)a[y*imgW+x].b - b[y2*imgW+x2].b ) + 128 );
			
			//dst[y*imgW+x].a = saturate( ( (int)a[y*imgW+x].a - b[y*imgW+x].a ) + 128 );
		}
	}
}

void motionVectors( s16x2* mv, u8x4* a, u8x4* b, int blockW, int blockH, int imgW, int imgH, u32x4 region ) {
	for( int by = region.y / blockH; by < ( region.y + region.h ) / blockH; by++ ) {
		for( int bx = region.x / blockW; bx < ( region.x + region.w ) / blockW; bx++ ) {
			int leastErrX = 0;
			int leastErrY = 0;
			int leastErr = INT_MAX;
			
			for( int ofy = -16; ofy < 16; ofy++ ) {
				for( int ofx = -16; ofx < 16; ofx++ ) {
					int mse = 0;
					
					for( int y = 0; y < blockH; y++ ) {
						for( int x = 0; x < blockW; x++ ) {
							int x1 = clamp( bx * blockW + x + ofx, 0, imgW - 1 );
							int y1 = clamp( by * blockH + y + ofy, 0, imgH - 1 );
							int i1 = y1 * imgW + x1;
							
							int r1 = a[i1].r;
							int g1 = a[i1].g;
							int b1 = a[i1].b;
							
							int i2 = ( by * blockH + y ) * imgW + bx * blockW + x;
							int r2 = b[i2].r;
							int g2 = b[i2].g;
							int b2 = b[i2].b;
							
							int e = abs( r1 - r2 ) + abs( g1 - g2 ) * 2 + abs( b1 - b2 );
							
							mse += e * e;
						}
					}
					
					if( mse < leastErr ) {
						leastErr = mse;
						leastErrX = ofx;
						leastErrY = ofy;
					}
				}
			}
			
			//printf( "block %d, %d  MV %d, %d\n", bx, by, leastErrX, leastErrY );
			mv[by*(imgW/blockW)+bx] = (s16x2){ leastErrX, leastErrY };
		}
	}
}

void resetCodebook( u8* cb, int cbSize, int tileW, int tileH, int cbStart, int cbNum ) {
	static u32 n = 0;
	for( int i = cbStart * tileW * tileH; i < ( cbStart + cbNum ) * tileW * tileH; i++ ) {
		cb[i] = n >> 24;
		n = n * 1664525 + 1013904223;
	}
}
