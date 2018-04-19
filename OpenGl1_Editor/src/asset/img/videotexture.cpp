#include "Engine.h"

#ifndef DISABLE_AV_CODECS
void VideoTexture::_Init_ffmpeg(const string& path) {
#define fail {Debug::Warning("VideoTexture", ffmpeg_getmsg(err)); return;}
	auto err = avformat_open_input(&formatCtx, &path[0], NULL, NULL);
	if (!!err) fail;
	err = avformat_find_stream_info(formatCtx, NULL);
	if (!!err) fail;
	av_dump_format(formatCtx, 0, &path[0], 0);
	for (uint i = 0; i < formatCtx->nb_streams; i++) {
		if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStrm = i;
			break;
		}
	}
	if (videoStrm == -1) {
		Debug::Warning("VideoTexture", "No video data found!");
		return;
	}
	codecCtx0 = formatCtx->streams[videoStrm]->codecpar;
	codec = avcodec_find_decoder(codecCtx0->codec_id);
	if (!codec) {
		Debug::Warning("VideoTexture", "Unsupported codec!");
		return;
	}
	codecCtx = avcodec_parameters_alloc();
	if (!!avcodec_parameters_copy(codecCtx, codecCtx0)) {
		Debug::Warning("VideoTexture", "Could not copy codec!");
		return;
	}
	width = codecCtx->width;
	height = codecCtx->height;

	swsCtx = sws_getContext(width, height, (AVPixelFormat)codecCtx->format, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, 0, 0, 0);
	Debug::Message("VideoTexture", "loaded " + path);
#undef fail
}

#if defined(PLATFORM_WIN)
void VideoTexture::_Init_win(const string& path) {

}

#elif defined(PLATFORM_ADR)
void VideoTexture::_Init_adr(const string& path) {

}
#endif

void VideoTexture::GetFrame() {
	Debug::Warning("VT", "get frame failed!");
}

#endif