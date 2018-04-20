#include "Engine.h"
#include "Editor.h"

ArmatureBone::ArmatureBone(uint id, Vec3 pos, Quat rot, Vec3 scl, float lgh, bool conn, Transform* tr, ArmatureBone* par) :
	id(id), restPosition(pos), restRotation(rot), restScale(scl), restMatrix(MatFunc::FromTRS(pos, rot, scl)),
	restMatrixInv(glm::inverse(restMatrix)), restMatrixAInv(par ? restMatrixInv*par->restMatrixAInv : restMatrixInv),
	length(lgh), connected(conn), tr(tr), name(tr->object->name), fullName((par ? par->fullName + name : name) + "/"), parent(par) {}
#define _dw 0.1f
const Vec3 ArmatureBone::boneVecs[6] = {
	Vec3(0, 0, 0),
	Vec3(_dw, _dw, _dw),
	Vec3(_dw, -_dw, _dw),
	Vec3(-_dw, -_dw, _dw),
	Vec3(-_dw, _dw, _dw),
	Vec3(0, 0, 1)
};
const uint ArmatureBone::boneIndices[24] = { 0,1,0,2,0,3,0,4,1,2,2,3,3,4,4,1,4,5,3,5,2,5,1,5 };
#undef _dw
const Vec3 ArmatureBone::boneCol = Vec3(0.6f, 0.6f, 0.6f);
const Vec3 ArmatureBone::boneSelCol = Vec3(1, 190.0f / 255, 0);

Armature::Armature(std::ifstream& stream, SceneObject* o, long pos) : Component("Armature", COMP_ARM, DRAWORDER_OVERLAY) {}

Armature::Armature(string path, SceneObject* o) : Component("Armature", COMP_ARM, DRAWORDER_OVERLAY, o), overridePos(false), restPosition(o->transform.position()), restRotation(o->transform.rotation()), restScale(o->transform.localScale()) {
	std::ifstream strm(path, std::ios::binary);
	if (!strm.is_open()) {
		Debug::Error("Armature", "File not found!");
		return;
	}
	char* c = new char[4];
	strm.getline(c, 4, 0);
	string s(c);
	if (s != "ARM") {
		Debug::Error("Armature", "File signature wrong!");
		return;
	}
	delete[](c);
	std::vector<ArmatureBone*> boneList;
	char b = strm.get();
	uint i = 0;
	while (b == 'B') {
		//std::cout << " >" << std::to_string(strm.tellg());
		AddBone(strm, _bones, boneList, object.raw(), i);
		b = strm.get();
		//std::cout << " " << std::to_string(strm.tellg()) << std::endl;
	}
	_allbonenames.reserve(i);
	_allbones.reserve(i);
	_boneAnimIds.resize(i, -1);
	GenMap();
	Scene::active->_preRenderComps.push_back(this);
}

Armature::~Armature() {
	auto& prc = Scene::active->_preRenderComps;
	prc.erase(std::find(prc.begin(), prc.end(), this));
}

ArmatureBone* Armature::MapBone(string nm) {
	uint i = 0;
	for (string& name : _allbonenames) {
		if (name == nm) return _allbones[i];
		i++;
	}
	return nullptr;
}

void Armature::AddBone(std::ifstream& strm, std::vector<ArmatureBone*>& bones, std::vector<ArmatureBone*>& blist, SceneObject* o, uint& id) {
	//const Vec3 viz(1,1,-1);
	char* c = new char[100];
	strm.getline(c, 100, 0);
	string nm = string(c);
	//std::cout << "b> " << nm << std::endl;
	strm.getline(c, 100, 0);
	string pr = string(c);
	ArmatureBone* prt = nullptr;
	if (c[0] != 0) {
		for (auto& bb : blist) {
			if (bb->tr->object->name == pr) {
				prt = bb;
				break;
			}
		}
	}
	Vec3 pos, tal, fwd;
	Quat rot;
	byte mask;
	_Strm2Val(strm, pos.x);
	_Strm2Val(strm, pos.y);
	_Strm2Val(strm, pos.z);
	_Strm2Val(strm, tal.x);
	_Strm2Val(strm, tal.y);
	_Strm2Val(strm, tal.z);
	_Strm2Val(strm, fwd.x);
	_Strm2Val(strm, fwd.y);
	_Strm2Val(strm, fwd.z);
	mask = strm.get();
	rot = QuatFunc::LookAt((tal - pos), fwd);
	std::vector<ArmatureBone*>& bnv = (prt) ? prt->_children : bones;
	SceneObject* scp = (prt) ? prt->tr->object.raw() : o;
	auto oo = SceneObject::New(nm, pos, rot, Vec3(1.0f));
	ArmatureBone* bn = new ArmatureBone(id++, (prt) ? pos + Vec3(0, 0, 1)*prt->length : pos, rot, Vec3(1.0f), glm::length(tal - pos), (mask & 0xf0) != 0, &oo->transform, prt);
	bnv.push_back(bn);
	scp->AddChild(oo);
	blist.push_back(bn);
	std::cout << nm << " " << scp->name << " " << std::to_string((tal - pos)) << std::to_string(fwd) << std::endl;
	if (prt) {
		//std::cout << " " << std::to_string((tal - pos)) << std::to_string(fwd) << std::endl;
		oo->transform.localRotation(rot);
		oo->transform.localPosition(pos + Vec3(0, 0, 1)*prt->length);
	}
	else {
		//std::cout << " " << std::to_string((tal - pos)) << std::to_string(fwd) << std::endl;
		oo->transform.localRotation(rot);
		oo->transform.localPosition(pos);
	}
	char b = strm.get();
	if (b == 'b') return;
	else while (b == 'B') {
		AddBone(strm, bn->_children, blist, oo.get(), id);
		b = strm.get();
	}
}

void Armature::GenMap(ArmatureBone* b) {
	auto& bn = b ? b->_children : _bones;
	for (auto bb : bn) {
		_allbonenames.push_back(bb->name);
		_allbones.push_back(bb);
		GenMap(bb);
	}
}

void Armature::UpdateAnimIds() {
	uint i = 0;
	for (auto bn : _allbones) {
		_boneAnimIds[i] = _anim->IdOf(bn->fullName + (char)AK_BoneLoc);
		i++;
	}
}

void Armature::Animate() {
	if (!object->parent()) return;
	Animator* anm = object->parent->GetComponent<Animator>().get();
	if (!anm || !anm->animation) return;

	anm->OnPreLUpdate();
	if (anm != _anim || object->parent->dirty) {
		_anim = anm;
		UpdateAnimIds();
		object->parent->dirty = false;
	}

	uint i = 0;
	for (auto& bn : _allbones) {
		auto id = _boneAnimIds[i];
		if (id != -1) {
			Vec4 loc = anm->Get(id);
			Vec4 rot = anm->Get(id + 1);
			Vec4 scl = anm->Get(id + 2);
			bn->tr->localPosition(loc / animationScale);
			bn->tr->localRotation(*(Quat*)&rot);
			bn->tr->localScale(scl);
		}
		i++;
	}
}

void Armature::UpdateMats(ArmatureBone* b) {
	auto& bn = b ? b->_children : _bones;
	for (auto bb : bn) {
		if (bb->parent) bb->newMatrix = bb->parent->newMatrix * bb->tr->_localMatrix;
		else bb->newMatrix = bb->tr->_localMatrix;
		//bb->animMatrix = bb->newMatrix*bb->restMatrixAInv;
		_animMatrices[bb->id] = bb->newMatrix*bb->restMatrixAInv;
		UpdateMats(bb);
	}
}

void Armature::OnPreRender() {
	Animate();
	UpdateMats();
}

#ifdef IS_EDITOR

void ArmatureBone::Draw(EB_Viewer* ebv) {
	bool sel = (ebv->editor->selected == tr->object);
	MVP::Push();
	MVP::Mul(tr->localMatrix());
	MVP::Push();
	MVP::Scale(length, length, length);
	glVertexPointer(3, GL_FLOAT, 0, &boneVecs[0]);
	glLineWidth(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (sel) glColor4f(boneSelCol.r, boneSelCol.g, boneSelCol.b, 1);
	else glColor4f(boneCol.r, boneCol.g, boneCol.b, 1);
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, &boneIndices[0]);
	MVP::Pop();

	for (auto a : _children) a->Draw(ebv);
	MVP::Pop();
}

void Armature::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	glEnableClientState(GL_VERTEX_ARRAY);
	MVP::Push();
	MVP::Mul(object->transform.localMatrix());
	if (xray) {
		glDepthFunc(GL_ALWAYS);
		glDepthMask(false);
	}
	for (auto a : _bones) a->Draw(ebv);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	MVP::Pop();
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Armature::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	Armature* cam = (Armature*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Anim Scale", e->font, white());
		try {
			animationScale = std::stof(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), std::to_string(animationScale), e->font, true, nullptr, white()));
		}
		catch (std::logic_error e) {
			animationScale = 1;
		}
		animationScale = max(animationScale, 0.0001f);
		pos += 17;
	}
	else pos += 17;
}

#endif