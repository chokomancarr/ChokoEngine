#pragma once
#include "AssetObjects.h"

#ifdef FEATURE_AV_CODECS
class VideoTexture : public Texture {
public:
	VideoTexture(const string& path) : formatCtx(0), codecCtx0(0), codecCtx(0), codec(0), videoStrm(-1), audioStrm(-1) {
#ifdef FEATURE_AV_CODECS
		_Init_ffmpeg(path);
#elif defined(PLATFORM_WIN)
		_Init_win(path);
#elif defined(PLATFORM_ADR)
		_Init_adr(path);
#endif
	}

	void Play(), Pause(), Stop();
	//protected:
	GLuint d_fbo;
	std::vector<byte> buffer;

#ifdef FEATURE_AV_CODECS
	AVFormatContext* formatCtx = 0;
	AVCodecParameters* codecCtx0 = 0;
	AVCodecParameters* codecCtx = 0;
	AVCodec* codec = 0;
	SwsContext *swsCtx = 0;
	uint videoStrm, audioStrm;
#endif

	uint width, height;

	void GetFrame();

private:
#if defined(FEATURE_AV_CODECS) || defined(IS_EDITOR) || defined(CHOKO_LAIT_BUILD)
	void _Init_ffmpeg(const string& path);
#endif
#if defined(PLATFORM_WIN)
	void _Init_win(const string& path);
#elif defined(PLATFORM_ADR)
	void _Init_adr(const string& path);
#endif
};
#endif
