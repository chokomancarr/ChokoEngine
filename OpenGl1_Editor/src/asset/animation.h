#pragma once
#include "AssetObjects.h"

class AnimFrameItem {
public:
	AnimFrameItem();
	friend class Animation;
	friend class Anim_State;
protected:
	byte type;
	string name;
	Vec3 transform, scale;
	Quat rotation;
};

class Anim_State {
public:
	Anim_State(bool blend = false) : Anim_State(Vec2()) {}

	string name;
	void SetClip(AnimClip* clip);

	friend class Animation;
	friend class Animator;
	friend class EB_AnimEditor;
protected:
	Anim_State(Vec2 pos, bool blend = false) : name(blend ? "newBlendState" : "newState"), isBlend(blend), speed(1), length(0), time(0), _clip(-1), editorPos(pos), editorExpand(true) {}
	bool isBlend;
	float speed, length, time;

	std::vector<string> keynames;
	std::vector<Vec4> keyvals;

	AnimClip* clip;
	ASSETID _clip;
	Vec2 editorPos;
	bool editorExpand, editorShowGraph;

	std::vector<AnimClip*> blendClips;
	std::vector<ASSETID> _blendClips;
	std::vector<float> blendOffsets;

	void Eval(float dt);
};

class Anim_Transition {
public:
	Anim_Transition();
	friend class Animation;
protected:
	bool canInterrupt;
	float length, time;

};

class Animation : public AssetObject {

	friend class EB_AnimEditor;
	friend class Editor;
	friend class SkinnedMeshRenderer;
	friend class Animator;
	_allowshared(Animation);
protected:
	Animation();
	Animation(string s);
	Animation(std::ifstream& stream, uint offset);
	uint activeState, nextState;
	float stateTransition;

	std::vector<Anim_State*> states;
	std::vector<Anim_Transition*> transitions;

	std::vector<string> keynames;
	std::vector<Vec4> keyvals;

	int IdOf(const string& s);

	Vec4 Get(const string& s);
	Vec4 Get(uint id);

	void Save(string s);
};