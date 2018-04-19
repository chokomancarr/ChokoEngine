#pragma once
#include "AssetObjects.h"

class AnimClip_Key {
public:
	AnimClip_Key(string name, AnimKeyType type, uint cnt = 0, float* loc = 0) : name(name), type(type),
		dim((type == AK_ShapeKey) ? 1 : ((type & 0x0f) == 0x01) ? 4 : 3) {
		if (!!cnt) AddFrames(cnt, loc);
	}

	std::vector<std::pair<int, Vec4>> frames;
	uint frameCount;

	string name;
	const AnimKeyType type;
	const byte dim;

	void AddFrames(uint cnt, float* loc);
	Vec4 Eval(float t);
};

class AnimClip : public AssetObject {
public:
	std::vector<AnimClip_Key*> keys;
	ushort keyCount;
	ushort frameStart, frameEnd;
	bool repeat;

	std::vector<string> keynames;
	std::vector<Vec4> keyvals;

	Interpolation interpolation;

	void Eval(float t);

	friend class Editor;
	friend class AssetManager;
	_allowshared(AnimClip);
protected:
	AnimClip(Editor* e, int i);
	AnimClip(std::ifstream& strm, uint offset);
	AnimClip(string path);
};
