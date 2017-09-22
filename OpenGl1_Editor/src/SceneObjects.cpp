//#include "SceneObjects.h"
#include "Engine.h"
#include "Editor.h"
#include <sstream>

using namespace ChokoEngine;

Object::Object(string nm) : id(Engine::GetNewId()), name(nm) {}

bool DrawComponentHeader(Editor* e, Vec4 v, uint pos, Component* c) {
	Engine::DrawQuad(v.r, v.g + pos, v.b - 17, 16, grey2());
	//bool hi = expand;
	if (Engine::EButton((e->editorLayer == 0), v.r, v.g + pos, 16, 16, c->_expanded ? e->collapse : e->expand, white()) == MOUSE_RELEASE) {
		c->_expanded = !c->_expanded;//hi = !expand;
	}
	//Engine::DrawTexture(v.r, v.g + pos, 16, 16, c->_expanded ? e->collapse : e->expand);
	if (Engine::EButton(e->editorLayer == 0, v.r + v.b - 16, v.g + pos, 16, 16, e->buttonX, white(1, 0.7f)) == MOUSE_RELEASE) {
		//delete
		c->object->RemoveComponent(c);
		if (c == nullptr)
			e->flags |= WAITINGREFRESHFLAG;
		return false;
	}
	Engine::Label(v.r + 20, v.g + pos, 12, c->name, e->font, white());
	return c->_expanded;
}

Component::Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder, SceneObject* o, std::vector<COMPONENT_TYPE> dep) : Object(name), componentType(t), active(true), drawOrder(drawOrder), _expanded(true), dependancies(dep), object(o) {
	for (COMPONENT_TYPE t : dependancies) {
		dependacyPointers.push_back(nullptr);
	}
}

COMPONENT_TYPE Component::Name2Type(string nm) {
	if (nm == "Camera")
		return COMP_CAM;
	if (nm == "MeshFilter")
		return COMP_MFT;
	if (nm == "MeshRenderer")
		return COMP_MRD;
	if (nm == "TextureRenderer")
		return COMP_TRD;
	return COMP_UNDEF;
}

int Camera::camVertsIds[19] = { 0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 4, 4, 3, 3, 1, 1, 2, 5 };

Camera::Camera() : Component("Camera", COMP_CAM, DRAWORDER_NONE), ortographic(false), fov(60), orthoSize(10), screenPos(0.3f, 0.1f, 0.6f, 0.4f), clearType(CAM_CLEAR_COLOR), clearColor(black(1)), _tarRT(-1), nearClip(0.01f), farClip(500) {
	UpdateCamVerts();
#ifndef IS_EDITOR
	InitGBuffer();
#endif
}

Camera::Camera(std::ifstream& stream, SceneObject* o, long pos) : Camera() {
	if (pos >= 0)
		stream.seekg(pos);
	_Strm2Val(stream, fov);
	_Strm2Val(stream, screenPos.x);
	_Strm2Val(stream, screenPos.y);
	_Strm2Val(stream, screenPos.w);
	_Strm2Val(stream, screenPos.h);

/*
#ifndef IS_EDITOR
	if (d_skyProgram == 0) {
		InitShaders();
	}
#endif
*/
}

void Camera::ApplyGL() {
	switch (clearType) {
	case CAM_CLEAR_COLOR:
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);
		break;
	case CAM_CLEAR_DEPTH:
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);
		break;
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Quat q = glm::inverse(object->transform.rotation());
	glMultMatrixf(glm::value_ptr(glm::perspectiveFov(fov * deg2rad, (float)Display::width, (float)Display::height, 0.01f, 500.0f)));
	glScalef(1, 1, -1);
	glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(q)));
	Vec3 pos = -object->transform.worldPosition();
	glTranslatef(pos.x, pos.y, pos.z);
}

void GenShaderFromPath(GLuint vertex_shader, const string& path, GLuint* program) {
	GLuint fragment_shader;
	std::string err;
	std::ifstream strm(path);
	if (strm.fail()) {
		Debug::Error("Cam Shader Compiler", "fs frag not found!");
		abort();
	}
	std::stringstream ss;
	ss << strm.rdbuf();
	if (!ShaderBase::LoadShader(GL_FRAGMENT_SHADER, ss.str(), fragment_shader, &err)) {
		Debug::Error("Cam Shader Compiler", path + "! " + err);
		abort();
	}

	*program = glCreateProgram();
	glAttachShader(*program, vertex_shader);
	glAttachShader(*program, fragment_shader);
	glLinkProgram(*program);
	GLint link_result;
	glGetProgramiv(*program, GL_LINK_STATUS, &link_result);
	if (link_result == GL_FALSE)
	{
		int info_log_length = 0;
		glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector<char> program_log(info_log_length);
		glGetProgramInfoLog(*program, info_log_length, NULL, &program_log[0]);
		std::cout << "cam shader error" << std::endl << &program_log[0] << std::endl;
		glDeleteProgram(*program);
		*program = 0;
		abort();
	}
	glDetachShader(*program, vertex_shader);
	glDetachShader(*program, fragment_shader);
	glDeleteShader(fragment_shader);
}

void Camera::InitShaders() {
	int link_result = 0;
	GLuint vertex_shader;
	string err = "";
	std::ifstream strm("D:\\lightPassVert.txt");
	std::stringstream ss;
	ss << strm.rdbuf();

	if (!ShaderBase::LoadShader(GL_VERTEX_SHADER, ss.str(), vertex_shader, &err)) {
		Debug::Error("Cam Shader Compiler", "v! " + err);
		abort();
	}

	GenShaderFromPath(vertex_shader, "D:\\blurPassFrag.txt", &d_blurProgram);
	GenShaderFromPath(vertex_shader, "D:\\lightPassFrag_Sky.txt", &d_skyProgram);
	GenShaderFromPath(vertex_shader, "D:\\lightPassFrag_Point.txt", &d_pLightProgram);
	GenShaderFromPath(vertex_shader, "D:\\lightPassFrag_Spot.txt", &d_sLightProgram);
	GenShaderFromPath(vertex_shader, "D:\\lightPassFrag_Spot_ContShad.txt", &d_sLightCSProgram);
	GenShaderFromPath(vertex_shader, "D:\\lightPassFrag_Spot_GI_RSM.txt", &d_sLightRSMProgram);
	GenShaderFromPath(vertex_shader, "D:\\lightPassFrag_Spot_GI_FluxPrep.txt", &d_sLightRSMFluxProgram);
	GenShaderFromPath(vertex_shader, "D:\\lightPassFrag_ProbeMask.txt", &d_probeMaskProgram);

	glDeleteShader(vertex_shader);

	Light::ScanParams();
}

void Camera::UpdateCamVerts() {
#ifdef IS_EDITOR
	Vec3 cst = Vec3(cos(fov*0.5f * 3.14159265f / 180), sin(fov*0.5f * 3.14159265f / 180), tan(fov*0.5f * 3.14159265f / 180))*cos(fov*0.618f * 3.14159265f / 180);
	camVerts[1] = Vec3(cst.x, cst.y, 1 - cst.z) * 2.0f;
	camVerts[2] = Vec3(-cst.x, cst.y, 1 - cst.z) * 2.0f;
	camVerts[3] = Vec3(cst.x, -cst.y, 1 - cst.z) * 2.0f;
	camVerts[4] = Vec3(-cst.x, -cst.y, 1 - cst.z) * 2.0f;
	camVerts[5] = Vec3(0, cst.y * 1.5f, 1 - cst.z) * 2.0f;
#endif
}

void Camera::DrawEditor(EB_Viewer* ebv, GLuint shader) {
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
		Engine::Label(v.r + 2, v.g + pos + 17, 12, "Field of view", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, grey1());
		Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 17, 12, to_string(cam->fov), e->font, white());
		cam->fov = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f-1, 16, 0.1f, 179.9f, cam->fov, grey1(), white());
		Engine::Label(v.r + 2, v.g + pos + 35, 12, "Frustrum", e->font, white());
		Engine::Label(v.r + 4, v.g + pos + 47, 12, "X", e->font, white());
		Engine::DrawQuad(v.r + 20, v.g + pos + 47, v.b*0.3f - 20, 16, grey1());
		Engine::Label(v.r + v.b*0.3f + 4, v.g + pos + 47, 12, "Y", e->font, white());
		Engine::DrawQuad(v.r + v.b*0.3f + 20, v.g + pos + 47, v.b*0.3f - 20, 16, grey1());
		Engine::Label(v.r + 4, v.g + pos + 64, 12, "W", e->font, white());
		Engine::DrawQuad(v.r + 20, v.g + pos + 64, v.b*0.3f - 20, 16, grey1());
		Engine::Label(v.r + v.b*0.3f + 4, v.g + pos + 64, 12, "H", e->font, white());
		Engine::DrawQuad(v.r + v.b*0.3f + 20, v.g + pos + 64, v.b*0.3f - 20, 16, grey1());
		float dh = ((v.b*0.35f - 1)*Display::height / Display::width) - 1;
		Engine::DrawQuad(v.r + v.b*0.65f, v.g + pos + 35, v.b*0.35f - 1, dh, grey1());
		Engine::DrawQuad(v.r + v.b*0.65f + ((v.b*0.35f - 1)*screenPos.x), v.g + pos + 35 + dh*screenPos.y, (v.b*0.35f - 1)*screenPos.w, dh*screenPos.h, grey2());
		pos += (uint)max(37 + dh, 87);
		Engine::Label(v.r + 2, v.g + pos + 1, 12, "Filtering", e->font, white());
		std::vector<string> clearNames = { "None", "Color and Depth", "Depth only", "Skybox" };
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.3f, v.g + pos + 1, v.b * 0.7f - 1, 14, grey2(), clearNames[clearType], 12, e->font, white()) == MOUSE_PRESS) {
			e->RegisterMenu(nullptr, "", clearNames, { &_SetClear0, &_SetClear1, &_SetClear2, &_SetClear3 }, 0, Vec2(v.r + v.b * 0.3f, v.g + pos));
		}
		Engine::Label(v.r + 2, v.g + pos + 17, 12, "Target", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE_REND, 12, e->font, &_tarRT, nullptr, this);
		pos += 34;
		uint ess = effects.size();
		Engine::Label(v.r + 2, v.g + pos, 12, "Effects: " + to_string(ess), e->font, white());
		pos += 17;
		Engine::DrawQuad(v.r + 2, v.g + pos, v.b - 4, 17 * ess + 2.0f, grey1()*0.5f);
		int pendingDel = -1;
		for (uint es = 0; es < ess; es++) {
			if (Engine::EButton(e->editorLayer == 0, v.r + 4, v.g + pos + 1, 16, 16, e->buttonX, white()) == MOUSE_RELEASE) {
				pendingDel = es;
			}
			e->DrawAssetSelector(v.r + 21, v.g + pos + 1, v.b - 24, 16, grey1(), ASSETTYPE_CAMEFFECT, 12, e->font, &_effects[es], nullptr, this);
			pos += 17;
		}
		pos += 3;
		if (Engine::EButton(e->editorLayer == 0, v.r + 2, v.g + pos, 14, 14, e->buttonPlus, white()) == MOUSE_RELEASE) {
			effects.push_back(nullptr);
			_effects.push_back(-1);
		}
		if (pendingDel > -1) {
			effects.erase(effects.begin() + pendingDel);
			_effects.erase(_effects.begin() + pendingDel);
		}
	}
	else pos += 17;
}

void Camera::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWrite(&fov, stream, 4);
	_StreamWrite(&screenPos.x, stream, 4);
	_StreamWrite(&screenPos.y, stream, 4);
	_StreamWrite(&screenPos.w, stream, 4);
	_StreamWrite(&screenPos.h, stream, 4);
}

void Camera::_SetClear0(EditorBlock* b) {
	Editor::instance->selected->GetComponent<Camera>()->clearType = CAM_CLEAR_NONE;
}
void Camera::_SetClear1(EditorBlock* b) {
	Editor::instance->selected->GetComponent<Camera>()->clearType = CAM_CLEAR_COLOR;
}
void Camera::_SetClear2(EditorBlock* b) {
	Editor::instance->selected->GetComponent<Camera>()->clearType = CAM_CLEAR_DEPTH;
}
void Camera::_SetClear3(EditorBlock* b) {
	Editor::instance->selected->GetComponent<Camera>()->clearType = CAM_CLEAR_SKY;
}

MeshFilter::MeshFilter() : Component("Mesh Filter", COMP_MFT, DRAWORDER_NONE), _mesh(-1) {

}

void MeshFilter::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	//MeshFilter* mft = (MeshFilter*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Mesh", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_MESH, 12, e->font, &_mesh, &_UpdateMesh, this);
		pos += 34;
		Engine::Label(v.r + 2, v.g + pos, 12, "Show Bounding Box", e->font, white());
		showBoundingBox = Engine::DrawToggle(v.r + v.b*0.3f, v.g + pos, 12, e->checkbox, showBoundingBox, white(), ORIENT_HORIZONTAL);
		pos += 17;
	}
	else pos += 17;
}

void MeshFilter::SetMesh(int i) {
	_mesh = i;
	if (i >= 0) {
		mesh = _GetCache<Mesh>(ASSETTYPE_MESH, i);
	}
	else
		mesh = nullptr;
	object->Refresh();
}

void MeshFilter::_UpdateMesh(void* i) {
	MeshFilter* mf = (MeshFilter*)i;
	if (mf->_mesh >= 0) {
		mf->mesh = _GetCache<Mesh>(ASSETTYPE_MESH, mf->_mesh);
	}
	else
		mf->mesh = nullptr;
	mf->object->Refresh();
}

void MeshFilter::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_MESH, _mesh);
}

MeshFilter::MeshFilter(std::ifstream& stream, SceneObject* o, long pos) : Component("Mesh Filter", COMP_MFT, DRAWORDER_NONE, o), _mesh(-1) {
	if (pos >= 0)
		stream.seekg(pos);
	ASSETTYPE t;
	_Strm2Asset(stream, Editor::instance, t, _mesh);
	if (_mesh >= 0)
		mesh = _GetCache<Mesh>(ASSETTYPE_MESH, _mesh);
	object->Refresh();
}

MeshRenderer::MeshRenderer() : Component("Mesh Renderer", COMP_MRD, DRAWORDER_SOLID | DRAWORDER_TRANSPARENT, nullptr, {COMP_MFT}) {
	_materials.push_back(-1);
}

MeshRenderer::MeshRenderer(std::ifstream& stream, SceneObject* o, long pos) : Component("Mesh Renderer", COMP_MRD, DRAWORDER_SOLID | DRAWORDER_TRANSPARENT, o, { COMP_MFT }) {
	_UpdateMat(this);
	if (pos >= 0)
		stream.seekg(pos);
	ASSETTYPE t;
	int s;
	_Strm2Val(stream, s);
	for (int q = 0; q < s; q++) {
		Material* m;
		ASSETID i;
		_Strm2Asset(stream, Editor::instance, t, i);
		m = _GetCache<Material>(ASSETTYPE_MATERIAL, i);
		materials.push_back(m);
		_materials.push_back(i);
	}
	//_Strm2Asset(stream, Editor::instance, t, _mat);
}

void MeshRenderer::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0];
	if (mf->mesh == nullptr || !mf->mesh->loaded)
		return;
	bool isE = (ebv != nullptr);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, (!isE || ebv->selectedShading == 0) ? GL_FILL : GL_LINE);
	if (!isE || ebv->selectedShading == 0) glEnable(GL_CULL_FACE);
	glVertexPointer(3, GL_FLOAT, 0, &(mf->mesh->vertices[0]));
	glLineWidth(1);
	GLfloat matrix[16], matrix2[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	glGetFloatv(GL_PROJECTION_MATRIX, matrix2);
	Mat4x4 m1(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]);
	Mat4x4 m2(matrix2[0], matrix2[1], matrix2[2], matrix2[3], matrix2[4], matrix2[5], matrix2[6], matrix2[7], matrix2[8], matrix2[9], matrix2[10], matrix2[11], matrix2[12], matrix2[13], matrix2[14], matrix2[15]);
	for (uint m = 0; m < mf->mesh->materialCount; m++) {
		if (materials[m] == nullptr)
			continue;
		if (shader == 0) materials[m]->ApplyGL(m1, m2);
		else glUseProgram(shader);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->vertices[0]));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->uv0[0]));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->normals[0]));
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->tangents[0]));
		glDrawElements(GL_TRIANGLES, mf->mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &(mf->mesh->_matTriangles[m][0]));
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
	}
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_CULL_FACE);
	if (isE && mf->showBoundingBox) {
		BBox& b = mf->mesh->boundingBox;
		Engine::DrawCubeLinesW(b.x0, b.x1, b.y0, b.y1, b.z0, b.z1, 1, white(1, 0.5f));
	}
}

void MeshRenderer::DrawDeferred(GLuint shader) {
	DrawEditor(nullptr, shader);
}

void MeshRenderer::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		MeshFilter* mft = (MeshFilter*)dependacyPointers[0];
		if (mft->mesh == nullptr) {
			Engine::Label(v.r + 2, v.g + pos + 20, 12, "No Mesh Assigned!", e->font, white());
			pos += 34;
		}
		else {
			Engine::Label(v.r + 2, v.g + pos + 18, 12, "Materials: " + to_string(mft->mesh->materialCount), e->font, white());
			pos += 35;
			for (uint a = 0; a < mft->mesh->materialCount; a++) {
				Engine::Label(v.r + 2, v.g + pos , 12, "Material " + to_string(a), e->font, white());
				e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_MATERIAL, 12, e->font, &_materials[a], & _UpdateMat, this);
				pos += 17;
				if (materials[a] == nullptr)
					continue;
				if (Engine::EButton(e->editorLayer == 0, v.r, v.g + pos, 16, 16, e->expand, white()) == MOUSE_RELEASE) {
					materials[a]->_maskExpanded = !materials[a]->_maskExpanded;
				}
				Engine::Label(v.r + 17, v.g + pos, 12, "Write Mask", e->font, white());
				pos += 17;
				if (materials[a]->_maskExpanded) {
					for (uint ea = 0; ea < GBUFFER_NUM_TEXTURES - 1; ea++) {
						materials[a]->writeMask[ea] = Engine::DrawToggle(v.r + 17, v.g + pos, 16, e->checkbox, materials[a]->writeMask[ea], white(), ORIENT_HORIZONTAL);
						Engine::Label(v.r + 38, v.g + pos, 12, Camera::_gbufferNames[ea], e->font, white());
						pos += 17;
					}
				}
				for (uint q = 0, qq = materials[a]->valOrders.size(); q < qq; q++) {
					Engine::Label(v.r + 20, v.g + pos, 12, materials[a]->valNames[materials[a]->valOrders[q]][materials[a]->valOrderIds[q]], e->font, white());
					Engine::DrawTexture(v.r + 3, v.g + pos, 16, 16, e->matVarTexs[materials[a]->valOrders[q]]);
					void* bbs = materials[a]->vals[materials[a]->valOrders[q]][materials[a]->valOrderGLIds[q]];
					switch (materials[a]->valOrders[q]) {
					case SHADER_INT:
						Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.3f + 17, v.g + pos , v.b*0.7f - 17, 16, grey1(), LerpVec4(grey1(), white(), 0.1f), LerpVec4(grey1(), black(), 0.5f), to_string(*(int*)bbs), 12, e->font, white());
						*(int*)bbs = (int)round(Engine::DrawSliderFill(v.r + v.b*0.6f + 18, v.g + pos, v.b*0.4f - 19, 16, 0, 1, (float)(*(int*)bbs), grey2(), white()));
						break;
					case SHADER_FLOAT:
						Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.3f + 17, v.g + pos , v.b*0.6f - 17, 16, grey1(), LerpVec4(grey1(), white(), 0.1f), LerpVec4(grey1(), black(), 0.5f), to_string(*(float*)bbs), 12, e->font, white());
						*(float*)bbs = Engine::DrawSliderFill(v.r + v.b*0.6f + 18, v.g + pos, v.b*0.4f - 19, 16, 0, 1, *(float*)bbs, grey2(), white());
						break;
					case SHADER_SAMPLER:
						e->DrawAssetSelector(v.r + v.b * 0.3f + 17, v.g + pos, v.b*0.7f - 17, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &((MatVal_Tex*)bbs)->id, _UpdateTex, bbs);
						break;
					}
					pos += 17;
				}
				pos++;
			}
		}
	}
	else pos += 17;
}

void MeshRenderer::_UpdateMat(void* i) {
	MeshRenderer* mf = (MeshRenderer*)i;
	for (int q = mf->_materials.size() - 1; q >= 0; q--) {
		mf->materials[q] = _GetCache<Material>(ASSETTYPE_MATERIAL, mf->_materials[q]);
	}
}

void MeshRenderer::_UpdateTex(void* i) {
	MatVal_Tex* v = (MatVal_Tex*)i;
	v->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, v->id);
}

void MeshRenderer::Serialize(Editor* e, std::ofstream* stream) {
	int s = _materials.size();
	_StreamWrite(&s, stream, 4);
	for (ASSETID i : _materials)
		_StreamWriteAsset(e, stream, ASSETTYPE_MATERIAL, i);
}

void MeshRenderer::Refresh() {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0];
	if (mf == nullptr || mf->mesh == nullptr || !mf->mesh->loaded) {
		materials.clear();
		_materials.clear();
	}
	else {
		materials.resize(mf->mesh->materialCount, nullptr);
		_materials.resize(mf->mesh->materialCount, -1);
	}
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

void TextureRenderer::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_TEXTURE, _texture);
}

TextureRenderer::TextureRenderer(std::ifstream& stream, SceneObject* o, long pos) : Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY, o) {
	if (pos >= 0)
		stream.seekg(pos);
	ASSETTYPE t;
	_Strm2Asset(stream, Editor::instance, t, _texture);
}

//-----------------Skinned Mesh Renderer-------------
SkinnedMeshRenderer::SkinnedMeshRenderer(std::ifstream& stream, SceneObject* o, long pos) : Component("Skinned Mesh Renderer", COMP_SRD, DRAWORDER_SOLID | DRAWORDER_TRANSPARENT, o) {
	
}

void SkinnedMeshRenderer::ApplyAnim() {

}

void Light::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	if (ebv->editor->selected != object) return;
	switch (_lightType) { 
	case LIGHTTYPE_POINT:
		if (minDist > 0) {
			Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 1, 0), minDist, 32, color, 1);
			Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 0, 1), minDist, 32, color, 1);
			Engine::DrawCircleW(Vec3(), Vec3(0, 1, 0), Vec3(0, 0, 1), minDist, 32, color, 1);
		}
		Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 1, 0), maxDist, 32, Lerp(color, black(), 0.5f), 1);
		Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 0, 1), maxDist, 32, Lerp(color, black(), 0.5f), 1);
		Engine::DrawCircleW(Vec3(), Vec3(0, 1, 0), Vec3(0, 0, 1), maxDist, 32, Lerp(color, black(), 0.5f), 1);
		break;
	case LIGHTTYPE_DIRECTIONAL:
		Engine::DrawLineW(Vec3(0, 0, 0.2f), Vec3(0, 0, 1), color, 1);
		Engine::DrawLineW(Vec3(0, 0, 1), Vec3(0, 0.2f, 0.7f), color, 1);
		Engine::DrawLineW(Vec3(0, 0, 1), Vec3(0, -0.2f, 0.7f), color, 1);
		Engine::DrawLineW(Vec3(0, 0, 1), Vec3(0.2f, 0, 0.7f), color, 1);
		Engine::DrawLineW(Vec3(0, 0, 1), Vec3(-0.2f, 0, 0.7f), color, 1);
		/*
		Engine::DrawLineW(Vec3(0, 0, -0.3f), Vec3(0, 0, -0.5f), color, 1);
		Engine::DrawLineW(Vec3(0, -0.3f, 0), Vec3(0, -0.5f, 0), color, 1);
		Engine::DrawLineW(Vec3(0, 0.3f, 0), Vec3(0, 0.5f, 0), color, 1);
		Engine::DrawLineW(Vec3(-0.3f, 0, 0), Vec3(-0.5f, 0, 0), color, 1);
		Engine::DrawLineW(Vec3(0.3f, 0, 0), Vec3(0.5f, 0, 0), color, 1);
		*/
		Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 1, 0), 0.2f, 12, color, 1);
		Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 0, 1), 0.2f, 12, color, 1);
		Engine::DrawCircleW(Vec3(), Vec3(0, 1, 0), Vec3(0, 0, 1), 0.2f, 12, color, 1);
		break;
	case LIGHTTYPE_SPOT:
		if (ebv->editor->selected == object) {
			Engine::DrawLineWDotted(Vec3(0, 0, 1) * minDist, Vec3(0, 0, 1) * maxDist, white(0.5f, 0.5f), 1, 0.2f, true);
			float rd = minDist*tan(deg2rad*angle*0.5f);
			float rd2 = maxDist*tan(deg2rad*angle*0.5f);
			if (minDist > 0)
				Engine::DrawCircleW(Vec3(0, 0, 1) * minDist, Vec3(1, 0, 0), Vec3(0, 1, 0), rd, 16, color, 1);
			Engine::DrawCircleW(Vec3(0, 0, 1) * maxDist, Vec3(1, 0, 0), Vec3(0, 1, 0), rd2, 32, color, 1);
			Engine::DrawLineW(Vec3(rd, 0, minDist), Vec3(rd2, 0, maxDist), color, 1);
			Engine::DrawLineW(Vec3(-rd, 0, minDist), Vec3(-rd2, 0, maxDist), color, 1);
			Engine::DrawLineW(Vec3(0, rd, minDist), Vec3(0, rd2, maxDist), color, 1);
			Engine::DrawLineW(Vec3(0, -rd, minDist), Vec3(0, -rd2, maxDist), color, 1);
		}
		break;
	}
}

void Light::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		if (Engine::EButton(e->editorLayer == 0, v.r, v.g + pos, v.b * 0.33f - 1, 16, (_lightType == LIGHTTYPE_POINT) ? white(1, 0.5f) : grey1(), "Point", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_POINT;
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.33f, v.g + pos, v.b * 0.34f - 1, 16, (_lightType == LIGHTTYPE_DIRECTIONAL) ? white(1, 0.5f) : grey1(), "Directional", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_DIRECTIONAL;
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.67f, v.g + pos, v.b * 0.33f - 1, 16, (_lightType == LIGHTTYPE_SPOT) ? white(1, 0.5f) : grey1(), "Spot", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_SPOT;

		Engine::Label(v.r + 2, v.g + pos + 17, 12, "Intensity", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, grey1());
		Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 17, 12, to_string(intensity), e->font, white());
		intensity = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f - 1, 16, 0, 20, intensity, grey1(), white());
		pos += 34;
		Engine::Label(v.r + 2, v.g + pos, 12, "Color", e->font, white());
		e->DrawColorSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), 12, e->font, &color);
		pos += 17;

		switch (_lightType) {
		case LIGHTTYPE_POINT:
			Engine::Label(v.r + 2, v.g + pos, 12, "core radius", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, to_string(minDist), e->font, white());
			minDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, maxDist, minDist, grey1(), white());
			pos += 17;
			Engine::Label(v.r + 2, v.g + pos, 12, "distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, to_string(maxDist), e->font, white());
			maxDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 20, maxDist, grey1(), white());
			pos += 17;
			Engine::Label(v.r + 2, v.g + pos, 12, "falloff", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, to_string(falloff), e->font, white());
			falloff = (LIGHT_FALLOFF)((int)round(Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 20, (float)falloff, grey1(), white())));
			pos += 17;
			break;
		case LIGHTTYPE_DIRECTIONAL:
			
			break;
		case LIGHTTYPE_SPOT:
			Engine::Label(v.r + 2, v.g + pos, 12, "angle", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, to_string(angle), e->font, white());
			angle = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 180, angle, grey1(), white());
			pos += 17;
			Engine::Label(v.r + 2, v.g + pos, 12, "start distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, to_string(minDist), e->font, white());
			minDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0.0001f, maxDist, minDist, grey1(), white());
			pos += 17;
			Engine::Label(v.r + 2, v.g + pos, 12, "end distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, to_string(maxDist), e->font, white());
			maxDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0.0002f, 50, maxDist, grey1(), white());
			pos += 17;
			break;
		}
		Engine::Label(v.r + 2, v.g + pos, 12, "cookie", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_cookie, &_SetCookie, this);
		pos += 17;
		Engine::Label(v.r + 2, v.g + pos, 12, "cookie strength", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
		Engine::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, to_string(cookieStrength), e->font, white());
		cookieStrength = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, cookieStrength, grey1(), white());
		pos += 17;
		Engine::Label(v.r + 2, v.g + pos, 12, "square shape", e->font, white());
		square = Engine::DrawToggle(v.r + v.b * 0.3f, v.g + pos, 12, e->checkbox, square, white(), ORIENT_HORIZONTAL);
		pos += 17;
		if (_lightType != LIGHTTYPE_DIRECTIONAL) {
			if (Engine::EButton(e->editorLayer == 0, v.r, v.g + pos, v.b * 0.5f - 1, 16, (!drawShadow) ? white(1, 0.5f) : grey1(), "No Shadow", 12, e->font, white()) == MOUSE_RELEASE)
				drawShadow = false;
			if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.5f, v.g + pos, v.b * 0.5f - 1, 16, (drawShadow) ? white(1, 0.5f) : grey1(), "Shadow", 12, e->font, white()) == MOUSE_RELEASE)
				drawShadow = true;
			pos += 17;
			if (drawShadow) {
				Engine::Label(v.r + 2, v.g + pos, 12, "shadow bias", e->font, white());
				Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
				Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, to_string(shadowBias), e->font, white());
				shadowBias = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 0.02f, shadowBias, grey1(), white());
				pos += 17;
				Engine::Label(v.r + 2, v.g + pos, 12, "shadow strength", e->font, white());
				Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
				Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, to_string(shadowStrength), e->font, white());
				shadowStrength = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, shadowStrength, grey1(), white());
				pos += 17;

				Engine::Label(v.r + 2, v.g + pos, 12, "Contact Shadows", e->font, white());
				contactShadows = Engine::DrawToggle(v.r + v.b * 0.3f, v.g + pos, 16, e->checkbox, contactShadows, white(), ORIENT_HORIZONTAL);
				pos += 17;
				if (contactShadows) {
					Engine::Label(v.r + 2, v.g + pos, 12, "  samples", e->font, white());
					Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
					Engine::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, to_string(contactShadowSamples), e->font, white());
					contactShadowSamples = (uint)Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 5, 50, (float)contactShadowSamples, grey1(), white());
					pos += 17;
					Engine::Label(v.r + 2, v.g + pos, 12, "  distance", e->font, white());
					Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
					Engine::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, to_string(contactShadowDistance), e->font, white());
					contactShadowDistance = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, contactShadowDistance, grey1(), white());
					pos += 17;
				}
			}
		}
	}
	else pos += 17;
}

void Light::_SetCookie(void* v) {
	Light* l = (Light*)v;
	l->cookie = _GetCache<Texture>(ASSETTYPE_TEXTURE, l->_cookie);
}

void Light::CalcShadowMatrix() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (_lightType == LIGHTTYPE_SPOT || _lightType == LIGHTTYPE_POINT) {
		Quat q = glm::inverse(object->transform.rotation());
		glMultMatrixf(glm::value_ptr(glm::perspectiveFov(angle * deg2rad, 1024.0f, 1024.0f, minDist, maxDist)));
		glScalef(1, 1, -1);
		glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(q)));
		Vec3 pos = -object->transform.worldPosition();
		glTranslatef(pos.x, pos.y, pos.z);
	}
	//else
		//_shadowMatrix = Mat4x4();
}

void Light::Serialize(Editor* e, std::ofstream* stream) {
	byte b = _lightType;
	if (drawShadow) b |= 0xf0;
	_StreamWrite(&b, stream, 1);
	_StreamWrite(&intensity, stream, 4);
	_StreamWrite(&minDist, stream, 4);
	_StreamWrite(&maxDist, stream, 4);
	_StreamWrite(&angle, stream, 4);
	_StreamWrite(&shadowBias, stream, 4);
	_StreamWrite(&shadowStrength, stream, 4);
	_StreamWrite(&color.r, stream, 4);
	_StreamWrite(&color.g, stream, 4);
	_StreamWrite(&color.b, stream, 4);
	_StreamWrite(&color.a, stream, 4);
}

Light::Light() : Component("Light", COMP_LHT, DRAWORDER_LIGHT), _lightType(LIGHTTYPE_POINT), color(Vec4(1, 1, 1, 1)) {}

Light::Light(std::ifstream& stream, SceneObject* o, long pos) : Light() {
	if (pos >= 0)
		stream.seekg(pos);

	_Strm2Val(stream, _lightType);
	drawShadow = (_lightType & 0xf0) != 0;
	_lightType = LIGHTTYPE(_lightType & 0x0f);
	_Strm2Val(stream, intensity);
	_Strm2Val(stream, minDist);
	_Strm2Val(stream, maxDist);
	_Strm2Val(stream, angle);
	_Strm2Val(stream, shadowBias);
	_Strm2Val(stream, shadowStrength);
	_Strm2Val(stream, color.r);
	_Strm2Val(stream, color.g);
	_Strm2Val(stream, color.b);
	_Strm2Val(stream, color.a);
	_cookie = -1;
}

void ReflectionProbe::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 1, 0), 0.2f, 12, Vec4(1, 0.76f, 0.80, 1), 1);
	Engine::DrawCircleW(Vec3(), Vec3(0, 1, 0), Vec3(0, 0, 1), 0.2f, 12, Vec4(1, 0.76f, 0.80, 1), 1);
	Engine::DrawCircleW(Vec3(), Vec3(0, 0, 1), Vec3(1, 0, 0), 0.2f, 12, Vec4(1, 0.76f, 0.80, 1), 1);

	if (ebv->editor->selected == object) {
		Engine::DrawLineWDotted(range*Vec3(1, 1, -1), range*Vec3(1, 1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(1, 1, 1), range*Vec3(-1, 1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, 1, 1), range*Vec3(-1, 1, -1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, 1, -1), range*Vec3(1, 1, -1), white(), 1, 0.1f);

		Engine::DrawLineWDotted(range*Vec3(1, 1, 1), range*Vec3(1, -1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(1, 1, -1), range*Vec3(1, -1, -1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, 1, 1), range*Vec3(-1, -1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, 1, -1), range*Vec3(-1, -1, -1), white(), 1, 0.1f);

		Engine::DrawLineWDotted(range*Vec3(1, -1, -1), range*Vec3(1, -1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(1, -1, 1), range*Vec3(-1, -1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, -1, 1), range*Vec3(-1, -1, -1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, -1, -1), range*Vec3(1, -1, -1), white(), 1, 0.1f);
	}

	/*
	glPushMatrix();
	Vec3 v = object->transform.position;
	Vec3 vv = object->transform.scale;
	Quat vvv = object->transform.rotation;
	glTranslatef(v.x, v.y, v.z);
	glScalef(vv.x, vv.y, vv.z);
	glMultMatrixf(glm::value_ptr(Quat2Mat(vvv)));
	glPopMatrix();
	*/
}

void ReflectionProbe::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Intensity", e->font, white());
		pos += 17;
	}
	else pos += 17;
}

ReflectionProbe::ReflectionProbe(ushort size) : Component("Reflection Probe", COMP_RDP, DRAWORDER_LIGHT), size(size), map(new CubeMap(size, true)), updateMode(ReflProbe_UpdateMode_Start), intensity(1), clearType(ReflProbe_Clear_Sky), clearColor(), range(1, 1, 1), softness(0) {
	glGenFramebuffers(7, mipFbos);
	for (byte a = 0; a < 7; a++) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mipFbos[a]);
		for (byte i = 0; i < 7; i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, (a == 0) ? map->facePointers[i] : map->facePointerMips[i][a-1], 0);
		}

		GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
		glDrawBuffers(6, DrawBuffers);

		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (Status != GL_FRAMEBUFFER_COMPLETE)
			Debug::Error("ReflProbe (" + name + ")", "FB error:" + Status);
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

ReflectionProbe::ReflectionProbe(std::ifstream& stream, SceneObject* o, long pos) : Component("Reflection Probe", COMP_RDP, DRAWORDER_LIGHT) {
	if (pos >= 0)
		stream.seekg(pos);
	_Strm2Val(stream, size);
	_Strm2Val(stream, updateMode);
	_Strm2Val(stream, intensity);
	_Strm2Val(stream, clearType);
	_Strm2Val(stream, clearColor.r);
	_Strm2Val(stream, clearColor.g);
	_Strm2Val(stream, clearColor.b);
	_Strm2Val(stream, clearColor.a);
	_Strm2Val(stream, range.x);
	_Strm2Val(stream, range.y);
	_Strm2Val(stream, range.z);
	_Strm2Val(stream, softness);

	map = new CubeMap(size);
}

void ReflectionProbe::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWrite(&size, stream, 2);
	_StreamWrite(&updateMode, stream, 1);
	_StreamWrite(&intensity, stream, 4);
	_StreamWrite(&clearType, stream, 1);
	_StreamWrite(&clearColor.r, stream, 4);
	_StreamWrite(&clearColor.g, stream, 4);
	_StreamWrite(&clearColor.b, stream, 4);
	_StreamWrite(&clearColor.a, stream, 4);
	_StreamWrite(&range.x, stream, 4);
	_StreamWrite(&range.y, stream, 4);
	_StreamWrite(&range.z, stream, 4);
	_StreamWrite(&softness, stream, 4);
}

Armature::Armature(string path, SceneObject* o) : Component("Armature", COMP_ARM, DRAWORDER_OVERLAY, o), overridePos(false), restPosition(o->transform.position), restRotation(o->transform.rotation()), restScale(o->transform.scale), _anim(-1) {
	std::ifstream strm(path);
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
	while (b == 'B') {
		AddBone(strm, _bones, boneList, object);
		b = strm.get();
	}
}

Armature::Armature(std::ifstream& stream, SceneObject* o, long pos) : Component("Armature", COMP_ARM, DRAWORDER_OVERLAY) {

}

void Armature::AddBone(std::ifstream& strm, std::vector<ArmatureBone*>& bones, std::vector<ArmatureBone*>& blist, SceneObject* o) {
	char* c = new char[100];
	strm.getline(c, 100, 0);
	string nm = string(c);
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
	Vec3 pos, tal, rht;
	Quat rot;
	byte mask;
	_Strm2Val(strm, pos.x);
	_Strm2Val(strm, pos.y);
	_Strm2Val(strm, pos.z);
	_Strm2Val(strm, tal.x);
	_Strm2Val(strm, tal.y);
	_Strm2Val(strm, tal.z);
	_Strm2Val(strm, rht.x);
	_Strm2Val(strm, rht.y);
	_Strm2Val(strm, rht.z);
	mask = strm.get();
	SceneObject* oo = new SceneObject(nm, (prt == nullptr) ? pos : pos + prt->tailPos, Quat(), Vec3(1.0f));
	auto bn = new ArmatureBone((prt == nullptr) ? pos : pos + prt->tailPos, Quat(), Vec3(), tal, (mask & 0xf0) != 0, &oo->transform);
	if (prt == nullptr) {
		bones.push_back(bn);
		o->AddChild(oo);
	}
	else {
		prt->_children.push_back(bn);
		prt->tr->object->AddChild(oo);
	}
	blist.push_back(bn);
	char b = strm.get();
	if (b == 'b') return;
	else while (b == 'B') {
		AddBone(strm, bn->_children, blist, oo);
		b = strm.get();
	}
}

SceneScript::SceneScript(Editor* e, ASSETID id) : Component(e->headerAssets[id] + " (Script)", COMP_SCR, DRAWORDER_NONE), _script(id) {
	if (id < 0) {
		name = "missing script!";
		return;
	}
	std::ifstream strm(e->projectFolder + "Assets\\" + e->headerAssets[id] + ".meta", std::ios::in | std::ios::binary);
	if (!strm.is_open()) {
		e->_Error("Script Component", "Cannot read meta file!");
		_script = -1;
		return;
	}
	ushort sz;
	_Strm2Val<ushort>(strm, sz);
	_vals.resize(sz);
	SCR_VARTYPE t;
	for (ushort x = 0; x < sz; x++) {
		_Strm2Val<SCR_VARTYPE>(strm, t);
		_vals[x].second.first = t;
		char c[100];
		strm.getline(&c[0], 100, 0);
		_vals[x].first += string(c);
		switch (t) {
		case SCR_VAR_INT:
			_vals[x].second.second = new int(0);
			break;
		case SCR_VAR_FLOAT:
			_vals[x].second.second = new float(0);
			break;
		case SCR_VAR_STRING:
			_vals[x].second.second = new string("");
			break;
		case SCR_VAR_TEXTURE:
			_vals[x].second.second = new ASSETID(-1);
			break;
		case SCR_VAR_COMPREF:
			_vals[x].second.second = new CompRef((COMPONENT_TYPE)c[0]);
			_vals[x].first = _vals[x].first.substr(1);
			break;
		}
	}
}

SceneScript::SceneScript(std::ifstream& strm, SceneObject* o) : Component("(Script)", COMP_SCR, DRAWORDER_NONE), _script(-1) {
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
				_vals[x].second.first = t;
				char c[100];
				strm.getline(&c[0], 100, 0);
				_vals[x].first += string(c);
				switch (t) {
				case SCR_VAR_INT:
					_vals[x].second.second = new int(0);
					break;
				case SCR_VAR_FLOAT:
					_vals[x].second.second = new float(0);
					break;
				case SCR_VAR_STRING:
					_vals[x].second.second = new string("");
					break;
				case SCR_VAR_TEXTURE:
					_vals[x].second.second = new ASSETID(-1);
					break;
				case SCR_VAR_COMPREF:
					_vals[x].second.second = new CompRef((COMPONENT_TYPE)c[0]);
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
			if (vl.first == ss && vl.second.first == SCR_VARTYPE(tp)) {
				switch (vl.second.first) {
				case SCR_VAR_INT:
					int ii;
					_Strm2Val<int>(strm, ii);
					*(int*)vl.second.second += ii;
					break;
				case SCR_VAR_FLOAT:
					float ff;
					_Strm2Val<float>(strm, ff);
					*(float*)vl.second.second += ff;
					break;
				case SCR_VAR_STRING:
					strm.getline(c, 100, 0);
					*(string*)vl.second.second += string(c);
					break;
				case SCR_VAR_TEXTURE:
					ASSETTYPE tdd;
					ASSETID idd;
					//strm.getline(c, 100, 0);
					//string sss(c);
					_Strm2Asset(strm, Editor::instance, tdd, idd);
					*(ASSETID*)vl.second.second += 1 + idd;
				}
			}
		}
	}
	delete[](c);
}

SceneScript::~SceneScript() {
#ifdef IS_EDITOR
	for (auto& a : _vals) {
		delete(a.second.second);
	}
#endif
}

void SceneScript::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_SCRIPT_H, _script);
	ushort sz = (ushort)_vals.size();
	_StreamWrite(&sz, stream, 2);
	for (auto& a : _vals) {
		_StreamWrite(&a.second.first, stream, 1);
		*stream << a.first << char0;
		switch (a.second.first) {
		case SCR_VAR_INT:
		case SCR_VAR_FLOAT:
			_StreamWrite(a.second.second, stream, 4);
			break;
		case SCR_VAR_TEXTURE:
			_StreamWriteAsset(e, stream, ASSETTYPE_TEXTURE, *(ASSETID*)a.second.second);
			break;
		}
	}
}

void SceneScript::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	SceneScript* scr = (SceneScript*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		for (auto& p : _vals){
			Engine::Label(v.r + 20, v.g + pos + 19, 12, p.first, e->font, white(1, (p.second.first == SCR_VAR_COMMENT)? 0.5f : 1));
			switch (p.second.first) {
			case SCR_VAR_FLOAT:
				Engine::Button(v.r + v.b*0.3f, v.g + pos + 17, v.b*0.7f - 1, 16, grey2());
				Engine::Label(v.r + v.b*0.3f + 2, v.g + pos + 19, 12, to_string(*(float*)p.second.second), e->font, white());
				break;
			case SCR_VAR_INT:
				Engine::Button(v.r + v.b*0.3f, v.g + pos + 17, v.b*0.7f - 1, 16, grey2());
				Engine::Label(v.r + v.b*0.3f + 2, v.g + pos + 19, 12, to_string(*(int*)p.second.second), e->font, white());
				break;
			case SCR_VAR_TEXTURE:
				e->DrawAssetSelector(v.r + v.b*0.3f, v.g + pos + 17, v.b*0.7f - 1, 16, grey2(), ASSETTYPE_TEXTURE, 12, e->font, (ASSETID*)p.second.second);
				break;
			case SCR_VAR_COMPREF:
				e->DrawCompSelector(v.r + v.b*0.3f, v.g + pos + 17, v.b*0.7f - 1, 16, grey2(), 12, e->font, (CompRef*)p.second.second);
				break;
			}
			pos += 17;
		}
	}
	pos += 17;
}

void SceneScript::Parse(string s, Editor* e) {
	string p = e->projectFolder + "Assets\\" + s;
	std::ifstream strm(p.c_str(), std::ios::in);
	char* c = new char[100];
	int flo = s.find_last_of('\\') + 1;
	if (flo == string::npos) flo = 0;
	string sc = ("class" + s.substr(flo, s.size() - 2 - flo) + ":publicSceneScript{");
	bool hasClass = false, isPublic = false;
	ushort count = 0;
	while (!strm.eof()) {
		strm.getline(c, 100);
		string ss;
		for (uint a = 0; a < 100; a++) {
			if (c[a] != ' ' && c[a] != '\t' && c[a] != '\r') {
				ss += c[a];
			}
		}
		if (strcmp(sc.c_str(), ss.c_str()) == 0) {
			hasClass = true;
			string op = p + ".meta";
			std::ofstream oStrm(op.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
			oStrm << "XX";
			//read variables
			while (!strm.eof()) {
				strm.getline(c, 100);
				string sss(c);
				while (sss != "" && sss[0] == ' ' || sss[0] == '\t')
					sss = sss.substr(1);
				if (sss == "")
					continue;
				int cmtPos = sss.find("//");
				int ccc = sss.find('(');
				if (ccc != string::npos && (cmtPos == string::npos || ccc < cmtPos))
					continue;
				if (cmtPos == 0 && sss[2] == '#' && sss.size() > 3) {
					oStrm << (char)SCR_VAR_COMMENT << sss.substr(3) << char0;
					count++;
					continue;
				}
				if (!isPublic && sss == "public:") {
					isPublic = true;
					continue;
				}
				else if (isPublic && (sss == "protected:" || sss == "private:" || sss == "};"))
					break;
				if (!isPublic)
					continue;
				int spos = sss.find_first_of(" ");
				if (spos == string::npos)
					continue;
				string tp = sss.substr(0, spos);
				if (tp == "int") {
					oStrm << (char)SCR_VAR_INT;
				}
				else if (tp == "float") {
					oStrm << (char)SCR_VAR_FLOAT;
				}
				else if (tp == "string" || tp == "std::string") {
					oStrm << (char)SCR_VAR_STRING;
				}
				else if (tp == "Texture*") {
					oStrm << (char)SCR_VAR_TEXTURE;
				}
				else {
					if (tp[tp.size() - 1] != '*') continue;
					COMPONENT_TYPE ct = Component::Name2Type(tp.substr(0, tp.size()-1));
					if (ct == COMP_UNDEF) continue;
					oStrm << (char)SCR_VAR_COMPREF << (char)ct;
				}
				int spos2 = min(min(sss.find_first_of(" ", spos + 1), sss.find_first_of(";", spos + 1)), sss.find_first_of("=", spos + 1));
				if (spos2 == string::npos) {
					long l = (long)oStrm.tellp();
					oStrm.seekp(l - 1);
					continue;
				}
				string s2 = sss.substr(spos + 1, spos2 - spos - 1);
				while (sss != "" && sss[0] == ' ' || sss[0] == '\t')
					sss = sss.substr(1);
				if (sss == ""){
					long l = (long)oStrm.tellp();
					oStrm.seekp(l - 1);
					continue;
				}
				oStrm << s2 << char0;
				count++;
			}
			oStrm.seekp(0);
			_StreamWrite(&count, &oStrm, 2);
			oStrm.close();
		}
	}
	if (!hasClass)
		e->_Error("Script Importer", "SceneScript class for " + s + " not found!");
}



//-----------------transform class-------------------
Transform::Transform(SceneObject* sc, Vec3 pos, Quat rot, Vec3 scl) : object(sc), _rotation(rot) {
	Translate(pos)->Scale(scl);
	_UpdateEuler();
	_UpdateLMatrix();
}

const Vec3 Transform::worldPosition() {
	/*
	Vec3 w = position;
	SceneObject* o = object;
	while (o->parent != nullptr) {
	o = o->parent;
	w = w*o->transform.scale + o->transform.position;
	}
	return w;
	*/
	Vec4 v = _worldMatrix * Vec4(0, 0, 0, 1);
	return Vec3(v.x, v.y, v.z);
}

const Vec3 Transform::forward() {
	return rotate(Vec3(0, 0, 1), _rotation);
}

const Vec3 Transform::right() {
	return rotate(Vec3(1, 0, 0), _rotation);
}

const Vec3 Transform::up() {
	return rotate(Vec3(0, 1, 0), _rotation);
}

const Quat Transform::rotation() {
	return _rotation;
}

const Quat Transform::worldRotation() {
	return Quat();
}

void Transform::eulerRotation(Vec3 r) {
	_eulerRotation = r;
	_eulerRotation.x = repeat(_eulerRotation.x, 0, 360);
	_eulerRotation.y = repeat(_eulerRotation.y, 0, 360);
	_eulerRotation.z = repeat(_eulerRotation.z, 0, 360);
	_UpdateQuat();
	_UpdateLMatrix();
}

void Transform::_UpdateQuat() {
	_rotation = Quat(deg2rad*_eulerRotation);
}

void Transform::_UpdateEuler() {
	std::cout << to_string(_rotation) << std::endl;
	_eulerRotation = QuatFunc::ToEuler(_rotation);
}

void Transform::_UpdateLMatrix() {
	_localMatrix = glm::scale(scale);
	_localMatrix = QuatFunc::ToMatrix(_rotation) * _localMatrix;
	_localMatrix = glm::translate(position) * _localMatrix;
	_UpdateWMatrix(object->parent ? object->parent->transform._worldMatrix : Mat4x4());
}

void Transform::_UpdateWMatrix(const Mat4x4& mat) {
	_worldMatrix = mat*_localMatrix;

	for (uint a = 0; a < object->childCount; a++)
		object->children[a]->transform._UpdateWMatrix(_worldMatrix);
}

Transform* Transform::Translate(Vec3 v) {
	position += v;
	_UpdateLMatrix();
	return this;
}
Transform* Transform::Rotate(Vec3 v, TransformSpace sp) {
	if (sp == Space_Self) {
		Quat qx = QuatFunc::FromAxisAngle(Vec3(1, 0, 0), v.x);
		Quat qy = QuatFunc::FromAxisAngle(Vec3(0, 1, 0), v.y);
		Quat qz = QuatFunc::FromAxisAngle(Vec3(0, 0, 1), v.z);
		_rotation *= qx * qy * qz;
	}
	else if (object->parent == nullptr) {
		Quat qx = QuatFunc::FromAxisAngle(Vec3(1, 0, 0), v.x);
		Quat qy = QuatFunc::FromAxisAngle(Vec3(0, 1, 0), v.y);
		Quat qz = QuatFunc::FromAxisAngle(Vec3(0, 0, 1), v.z);
		_rotation = qx * qy * qz * _rotation;
	}
	else {
		Mat4x4 im = glm::inverse(object->parent->transform._worldMatrix);
		Vec4 vx = im*Vec4(1, 0, 0, 0);
		Vec4 vy = im*Vec4(0, 1, 0, 0);
		Vec4 vz = im*Vec4(0, 0, 1, 0);
		Quat qx = QuatFunc::FromAxisAngle(Normalize(to_vec3(vx)), v.x);
		Quat qy = QuatFunc::FromAxisAngle(Normalize(to_vec3(vy)), v.y);
		Quat qz = QuatFunc::FromAxisAngle(Normalize(to_vec3(vz)), v.z);
		_rotation = qx * qy * qz * _rotation;
	}

	_UpdateEuler();
	_UpdateLMatrix();
	return this;
}

Transform* Transform::Scale(Vec3 v) {
	scale += v;
	_UpdateLMatrix();
	return this;
}


SceneObject::SceneObject() : SceneObject(Vec3(), Quat(), Vec3(1, 1, 1)) {}
SceneObject::SceneObject(string s) : SceneObject(s, Vec3(), Quat(), Vec3(1, 1, 1)) {}
SceneObject::SceneObject(Vec3 pos, Quat rot, Vec3 scale) : SceneObject("New Object", Vec3(), Quat(), Vec3(1, 1, 1)) {}
SceneObject::SceneObject(string s, Vec3 pos, Quat rot, Vec3 scale) : active(true), transform(this, pos, rot, scale), childCount(0), _expanded(true), Object(s) {
	id = Engine::GetNewId();
	
}

SceneObject::~SceneObject() {
	for (Component* c : _components)
		delete(c);
	_components.clear();
}

void SceneObject::SetActive(bool a, bool enableAll) {
	active = a;
}

Component* ComponentFromType (COMPONENT_TYPE t){
	switch (t) {
	case COMP_CAM:
		return new Camera();
	case COMP_MFT:
		return new MeshFilter();
	default:
		return nullptr;
	}
}

SceneObject* SceneObject::AddChild(SceneObject* child) { 
	childCount++; 
	children.push_back(child); 
	child->parent = this;
	child->transform._UpdateWMatrix(transform._worldMatrix);
	return this;
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
		if ((cc->componentType == c->componentType) && cc->componentType != COMP_SCR) {
			Debug::Message("Add Component", "Same component already exists!");
			delete(c);
			return cc;
		}
	}
	_components.push_back(c);
	return c;
}

/// <summary>you should probably use GetComponent&lt;T&gt;() instead.</summary>
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

void SceneObject::Refresh() {
	for (Component* c : _components) {
		c->Refresh();
	}
}