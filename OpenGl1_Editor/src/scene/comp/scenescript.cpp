#include "Engine.h"
#include "Editor.h"

SceneScript::~SceneScript() {
#ifdef IS_EDITOR
	for (auto& a : _vals) {
		delete(a.second.val);
	}
#endif
}

#ifdef IS_EDITOR

SceneScript::SceneScript(Editor* e, string s) : Component(s + " (Script)", COMP_SCR, DRAWORDER_NONE), _script(0) {
	if (s == "") {
		name = "missing script!";
		return;
	}
	std::ifstream strm(e->projectFolder + "Assets\\" + s + ".meta", std::ios::in | std::ios::binary);
	if (!strm.is_open()) {
		e->_Error("Script Component", "Cannot read meta file!");
		_script = -1;
		return;
	}
	ushort sz;
	_Strm2Val<ushort>(strm, sz);
	_vals.resize(sz);
	SCR_VARTYPE t;
	byte t2 = 0;
	for (ushort x = 0; x < sz; x++) {
		_Strm2Val<SCR_VARTYPE>(strm, t);
		_vals[x].second.type = t;
		if (t == SCR_VAR_ASSREF || t == SCR_VAR_COMPREF)
			_Strm2Val<byte>(strm, t2);
		char c[500];
		strm.getline(&c[0], 100, 0);
		_vals[x].first += string(c);
		switch (t) {
		case SCR_VAR_INT:
		case SCR_VAR_UINT:
		case SCR_VAR_FLOAT:
			_vals[x].second.val = new int();
			_Strm2Val<int>(strm, *(int*)_vals[x].second.val);
			break;
		case SCR_VAR_V2:
			_vals[x].second.val = new Vec2();
			_Strm2Val<Vec2>(strm, *(Vec2*)_vals[x].second.val);
			break;
		case SCR_VAR_V3:
			_vals[x].second.val = new Vec3();
			_Strm2Val<Vec3>(strm, *(Vec3*)_vals[x].second.val);
			break;
		case SCR_VAR_V4:
			_vals[x].second.val = new Vec4();
			_Strm2Val<Vec4>(strm, *(Vec4*)_vals[x].second.val);
			break;
		case SCR_VAR_STRING:
			strm.getline(&c[0], 500, 0);
			_vals[x].second.val = new string(c);
			break;
		case SCR_VAR_ASSREF:
			_vals[x].second.val = new AssRef((ASSETTYPE)t2);
			break;
		case SCR_VAR_COMPREF:
			_vals[x].second.val = new CompRef((COMPONENT_TYPE)t2);
			break;
		}
	}
}

SceneScript::SceneScript(std::ifstream& strm, SceneObject* o) : Component("", COMP_SCR, DRAWORDER_NONE), _script(-1) {
	char* c = new char[100];
	strm.getline(c, 100, 0);
	string s(c);
	int i = 0;
	for (auto a : Editor::instance->headerAssets) {
		if (a == s) {
			_script = i;
			name = a + " (Script)";

			std::ifstream strm(Editor::instance->projectFolder + "Assets\\" + a + ".meta", std::ios::in | std::ios::binary);
			if (!strm.is_open()) {
				Editor::instance->_Error("Script Component", "Cannot read meta file!");
				_script = -1;
				return;
			}
			ushort sz;
			_Strm2Val<ushort>(strm, sz);
			_vals.resize(sz);
			SCR_VARTYPE t;
			for (ushort x = 0; x < sz; x++) {
				_Strm2Val<SCR_VARTYPE>(strm, t);
				_vals[x].second.type = t;
				char c[100];
				strm.getline(&c[0], 100, 0);
				_vals[x].first += string(c);
				switch (t) {
				case SCR_VAR_INT:
					_vals[x].second.val = new int(0);
					break;
				case SCR_VAR_FLOAT:
					_vals[x].second.val = new float(0);
					break;
				case SCR_VAR_STRING:
					_vals[x].second.val = new string("");
					break;
				case SCR_VAR_COMPREF:
					_vals[x].second.val = new CompRef((COMPONENT_TYPE)c[0]);
					_vals[x].first = _vals[x].first.substr(1);
					break;
				}
			}
			break;
		}
		i++;
	}
	ushort sz;
	byte tp;
	_Strm2Val<ushort>(strm, sz);
	for (ushort aa = 0; aa < sz; aa++) {
		_Strm2Val<byte>(strm, tp);
		strm.getline(c, 100, 0);
		string ss(c);
		for (auto& vl : _vals) {
			if (vl.first == ss && vl.second.type == SCR_VARTYPE(tp)) {
				switch (vl.second.type) {
				case SCR_VAR_INT:
					int ii;
					_Strm2Val<int>(strm, ii);
					*(int*)vl.second.val += ii;
					break;
				case SCR_VAR_FLOAT:
					float ff;
					_Strm2Val<float>(strm, ff);
					*(float*)vl.second.val += ff;
					break;
				case SCR_VAR_STRING:
					strm.getline(c, 100, 0);
					*(string*)vl.second.val += string(c);
					break;
				}
			}
		}
	}
	delete[](c);
}

void SceneScript::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_SCRIPT_H, _script);
	ushort sz = (ushort)_vals.size();
	_StreamWrite(&sz, stream, 2);
	for (auto& a : _vals) {
		_StreamWrite(&a.second.type, stream, 1);
		*stream << a.first << char0;
		switch (a.second.type) {
		case SCR_VAR_INT:
		case SCR_VAR_FLOAT:
			_StreamWrite(a.second.val, stream, 4);
			break;
		}
	}
}

void SceneScript::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	SceneScript* scr = (SceneScript*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		for (auto& p : _vals) {
			UI::Label(v.r + 20, v.g + pos + 2, 12, p.first, e->font, white(1, (p.second.type == SCR_VAR_COMMENT) ? 0.5f : 1));
			auto& val = p.second.val;
			switch (p.second.type) {
			case SCR_VAR_INT:
				*(int*)val = TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), std::to_string(*(int*)val), e->font, true, nullptr, white()), *(int*)val);
				break;
			case SCR_VAR_UINT:
				*(uint*)val = TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), std::to_string(*(uint*)val), e->font, true, nullptr, white()), *(uint*)val);
				break;
			case SCR_VAR_V2:
				((Vec2*)val)->x = TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.35f - 1, 16, 12, Vec4(0.4f, 0.2f, 0.2f, 1), std::to_string(((Vec3*)val)->x), e->font, false, nullptr, white(), hlCol(), white(), false), ((Vec3*)val)->x);
				((Vec2*)val)->y = TryParse(UI::EditText(v.r + v.b*0.65f, v.g + pos, v.b*0.35f - 1, 16, 12, Vec4(0.2f, 0.4f, 0.2f, 1), std::to_string(((Vec3*)val)->y), e->font, false, nullptr, white(), hlCol(), white(), false), ((Vec3*)val)->y);
				break;
			case SCR_VAR_V3:
				((Vec3*)val)->x = TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.23f - 1, 16, 12, Vec4(0.4f, 0.2f, 0.2f, 1), std::to_string(((Vec3*)val)->x), e->font, false, nullptr, white(), hlCol(), white(), false), ((Vec3*)val)->x);
				((Vec3*)val)->y = TryParse(UI::EditText(v.r + v.b*0.53f, v.g + pos, v.b*0.23f - 1, 16, 12, Vec4(0.2f, 0.4f, 0.2f, 1), std::to_string(((Vec3*)val)->y), e->font, false, nullptr, white(), hlCol(), white(), false), ((Vec3*)val)->y);
				((Vec3*)val)->z = TryParse(UI::EditText(v.r + v.b*0.77f, v.g + pos, v.b*0.23f - 1, 16, 12, Vec4(0.2f, 0.2f, 0.4f, 1), std::to_string(((Vec3*)val)->z), e->font, false, nullptr, white(), hlCol(), white(), false), ((Vec3*)val)->z);
				break;
			case SCR_VAR_V4:
				((Vec4*)val)->x = TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.35f - 1, 16, 12, Vec4(0.4f, 0.2f, 0.2f, 1), std::to_string(((Vec3*)val)->x), e->font, false, nullptr, white(), hlCol(), white(), false), ((Vec3*)val)->x);
				((Vec4*)val)->y = TryParse(UI::EditText(v.r + v.b*0.65f, v.g + pos, v.b*0.35f - 1, 16, 12, Vec4(0.2f, 0.4f, 0.2f, 1), std::to_string(((Vec3*)val)->y), e->font, false, nullptr, white(), hlCol(), white(), false), ((Vec3*)val)->y);
				break;
			case SCR_VAR_FLOAT:
				*(float*)val = TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), std::to_string(*(float*)val), e->font, true, nullptr, white()), *(float*)val);
				break;
			case SCR_VAR_STRING:
				*(string*)val = UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), *(string*)val, e->font, true, nullptr, white());
				break;
			case SCR_VAR_ASSREF:
				e->DrawAssetSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey2(), ((AssRef*)val)->type, 12, e->font, &((AssRef*)val)->_asset, &AssRef::SetObj, val);
				break;
			case SCR_VAR_COMPREF:
				e->DrawCompSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey2(), 12, e->font, (CompRef*)val);
				break;
			}
			pos += 17;
		}
	}
	pos += 17;
}

std::vector<string> SceneScript::userClasses = {};

bool SceneScript::Check(string s, Editor* e) {
	auto className = s.substr(s.find_last_of('\\') + 1);
	className = className.substr(0, className.size() - 2);
	string p = e->projectFolder + "System\\xml\\class" + className + ".xml";
	//string p2 = e->projectFolder + "Assets\\" + s + ".h.meta";
	//std::ofstream strm(p2, std::ios::out || std::ios::binary);
	if (!IO::HasFile(p)) {
		//	strm << "\0";
		return false;
	}
	auto xml = Xml::Parse(p);
	auto& base = xml->children[0].children[0];
	if (base.children[1].name != "basecompoundref" || base.children[1].value != "SceneScript") {
		//	strm << "\0";
		return false;
	}
	return true;
}

void SceneScript::Parse(string s, Editor* e) {
	auto className = s.substr(s.find_last_of('\\') + 1);
	className = className.substr(0, className.size() - 2);
	string p = e->projectFolder + "System\\xml\\class" + className + ".xml";
	string p2 = e->projectFolder + "Assets\\" + s + ".meta";
	std::ofstream strm(p2, std::ios::out | std::ios::binary);
	auto xml = Xml::Parse(p);
	auto& base = xml->children[0].children[0];

	std::cout << ": " << p2 << std::endl;

	uint i = 0;
	auto pos = strm.tellp();
	strm << "00";
	for (auto& c : base.children) {
		if (c.name == "sectiondef" && c.params["kind"] == "public-attrib") {
			for (auto& cc : c.children) {
				if (cc.params["kind"] == "variable" && cc.params["static"] == "no") {
					auto& tp = cc.children[0].value;
					auto& nm = cc.children[3].value;
					auto vt = String2Type(tp);
					byte val[100] = {};
					byte valSz = 0;
					if (vt == SCR_VAR_UNDEF) {
						auto v2 = String2Asset(tp);
						if (v2 == ASSETTYPE_UNDEF) {
							auto v3 = String2Comp(tp);
							if (v3 == COMP_UNDEF) continue;
							vt = SCR_VAR_COMPREF;
							_StreamWrite(&vt, &strm, 1);
							_StreamWrite(&v3, &strm, 1);
						}
						else {
							vt = SCR_VAR_ASSREF;
							_StreamWrite(&vt, &strm, 1);
							_StreamWrite(&v2, &strm, 1);
						}
					}
					else {
						_StreamWrite(&vt, &strm, 1);
						switch (vt) {
						case SCR_VAR_V2:
							valSz = 8;
							break;
						case SCR_VAR_V3:
							valSz = 12;
							break;
						case SCR_VAR_V4:
							valSz = 16;
							break;
						case SCR_VAR_STRING:
							valSz = 1;
							break;
						default:
							valSz = 4;
							break;
						}
					}
					strm << nm << char0;
					_StreamWrite(val, &strm, valSz);
					i++;
				}
			}
			strm.seekp(pos);
			_StreamWrite(&i, &strm, 2);
			break;
		}
	}
}

#define test(val) else if (s == #val) return
SCR_VARTYPE SceneScript::String2Type(const string& s) {
	if (s.substr(0, 3) == "//#") return SCR_VAR_COMMENT;
	test(int) SCR_VAR_INT;
	test(unsigned int) SCR_VAR_UINT;
	test(uint) SCR_VAR_UINT;
	test(float) SCR_VAR_FLOAT;
	test(Vec2) SCR_VAR_V2;
	test(Vec3) SCR_VAR_V3;
	test(Vec4) SCR_VAR_V4;
	test(string) SCR_VAR_STRING;
	else return SCR_VAR_UNDEF;
}

ASSETTYPE SceneScript::String2Asset(const string& s) { //only references are allowed
	if (s == "rTexture") return ASSETTYPE_TEXTURE;
	test(rBackground) ASSETTYPE_HDRI;
	//test(rCubeMap) ASSETTYPE_TEXCUBE;
	test(rShader) ASSETTYPE_SHADER;
	test(rMaterial) ASSETTYPE_MATERIAL;
	//test() ASSETTYPE_BLEND;
	test(rMesh) ASSETTYPE_MESH;
	test(rAnimClip) ASSETTYPE_ANIMCLIP;
	test(rAnimation) ASSETTYPE_ANIMATION;
	//test(rCameraEffect) ASSETTYPE_CAMEFFECT;
	test(rAudioClip) ASSETTYPE_AUDIOCLIP;
	test(rRenderTexture) ASSETTYPE_TEXTURE_REND;
	else return ASSETTYPE_UNDEF;
}

COMPONENT_TYPE SceneScript::String2Comp(const string& s) {
	if (s == "rCamera") return COMP_CAM; //camera
	test(rMeshFilter) COMP_MFT; //mesh filter
	test(rMeshRenderer) COMP_MRD; //mesh renderer
	test(rTextureRenderer) COMP_TRD; //texture renderer
	test(rSkinnedMeshRenderer) COMP_SRD; //skinned mesh renderer
	test(rVoxelRenderer) COMP_VRD; //voxel renderer
	test(rLight) COMP_LHT; //light
	test(rReflectiveQuad) COMP_RFQ; //reflective quad
	test(rReflectionProbe) COMP_RDP; //render probe
	test(rArmature) COMP_ARM; //armature
	test(rAnimator) COMP_ANM; //animator
	test(InverseKinematics) COMP_INK; //inverse kinematics
	test(rAudioSource) COMP_AUS; //audio source
	test(rAudioListener) COMP_AUL; //audio listener
	test(rParticleSystem) COMP_PST; //particle system
	else return COMP_UNDEF;
}
#undef test

int SceneScript::String2Script(const string& s) {
	int i = 0;
	for (auto& h : Editor::instance->headerAssets) {
		if (h == "r" + s) return i;
		i++;
	}
	return -1;
}

#endif