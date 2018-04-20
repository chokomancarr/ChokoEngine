#include "Engine.h"
#include "Editor.h"

#ifndef DISABLE_AV_CODECS
void AudioClip::_Init_ffmpeg(const string& path) {
#define fail {Debug::Warning("AudioClip", ffmpeg_getmsg(err)); return;}
	auto err = avformat_open_input(&formatCtx, &path[0], NULL, NULL);
	if (!!err) fail;
	err = avformat_find_stream_info(formatCtx, NULL);
	if (!!err) fail;
	av_dump_format(formatCtx, 0, &path[0], 0);
	audioStrm = -1;
	for (uint i = 0; i < formatCtx->nb_streams; i++) {
		if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStrm = i;
			break;
		}
	}
	if (audioStrm == -1) {
		Debug::Warning("AudioClip", "No audio data found!");
		return;
	}
	codecCtx0 = formatCtx->streams[audioStrm]->codecpar;
	codec = avcodec_find_decoder(codecCtx0->codec_id);
	if (!codec) {
		Debug::Warning("AudioClip", "Unsupported codec!");
		return;
	}
	codecCtx = avcodec_parameters_alloc();
	if (!!avcodec_parameters_copy(codecCtx, codecCtx0)) {
		Debug::Warning("AudioClip", "Could not copy codec!");
		return;
	}

	sampleRate = codecCtx->sample_rate;
	channels = codecCtx->channels;

	AVCodecContext* _codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(_codecCtx, codecCtx);
	avcodec_open2(_codecCtx, codec, NULL);

	AVPacket packet;
	AVFrame* frame = av_frame_alloc();
	dataSize = 0;
	//uint fds = 0;
	float ds = AudioEngine::samplesPerSec / (float)sampleRate;
	while (av_read_frame(formatCtx, &packet) >= 0) {
		if (packet.stream_index == audioStrm) {
			avcodec_send_packet(_codecCtx, &packet);
			av_packet_unref(&packet);
			while (!avcodec_receive_frame(_codecCtx, frame)) {
				//auto ds = av_samples_get_buffer_size(NULL, channels, frame->nb_samples, _codecCtx->sample_fmt, 1);
				//fds += ds*channels*frame->nb_samples;
				uint ds2 = (uint)ceil(dataSize + ds*channels*frame->nb_samples);
				uint sc2 = ds2 - dataSize;
				_data.resize(ds2);
				short* srl = (short*)frame->data[0];
				if (channels > 1) {
					short* srr = (short*)frame->data[1];
					for (uint a = 0; a < sc2 / 2; a++) {
						uint i = (uint)floor(a / ds);
						_data[dataSize++] = (ushort)(srl[i] + (65535 / 2));//(ushort)(srl[i] * 2.0f / 65535);
						_data[dataSize++] = (ushort)(srr[i] + (65535 / 2));//(ushort)(srr[i] * 2.0f / 65535);
					}
				}
				else {
					for (uint a = 0; a < sc2; a++) {
						uint i = (uint)floor(a / ds);
						_data[dataSize++] = (ushort)(srl[i] + (65535 / 2));//(ushort)(srl[i] * 2.0f / 65535);
					}
				}
			}
		}
	}
	av_frame_free(&frame);
	Debug::Message("AudioClip", "loaded " + path);

#undef fail
	loaded = true;
}
#endif

#if defined(PLATFORM_WIN)
void AudioClip::_Init_win(const string& path) {

}

#elif defined(PLATFORM_ADR)
void AudioClip::_Init_adr(const string& path) {

}
#endif

#ifdef IS_EDITOR
AudioClip::AudioClip(uint i, Editor* e) : AssetObject(ASSETTYPE_AUDIOCLIP), channels(0) {
	auto s = e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_AUDIOCLIP][i] + ".meta";
	std::ifstream strm(s, std::ios::in | std::ios::binary);

	_Strm2Val(strm, sampleRate);
	_Strm2Val(strm, (byte&)channels);
	_Strm2Val(strm, dataSize);
	_data.resize(dataSize);
	strm.read((char*)&_data[0], dataSize * sizeof(ushort));
	length = (float)dataSize / channels / sampleRate;

	for (uint a = 0; a < 256; a++) {
		_eData[a] = (_data[(uint)floor(a * dataSize / 256.0f)] / 65535.0f) * 2.0f - 1.0f;
		_eDataV[a] = Vec3((a / 256.0f) * 2.0f - 1.0f, _eData[a], 0);
	}
}

bool AudioClip::Parse(string path) {
	auto clip = AudioClip(path);
	std::ofstream strm(path + ".meta", std::ios::out | std::ios::binary);
	_StreamWrite(&clip.sampleRate, &strm, 4);
	_StreamWrite(&clip.channels, &strm, 1);
	_StreamWrite(&clip.dataSize, &strm, 4);
	_StreamWrite(&clip._data[0], &strm, clip.dataSize * sizeof(ushort));
	return true;
}
#endif