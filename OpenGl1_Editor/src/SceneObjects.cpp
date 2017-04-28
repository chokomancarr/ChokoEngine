//#include "SceneObjects.h"
#include "Engine.h"
#include "Editor.h"
#include <sstream>

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
			e->WAITINGREFRESHFLAG = true;
		return false;
	}
	Engine::Label(v.r + 20, v.g + pos + 3, 12, c->name, e->font, white());
	return c->_expanded;
}

Component::Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder, SceneObject* o, vector<COMPONENT_TYPE> dep) : Object(name), componentType(t), active(true), drawOrder(drawOrder), _expanded(true), dependancies(dep), object(o) {
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
	InitGBuffer();
}

Camera::Camera(ifstream& stream, SceneObject* o, long pos) : Camera() {
	if (pos >= 0)
		stream.seekg(pos);
	_Strm2Val(stream, fov);
	_Strm2Val(stream, screenPos.x);
	_Strm2Val(stream, screenPos.y);
	_Strm2Val(stream, screenPos.w);
	_Strm2Val(stream, screenPos.h);

#ifndef IS_EDITOR
	if (d_skyProgram == 0) {
		int link_result = 0;
		GLuint vertex_shader, fragment_shader;
		string err = "";
		ifstream strm("D:\\lightPassVert.txt");
		stringstream ss, ss2, ss3, ss4;
		ss << strm.rdbuf();

		if (!ShaderBase::LoadShader(GL_VERTEX_SHADER, ss.str(), vertex_shader, &err)) {
			Debug::Error("Cam Shader Compiler", "v! " + err);
			abort();
		}
		strm.close();
		strm.open("D:\\lightPassFrag_Sky.txt");
		ss2 << strm.rdbuf();
		if (!ShaderBase::LoadShader(GL_FRAGMENT_SHADER, ss2.str(), fragment_shader, &err)) {
			Debug::Error("Cam Shader Compiler", "fs! " + err);
			abort();
		}

		d_skyProgram = glCreateProgram();
		glAttachShader(d_skyProgram, vertex_shader);
		glAttachShader(d_skyProgram, fragment_shader);
		glLinkProgram(d_skyProgram);
		glGetProgramiv(d_skyProgram, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			int info_log_length = 0;
			glGetProgramiv(d_skyProgram, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(d_skyProgram, info_log_length, NULL, &program_log[0]);
			cout << "cam shader(sky) error" << endl << &program_log[0] << endl;
			glDeleteProgram(d_skyProgram);
			d_skyProgram = 0;
			abort();
		}
		cout << "cam shader(sky) ok" << endl;
		glDetachShader(d_skyProgram, vertex_shader);
		glDetachShader(d_skyProgram, fragment_shader);
		glDeleteShader(fragment_shader);

		strm.close();
		strm.open("D:\\lightPassFrag_Point.txt");
		ss3 << strm.rdbuf();
		if (!ShaderBase::LoadShader(GL_FRAGMENT_SHADER, ss3.str(), fragment_shader, &err)) {
			Debug::Error("Cam Shader Compiler", "fp! " + err);
			abort();
		}

		d_pLightProgram = glCreateProgram();
		glAttachShader(d_pLightProgram, vertex_shader);
		glAttachShader(d_pLightProgram, fragment_shader);
		glLinkProgram(d_pLightProgram);
		glGetProgramiv(d_pLightProgram, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			int info_log_length = 0;
			glGetProgramiv(d_pLightProgram, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(d_pLightProgram, info_log_length, NULL, &program_log[0]);
			cout << "cam shader(pl) error" << endl << &program_log[0] << endl;
			glDeleteProgram(d_pLightProgram);
			d_pLightProgram = 0;
			abort();
		}
		cout << "cam shader(pl) ok" << endl;
		glDetachShader(d_pLightProgram, vertex_shader);
		glDetachShader(d_pLightProgram, fragment_shader);
		glDeleteShader(fragment_shader);

		strm.close();
		strm.open("D:\\lightPassFrag_Spot.txt");
		ss4 << strm.rdbuf();
		if (!ShaderBase::LoadShader(GL_FRAGMENT_SHADER, ss4.str(), fragment_shader, &err)) {
			Debug::Error("Cam Shader Compiler", "fc! " + err);
			abort();
		}

		d_sLightProgram = glCreateProgram();
		glAttachShader(d_sLightProgram, vertex_shader);
		glAttachShader(d_sLightProgram, fragment_shader);
		glLinkProgram(d_sLightProgram);
		glGetProgramiv(d_sLightProgram, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			int info_log_length = 0;
			glGetProgramiv(d_sLightProgram, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(d_sLightProgram, info_log_length, NULL, &program_log[0]);
			cout << "cam shader(sl) error" << endl << &program_log[0] << endl;
			glDeleteProgram(d_sLightProgram);
			d_sLightProgram = 0;
			abort();
		}
		cout << "cam shader(sl) ok" << endl;
		glDetachShader(d_sLightProgram, vertex_shader);
		glDetachShader(d_sLightProgram, fragment_shader);
		glDeleteShader(fragment_shader);


		glDeleteShader(vertex_shader);
	}
#endif
}

/// <summary>Clear, Reset Projection Matrix</summary>
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
	Quat q = glm::inverse(object->transform.rotation);
	glMultMatrixf(glm::value_ptr(glm::perspectiveFov(fov * deg2rad, (float)Display::width, (float)Display::height, 0.01f, 500.0f)));
	glScalef(1, 1, -1);
	glMultMatrixf(glm::value_ptr(Quat2Mat(q)));
	Vec3 pos = -object->transform.worldPosition();
	glTranslatef(pos.x, pos.y, pos.z);
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
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, grey1());
		Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 20, 12, to_string(cam->fov), e->font, white());
		cam->fov = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f-1, 16, 0.1f, 179.9f, cam->fov, grey1(), white());
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
		pos += (uint)max(37 + dh, 87);
		Engine::Label(v.r + 2, v.g + pos + 4, 12, "Filtering", e->font, white());
		vector<string> clearNames = { "None", "Color and Depth", "Depth only", "Skybox" };
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.3f, v.g + pos + 1, v.b * 0.7f - 1, 14, grey2(), clearNames[clearType], 12, e->font, white()) == MOUSE_PRESS) {
			e->RegisterMenu(nullptr, "", clearNames, { &_SetClear0, &_SetClear1, &_SetClear2, &_SetClear3 }, 0, Vec2(v.r + v.b * 0.3f, v.g + pos));
		}
		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Target", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_RENDERTEXTURE, 12, e->font, &_tarRT, nullptr, this);
		pos += 34;
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
	}
	else pos += 17;
}

void MeshFilter::SetMesh(int i) {
	_mesh = i;
	if (i >= 0)
		mesh = _GetCache<Mesh>(ASSETTYPE_MESH, i);
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

void MeshFilter::Serialize(Editor* e, ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_MESH, _mesh);
}

MeshFilter::MeshFilter(ifstream& stream, SceneObject* o, long pos) : Component("Mesh Filter", COMP_MFT, DRAWORDER_NONE, o), _mesh(-1) {
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

MeshRenderer::MeshRenderer(ifstream& stream, SceneObject* o, long pos) : Component("Mesh Renderer", COMP_MRD, DRAWORDER_SOLID | DRAWORDER_TRANSPARENT, o, { COMP_MFT }) {
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

void MeshRenderer::DrawEditor(EB_Viewer* ebv) {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0];
	if (mf == nullptr || mf->mesh == nullptr || !mf->mesh->loaded)
		return;
	glEnableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, (ebv->selectedShading == 0) ? GL_FILL : GL_LINE);
	glEnable(GL_CULL_FACE);
	glVertexPointer(3, GL_FLOAT, 0, &(mf->mesh->vertices[0]));
	glLineWidth(1);
	GLfloat matrix[16], matrix2[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	glGetFloatv(GL_PROJECTION_MATRIX, matrix2);
	glm::mat4 m1(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]);
	glm::mat4 m2(matrix2[0], matrix2[1], matrix2[2], matrix2[3], matrix2[4], matrix2[5], matrix2[6], matrix2[7], matrix2[8], matrix2[9], matrix2[10], matrix2[11], matrix2[12], matrix2[13], matrix2[14], matrix2[15]);
	for (uint m = 0; m < mf->mesh->materialCount; m++) {
		if (materials[m] == nullptr)
			continue;
		materials[m]->ApplyGL(m1, m2);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->vertices[0]));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->uv0[0]));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->normals[0]));
		glDrawElements(GL_TRIANGLES, mf->mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &(mf->mesh->_matTriangles[m][0]));
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_CULL_FACE);
}

void MeshRenderer::DrawDeferred() {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0];
	if (mf == nullptr || mf->mesh == nullptr || !mf->mesh->loaded)
		return;
	glEnableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
	glVertexPointer(3, GL_FLOAT, 0, &(mf->mesh->vertices[0]));
	GLfloat matrix[16], matrix2[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix); //model -> world
	glGetFloatv(GL_PROJECTION_MATRIX, matrix2); //world -> screen
	glm::mat4 m1(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]);
	glm::mat4 m2(matrix2[0], matrix2[1], matrix2[2], matrix2[3], matrix2[4], matrix2[5], matrix2[6], matrix2[7], matrix2[8], matrix2[9], matrix2[10], matrix2[11], matrix2[12], matrix2[13], matrix2[14], matrix2[15]);
	for (uint m = 0; m < mf->mesh->materialCount; m++) {
		if (materials[m] == nullptr)
			continue;
		materials[m]->ApplyGL(m1, m2);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->vertices[0]));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->uv0[0]));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->normals[0]));
		glDrawElements(GL_TRIANGLES, mf->mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &(mf->mesh->_matTriangles[m][0]));
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_CULL_FACE);
}

void MeshRenderer::DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) {
	//MeshRenderer* mrd = (MeshRenderer*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		MeshFilter* mft = (MeshFilter*)dependacyPointers[0];
		if (mft->mesh == nullptr) {
			Engine::Label(v.r + 2, v.g + pos + 20, 12, "No Mesh Assigned!", e->font, white());
			pos += 34;
		}
		else {
			Engine::Label(v.r + 2, v.g + pos + 20, 12, "Materials: " + to_string(mft->mesh->materialCount), e->font, white());
			for (uint a = 0; a < mft->mesh->materialCount; a++) {
				Engine::Label(v.r + 2, v.g + pos + 37, 12, "Material " + to_string(a), e->font, white());
				e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 34, v.b*0.7f, 16, grey1(), ASSETTYPE_MATERIAL, 12, e->font, &_materials[a], & _UpdateMat, this);
				pos += 17;
				if (materials[a] == nullptr)
					continue;
				for (uint q = 0; q < materials[a]->valOrders.size(); q++) {
					Engine::Label(v.r + 20, v.g + pos + 38, 12, materials[a]->valNames[materials[a]->valOrders[q]][materials[a]->valOrderIds[q]], e->font, white());
					Engine::DrawTexture(v.r + 3, v.g + pos + 35, 16, 16, e->matVarTexs[materials[a]->valOrders[q]]);
					void* bbs = materials[a]->vals[materials[a]->valOrders[q]][materials[a]->valOrderGLIds[q]];
					switch (materials[a]->valOrders[q]) {
					case SHADER_INT:
						Engine::Button(v.r + v.b * 0.3f + 17, v.g + pos + 35, v.b*0.7f - 17, 16, grey1(), to_string(*(int*)bbs), 12, e->font, white());
						break;
					case SHADER_FLOAT:
						Engine::Button(v.r + v.b * 0.3f + 17, v.g + pos + 35, v.b*0.7f - 17, 16, grey1(), to_string(*(float*)bbs), 12, e->font, white());
						break;
					case SHADER_SAMPLER:
						e->DrawAssetSelector(v.r + v.b * 0.3f + 17, v.g + pos + 35, v.b*0.7f - 17, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &((MatVal_Tex*)bbs)->id, _UpdateTex, bbs);
						break;
					}
					pos += 17;
				}
				pos++;
				/*
				for (auto aa : materials[a]->vals) {
					int r = 0;
					for (auto bb : aa.second) {
						Engine::Label(v.r + 27, v.g + pos + 38, 12, materials[a]->valNames[aa.first][r], e->font, white());

						Engine::DrawTexture(v.r + v.b * 0.3f, v.g + pos + 35, 16, 16, e->matVarTexs[aa.first]);
						switch (aa.first) {
						case SHADER_INT:
							Engine::Button(v.r + v.b * 0.3f + 17, v.g + pos + 35, v.b*0.7f - 17, 16, grey1(), to_string(*(int*)bb.second), 12, e->font, white());
							break;
						case SHADER_FLOAT:
							Engine::Button(v.r + v.b * 0.3f + 17, v.g + pos + 35, v.b*0.7f - 17, 16, grey1(), to_string(*(float*)bb.second), 12, e->font, white());
							break;
						case SHADER_SAMPLER:
							e->DrawAssetSelector(v.r + v.b * 0.3f + 17, v.g + pos + 35, v.b*0.7f - 17, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &((MatVal_Tex*)bb.second)->id, _UpdateTex, bb.second);
							break;
						}
						r++;
						pos += 17;
					}
				}
				pos += 1;
				*/
			}
			pos += 34;
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

void MeshRenderer::Serialize(Editor* e, ofstream* stream) {
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

void TextureRenderer::Serialize(Editor* e, ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_TEXTURE, _texture);
}

TextureRenderer::TextureRenderer(ifstream& stream, SceneObject* o, long pos) : Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY, o) {
	if (pos >= 0)
		stream.seekg(pos);
	ASSETTYPE t;
	_Strm2Asset(stream, Editor::instance, t, _texture);
}

//-----------------mesh class------------------------
Mesh::Mesh(Editor* e, int i) : AssetObject(ASSETTYPE_MESH) {
	Mesh* m2 = _GetCache<Mesh>(type, i);
	vertices = m2->vertices;
	vertexCount = m2->vertexCount;
	normals = m2->normals;
	triangles = m2->triangles;
	triangleCount = m2->triangleCount;
	loaded = m2->loaded;
}

Mesh::Mesh(ifstream& stream, uint offset) : AssetObject(ASSETTYPE_MESH), loaded(false), vertexCount(0), triangleCount(0), materialCount(0) {
	if (stream.is_open()) {
		stream.seekg(offset);

		char* c = new char[100];
		stream.read(c, 6);
		c[6] = (char)0;
		if (string(c) != "KTO123") {
			Debug::Error("Mesh importer", "file not supported");
			return;
		}
		stream.getline(c, 100, 0);
		name += string(c);
		delete[](c);

		char cc;
		cc = stream.get();

		while (cc != 0) {
			if (cc == 'V') {
				_Strm2Val(stream, vertexCount);
				for (uint vc = 0; vc < vertexCount; vc++) {
					Vec3 v;
					_Strm2Val(stream, v.x);
					_Strm2Val(stream, v.y);
					_Strm2Val(stream, v.z);
					vertices.push_back(v);
					_Strm2Val(stream, v.x);
					_Strm2Val(stream, v.y);
					_Strm2Val(stream, v.z);
					normals.push_back(v);
				}
			}
			else if (cc == 'F') {
				_Strm2Val(stream, triangleCount);
				for (uint fc = 0; fc < triangleCount; fc++) {
					byte m;
					_Strm2Val(stream, m);
					while (materialCount <= m) {
						_matTriangles.push_back(vector<int>());
						materialCount++;
					}
					uint i;
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
				}
			}
			else if (cc == 'U') {
				byte c;
				_Strm2Val(stream, c);
				for (uint vc = 0; vc < vertexCount; vc++) {
					Vec2 i;
					_Strm2Val(stream, i.x);
					_Strm2Val(stream, i.y);
					uv0.push_back(i);
				}
				if (c > 1) {
					for (uint vc = 0; vc < vertexCount; vc++) {
						Vec2 i;
						_Strm2Val(stream, i.x);
						_Strm2Val(stream, i.y);
						uv1.push_back(i);
					}
				}
			}
			else {
				Debug::Error("Mesh Importer", "Unknown char: " + to_string(cc));
			}
			cc = stream.get();
		}

		if (vertexCount > 0) {
			if (normals.size() != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (normals incomplete)!");
				return;
			}
			else if (uv0.size() == 0)
				uv0.resize(vertexCount);
			else if (uv0.size() != vertexCount){
				Debug::Error("Mesh Importer", "mesh metadata corrupted (uv0 incomplete)!");
				return;
			}
			if (uv1.size() == 0)
				uv1.resize(vertexCount);
			else if (uv1.size() != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (uv1 incomplete)!");
				return;
			}
			loaded = true;
		}
		return;
	}
}

Mesh::Mesh(string path) : AssetObject(ASSETTYPE_MESH), loaded(false), vertexCount(0), triangleCount(0), materialCount(0) {
	string p = Editor::instance->projectFolder + "Assets\\" + path + ".mesh.meta";
	ifstream stream(p.c_str(), ios::in | ios::binary);
	if (!stream.good()) {
		cout << "mesh file not found!" << endl;
		return;
	}

	char* c = new char[100];
	stream.read(c, 6);
	c[6] = (char)0;
	if (string(c) != "KTO123") {
		Debug::Error("Mesh importer", "file not supported");
		return;
	}
	stream.getline(c, 100, 0);
	name += string(c);
	delete[](c);

	char cc;
	stream.read(&cc, 1);

	while (cc != 0) {
		if (cc == 'V') {
			_Strm2Val(stream, vertexCount);
			for (uint vc = 0; vc < vertexCount; vc++) {
				Vec3 v;
				_Strm2Val(stream, v.x);
				_Strm2Val(stream, v.y);
				_Strm2Val(stream, v.z);
				vertices.push_back(v);
				_Strm2Val(stream, v.x);
				_Strm2Val(stream, v.y);
				_Strm2Val(stream, v.z);
				normals.push_back(v);
			}
		}
		else if (cc == 'F') {
			_Strm2Val(stream, triangleCount);
			for (uint fc = 0; fc < triangleCount; fc++) {
				byte m;
				_Strm2Val(stream, m);
				while (materialCount <= m) {
					_matTriangles.push_back(vector<int>());
					materialCount++;
				}
				uint i;
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
			}
		}
		else if (cc == 'U') {
			byte c;
			_Strm2Val(stream, c);
			for (uint vc = 0; vc < vertexCount; vc++) {
				Vec2 i;
				_Strm2Val(stream, i.x);
				_Strm2Val(stream, i.y);
				uv0.push_back(i);
			}
			if (c > 1) {
				for (uint vc = 0; vc < vertexCount; vc++) {
					Vec2 i;
					_Strm2Val(stream, i.x);
					_Strm2Val(stream, i.y);
					uv1.push_back(i);
				}
			}
		}
		else {
			Debug::Error("Mesh Importer", "Unknown char: " + to_string(cc));
		}
		stream.read(&cc, 1);
	}

	if (vertexCount > 0) {
		if (normals.size() != vertexCount) {
			Debug::Error("Mesh Importer", "mesh metadata corrupted (normals incomplete)!");
			return;
		}
		else if (uv0.size() == 0)
			uv0.resize(vertexCount);
		else if (uv0.size() != vertexCount){
			Debug::Error("Mesh Importer", "mesh metadata corrupted (uv0 incomplete)!");
			return;
		}
		if (uv1.size() == 0)
			uv1.resize(vertexCount);
		else if (uv1.size() != vertexCount) {
			Debug::Error("Mesh Importer", "mesh metadata corrupted (uv1 incomplete)!");
			return;
		}
		loaded = true;
	}
	return;
}

bool Mesh::ParseBlend(Editor* e, string s) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	HANDLE stdOutR, stdOutW, stdInR, stdInW;
	if (!CreatePipe(&stdInR, &stdInW, &sa, 0)) {
		cout << "failed to create pipe for stdin!";
		return false;
	}
	if (!SetHandleInformation(stdInW, HANDLE_FLAG_INHERIT, 0)){
		cout << "failed to set handle for stdin!";
		return false;
	}
	if (!CreatePipe(&stdOutR, &stdOutW, &sa, 0)) {
		cout << "failed to create pipe for stdout!";
		return false;
	}
	if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0)){
		cout << "failed to set handle for stdout!";
		return false;
	}
	STARTUPINFO startInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startInfo, sizeof(STARTUPINFO));
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
	startInfo.cb = sizeof(STARTUPINFO);
	startInfo.hStdInput = stdInR;
	startInfo.hStdOutput = stdOutW;
	startInfo.dwFlags |= STARTF_USESTDHANDLES;

	//create meta directory
	string ss = s.substr(0, s.find_last_of('.'));
	string sss = ss + "_blend";
	if (!CreateDirectory(sss.c_str(), NULL)) {
		for (string file : IO::GetFiles(sss))
			DeleteFile(file.c_str());
	}
	SetFileAttributes(sss.c_str(), FILE_ATTRIBUTE_HIDDEN);
	string ms(s + ".meta");
	DeleteFile(ms.c_str());

	bool failed = true;
	string cmd1(e->_blenderInstallationPath.substr(0, 2) + "\n"); //root
	string cmd2("cd " + e->_blenderInstallationPath.substr(0, e->_blenderInstallationPath.find_last_of("\\")) + "\n");
	string cmd3("blender \"" + s + "\" --background --python \"" + e->dataPath + "\\Python\\blend_exporter.py\" -- \"" + s.substr(0, s.find_last_of('\\')) + "?" + ss.substr(ss.find_last_of('\\') + 1, string::npos) + "\"\n");
	//outputs object list, and meshes in subdir
	if (CreateProcess("C:\\Windows\\System32\\cmd.exe", 0, NULL, NULL, true, CREATE_NO_WINDOW, NULL, "D:\\TestProject\\", &startInfo, &processInfo) != 0) {
		cout << "executing Blender..." << endl;
		bool bSuccess = false;
		DWORD dwWrite;
		bSuccess = WriteFile(stdInW, cmd1.c_str(), cmd1.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			cout << "can't get to root!" << endl;
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd2.c_str(), cmd2.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			cout << "can't navigate to blender dir!" << endl;
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd3.c_str(), cmd3.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			cout << "can't execute blender!" << endl;
			return false;
		}
		DWORD w;
		bool finish = false;
		do {
			w = WaitForSingleObject(processInfo.hProcess, DWORD(200));
			DWORD dwRead;
			CHAR chBuf[4096];
			string out = "";
			bSuccess = ReadFile(stdOutR, chBuf, 4096, &dwRead, NULL) != 0;
			if (bSuccess && dwRead > 0) {
				string s(chBuf, dwRead);
				out += s;
			}
			for (uint r = 0; r < out.size();) {
				int rr = out.find_first_of('\n', r);
				if (rr == string::npos)
					rr = out.size() - 1;
				string sss = out.substr(r, rr - r);
				cout << sss << endl;
				r = rr + 1;
				if (sss.size() > 1 && sss[0] == '!')
					e->_Message("Blender", sss.substr(1, string::npos));
				if (sss.size() > 12 && sss.substr(0, 12) == "Blender quit"){
					TerminateProcess(processInfo.hProcess, 0);
					failed = false;
					finish = true;
				}
			}
		} while (w == WAIT_TIMEOUT && !finish);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		if (failed)
			return false;
	}
	else {
		cout << "Cannot start Blender!" << endl;
		CloseHandle(stdOutR);
		CloseHandle(stdOutW);
		CloseHandle(stdInR);
		CloseHandle(stdInW);
		return false;
	}
	CloseHandle(stdOutR);
	CloseHandle(stdOutW);
	SetFileAttributes(ms.c_str(), FILE_ATTRIBUTE_HIDDEN);
	return true;
}


AnimClip::AnimClip(string path) : AssetObject(ASSETTYPE_ANIMCLIP) {
	string p = Editor::instance->projectFolder + "Assets\\" + path + ".animclip.meta";
	ifstream stream(p.c_str(), ios::in | ios::binary);
	if (!stream.good()) {
		cout << "animclip file not found!" << endl;
		return;
	}

	char* c = new char[100];
	stream.read(c, 4);
	c[4] = (char)0;
	if (string(c) != "ANIM") {
		Debug::Error("AnimClip importer", "file not supported");
		return;
	}

	_Strm2Val(stream, curveLength);
	for (ushort aa = 0; aa < curveLength; aa++) {
		if (stream.eof()) {
			Debug::Error("AnimClip", "Unexpected eof");
			return;
		}
		curves.push_back(FCurve());
		_Strm2Val(stream, curves[aa].type);
		stream.getline(c, 100, 0);
		curves[aa].name += string(c);
		_Strm2Val(stream, curves[aa].keyCount);
		for (ushort bb = 0; bb < curves[aa].keyCount; bb++) {
			float ax, ay, bx, by, cx, cy;
			_Strm2Val(stream, ax);
			_Strm2Val(stream, ay);
			_Strm2Val(stream, bx);
			_Strm2Val(stream, by);
			_Strm2Val(stream, cx);
			_Strm2Val(stream, cy);
			curves[aa].keys.push_back(FCurve_Key(Vec2(ax, ay), Vec2(bx, by), Vec2(cx, cy)));
		}
		curves[aa].startTime += curves[aa].keys[0].point.x;
		curves[aa].endTime += curves[aa].keys[curves[aa].keyCount-1].point.x;
	}
}

Animator::Animator() : AssetObject(ASSETTYPE_ANIMATOR), activeState(0), nextState(0), stateTransition(0), states(), transitions() {

}

Animator::Animator(string path) : Animator() {
	string p = Editor::instance->projectFolder + "Assets\\" + path;
	ifstream stream(p.c_str());
	if (!stream.good()) {
		cout << "animator not found!" << endl;
		return;
	}
	char* c = new char[4];
	stream.read(c, 3);
	c[3] = (char)0;
	string ss(c);
	if (ss != "ANT") {
		cerr << "file not supported" << endl;
		return;
	}
	byte stateCount, transCount;
	_Strm2Val(stream, stateCount);
	for (byte a = 0; a < stateCount; a++) {
		states.push_back(new Anim_State());
		_Strm2Val(stream, states[a]->isBlend);
		ASSETTYPE t;
		if (!states[a]->isBlend) {
			_Strm2Asset(stream, Editor::instance, t, states[a]->_clip);
		}
		else {
			byte bc;
			_Strm2Val(stream, bc);
			for (byte c = 0; c < bc; c++) {
				states[a]->_blendClips.push_back(-1);
				_Strm2Asset(stream, Editor::instance, t, states[a]->_blendClips[c]);
			}
		}
		_Strm2Val(stream, states[a]->speed);
		_Strm2Val(stream, states[a]->editorPos.x);
		_Strm2Val(stream, states[a]->editorPos.y);
	}
	_Strm2Val(stream, transCount);
	
}

void Light::DrawEditor(EB_Viewer* ebv) {
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

		Engine::Label(v.r + 2, v.g + pos + 20, 12, "Intensity", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, grey1());
		Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 20, 12, to_string(intensity), e->font, white());
		intensity = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f - 1, 16, 0, 20, intensity, grey1(), white());
		pos += 34;
		Engine::Label(v.r + 2, v.g + pos + 2, 12, "Color", e->font, white());
		e->DrawColorSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), 12, e->font, &color);
		pos += 17;

		switch (_lightType) {
		case LIGHTTYPE_POINT:
			Engine::Label(v.r + 2, v.g + pos + 2, 12, "core radius", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 2, 12, to_string(minDist), e->font, white());
			minDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, maxDist, minDist, grey1(), white());
			pos += 17;
			Engine::Label(v.r + 2, v.g + pos + 2, 12, "distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 2, 12, to_string(maxDist), e->font, white());
			maxDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 20, maxDist, grey1(), white());
			pos += 17;
			break;
		case LIGHTTYPE_DIRECTIONAL:
			
			break;
		case LIGHTTYPE_SPOT:
			Engine::Label(v.r + 2, v.g + pos + 2, 12, "angle", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 2, 12, to_string(angle), e->font, white());
			angle = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 180, angle, grey1(), white());
			pos += 17;
			Engine::Label(v.r + 2, v.g + pos + 2, 12, "start distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 2, 12, to_string(minDist), e->font, white());
			minDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, maxDist, minDist, grey1(), white());
			pos += 17;
			Engine::Label(v.r + 2, v.g + pos + 2, 12, "end distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 2, 12, to_string(maxDist), e->font, white());
			maxDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 50, maxDist, grey1(), white());
			pos += 17;
			break;
		}
		if (_lightType != LIGHTTYPE_POINT) {
			if (Engine::EButton(e->editorLayer == 0, v.r, v.g + pos, v.b * 0.5f - 1, 16, (!drawShadow) ? white(1, 0.5f) : grey1(), "No Shadow", 12, e->font, white()) == MOUSE_RELEASE)
				drawShadow = false;
			if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.5f, v.g + pos, v.b * 0.5f - 1, 16, (drawShadow) ? white(1, 0.5f) : grey1(), "Shadow", 12, e->font, white()) == MOUSE_RELEASE)
				drawShadow = true;
			pos += 17;
			if (drawShadow) {
				Engine::Label(v.r + 2, v.g + pos + 2, 12, "shadow bias", e->font, white());
				Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
				Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 2, 12, to_string(shadowBias), e->font, white());
				shadowBias = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 0.1f, shadowBias, grey1(), white());
				pos += 17;
				Engine::Label(v.r + 2, v.g + pos + 2, 12, "shadow strength", e->font, white());
				Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
				Engine::Label(v.r + v.b * 0.3f + 2, v.g + pos + 2, 12, to_string(shadowStrength), e->font, white());
				shadowStrength = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, shadowStrength, grey1(), white());
				pos += 17;
			}
		}
	}
	else pos += 17;
}

void Light::CalcShadowMatrix() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (_lightType == LIGHTTYPE_SPOT) {
		Quat q = glm::inverse(object->transform.rotation);
		glMultMatrixf(glm::value_ptr(glm::perspectiveFov(angle * deg2rad, 1024.0f, 1024.0f, minDist, maxDist)));
		glScalef(1, 1, -1);
		glMultMatrixf(glm::value_ptr(Quat2Mat(q)));
		Vec3 pos = -object->transform.worldPosition();
		glTranslatef(pos.x, pos.y, pos.z);
	}
	//else
		//_shadowMatrix = glm::mat4();
}

void Light::Serialize(Editor* e, ofstream* stream) {
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

Light::Light(ifstream& stream, SceneObject* o, long pos) : Component("Light", COMP_LHT, DRAWORDER_LIGHT, o) {
	if (pos >= 0)
		stream.seekg(pos);

	_Strm2Val(stream, _lightType);
	drawShadow = (_lightType & 0xf0) != 0;
	_lightType = LIGHTTYPE(_lightType & 0x0f);
	if (drawShadow)
		InitShadow();
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
}

Armature::Armature(string path, SceneObject* o) : Component("Armature", COMP_ARM, DRAWORDER_OVERLAY), overridePos(false), restPosition(o->transform.position), restRotation(o->transform.rotation), restScale(o->transform.scale), _anim(-1) {
	ifstream strm(path);
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
	char b = strm.get();
	while (b == 'B') {
		AddBone(strm, _bones);
	}
}

Armature::Armature(ifstream& stream, SceneObject* o, long pos) : Component("Armature", COMP_ARM, DRAWORDER_OVERLAY) {

}

void Armature::AddBone(ifstream& strm, vector<ArmatureBone*>& bones) {
	char* c = new char[100];
	strm.getline(c, 100, 0);
	Vec3 pos, fwd, rht;
	Quat rot;
	byte mask;
	_Strm2Val(strm, pos.x);
	_Strm2Val(strm, pos.y);
	_Strm2Val(strm, pos.z);
	_Strm2Val(strm, fwd.x);
	_Strm2Val(strm, fwd.y);
	_Strm2Val(strm, fwd.z);
	_Strm2Val(strm, rht.x);
	_Strm2Val(strm, rht.y);
	_Strm2Val(strm, rht.z);
	mask = strm.get();
	bones.push_back(new ArmatureBone(pos, Quat(), Vec3(), (mask & 0xf0) != 0));
}

SceneScript::SceneScript(Editor* e, ASSETID id) : Component(e->headerAssets[id] + " (Script)", COMP_SCR, DRAWORDER_NONE), _script(id) {
	if (id < 0) {
		name = "missing script!";
		return;
	}
	ifstream strm(e->projectFolder + "Assets\\" + e->headerAssets[id] + ".meta", ios::in | ios::binary);
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

SceneScript::SceneScript(ifstream& strm, SceneObject* o) : Component("(Script)", COMP_SCR, DRAWORDER_NONE), _script(-1) {
	char* c = new char[100];
	strm.getline(c, 100, 0);
	string s(c);
	int i = 0;
	for (auto a : Editor::instance->headerAssets) {
		if (a == s) {
			_script = i;
			name = a + " (Script)";

			ifstream strm(Editor::instance->projectFolder + "Assets\\" + a + ".meta", ios::in | ios::binary);
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
		for (auto vl : _vals) {
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
	for (auto a : _vals) {
		delete(a.second.second);
	}
#endif
}

void SceneScript::Serialize(Editor* e, ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_SCRIPT_H, _script);
	ushort sz = (ushort)_vals.size();
	_StreamWrite(&sz, stream, 2);
	for (auto a : _vals) {
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
		for (auto p : _vals){
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
	ifstream strm(p.c_str(), ios::in);
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
			ofstream oStrm(op.c_str(), ios::out | ios::binary | ios::trunc);
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
				oStrm << s2 << (char)0;
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
	default:
		return nullptr;
	}
}

SceneObject* SceneObject::AddChild(SceneObject* child) { 
	childCount++; 
	children.push_back(child); 
	child->parent = this;
	return this;
}

void SceneObject::RemoveChild(SceneObject* o) {
	auto it = std::find(children.begin(), children.end(), o);
	if (it != children.end()) {
		swap(*it, children.back());
		children.pop_back();
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