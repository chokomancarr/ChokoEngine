#include "SceneObjects.h"
#include "Engine.h"
#include "Editor.h"

bool DrawComponentHeader(Editor* e, Vec4 v, float pos, Component* c) {
	Engine::DrawQuad(v.r, v.g + pos, v.b - 17, 16, grey2());
	//bool hi = expand;
	//if (Engine::EButton((e->editorLayer == 0), v.r, v.g + pos, v.b, 16, grey2(), white(1, 0.7f), grey1()) == MOUSE_RELEASE) {
	//	hi = !expand;
	//}
	Engine::DrawTexture(v.r, v.g + pos, 16, 16, c->_expanded ? e->collapse : e->expand);
	if (Engine::EButton(e->editorLayer == 0, v.r + v.b - 16, v.g + pos, 16, 16, e->buttonX, white(1, 0.7f)) == MOUSE_RELEASE) {
		//delete
		c->object->RemoveComponent(c);
		if (c == nullptr)
			e->WAITINGREFRESHFLAG = true;
		return false;
	}
	Engine::Label(v.r + 20, v.g + pos + 3, 12, c->name, e->font, white());
	return true;
}

Component::Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder, vector<COMPONENT_TYPE> dep) : Object(name), componentType(t), active(true), drawOrder(drawOrder), _expanded(true), dependancies(dep) {
	for (COMPONENT_TYPE t : dependancies) {
		dependacyPointers.push_back(nullptr);
	}
}

int Camera::camVertsIds[19] = { 0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 4, 4, 3, 3, 1, 1, 2, 5 };

Camera::Camera() : Component("Camera", COMP_CAM, DRAWORDER_NOT), ortographic(false), fov(60), orthoSize(10), screenPos(0.3f, 0.1f, 0.6f, 0.4f) {
	//camVerts[0] = Vec3();
	UpdateCamVerts();
}

Camera::Camera(ifstream& stream, long pos) : Component("Camera", COMP_CAM, DRAWORDER_NOT), ortographic(false), orthoSize(10) {
	if (pos >= 0)
		stream.seekg(pos);
	stream >> fov;
	stream >> screenPos.x;
	stream >> screenPos.y;
	stream >> screenPos.w;
	stream >> screenPos.h;
	//camVerts[0] = Vec3();
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

void Camera::DrawEditor(EB_Viewer* ebv) {
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

void Camera::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	Camera* cam = (Camera*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
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
	_StreamWrite(&screenPos.x, stream, 4);
	_StreamWrite(&screenPos.y, stream, 4);
	_StreamWrite(&screenPos.w, stream, 4);
	_StreamWrite(&screenPos.h, stream, 4);
}

MeshFilter::MeshFilter() : Component("Mesh Filter", COMP_MFT, DRAWORDER_NOT), _mesh(-1) {

}

void MeshFilter::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	//MeshFilter* mft = (MeshFilter*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Mesh", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_MESH, 12, e->font, &_mesh, &_UpdateMesh, this);
		pos += 34;
	}
	else pos += 17;
}

void MeshFilter::Serialize(Editor* e, ofstream* stream) {

}

void MeshFilter::SetMesh(int i) {
	_mesh = i;
	if (i >= 0) {
		mesh = (Mesh*)Editor::instance->GetCache(ASSETTYPE_MESH, i);
	}
	else
		mesh = nullptr;
}

void MeshFilter::_UpdateMesh(void* i) {
	MeshFilter* mf = (MeshFilter*)i;
	if (mf->_mesh >= 0) {
		mf->mesh = (Mesh*)Editor::instance->GetCache(ASSETTYPE_MESH, mf->_mesh);
	}
	else
		mf->mesh = nullptr;
}

MeshRenderer::MeshRenderer() : Component("Mesh Renderer", COMP_MRD, DRAWORDER_SOLID | DRAWORDER_TRANSPARENT, {COMP_MFT}) {
	_materials.push_back(-1);
}

void MeshRenderer::DrawEditor(EB_Viewer* ebv) {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0];
	if (mf == nullptr || mf->mesh == nullptr || !mf->mesh->loaded)
		return;
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &(mf->mesh->vertices[0]));
	glPolygonMode(GL_FRONT_AND_BACK, (ebv->selectedShading == 0) ? GL_FILL : GL_LINE);
	glColor4f(1, 1, 1, 1);
	glLineWidth(1);
	glDrawElements(GL_TRIANGLES, mf->mesh->triangles.size(), GL_UNSIGNED_INT, &(mf->mesh->triangles[0]));
	glDisableClientState(GL_VERTEX_ARRAY);
}

void MeshRenderer::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	//MeshRenderer* mrd = (MeshRenderer*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Material0", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_MATERIAL, 12, e->font, &_materials[0]);
		pos += 34;
	}
	else pos += 17;
}

void TextureRenderer::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	//MeshRenderer* mrd = (MeshRenderer*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Texture", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_texture);
		pos += 34;
	}
	else pos += 17;
}

void TextureRenderer::Serialize(Editor* e, ofstream* stream) {
	_StreamWrite(&_texture, stream, 4);
}

TextureRenderer::TextureRenderer(ifstream& stream, long pos) : Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY) {
	if (pos >= 0)
		stream.seekg(pos);
	stream >> _texture;
}

SceneScript::SceneScript(Editor* e, string name) : Component(name + " (Script)", COMP_SCR, DRAWORDER_NOT) {

}
void SceneScript::Serialize(Editor* e, ofstream* stream) {
	for (int a = e->headerAssets.size() - 1; a >= 0; a--) {
		if (e->headerAssets[a] == name) {
			_StreamWrite(&a, stream, 4);
		}
	}
}

void SceneScript::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	SceneScript* scr = (SceneScript*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
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

Component* ComponentFromType (COMPONENT_TYPE t){
	switch (t) {
	case COMP_CAM:
		return new Camera();
	case COMP_MFT:
		return new MeshFilter();
	}
}

Component* SceneObject::AddComponent(Component* c) {
	c->object = this;
	int i = 0;
	for (COMPONENT_TYPE t : c->dependancies) {
		c->dependacyPointers[i] = GetComponent(t);
		if (c->dependacyPointers[i] == nullptr) {
			c->dependacyPointers[i] = AddComponent(ComponentFromType(t));
		}
		i++;
	}
	for (Component* cc : _components)
	{
		if (cc->componentType == c->componentType) {
			cout << "same component already exists!" << endl;
			delete(c);
			return cc;
		}
	}
	_components.push_back(c);
	return c;
}

Component* SceneObject::GetComponent(COMPONENT_TYPE type) {
	for (Component* cc : _components)
	{
		if (cc->componentType == type) {
			return cc;
		}
	}
	return nullptr;
}

void SceneObject::RemoveComponent(Component*& c) {
	for (int a = _components.size()-1; a >= 0; a--) {
		if (_components[a] == c) {
			for (int aa = _components.size()-1; aa >= 0; aa--) {
				for (COMPONENT_TYPE t : _components[aa]->dependancies) {
					if (t == c->componentType) {
						Editor::instance->_Warning("Component Deleter", "Cannot delete " + c->name + " because other components depend on it!");
						return;
					}
				}
			}
			_components.erase(_components.begin() + a);
			c = nullptr;
			return;
		}
	}
	Debug::Warning("SceneObject", "component to delete is not found");
}