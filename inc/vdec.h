#ifndef _VDEC_H
#define _VDEC_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "types.h"

typedef struct {
	AVFormatContext* formatCtx;
	int videoStream;
	AVCodecContext* codecCtx;
	AVCodec* codec;
	AVFrame* frame;
	AVFrame* frameRGB;
	//AVPacket packet;
	//int frameEnd;
	//int numBytes;
	u8* buf;
	AVDictionary* options;
	struct SwsContext* swsCtx;
} vdecCtx_t;

void vdecInit( void );
s16x2 vdecOpen( vdecCtx_t* ctx, const char* filename );
u8x4* vdecFrame( vdecCtx_t* ctx );

#endif
