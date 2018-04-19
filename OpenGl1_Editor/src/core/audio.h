#pragma once
#include "Engine.h"

class Audio {
public:
	class Playback {
	public:
		uint pos;
		AudioClip* clip;

		friend class Audio;
	protected:
		Playback(AudioClip* clip, float pos);

		bool Gen(float* data, uint count);
	};

	/*!
	Plays an audio clip directly into stream, ignoring effects
	[av] */
	static Playback* Play(AudioClip* clip, float pos = 0);

	//protected:
	static std::vector<Playback*> sources;

	static bool Gen(byte* data, uint count);
};
