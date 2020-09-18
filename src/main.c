#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <pthread.h>

#include "tigr.h"

#include "types.h"
#include "vdec.h"
#include "vq.h"
#include "vq_avx.h"
#include "lzss.h"

#define USE_AVX

pthread_barrier_t frameStage1;
pthread_barrier_t frameStage2;
pthread_barrier_t frameStage3;
pthread_barrier_t frameStage4;
pthread_barrier_t frameStage5;

typedef struct {
	u32x4 region;
	int cbpStart, cbpLen;
	u8x4* cb;
	int cbSize;
	u16* vf;
	s16x2* mv;
	int iw, ih;
	int tileW, tileH;
	int blockW, blockH;
	u8x4** lastFrame;
	u8x4** oldFrame;
	u8x4** buf;
	u8x4* src;
	int* newFrame;
} threadArgs_t;

int threadFn( volatile threadArgs_t* args ) {
	while( TRUE ) {
		// start iteration
		pthread_barrier_wait( &frameStage1 );
		
#ifdef USE_AVX
		int maxErrCb = encodeFrameAVX( args->vf, args->src, args->iw, args->ih, args->cb, args->cbSize, args->region );
#else
		int maxErrCb = encodeFrame( args->vf, args->src, args->iw, args->ih, args->tileW, args->tileH, args->cb, args->cbSize, args->region );
#endif
		
		// wait for encode
		pthread_barrier_wait( &frameStage2 );
		
		if( *args->newFrame ) {
			// swap buffers
			pthread_barrier_wait( &frameStage3 );
			
			renderFrame( *args->lastFrame, *args->oldFrame, args->tileW, args->tileH, args->vf, args->iw, args->ih, args->cb, args->mv, args->blockW, args->blockH, TRUE, args->region );
			
			// sync?
			pthread_barrier_wait( &frameStage4 );
			
#ifdef USE_AVX
			motionVectorsAVX2( args->mv, *args->lastFrame, *args->buf, args->iw, args->ih, args->region );
#else
			motionVectors( args->mv, *args->lastFrame, *args->buf, args->blockW, args->blockH, args->iw, args->ih, args->region );
#endif
			
			frameDelta( args->src, *args->buf, *args->lastFrame, args->iw, args->ih, args->mv, args->blockW, args->blockH, args->region );
			
			resetCodebook( (u8*)args->cb, args->cbSize, args->tileW, args->tileH, args->cbpStart, args->cbpLen );
			
			// free old buffers
			pthread_barrier_wait( &frameStage5 );
			
		} else {
			refineCodebook( maxErrCb, args->cb, args->cbSize, args->vf, args->tileW, args->tileH, args->src, args->iw, args->ih, args->cbpStart, args->cbpLen );
			//refineCodebookAVX2( maxErrCb, args->cb, args->cbSize, args->vf, args->src, args->iw, args->ih, args->cbpStart, args->cbpLen );
		}
	}
}

// * motion compensation
// * vector quantization
// + huffman coding
// + rle / lzss
// - luma weighted squared error for AVX path

int main( int argc, char* argv[] ) {
	
	vdecInit();
	vdecCtx_t vctx;
	s16x2 dim = vdecOpen( &vctx, argv[1] );
	int iw = dim.x;
	int ih = dim.y;
	u8x4* volatile buf = vdecFrame( &vctx );
	
	//FILE* outfd = fopen( "enc.bin", "wb" );
	
	Tigr* win = tigrWindow( iw, ih, "VQ", TIGR_FIXED );
	
	int codebookSize = 16;
	int tileW = 4;
	int tileH = 4;
	
	int indexBits = 4;
	int motionBits = 10;
	
	int blockW = 8;
	int blockH = 8;
	
	int iterations = 256;
	
	int cbsz = /*codebookSize * tileW * tileH * 4;//*/codebookSize * tileW * tileH * 3;
	int fsz = /*( iw / tileW ) * ( ih / tileH ) * 2;//*/( ( iw / tileW ) * ( ih / tileH ) * indexBits + 7 ) / 8;
	int mvsz = /*( iw / blockW ) * ( ih / blockH ) * 4;//*/( ( iw / blockW ) * ( ih / blockH ) * motionBits + 7 ) / 8;
	
	printf( "codebook size       : %d bytes\n", cbsz );
	printf( "frame size          : %d bytes\n", fsz );
	printf( "motion vector size  : %d bytes\n", mvsz );
	int tsz = cbsz + fsz + mvsz;
	printf( "total size          : %d bytes\n", tsz );
	
	//char* testStr = "codebook size frame size motion vector size total size";
	//rle( NULL, NULL, testStr, strlen( testStr ), 1 );
	//huff( NULL, NULL, testStr, strlen( testStr ), 1 );
	
	fflush( stdout );
	
	u8x4 (*cb)[tileH][tileW];
	cb = _mm_malloc( codebookSize * tileW * tileH * 4, 32 );
	resetCodebook( (u8*)cb, codebookSize, tileW, tileH, 0, codebookSize );
	
	u16* vf = _mm_malloc( ( iw / tileW ) * ( ih / tileH ) * 2, 32 );
	
	int frame = 0;
	int iteration = 0;
	
	u8x4* src = _mm_malloc( iw * ih * 4, 32 );
	//memcpy( src, img->pix, img->w * img->h * 4 );
	
	s16x2* mv = _mm_malloc( ( iw / blockW ) * ( ih / blockH ) * 4, 32 );
	memset( mv, 0, ( iw / blockW ) * ( ih / blockH ) * 4 );
	
	u8x4* volatile lastFrame = _mm_malloc( iw * ih * 4, 32 );
	memset( lastFrame, 128, iw * ih * 4 );
	
	frameDelta( src, buf, lastFrame, iw, ih, mv, blockW, blockH, (u32x4){ 0, 0, iw, ih } );
	
	volatile int newFrame = FALSE;
	
	u8x4* volatile oldFrame = lastFrame;
	
	int numThreads = 8;
	
	pthread_barrier_init( &frameStage1, NULL, numThreads + 1 );
	pthread_barrier_init( &frameStage2, NULL, numThreads + 1 );
	pthread_barrier_init( &frameStage3, NULL, numThreads + 1 );
	pthread_barrier_init( &frameStage4, NULL, numThreads );
	pthread_barrier_init( &frameStage5, NULL, numThreads + 1 );
	
	threadArgs_t targs[numThreads];
	pthread_t threads[numThreads];
	for( int i = 0; i < numThreads; i++ ) {
		targs[i].region = (u32x4){ ( iw / numThreads ) * i, 0, iw / numThreads, ih };
		targs[i].cbpStart = ( codebookSize / numThreads ) * i;
		targs[i].cbpLen = codebookSize / numThreads;
		targs[i].cb = cb;
		targs[i].cbSize = codebookSize;
		targs[i].vf = vf;
		targs[i].mv = mv;
		targs[i].iw = iw;
		targs[i].ih = ih;
		targs[i].tileW = tileW, 
		targs[i].tileH = tileH;
		targs[i].blockW = blockW;
		targs[i].blockH = blockH;
		targs[i].lastFrame = &lastFrame;
		targs[i].oldFrame = &oldFrame;
		targs[i].buf = &buf;
		targs[i].src = src;
		targs[i].newFrame = &newFrame;
		
		pthread_create( &threads[i], NULL, threadFn, &targs[i] );
	}
	
	while( !tigrClosed( win ) && frame < 240 ) {
		pthread_barrier_wait( &frameStage1 );
		//printf( "stage 1\n" );  fflush( stdout );
		
		// parallel - encode
		iteration++;
		newFrame = ( iteration == iterations );
		
		pthread_barrier_wait( &frameStage2 );
		//printf( "stage 2\n" );  fflush( stdout );
		
		if( newFrame ) {
			oldFrame = lastFrame;
			lastFrame = _mm_malloc( iw * ih * 4, 32 );
			
			pthread_barrier_wait( &frameStage3 );
			//printf( "stage 3\n" );  fflush( stdout );
			
			// parallel - render/delta
			//pthread_barrier_wait( &frameStage4 );
			
			pthread_barrier_wait( &frameStage5 );
			//printf( "stage 4\n" );  fflush( stdout );
			
			_mm_free( oldFrame );
			
			buf = vdecFrame( &vctx );
			
			//lzss( vf, ( iw / tileW ) * ( ih / tileH ) * 2 );
			//lzss( mv, ( iw / blockW ) * ( ih / blockH ) * 4 );
			huff( NULL, 0, vf, ( iw / tileW ) * ( ih / tileH ) * 2 / 2, 2 );
			//huff( NULL, 0, mv, ( iw / blockW ) * ( ih / blockH ) * 4 / 2, 2 );
			
			memcpy( win->pix, lastFrame, iw * ih * 4 );
			tigrUpdate( win );
			
			iteration = 0;
			frame++;
			
		} else {
			// parallel - refine
		}
	}
	
	return 0;
}
