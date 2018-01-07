#include "Engine.h"
#include "Editor.h"

#define F2ISTREAM(_varname, _pathname) std::ifstream _f2i_ifstream((_pathname).c_str(), std::ios::in | std::ios::binary); \
std::istream _varname(_f2i_ifstream.rdbuf());

void AnimClip_Key::AddFrames(uint cnt, float* loc) {
	while (cnt > 0) {
		int t = *(int*)loc;
		loc++;
		Vec4 v;
		memcpy(&v, loc, dim*sizeof(float));
		frames.push_back(std::pair<int, Vec4>(t, v));
		loc += dim;
		cnt--;
	}
}

Vec4 AnimClip_Key::Eval(float t) {
	for (uint f = 0; f < frameCount - 1; f++) {
		if (frames[f + 1].first > t) {
			auto& fc = frames[f];
			auto& fn = frames[f + 1];
			return Lerp(fc.second, fn.second, InverseLerp(fc.first, fn.first, t));
		}
	}
	return frames[frameCount - 1].second;
}

//ANIM [curvecount 2] [framestart 2] [frameend 2] { [curvetype 1] name0 [uselerp 1] [framecount 2] { [pos vec2*] if uselerp([:before vec2*] [:after vec2*]) } }
//vec2* -> x = time, y = n-dim value
AnimClip::AnimClip(string p) : AssetObject(ASSETTYPE_ANIMCLIP) {
	//string p = Editor::instance->projectFolder + "Assets\\" + path + ".animclip.meta";
	//std::ifstream stream(p.c_str(), std::ios::in | std::ios::binary);
	F2ISTREAM(stream, p);
	if (!stream.good()) {
		std::cout << "animclip file not found!" << std::endl;
		return;
	}

	char* c = new char[100];
	stream.read(c, 4);
	c[4] = char0;
	if (string(c) != "ANIM") {
		Debug::Error("AnimClip importer", "file not supported");
		return;
	}

	AnimKeyType tp;
	byte ul;
	char nm[300];
	ushort fc;

	_Strm2Val(stream, keyCount);
	_Strm2Val(stream, frameStart);
	_Strm2Val(stream, frameEnd);
	keys.reserve(keyCount);
	keynames.resize(keyCount);
	keyvals.resize(keyCount);
	std::cout << "Clip data: " << keyCount << " keys @ " << frameStart << " ~ " << frameEnd << std::endl;
	for (ushort aa = 0; aa < keyCount; aa++) {
		if (stream.eof()) {
			Debug::Error("AnimClip", "Unexpected eof");
			return;
		}
		_Strm2Val(stream, tp);
		stream.getline(nm, 300, 0);
		_Strm2Val(stream, ul);

		AnimClip_Key* key = new AnimClip_Key(nm, tp);
		_Strm2Val(stream, fc);
		uint datac = fc * 4 * (key->dim + 1);
		char* data = new char[datac];
		stream.read(data, datac);
		key->AddFrames(fc, (float*)data);
		keys.push_back(key);
		delete[](data);

		keynames[aa] = (string(nm) + (char)tp);
	}
}

void AnimClip::Eval(float t) {
	t = Repeat<float>(t, frameStart, frameEnd);
	for (ushort k = 0; k < keyCount; k++) {
		keyvals[k] = keys[k]->Eval(t);
	}
}

void Anim_State::SetClip(AnimClip* nc) {
	clip = nc;
	keynames = clip->keynames;
	keyvals = clip->keyvals;
}

void Anim_State::Eval(float dt) {
	time += dt;
	if (!isBlend) {
		if (!clip) return;
		clip->Eval(time);
		keyvals = clip->keyvals;
	}
}


Animation::Animation() : AssetObject(ASSETTYPE_ANIMATION), activeState(0), nextState(0), stateTransition(0), states(), transitions() {
	
}

Animation::Animation(string p) : Animation() {
	F2ISTREAM(stream, p);
	if (!stream.good()) {
		std::cout << "animator not found!" << std::endl;
		return;
	}
	char* c = new char[4];
	stream.read(c, 3);
	c[3] = char0;
	string ss(c);
	if (ss != "ANT") {
		std::cerr << "file not supported" << std::endl;
		return;
	}
	byte stateCount, transCount;
	_Strm2Val(stream, stateCount);
	for (byte a = 0; a < stateCount; a++) {
		Anim_State* state = new Anim_State();
		_Strm2Val(stream, state->isBlend);
		ASSETTYPE t;
		if (!state->isBlend) {
			_Strm2Asset(stream, Editor::instance, t, state->_clip);
			state->SetClip(_GetCache<AnimClip>(ASSETTYPE_ANIMCLIP, state->_clip));
		}
		else {
			byte bc;
			_Strm2Val(stream, bc);
			state->blendClips.resize(bc);
			state->_blendClips.resize(bc);
			for (byte c = 0; c < bc; c++) {
				_Strm2Asset(stream, Editor::instance, t, state->_blendClips[c]);
				state->blendClips[c] = _GetCache<AnimClip>(ASSETTYPE_ANIMCLIP, state->_blendClips[c]);
			}
		}
		_Strm2Val(stream, state->speed);
		_Strm2Val(stream, state->editorPos.x);
		_Strm2Val(stream, state->editorPos.y);
		states.push_back(state);
	}
	_Strm2Val(stream, transCount);

}

int Animation::IdOf(const string& s) {
	for (uint a = 0; a < keynames.size(); a++) {
		if (keynames[a] == s) return a;
	}
	return -1;
}

Vec4 Animation::Get(const string& s) {
	for (uint a = 0; a < keynames.size(); a++) {
		if (keynames[a] == s) return keyvals[a];
	}
	return Vec4();
}

Vec4 Animation::Get(uint i) {
	if (i < 0 || i >= keyvals.size()) return Vec4();
	return keyvals[i];
}


Animator::Animator(Animation* anim) : Component("Animator", COMP_ANM), animation(anim) {
	Scene::active->_preLUpdateComps.push_back(this);
}

Animator::~Animator() {
	auto& prc = Scene::active->_preLUpdateComps;
	prc.erase(std::find(prc.begin(), prc.end(), this));
}

void Animator::OnPreLUpdate() {
	if (!animation) return;

	//
	animation->states[0]->Eval(Time::delta*fps);
	animation->keynames = animation->states[0]->keynames;
	animation->keyvals = animation->states[0]->keyvals;
}

void Animator::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Animation", e->font, white());
		e->DrawAssetSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), ASSETTYPE_ANIMATION, 12, e->font, &_animation, &_SetAnim, this);
		//if (_animation != -1) {

		//}
	}
	else pos += 17;
}

void Animator::_SetAnim(void* v) {
	Animator* r = (Animator*)v;
	r->animation = _GetCache<Animation>(ASSETTYPE_ANIMATION, r->_animation);
	r->object->dirty = true;
}
