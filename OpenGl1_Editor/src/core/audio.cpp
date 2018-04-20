#include "Engine.h"

std::vector<Audio::Playback*> Audio::sources = std::vector<Audio::Playback*>();

Audio::Playback::Playback(AudioClip* clip, float pos) : clip(clip), pos(0) {

}

bool Audio::Playback::Gen(float* data, uint count) {
	count *= 2;
	uint ac = clip->dataSize - pos + 1;
	uint c = min(ac, count);
	memcpy(data, &clip->_data[pos], c * sizeof(float));
	pos += c;
	return (count == c);
}

Audio::Playback* Audio::Play(AudioClip* clip, float pos) {
	if (!clip) return nullptr;
	auto pb = new Playback(clip, pos);
	sources.push_back(pb);
	return pb;
}

bool Audio::Gen(byte* data, uint count) {
#ifdef PLATFORM_WIN
	auto pc = count * AudioEngine::pwfx->nChannels;
	auto sz = sources.size();
	memset(data, 0, pc * sizeof(float));
	if (!sz) {
		return true;
	}
	else {
		float* d1 = (float*)data;
		float* d2 = new float[pc];
		for (int a = sz - 1; a >= 0; a--) {
			bool ct = sources[a]->Gen(d2, count);
			for (uint b = 0; b < pc; b++) {
				d1[b] += d2[b];
			}
			if (!ct) sources.erase(sources.begin() + a);
		}
	}
	return true;
#endif
	return false;
}