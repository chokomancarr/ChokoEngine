#pragma once
#include "AssetObjects.h"

class AudioClip : public AssetObject {
public:
	AudioClip(const string& path, bool stream = false) : AssetObject(ASSETTYPE_AUDIOCLIP) {
#ifdef FEATURE_AV_CODECS
		_Init_ffmpeg(path);
#elif defined(PLATFORM_WIN)
		_Init_win(path);
#elif defined(PLATFORM_ADR)
		_Init_adr(path);
#endif
	}

	bool loaded = false;

	float length;
	uint sampleRate, channels;
#ifdef IS_EDITOR
	std::array<float, 256> _eData;
	std::array<Vec3, 256> _eDataV;
#endif

	friend class Audio;
	friend class Editor;
	_allowshared(AudioClip);
protected:
	std::vector<ushort> _data;
	uint dataSize, loadPtr;

#ifdef IS_EDITOR

	AudioClip(uint i, Editor* e);

	static bool Parse(string path);
#endif

#ifdef FEATURE_AV_CODECS
	AVFormatContext* formatCtx = 0;
	AVCodecParameters* codecCtx0 = 0;
	AVCodecParameters* codecCtx = 0;
	AVCodec* codec = 0;
	uint audioStrm;
#endif

private:
#if defined(FEATURE_AV_CODECS)
	void _Init_ffmpeg(const string& path);
#endif
#if defined(PLATFORM_WIN)
	void _Init_win(const string& path);
#elif defined(PLATFORM_ADR)
	void _Init_adr(const string& path);
#endif
};
