#include "SceneObjects.h"
#include "Engine.h"
#include "Editor.h"

bool DrawComponentHeader(Editor* e, string name, Color v, float pos, bool expand, byte texId) {
	Engine::DrawQuad(v.r, v.g + pos, v.b - 17, 16, grey2());
	//bool hi = expand;
	//if (Engine::EButton((e->editorLayer == 0), v.r, v.g + pos, v.b, 16, grey2(), white(1, 0.7f), grey1()) == MOUSE_RELEASE) {
	//	hi = !expand;
	//}
	Engine::DrawTexture(v.r, v.g + pos, 16, 16, expand ? e->collapse : e->expand);
	if (Engine::EButton(e->editorLayer == 0, v.r + v.b - 16, v.g + pos, 16, 16, e->buttonX, white(1, 0.7f))) {
		//delete
		
	}
	Engine::Label(v.r + 20, v.g + pos + 3, 12, name, e->font, white());
	return true;
}

MeshFilter::MeshFilter() : Component(COMP_MFT, false) {

}

void MeshFilter::DrawInspector(Editor* e, Component* c, Color v, uint& pos) {
	//MeshFilter* mft = (MeshFilter*)c;
	if (DrawComponentHeader(e, "Mesh Filter", v, pos, c->_expanded, COMP_MFT)) {
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Mesh", e->font, white());
		pos += 34;
	}
	else pos += 17;
}

MeshRenderer::MeshRenderer() : Component(COMP_MRD, true) {

}

void MeshRenderer::DrawInspector(Editor* e, Component* c, Color v, uint& pos) {
	//MeshRenderer* mrd = (MeshRenderer*)c;
	if (DrawComponentHeader(e, "Mesh Renderer", v, pos, c->_expanded, COMP_MRD)) {
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Materials", e->font, white());
		pos += 34;
	}
	else pos += 17;
}

int Camera::camVertsIds[19] = { 0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 4, 4, 3, 3, 1, 1, 2, 5 };

Camera::Camera() : Component(COMP_CAM, true), ortographic(false), fov(60), orthoSize(10), screenPos(0.3f, 0.1f, 0.6f, 0.4f) {
	camVerts[0] = Vec3();
	UpdateCamVerts();
}

void Camera::UpdateCamVerts() {
	Vec3 cst = Vec3(cos(fov*0.5f * 3.14159265f / 180), sin(fov*0.5f * 3.14159265f / 180), tan(fov*0.5f * 3.14159265f / 180))*cos(fov*0.618f * 3.14159265f / 180);
	camVerts[1] = Vec3(cst.x, cst.y, 1 - cst.z) * 2.0f;
	camVerts[2] = Vec3(-cst.x, cst.y, 1 - cst.z) * 2.0f;
	camVerts[3] = Vec3(cst.x, -cst.y, 1 - cst.z) * 2.0f;
	camVerts[4] = Vec3(-cst.x, -cst.y, 1 - cst.z) * 2.0f;
	camVerts[5] = Vec3(0, cst.y * 1.5f, 1 - cst.z) * 2.0f;
}

void Camera::DrawEditor() {
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &camVerts[0]);
	glLineWidth(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor4f(0, 0, 0, 1);
	glDrawElements(GL_LINES, 16, GL_UNSIGNED_INT, &camVertsIds[0]);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &camVertsIds[16]);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Camera::DrawInspector(Editor* e, Component* c, Color v, uint& pos) {
	Camera* cam = (Camera*)c;
	if (DrawComponentHeader(e, "Camera", v, pos, c->_expanded, COMP_CAM)) {
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Field of view", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1());
		Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 20, 12, to_string(cam->fov), e->font, white());
		Engine::Label(v.r + 2, v.g + pos + 35, 12, "Frustrum", e->font, white());
		Engine::Label(v.r + 4, v.g + pos + 50, 12, "X", e->font, white());
		Engine::DrawQuad(v.r + 20, v.g + pos + 47, v.b*0.3f - 20, 16, grey1());
		Engine::Label(v.r + v.b*0.3f + 4, v.g + pos + 50, 12, "Y", e->font, white());
		Engine::DrawQuad(v.r + v.b*0.3f + 20, v.g + pos + 47, v.b*0.3f - 20, 16, grey1());
		Engine::Label(v.r + 4, v.g + pos + 67, 12, "W", e->font, white());
		Engine::DrawQuad(v.r + 20, v.g + pos + 64, v.b*0.3f - 20, 16, grey1());
		Engine::Label(v.r + v.b*0.3f + 4, v.g + pos + 67, 12, "H", e->font, white());
		Engine::DrawQuad(v.r + v.b*0.3f + 20, v.g + pos + 64, v.b*0.3f - 20, 16, grey1());
		float dh = ((v.b*0.35f - 1)*Display::height / Display::width) - 1;
		Engine::DrawQuad(v.r + v.b*0.65f, v.g + pos + 35, v.b*0.35f - 1, dh, grey1());
		Engine::DrawQuad(v.r + v.b*0.65f + ((v.b*0.35f - 1)*screenPos.x), v.g + pos + 35 + dh*screenPos.y, (v.b*0.35f - 1)*screenPos.w, dh*screenPos.h, grey2());
		pos += max(37 + dh, 87);
	}
	else pos += 17;
}

void Camera::Serialize(Editor* e, ofstream* stream) {
	_StreamWrite(&fov, stream, 4);
}

SceneScript::SceneScript(Editor* e, string name) : name(name), Component(COMP_SCR, false) {

}
void SceneScript::Serialize(Editor* e, ofstream* stream) {
	for (int a = e->headerAssets.size() - 1; a >= 0; a--) {
		if (e->headerAssets[a] == name) {
			_StreamWrite(&a, stream, 4);
		}
	}
}

void SceneScript::DrawInspector(Editor* e, Component* c, Color v, uint& pos) {
	SceneScript* scr = (SceneScript*)c;
	if (DrawComponentHeader(e, scr->name + "(Script)", v, pos, c->_expanded, COMP_SCR)) {
		pos += 100;
	}
	else pos += 17;
}

SceneObject::SceneObject() : SceneObject(Vec3(), Quat(), Vec3(1, 1, 1)) {}
SceneObject::SceneObject(string s) : SceneObject(s, Vec3(), Quat(), Vec3(1, 1, 1)) {}
SceneObject::SceneObject(Vec3 pos, Quat rot, Vec3 scale) : active(true), transform(this, pos, rot, scale), childCount(0), _expanded(true), name("New Object") {
	id = Engine::GetNewId();
}
SceneObject::SceneObject(string s, Vec3 pos, Quat rot, Vec3 scale) : active(true), transform(this, pos, rot, scale), childCount(0), _expanded(true), name(s) {
	id = Engine::GetNewId();
}

void SceneObject::Enable() {
	active = true;
}

void SceneObject::Enable(bool enableAll) {
	active = false;
}

Component* SceneObject::AddComponent(Component* c) {
	for each (Component* cc in _components)
	{
		if (cc->componentType == c->componentType) {
			cout << "same component already exists!" << endl;
			return nullptr;
		}
	}
	_components.push_back(c);
	c->object = this;
	return c;
}

Component* SceneObject::GetComponent(COMPONENT_TYPE type) {
	return nullptr;
}

void SceneObject::RemoveComponent(Component* c) {
	for (int a = _components.size(); a >= 0; a--) {
		if (_components[a] == c) {
			_components.erase(_components.begin() + a);
			return;
		}
	}
	Debug::Warning("SceneObject", "component to delete is not found");
}