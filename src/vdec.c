#include "vdec.h"

void vdecInit( void ) {
	av_register_all();
}

s16x2 vdecOpen( vdecCtx_t* ctx, const char* filename ) {
	memset( ctx, 0, sizeof( vdecCtx_t ) );
	
	avformat_open_input( &ctx->formatCtx, filename, NULL, NULL );
	avformat_find_stream_info( ctx->formatCtx, NULL );
	av_dump_format( ctx->formatCtx, 0, filename, 0 );
	
	ctx->videoStream = -1;
	for( int i = 0; i < ctx->formatCtx->nb_streams; i++ ) {
		if( ctx->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) {
			ctx->videoStream = i;
			break;
		}
	}
	
	ctx->codecCtx = ctx->formatCtx->streams[ctx->videoStream]->codec;
	ctx->codec = avcodec_find_decoder( ctx->codecCtx->codec_id );
	
	avcodec_open2( ctx->codecCtx, ctx->codec, &ctx->options );
	
	ctx->frame = av_frame_alloc();
	ctx->frameRGB = av_frame_alloc();
	
	size_t numBytes = avpicture_get_size( AV_PIX_FMT_RGB32, ctx->codecCtx->width, ctx->codecCtx->height );
	ctx->buf = (u8*)av_malloc( numBytes );
	
	ctx->swsCtx = sws_getContext(
		ctx->codecCtx->width,
		ctx->codecCtx->height,
		ctx->codecCtx->pix_fmt,
		ctx->codecCtx->width,
		ctx->codecCtx->height,
		AV_PIX_FMT_RGB32,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);
	
	avpicture_fill( (AVPicture*)ctx->frameRGB, ctx->buf, AV_PIX_FMT_RGB32, ctx->codecCtx->width, ctx->codecCtx->height );
	
	return (s16x2){ ctx->codecCtx->width, ctx->codecCtx->height };
}

u8x4* vdecFrame( vdecCtx_t* ctx ) {
	AVPacket packet;
	while( av_read_frame( ctx->formatCtx, &packet ) >= 0 ) {
		if( packet.stream_index == ctx->videoStream ) {
			int frameEnd;
			avcodec_decode_video2( ctx->codecCtx, ctx->frame, &frameEnd, &packet );
			
			if( frameEnd ) {
				sws_scale(
					ctx->swsCtx,
					(uint8_t const * const *)ctx->frame->data,
					ctx->frame->linesize,
					0,
					ctx->codecCtx->height,
					ctx->frameRGB->data,
					ctx->frameRGB->linesize
				);
				
				av_free_packet( &packet );
				break;
			}
		}
		
		av_free_packet( &packet );
	}
	
	return (u8x4*)*ctx->frameRGB->data;
}
