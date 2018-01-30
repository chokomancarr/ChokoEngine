#include "Engine.h"
#include "Editor.h"

Object::Object(string nm) : id(Engine::GetNewId()), name(nm) {}
Object::Object(ulong id, string nm) : id(id), name(nm) {}

bool DrawComponentHeader(Editor* e, Vec4 v, uint pos, Component* c) {
	Engine::DrawQuad(v.r, v.g + pos, v.b - 17, 16, grey2());
	//bool hi = expand;
	if (Engine::EButton((e->editorLayer == 0), v.r, v.g + pos, 16, 16, c->_expanded ? e->tex_collapse : e->tex_expand, white()) == MOUSE_RELEASE) {
		c->_expanded = !c->_expanded;//hi = !expand;
	}
	//UI::Texture(v.r, v.g + pos, 16, 16, c->_expanded ? e->collapse : e->expand);
	if (Engine::EButton(e->editorLayer == 0, v.r + v.b - 16, v.g + pos, 16, 16, e->tex_buttonX, white(1, 0.7f)) == MOUSE_RELEASE) {
		//delete
		c->object->RemoveComponent(get_shared<Component>(c));
		if (!c)
			e->flags |= WAITINGREFRESHFLAG;
		return false;
	}
	UI::Label(v.r + 20, v.g + pos, 12, c->name, e->font, white());
	return c->_expanded;
}

Component::Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder, SceneObject* o, std::vector<COMPONENT_TYPE> dep)
	: Object(name), componentType(t), active(true), drawOrder(drawOrder), _expanded(true), dependancies(dep) {
	for (COMPONENT_TYPE t : dependancies) {
		dependacyPointers.push_back(rComponent());
	}
	if (o) object(o);
}

COMPONENT_TYPE Component::Name2Type(string nm) {
	static const string names[] = {
		"Camera",
		"MeshFilter",
		"MeshRenderer",
		"TextureRenderer",
		"SkinnedMeshRenderer",
		"ParticleSystem",
		"Light",
		"ReflectiveQuad",
		"RenderProbe",
		"Armature",
		"Animator",
		"InverseKinematics",
		"Script"
	};
	static const COMPONENT_TYPE types[] = {
		COMP_CAM,
		COMP_MFT,
		COMP_MRD,
		COMP_TRD,
		COMP_SRD,
		COMP_PST,
		COMP_LHT,
		COMP_RFQ,
		COMP_RDP,
		COMP_ARM,
		COMP_ANM,
		COMP_INK,
		COMP_SCR
	};
	
	for (uint i = 0; i < sizeof(types); i++) {
		if ((nm) == names[i]) return types[i];
	}
	return COMP_UNDEF;
}

const int Camera::camVertsIds[19] = { 0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 4, 4, 3, 3, 1, 1, 2, 5 };

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
	Vec3 pos = -object->transform.position();
	glTranslatef(pos.x, pos.y, pos.z);
}

void Camera::GenShaderFromPath(const string& pathv, const string& pathf, GLuint* program) {
	GLuint vertex_shader;
	std::string err;
	if (!Shader::LoadShader(GL_VERTEX_SHADER, DefaultResources::GetStr(pathv), vertex_shader, &err)) {
		Debug::Error("Cam Shader Compiler", pathv + "! " + err);
		abort();
	}
	GenShaderFromPath(vertex_shader, pathf, program);
}

void Camera::GenShaderFromPath(GLuint vertex_shader, const string& path, GLuint* program) {
	GLuint fragment_shader;
	std::string err;
	/*
	std::ifstream strm(path);
	if (strm.fail()) {
		Debug::Error("Cam Shader Compiler", "fs frag not found!");
		abort();
	}
	std::stringstream ss;
	ss << strm.rdbuf();
	*/
	if (!Shader::LoadShader(GL_FRAGMENT_SHADER, DefaultResources::GetStr(path), fragment_shader, &err)) {
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

	if (!Shader::LoadShader(GL_VERTEX_SHADER, DefaultResources::GetStr("lightPassVert.txt"), vertex_shader, &err)) {
		Debug::Error("Cam Shader Compiler", "v! " + err);
		abort();
	}

	GenShaderFromPath(vertex_shader, "blurPassFrag.txt", &d_blurProgram);
	GenShaderFromPath(vertex_shader, "blurPassFrag_Skybox.txt", &d_blurSBProgram);
	GenShaderFromPath(vertex_shader, "lightPassFrag_Sky.txt", &d_skyProgram);
	GenShaderFromPath(vertex_shader, "lightPassFrag_Point.txt", &d_pLightProgram);
	GenShaderFromPath(vertex_shader, "lightPassFrag_Spot.txt", &d_sLightProgram);
	GenShaderFromPath(vertex_shader, "lightPassFrag_Spot_ContShad.txt", &d_sLightCSProgram);
	GenShaderFromPath(vertex_shader, "lightPassFrag_Spot_GI_RSM.txt", &d_sLightRSMProgram);
	GenShaderFromPath(vertex_shader, "lightPassFrag_Spot_GI_FluxPrep.txt", &d_sLightRSMFluxProgram);
	GenShaderFromPath(vertex_shader, "lightPassFrag_ProbeMask.txt", &d_probeMaskProgram);
	GenShaderFromPath(vertex_shader, "lightPassFrag_ReflQuad.txt", &d_reflQuadProgram);

	glDeleteShader(vertex_shader);

	Light::ScanParams();
	ReflectiveQuad::ScanParams();
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

void Camera::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	Camera* cam = (Camera*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		UI::Label(v.r + 2, v.g + pos + 17, 12, "Field of view", e->font, white());
		cam->fov = TryParse(UI::EditText(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, 12, grey1(), std::to_string(cam->fov), e->font, true, nullptr, white()), cam->fov);
		cam->fov = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f-1, 16, 0.1f, 179.9f, cam->fov, grey1(), white());
		UI::Label(v.r + 2, v.g + pos + 35, 12, "Frustrum", e->font, white());
		UI::Label(v.r + 4, v.g + pos + 47, 12, "X", e->font, white());
		Engine::DrawQuad(v.r + 20, v.g + pos + 47, v.b*0.3f - 20, 16, grey1());
		UI::Label(v.r + v.b*0.3f + 4, v.g + pos + 47, 12, "Y", e->font, white());
		Engine::DrawQuad(v.r + v.b*0.3f + 20, v.g + pos + 47, v.b*0.3f - 20, 16, grey1());
		UI::Label(v.r + 4, v.g + pos + 64, 12, "W", e->font, white());
		Engine::DrawQuad(v.r + 20, v.g + pos + 64, v.b*0.3f - 20, 16, grey1());
		UI::Label(v.r + v.b*0.3f + 4, v.g + pos + 64, 12, "H", e->font, white());
		Engine::DrawQuad(v.r + v.b*0.3f + 20, v.g + pos + 64, v.b*0.3f - 20, 16, grey1());
		float dh = ((v.b*0.35f - 1)*Display::height / Display::width) - 1;
		Engine::DrawQuad(v.r + v.b*0.65f, v.g + pos + 35, v.b*0.35f - 1, dh, grey1());
		Engine::DrawQuad(v.r + v.b*0.65f + ((v.b*0.35f - 1)*screenPos.x), v.g + pos + 35 + dh*screenPos.y, (v.b*0.35f - 1)*screenPos.w, dh*screenPos.h, grey2());
		pos += (uint)max(37 + dh, 87.0f);
		UI::Label(v.r + 2, v.g + pos + 1, 12, "Filtering", e->font, white());
		std::vector<string> clearNames = { "None", "Color and Depth", "Depth only", "Skybox" };
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.3f, v.g + pos + 1, v.b * 0.7f - 1, 14, grey2(), clearNames[clearType], 12, e->font, white()) == MOUSE_PRESS) {
			e->RegisterMenu(nullptr, "", clearNames, { &_SetClear0, &_SetClear1, &_SetClear2, &_SetClear3 }, 0, Vec2(v.r + v.b * 0.3f, v.g + pos));
		}
		UI::Label(v.r + 2, v.g + pos + 17, 12, "Target", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE_REND, 12, e->font, &_tarRT, nullptr, this);
		pos += 34;
		uint ess = effects.size();
		UI::Label(v.r + 2, v.g + pos, 12, "Effects: " + std::to_string(ess), e->font, white());
		pos += 17;
		Engine::DrawQuad(v.r + 2, v.g + pos, v.b - 4, 17 * ess + 2.0f, grey1()*0.5f);
		int pendingDel = -1;
		for (uint es = 0; es < ess; es++) {
			if (Engine::EButton(e->editorLayer == 0, v.r + 4, v.g + pos + 1, 16, 16, e->tex_buttonX, white()) == MOUSE_RELEASE) {
				pendingDel = es;
			}
			e->DrawAssetSelector(v.r + 21, v.g + pos + 1, v.b - 24, 16, grey1(), ASSETTYPE_CAMEFFECT, 12, e->font, &_effects[es], nullptr, this);
			pos += 17;
		}
		pos += 3;
		if (Engine::EButton(e->editorLayer == 0, v.r + 2, v.g + pos, 14, 14, e->tex_buttonPlus, white()) == MOUSE_RELEASE) {
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

#ifdef IS_EDITOR
void MeshFilter::SetMesh(int i) {
	_mesh = i;
	if (i >= 0) {
		mesh(_GetCache<Mesh>(ASSETTYPE_MESH, i));
	}
	else
		mesh.clear();
	object->Refresh();
}

void MeshFilter::_UpdateMesh(void* i) {
	MeshFilter* mf = (MeshFilter*)i;
	if (mf->_mesh != -1) {
		mf->mesh(_GetCache<Mesh>(ASSETTYPE_MESH, mf->_mesh));
	}
	else
		mf->mesh.clear();
	mf->object->Refresh();
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
#endif

void MeshFilter::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
#ifdef IS_EDITOR
	//MeshFilter* mft = (MeshFilter*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		UI::Label(v.r + 2, v.g + pos + 20, 12, "Mesh", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_MESH, 12, e->font, &_mesh, &_UpdateMesh, this);
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "Show Bounding Box", e->font, white());
		showBoundingBox = Engine::Toggle(v.r + v.b*0.3f, v.g + pos, 12, e->tex_checkbox, showBoundingBox, white(), ORIENT_HORIZONTAL);
		pos += 17;
	}
	else pos += 17;
#endif
}

void MeshFilter::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_MESH, _mesh);
}

MeshRenderer::MeshRenderer() : Component("Mesh Renderer", COMP_MRD, DRAWORDER_SOLID | DRAWORDER_TRANSPARENT, nullptr, {COMP_MFT}) {
	_materials.push_back(-1);
}

#ifdef IS_EDITOR
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
#endif

void MeshRenderer::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0].raw();
	if (!mf->mesh || !mf->mesh->loaded)
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
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->vertices[0]));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->uv0[0]));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->normals[0]));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->tangents[0]));
	for (uint m = 0; m < mf->mesh->materialCount; m++) {
		if (!materials[m])
			continue;
		if (shader == 0) materials[m]->ApplyGL(m1, m2);
		else glUseProgram(shader);

		glDrawElements(GL_TRIANGLES, mf->mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &(mf->mesh->_matTriangles[m][0]));
	}
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

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

void MeshRenderer::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		MeshFilter* mft = (MeshFilter*)dependacyPointers[0].raw();
		if (!mft->mesh) {
			UI::Label(v.r + 2, v.g + pos + 20, 12, "No Mesh Assigned!", e->font, white());
			pos += 34;
		}
		else {
			UI::Label(v.r + 2, v.g + pos + 18, 12, "Materials: " + std::to_string(mft->mesh->materialCount), e->font, white());
			pos += 35;
			for (uint a = 0; a < mft->mesh->materialCount; a++) {
				UI::Label(v.r + 2, v.g + pos , 12, "Material " + std::to_string(a), e->font, white());
				e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_MATERIAL, 12, e->font, &_materials[a], & _UpdateMat, this);
				pos += 17;
				if (!materials[a])
					continue;
				if (overwriteWriteMask) {
					if (Engine::EButton(e->editorLayer == 0, v.r + 5, v.g + pos, 16, 16, e->tex_expand, white()) == MOUSE_RELEASE) {
						materials[a]->_maskExpanded = !materials[a]->_maskExpanded;
					}
					UI::Label(v.r + 22, v.g + pos, 12, "Write Mask", e->font, white());
					pos += 17;
					if (materials[a]->_maskExpanded) {
						for (uint ea = 0; ea < GBUFFER_NUM_TEXTURES - 1; ea++) {
							materials[a]->writeMask[ea] = Engine::Toggle(v.r + 22, v.g + pos, 16, e->tex_checkbox, materials[a]->writeMask[ea], white(), ORIENT_HORIZONTAL);
							UI::Label(v.r + 43, v.g + pos, 12, Camera::_gbufferNames[ea], e->font, white());
							pos += 17;
						}
					}
				}
				for (uint q = 0, qq = materials[a]->valOrders.size(); q < qq; q++) {
					UI::Texture(v.r + 8, v.g + pos, 16, 16, e->matVarTexs[materials[a]->valOrders[q]]);
					UI::Label(v.r + 25, v.g + pos, 12, materials[a]->valNames[materials[a]->valOrders[q]][materials[a]->valOrderIds[q]], e->font, white());
					void* bbs = materials[a]->vals[materials[a]->valOrders[q]][materials[a]->valOrderGLIds[q]];
					switch (materials[a]->valOrders[q]) {
					case SHADER_INT:
						*(int*)bbs = TryParse(UI::EditText(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.3f, 16, 12, grey1(), std::to_string(*(int*)bbs), e->font, true, nullptr, white()), *(int*)bbs);
						*(int*)bbs = (int)round(Engine::DrawSliderFill(v.r + v.b*0.6f + 23, v.g + pos, v.b*0.4f - 19, 16, 0, 1, (float)(*(int*)bbs), grey2(), white()));
						break;
					case SHADER_FLOAT:
						*(float*)bbs = TryParse(UI::EditText(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.3f, 16, 12, grey1(), std::to_string(*(float*)bbs), e->font, true, nullptr, white()), *(float*)bbs);
						*(float*)bbs = Engine::DrawSliderFill(v.r + v.b*0.6f + 23, v.g + pos, v.b*0.4f - 19, 16, 0, 1, *(float*)bbs, grey2(), white());
						break;
					case SHADER_SAMPLER:
						e->DrawAssetSelector(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.7f - 17, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &((MatVal_Tex*)bbs)->id, _UpdateTex, bbs);
						break;
					}
					pos += 17;
				}
				pos++;
			}
			UI::Label(v.r + 2, v.g + pos, 12, "Override Masks", e->font, white());
			overwriteWriteMask = Engine::Toggle(v.r + v.b*0.3f, v.g + pos, 12, e->tex_checkbox, overwriteWriteMask, white(), ORIENT_HORIZONTAL);
			pos += 17;
		}
	}
	else pos += 17;
}

void MeshRenderer::_UpdateMat(void* i) {
	MeshRenderer* mf = (MeshRenderer*)i;
	for (int q = mf->_materials.size() - 1; q >= 0; q--) {
		mf->materials[q](_GetCache<Material>(ASSETTYPE_MATERIAL, mf->_materials[q]));
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
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0].raw();
	if (!mf || !mf->mesh || !mf->mesh->loaded) {
		materials.clear();
		_materials.clear();
	}
	else {
		materials.resize(mf->mesh->materialCount, nullptr);
		_materials.resize(mf->mesh->materialCount, -1);
	}
}

void TextureRenderer::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	//MeshRenderer* mrd = (MeshRenderer*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		UI::Label(v.r + 2, v.g + pos + 20, 12, "Texture", e->font, white());
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

ComputeShader* SkinnedMeshRenderer::skinningProg = 0;

SkinnedMeshRenderer::SkinnedMeshRenderer(SceneObject* o) : Component("Skinned Mesh Renderer", COMP_SRD, DRAWORDER_SOLID, o) {
	if (!o) {
		Debug::Error("SMR", "SceneObject cannot be null!");
	}
	rSceneObject& par = object->parent;
	while (par) {
		armature = par->GetComponent<Armature>().get();
		if (armature) break;
		else par = par->parent;
	}
	if (!armature) {
		Debug::Error("SkinnedMeshRenderer", "Cannot add Skin to object without armature!");
		dead = true;
	}
}

SkinnedMeshRenderer::SkinnedMeshRenderer(std::ifstream& stream, SceneObject* o, long pos) : Component("Skinned Mesh Renderer", COMP_SRD, DRAWORDER_SOLID, o) {

}

void SkinnedMeshRenderer::mesh(Mesh* m) {
	_mesh = m;
	InitWeights();
}

void SkinnedMeshRenderer::InitWeights() {
	std::vector<uint> noweights;
	weights.clear();
	auto bsz = armature->_allbones.size();
	weights.resize(_mesh->vertexCount);
	std::vector<SkinDats> dats(_mesh->vertexCount);
	for (uint i = 0; i < _mesh->vertexCount; i++) {
		byte a = 0;
		float tot = 0;
		for (auto& g : _mesh->vertexGroupWeights[i]) {
			auto bn = armature->MapBone(_mesh->vertexGroups[g.first]);
			if (!bn) continue;
			weights[i][a].first = bn;
			weights[i][a].second = g.second;
			dats[i].mats[a] = bn->id;
			tot += g.second;
			if (++a == 4) break;
		}
		for (byte b = a; b < 4; b++) {
			weights[i][b].first = armature->_bones[0];
		}
		if (a == 0) {
			noweights.push_back(i);
			weights[i][0].second = 1;
			dats[i].weights[0] = 1;
		}
		else {
			while (a > 0) {
				weights[i][a - 1].second /= tot;
				dats[i].weights[a - 1] = weights[i][a - 1].second;
				a--;
			}
		}
	}

	if (skinBufPoss) {
		delete(skinBufPoss);
		delete(skinBufNrms);
		delete(skinBufPossO);
		delete(skinBufNrmsO);
		delete(skinBufDats);
		delete(skinBufMats);
	}
	skinBufPoss = new ComputeBuffer<Vec4>(_mesh->vertexCount);
	skinBufNrms = new ComputeBuffer<Vec4>(_mesh->vertexCount);
	skinBufPossO = new ComputeBuffer<Vec4>(_mesh->vertexCount);
	skinBufNrmsO = new ComputeBuffer<Vec4>(_mesh->vertexCount);
	skinBufDats = new ComputeBuffer<SkinDats>(_mesh->vertexCount);
	skinBufMats = new ComputeBuffer<Mat4x4>(ARMATURE_MAX_BONES);
	skinBufPoss->Set(&_mesh->vertices[0].x, sizeof(Vec4), sizeof(Vec3));
	skinBufNrms->Set(&_mesh->normals[0].x, sizeof(Vec4), sizeof(Vec3));
	skinBufDats->Set(&dats[0]);
	skinDispatchGroups = (uint)ceilf(((float)_mesh->vertexCount) / SKINNED_THREADS_PER_GROUP);

	if (!!noweights.size()) 
		Debug::Warning("SMR", std::to_string(noweights.size()) + " vertices in \"" + _mesh->name + "\" have no weights assigned!");
}

void SkinnedMeshRenderer::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		UI::Label(v.r + 2, v.g + pos + 20, 12, "Mesh", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_MESH, 12, e->font, &_meshId, &_UpdateMesh, this);
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "Show Bounding Box", e->font, white());
		showBoundingBox = Engine::Toggle(v.r + v.b*0.3f, v.g + pos, 12, e->tex_checkbox, showBoundingBox, white(), ORIENT_HORIZONTAL);
		pos += 17;

		if (!_mesh) {
			UI::Label(v.r + 2, v.g + pos + 20, 12, "No Mesh Assigned!", e->font, white());
			pos += 34;
		}
		else {
			UI::Label(v.r + 2, v.g + pos, 12, "Materials: " + std::to_string(_mesh->materialCount), e->font, white());
			pos += 17;
			for (uint a = 0; a < _mesh->materialCount; a++) {
				UI::Label(v.r + 2, v.g + pos, 12, "Material " + std::to_string(a), e->font, white());
				e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_MATERIAL, 12, e->font, &_materials[a], &_UpdateMat, this);
				pos += 17;
				if (!materials[a])
					continue;
				if (overwriteWriteMask) {
					if (Engine::EButton(e->editorLayer == 0, v.r + 5, v.g + pos, 16, 16, e->tex_expand, white()) == MOUSE_RELEASE) {
						materials[a]->_maskExpanded = !materials[a]->_maskExpanded;
					}
					UI::Label(v.r + 22, v.g + pos, 12, "Write Mask", e->font, white());
					pos += 17;
					if (materials[a]->_maskExpanded) {
						for (uint ea = 0; ea < GBUFFER_NUM_TEXTURES - 1; ea++) {
							materials[a]->writeMask[ea] = Engine::Toggle(v.r + 22, v.g + pos, 16, e->tex_checkbox, materials[a]->writeMask[ea], white(), ORIENT_HORIZONTAL);
							UI::Label(v.r + 43, v.g + pos, 12, Camera::_gbufferNames[ea], e->font, white());
							pos += 17;
						}
					}
				}
				for (uint q = 0, qq = materials[a]->valOrders.size(); q < qq; q++) {
					UI::Texture(v.r + 8, v.g + pos, 16, 16, e->matVarTexs[materials[a]->valOrders[q]]);
					UI::Label(v.r + 25, v.g + pos, 12, materials[a]->valNames[materials[a]->valOrders[q]][materials[a]->valOrderIds[q]], e->font, white());
					void* bbs = materials[a]->vals[materials[a]->valOrders[q]][materials[a]->valOrderGLIds[q]];
					switch (materials[a]->valOrders[q]) {
					case SHADER_INT:
						*(int*)bbs = TryParse(UI::EditText(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.3f, 16, 12, grey1(), std::to_string(*(int*)bbs), e->font, true, nullptr, white()), *(int*)bbs);
						*(int*)bbs = (int)round(Engine::DrawSliderFill(v.r + v.b*0.6f + 23, v.g + pos, v.b*0.4f - 19, 16, 0, 1, (float)(*(int*)bbs), grey2(), white()));
						break;
					case SHADER_FLOAT:
						*(float*)bbs = TryParse(UI::EditText(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.3f, 16, 12, grey1(), std::to_string(*(float*)bbs), e->font, true, nullptr, white()), *(float*)bbs);
						*(float*)bbs = Engine::DrawSliderFill(v.r + v.b*0.6f + 23, v.g + pos, v.b*0.4f - 19, 16, 0, 1, *(float*)bbs, grey2(), white());
						break;
					case SHADER_SAMPLER:
						e->DrawAssetSelector(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.7f - 17, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &((MatVal_Tex*)bbs)->id, _UpdateTex, bbs);
						break;
					}
					pos += 17;
				}
				pos++;
			}
			UI::Label(v.r + 2, v.g + pos, 12, "Override Masks", e->font, white());
			overwriteWriteMask = Engine::Toggle(v.r + v.b*0.3f, v.g + pos, 12, e->tex_checkbox, overwriteWriteMask, white(), ORIENT_HORIZONTAL);
			pos += 17;
		}
	}
	else pos += 17;
}

void SkinnedMeshRenderer::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	if (!_mesh || !_mesh->loaded)
		return;
	bool isE = !!ebv;
	
	Skin();

	glEnableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, (!isE || ebv->selectedShading == 0) ? GL_FILL : GL_LINE);
	if (!isE || ebv->selectedShading == 0) glEnable(GL_CULL_FACE);
	glVertexPointer(3, GL_FLOAT, 0, &(_mesh->vertices[0]));
	glLineWidth(1);
	GLfloat matrix[16], matrix2[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	glGetFloatv(GL_PROJECTION_MATRIX, matrix2);
	Mat4x4 m1(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]);
	Mat4x4 m2(matrix2[0], matrix2[1], matrix2[2], matrix2[3], matrix2[4], matrix2[5], matrix2[6], matrix2[7], matrix2[8], matrix2[9], matrix2[10], matrix2[11], matrix2[12], matrix2[13], matrix2[14], matrix2[15]);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, skinBufPossO->pointer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, sizeof(Vec4), 0);
	glBindBuffer(GL_ARRAY_BUFFER, skinBufNrmsO->pointer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(Vec4), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, 0, &(_mesh->tangents[0]));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &(_mesh->uv0[0]));
	for (uint m = 0; m < _mesh->materialCount; m++) {
		if (!materials[m])
			continue;
		if (shader == 0) materials[m]->ApplyGL(m1, m2);
		else glUseProgram(shader);
		glDrawElements(GL_TRIANGLES, _mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &(_mesh->_matTriangles[m][0]));
	}
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_CULL_FACE);
	if (isE && showBoundingBox) {
		BBox& b = _mesh->boundingBox;
		Engine::DrawCubeLinesW(b.x0, b.x1, b.y0, b.y1, b.z0, b.z1, 1, white(1, 0.5f));
	}
}

void SkinnedMeshRenderer::DrawDeferred(GLuint shader) {
	DrawEditor(nullptr, shader);
}

void SkinnedMeshRenderer::InitSkinning() {
	skinningProg = new ComputeShader(DefaultResources::GetStr("gpuskin.txt"));
}

void SkinnedMeshRenderer::Skin() {
	skinBufMats->Set(&armature->_animMatrices[0][0].x);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, skinBufPoss->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, skinBufNrms->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, skinBufDats->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, skinBufMats->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, skinBufPossO->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, skinBufNrmsO->pointer);
	skinningProg->Dispatch(skinDispatchGroups, 1, 1);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, 0);

}

void SkinnedMeshRenderer::SetMesh(int i) {
	_meshId = i;
	if (i >= 0) {
		_mesh = _GetCache<Mesh>(ASSETTYPE_MESH, i);
		InitWeights();
	}
	else
		_mesh = nullptr;
	if (!_mesh || !_mesh->loaded) {
		materials.clear();
		_materials.clear();
	}
	else {
		materials.resize(_mesh->materialCount, nullptr);
		_materials.resize(_mesh->materialCount, -1);
	}
}

void SkinnedMeshRenderer::_UpdateMesh(void* i) {
	SkinnedMeshRenderer* mf = (SkinnedMeshRenderer*)i;
	if (mf->_meshId != -1) {
		mf->_mesh = _GetCache<Mesh>(ASSETTYPE_MESH, mf->_meshId);
	}
	else
		mf->_mesh = nullptr;
}
void SkinnedMeshRenderer::_UpdateMat(void* i) {
	SkinnedMeshRenderer* mf = (SkinnedMeshRenderer*)i;
	for (int q = mf->_materials.size() - 1; q >= 0; q--) {
		mf->materials[q] = _GetCache<Material>(ASSETTYPE_MATERIAL, mf->_materials[q]);
	}
}
void SkinnedMeshRenderer::_UpdateTex(void* i) {
	MatVal_Tex* v = (MatVal_Tex*)i;
	v->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, v->id);
}


//-----------------Particle System-------------

float ParticleSystemValue::Eval() {
	switch (type) {
	case ParticleSystemValue_Constant:
	case ParticleSystemValue_Curve:
		return val1;
	case ParticleSystemValue_Random:
	case ParticleSystemValue_RandomCurve:
		return Random::Range(val1, val2);
	default:
		return val1;
	}
}

int ParticleSystem::maxParticles() {
	return _maxParticles;
}

void ParticleSystem::maxParticles(int i) {
	particles.resize(i);
	_maxParticles = i;
}

void ParticleSystem::Emit(int n) {
	for (int i = 0; i < n; i++, particleCount++) {
		if (particleCount == maxParticles()) return;

	}
}

void ParticleSystem::Update() {
	clockOffset += Time::delta;
	double c = floor(clockOffset * _clock);
	for (int i = 0; i < c; i++) {

		for (int p = 0; p < particleCount; p++) {

		}

	}
	clockOffset -= c/_clock;
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

void Light::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		if (Engine::EButton(e->editorLayer == 0, v.r, v.g + pos, v.b * 0.33f - 1, 16, (_lightType == LIGHTTYPE_POINT) ? white(1, 0.5f) : grey1(), "Point", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_POINT;
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.33f, v.g + pos, v.b * 0.34f - 1, 16, (_lightType == LIGHTTYPE_DIRECTIONAL) ? white(1, 0.5f) : grey1(), "Directional", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_DIRECTIONAL;
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.67f, v.g + pos, v.b * 0.33f - 1, 16, (_lightType == LIGHTTYPE_SPOT) ? white(1, 0.5f) : grey1(), "Spot", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_SPOT;

		UI::Label(v.r + 2, v.g + pos + 17, 12, "Intensity", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, grey1());
		UI::Label(v.r + v.b * 0.3f + 2, v.g + pos + 17, 12, std::to_string(intensity), e->font, white());
		intensity = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f - 1, 16, 0, 20, intensity, grey1(), white());
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "Color", e->font, white());
		e->DrawColorSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), 12, e->font, &color);
		pos += 17;

		switch (_lightType) {
		case LIGHTTYPE_POINT:
			UI::Label(v.r + 2, v.g + pos, 12, "core radius", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(minDist), e->font, white());
			minDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, maxDist, minDist, grey1(), white());
			pos += 17;
			UI::Label(v.r + 2, v.g + pos, 12, "distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(maxDist), e->font, white());
			maxDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 20, maxDist, grey1(), white());
			pos += 17;
			UI::Label(v.r + 2, v.g + pos, 12, "falloff", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(falloff), e->font, white());
			falloff = (LIGHT_FALLOFF)((int)round(Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 20, (float)falloff, grey1(), white())));
			pos += 17;
			break;
		case LIGHTTYPE_DIRECTIONAL:
			
			break;
		case LIGHTTYPE_SPOT:
			UI::Label(v.r + 2, v.g + pos, 12, "angle", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, std::to_string(angle), e->font, white());
			angle = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 180, angle, grey1(), white());
			pos += 17;
			UI::Label(v.r + 2, v.g + pos, 12, "start distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(minDist), e->font, white());
			minDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0.0001f, maxDist, minDist, grey1(), white());
			pos += 17;
			UI::Label(v.r + 2, v.g + pos, 12, "end distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(maxDist), e->font, white());
			maxDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0.0002f, 50, maxDist, grey1(), white());
			pos += 17;
			break;
		}
		UI::Label(v.r + 2, v.g + pos, 12, "cookie", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_cookie, &_SetCookie, this);
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "cookie strength", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
		UI::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, std::to_string(cookieStrength), e->font, white());
		cookieStrength = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, cookieStrength, grey1(), white());
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "square shape", e->font, white());
		square = Engine::Toggle(v.r + v.b * 0.3f, v.g + pos, 12, e->tex_checkbox, square, white(), ORIENT_HORIZONTAL);
		pos += 17;
		if (_lightType != LIGHTTYPE_DIRECTIONAL) {
			if (Engine::EButton(e->editorLayer == 0, v.r, v.g + pos, v.b * 0.5f - 1, 16, (!drawShadow) ? white(1, 0.5f) : grey1(), "No Shadow", 12, e->font, white()) == MOUSE_RELEASE)
				drawShadow = false;
			if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.5f, v.g + pos, v.b * 0.5f - 1, 16, (drawShadow) ? white(1, 0.5f) : grey1(), "Shadow", 12, e->font, white()) == MOUSE_RELEASE)
				drawShadow = true;
			pos += 17;
			if (drawShadow) {
				UI::Label(v.r + 2, v.g + pos, 12, "shadow bias", e->font, white());
				Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
				UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(shadowBias), e->font, white());
				shadowBias = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 0.02f, shadowBias, grey1(), white());
				pos += 17;
				UI::Label(v.r + 2, v.g + pos, 12, "shadow strength", e->font, white());
				Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
				UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(shadowStrength), e->font, white());
				shadowStrength = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, shadowStrength, grey1(), white());
				pos += 17;

				UI::Label(v.r + 2, v.g + pos, 12, "Contact Shadows", e->font, white());
				contactShadows = Engine::Toggle(v.r + v.b * 0.3f, v.g + pos, 16, e->tex_checkbox, contactShadows, white(), ORIENT_HORIZONTAL);
				pos += 17;
				if (contactShadows) {
					UI::Label(v.r + 2, v.g + pos, 12, "  samples", e->font, white());
					Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
					UI::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, std::to_string(contactShadowSamples), e->font, white());
					contactShadowSamples = (uint)Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 5, 50, (float)contactShadowSamples, grey1(), white());
					pos += 17;
					UI::Label(v.r + 2, v.g + pos, 12, "  distance", e->font, white());
					Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
					UI::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, std::to_string(contactShadowDistance), e->font, white());
					contactShadowDistance = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, contactShadowDistance, grey1(), white());
					pos += 17;
				}
			}
		}
		UI::Label(v.r + 2, v.g + pos, 12, "HSV Map", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_hsvMap, &_SetHsvMap, this);

	}
	else pos += 17;
}

void Light::_SetCookie(void* v) {
	Light* l = (Light*)v;
	l->cookie = _GetCache<Texture>(ASSETTYPE_TEXTURE, l->_cookie);
}
void Light::_SetHsvMap(void* v) {
	Light* l = (Light*)v;
	l->hsvMap = _GetCache<Texture>(ASSETTYPE_TEXTURE, l->_hsvMap);
}

void Light::CalcShadowMatrix() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (_lightType == LIGHTTYPE_SPOT || _lightType == LIGHTTYPE_POINT) {
		Quat q = glm::inverse(object->transform.rotation());
		glMultMatrixf(glm::value_ptr(glm::perspectiveFov(angle * deg2rad, 1024.0f, 1024.0f, minDist, maxDist)));
		glScalef(1, 1, -1);
		glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(q)));
		Vec3 pos = -object->transform.position();
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


std::vector<GLint> ReflectiveQuad::paramLocs = std::vector<GLint>();

ReflectiveQuad::ReflectiveQuad(Texture* tex) : Component("Reflective Quad", COMP_RFQ, DRAWORDER_SOLID | DRAWORDER_LIGHT), texture(tex), size(1, 1), origin(-0.5f, -0.5f), intensity(1) {

}

void ReflectiveQuad::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	Engine::DrawLineW(Vec3(origin.x, origin.y, 0), Vec3(origin.x + size.x, origin.y, 0), yellow(), 2);
	Engine::DrawLineW(Vec3(origin.x, origin.y, 0), Vec3(origin.x, origin.y + size.y, 0), yellow(), 2);
}

void ReflectiveQuad::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Texture", e->font, white());
		e->DrawAssetSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_texture, &_SetTex, this);
		if (_texture == -1) {
			UI::Label(v.r + 2, v.g + pos + 17, 12, "A Texture is required!", e->font, yellow());
			return;
		}
		UI::Label(v.r + 2, v.g + pos + 17, 12, "Intensity", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, grey1());
		UI::Label(v.r + v.b * 0.3f + 2, v.g + pos + 17, 12, std::to_string(intensity), e->font, white());
		intensity = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f - 1, 16, 0, 5, intensity, grey1(), white());
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "Size", e->font, white());
		Engine::EButton((e->editorLayer == 0), v.r + v.b*0.3f, v.g + pos, v.b*0.35f - 1, 16, Vec4(0.4f, 0.2f, 0.2f, 1));
		UI::Label(v.r + v.b*0.33f, v.g + pos, 12, std::to_string(size.x), e->font, white());
		Engine::EButton((e->editorLayer == 0), v.r + v.b*0.65f, v.g + pos, v.b*0.35f - 1, 16, Vec4(0.2f, 0.4f, 0.2f, 1));
		UI::Label(v.r + v.b*0.67f, v.g + pos, 12, std::to_string(size.y), e->font, white());
	}
	else pos += 17;
}

void ReflectiveQuad::_SetTex(void* v) {
	ReflectiveQuad* r = (ReflectiveQuad*)v;
	r->texture = _GetCache<Texture>(ASSETTYPE_TEXTURE, r->_texture);
}

void ReflectiveQuad::Serialize(Editor* e, std::ofstream* stream) {

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

void ReflectionProbe::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		
		UI::Label(v.r + 2, v.g + pos + 20, 12, "Intensity", e->font, white());
		pos += 17;
	}
	else pos += 17;
}

ReflectionProbe::ReflectionProbe(ushort size) : Component("Reflection Probe", COMP_RDP, DRAWORDER_LIGHT), size(size), map(new CubeMap(size)), updateMode(ReflProbe_UpdateMode_Start), intensity(1), clearType(ReflProbe_Clear_Sky), clearColor(), range(1, 1, 1), softness(0) {
	
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

	//map = new CubeMap(size);
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


InverseKinematics::InverseKinematics() : Component("InverseKinematics", COMP_INK) {

}

void InverseKinematics::Apply() {
	auto t = target->transform.position();
	for (byte i = 0; i < iterations; i++) {
		auto& o = object();
		for (byte a = 0; a < length; a++) {
			if (!o->parent) return;

			auto p = object->transform.position();
			auto pp = o->parent->transform.position();

			o = o->parent();

			auto x = Normalize(t - pp);
			auto r = Normalize(p - pp);
			auto dt = Clamp(glm::dot(x, r), -1.0f, 1.0f);

			o->transform.rotation(QuatFunc::FromAxisAngle(glm::cross(r, x), rad2deg*acos(dt)) * o->transform.rotation());
		}
	}
}

void InverseKinematics::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	if (target) Apply();
}

void InverseKinematics::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Target", e->font, white());
		e->DrawObjectSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), 12, e->font, &target);
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Length", e->font, white());
		length = (byte)TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), std::to_string(length), e->font, true, nullptr, white()), 1);
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Iterations", e->font, white());
		iterations = (byte)TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), std::to_string(iterations), e->font, true, nullptr, white()), 1);
	}
	else pos += 17;
}


ArmatureBone::ArmatureBone(uint id, Vec3 pos, Quat rot, Vec3 scl, float lgh, bool conn, Transform* tr, ArmatureBone* par) : 
	id(id), restPosition(pos), restRotation(rot), restScale(scl), restMatrix(MatFunc::FromTRS(pos, rot, scl)),
	restMatrixInv(glm::inverse(restMatrix)), restMatrixAInv(par? restMatrixInv*par->restMatrixAInv : restMatrixInv),
	length(lgh), connected(conn), tr(tr), name(tr->object->name), fullName((par? par->fullName + name : name) + "/"), parent(par) {}
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
const Vec3 ArmatureBone::boneSelCol = Vec3(1, 190.0f/255, 0);

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

void ArmatureBone::Draw(EB_Viewer* ebv) {
	bool sel = (ebv->editor->selected == tr->object);
	glPushMatrix();
	glMultMatrixf(glm::value_ptr(tr->localMatrix()));
	glPushMatrix();
	glScalef(length, length, length);
	glVertexPointer(3, GL_FLOAT, 0, &boneVecs[0]);
	glLineWidth(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (sel) glColor4f(boneSelCol.r, boneSelCol.g, boneSelCol.b, 1);
	else glColor4f(boneCol.r, boneCol.g, boneCol.b, 1);
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, &boneIndices[0]);
	glPopMatrix();

	for (auto a : _children) a->Draw(ebv);
	glPopMatrix();
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
	std::vector<ArmatureBone*>& bnv = (prt)? prt->_children : bones;
	SceneObject* scp = (prt) ? prt->tr->object.raw() : o;
	auto oo = SceneObject::New(nm, pos, rot, Vec3(1.0f));
	ArmatureBone* bn = new ArmatureBone(id++, (prt) ? pos + Vec3(0, 0, 1)*prt->length : pos, rot, Vec3(1.0f), glm::length(tal-pos), (mask & 0xf0) != 0, &oo->transform, prt);
	bnv.push_back(bn);
	scp->AddChild(oo);
	blist.push_back(bn);
	std::cout << nm << " " << scp->name << " " << std::to_string((tal - pos)) << std::to_string(fwd) << std::endl;
	if (prt) {
		//std::cout << " " << std::to_string((tal - pos)) << std::to_string(fwd) << std::endl;
		oo->transform.localRotation(rot);
		oo->transform.localPosition(pos + Vec3(0,0,1)*prt->length);
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

void Armature::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glPushMatrix();
	glMultMatrixf(glm::value_ptr(object->transform.localMatrix()));
	if (xray) {
		glDepthFunc(GL_ALWAYS);
		glDepthMask(false);
	}
	for (auto a : _bones) a->Draw(ebv);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glPopMatrix();
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

void Armature::OnPreRender() {
	Animate();
	UpdateMats();
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

void SceneScript::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	SceneScript* scr = (SceneScript*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		for (auto& p : _vals){
			UI::Label(v.r + 20, v.g + pos + 19, 12, p.first, e->font, white(1, (p.second.first == SCR_VAR_COMMENT)? 0.5f : 1));
			switch (p.second.first) {
			case SCR_VAR_FLOAT:
				Engine::Button(v.r + v.b*0.3f, v.g + pos + 17, v.b*0.7f - 1, 16, grey2());
				UI::Label(v.r + v.b*0.3f + 2, v.g + pos + 19, 12, std::to_string(*(float*)p.second.second), e->font, white());
				break;
			case SCR_VAR_INT:
				Engine::Button(v.r + v.b*0.3f, v.g + pos + 17, v.b*0.7f - 1, 16, grey2());
				UI::Label(v.r + v.b*0.3f + 2, v.g + pos + 19, 12, std::to_string(*(int*)p.second.second), e->font, white());
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
	bool hasClass = false;
	if (!hasClass)
		e->_Error("Script Importer", "SceneScript class for " + s + " not found!");
}



//-----------------transform class-------------------
Transform::_offset_map Transform::_offsets = {};

void Transform::Init(pSceneObject& sc, Vec3 pos, Quat rot, Vec3 scl) {
	object(sc);
	_rotation = rot;
	Translate(pos).Scale(scl);
	_UpdateWEuler();
	_W2LQuat();
	_UpdateLMatrix();
}

void Transform::Init(pSceneObject& sc, byte* data) {
	Transform* tr = (Transform*)data;
	Init(sc, tr->_position, tr->_rotation, tr->_localScale);
}

void Transform::position(const Vec3& r) {
	_position = r;
	_W2LPos();
	_UpdateLMatrix();
}

void Transform::localPosition(const Vec3& r) {
	_localPosition = r;
	_L2WPos();
	_UpdateLMatrix();
}

void Transform::rotation(const Quat& r) {
	_rotation = r;
	_UpdateWEuler();
	_W2LQuat();
	_UpdateLMatrix();
}

void Transform::localRotation(const Quat& r) {
	_localRotation = r;
	_UpdateLEuler();
	_L2WQuat();
	_UpdateLMatrix();
}

void Transform::localScale(const Vec3& r) {
	_localScale = r;
	_UpdateLMatrix();
}

void Transform::eulerRotation(const Vec3& r) {
	_eulerRotation.x = Repeat(r.x, 0.0f, 360.0f);
	_eulerRotation.y = Repeat(r.y, 0.0f, 360.0f);
	_eulerRotation.z = Repeat(r.z, 0.0f, 360.0f);
	_UpdateWQuat();
	_UpdateLMatrix();
}

void Transform::localEulerRotation(const Vec3& r) {
	_localEulerRotation.x = Repeat(r.x, 0.0f, 360.0f);
	_localEulerRotation.y = Repeat(r.y, 0.0f, 360.0f);
	_localEulerRotation.z = Repeat(r.z, 0.0f, 360.0f);
	_UpdateLQuat();
	_UpdateLMatrix();
}

Vec3 Transform::forward() {
	auto v = _worldMatrix*Vec4(0, 0, 1, 0);
	return Vec3(v);
}
Vec3 Transform::right() {
	auto v = _worldMatrix*Vec4(1, 0, 0, 0);
	return Vec3(v);
}
Vec3 Transform::up() {
	auto v = _worldMatrix*Vec4(0, 1, 0, 0);
	return Vec3(v);
}

Vec3 Transform::Local2World(Vec3 vec) {
	auto v = _worldMatrix*Vec4(vec, 1);
	return Vec3(v);
}

Transform& Transform::Translate(Vec3 v, TransformSpace sp) {
	if (sp == Space_Self) {
		_localPosition += v;
		_L2WPos();
	}
	else {
		_position += v;
		_W2LPos();
	}
	_UpdateLMatrix();
	return *this;
}
Transform& Transform::Rotate(Vec3 v, TransformSpace sp) {
	if ((sp == Space_Self) || (!object->parent())) {
		Quat qx = QuatFunc::FromAxisAngle(Vec3(1, 0, 0), v.x);
		Quat qy = QuatFunc::FromAxisAngle(Vec3(0, 1, 0), v.y);
		Quat qz = QuatFunc::FromAxisAngle(Vec3(0, 0, 1), v.z);
		_localRotation = _localRotation * qx * qy * qz;
		_UpdateLEuler();
		_L2WQuat();
	}
	else {
		Mat4x4 im = glm::inverse(object->parent->transform._worldMatrix);
		Vec4 vx = im*Vec4(1, 0, 0, 0);
		Vec4 vy = im*Vec4(0, 1, 0, 0);
		Vec4 vz = im*Vec4(0, 0, 1, 0);
		Quat qx = QuatFunc::FromAxisAngle(vx, v.x);
		Quat qy = QuatFunc::FromAxisAngle(vy, v.y);
		Quat qz = QuatFunc::FromAxisAngle(vz, v.z);
		_rotation = qx * qy * qz * _rotation;
		_UpdateWEuler();
		_W2LQuat();
	}

	_UpdateLMatrix();
	return *this;
}

Transform& Transform::Scale(Vec3 v) {
	_localScale += v;
	_UpdateLMatrix();
	return *this;
}

void Transform::_UpdateWQuat() {
	_rotation = Quat(deg2rad*_eulerRotation);
	_W2LQuat();
}

void Transform::_UpdateLQuat() {
	_localRotation = Quat(deg2rad*_localEulerRotation);
	_L2WQuat();
}

void Transform::_UpdateWEuler() {
	_eulerRotation = QuatFunc::ToEuler(_rotation);
}
void Transform::_UpdateLEuler() {
	_localEulerRotation = QuatFunc::ToEuler(_localRotation);
}

void Transform::_L2WPos() {
	if (!object->parent) _position = _localPosition;
	else {
		auto pos = object->parent->transform._worldMatrix*Vec4(_localPosition, 1);
		_position = Vec3(pos.x, pos.y, pos.z);
	}
}
void Transform::_W2LPos() {
	if (!object->parent) _localPosition = _position;
	else {
		auto pos = glm::inverse(object->parent->transform._worldMatrix)*Vec4(_position, 1);
		_localPosition = Vec3(pos.x, pos.y, pos.z);
	}
}

void Transform::_L2WQuat() {
	if (!object->parent) _rotation = _localRotation;
	else {
		_rotation = object->parent->transform._rotation*_localRotation;
	}
	_UpdateWEuler();
}
void Transform::_W2LQuat() {
	if (!object->parent) _localRotation = _rotation;
	else {
		_localRotation = glm::inverse(object->parent->transform._rotation)*_rotation;
	}
	_UpdateLEuler();
}

void Transform::_UpdateLMatrix() {
	_localMatrix = MatFunc::FromTRS(_localPosition, _localRotation, _localScale);
	_UpdateWMatrix(object->parent ? object->parent->transform._worldMatrix : Mat4x4());
}

void Transform::_UpdateWMatrix(const Mat4x4& mat) {
	_worldMatrix = mat*_localMatrix;

	_L2WPos();
	_L2WQuat();

	for (uint a = 0; a < object->children.size(); a++) //using iterators cause error
		object->children[a]->transform._UpdateWMatrix(_worldMatrix);
}


SceneObject::_offset_map SceneObject::_offsets = {};

SceneObject::SceneObject(Vec3 pos, Quat rot, Vec3 scale) : SceneObject("New Object", pos, rot, scale) {}
SceneObject::SceneObject(string s, Vec3 pos, Quat rot, Vec3 scale) : _expanded(true), Object(s) {
	id = Engine::GetNewId();
}
SceneObject::SceneObject(byte* data) : _expanded(true), Object(*((uint*)data + 1), string((char*)data + SceneObject::_offsets.name)) {}

SceneObject::~SceneObject() {
	
}

void SceneObject::SetActive(bool a, bool enableAll) {
	active = a;
}

bool SO_DoFromId(const std::vector<pSceneObject>& objs, uint id, pSceneObject& res) {
	for (auto a : objs) {
		if (a->id == id) {
			res = a;
			return true;
		}
		if (SO_DoFromId(a->children, id, res)) return true;
	}
	return false;
}

pSceneObject SceneObject::_FromId(const std::vector<pSceneObject>& objs, ulong id) {
	if (!id) return nullptr;
	pSceneObject res = 0;
	SO_DoFromId(objs, id, res);
	return res;
}


pComponent ComponentFromType (COMPONENT_TYPE t){
	switch (t) {
	case COMP_CAM:
		return std::make_shared<Camera>();
	case COMP_MFT:
		return std::make_shared<MeshFilter>();
	default:
		return nullptr;
	}
}

void SceneObject::SetParent(pSceneObject nparent, bool retainLocal) {
	if (parent()) parent->children.erase(std::find_if(parent->children.begin(), parent->children.end(), [this](pSceneObject o)-> bool {return o.get() == this; }));
	parent->childCount--;
	if (nparent) parent->AddChild(pSceneObject(this), retainLocal);
}

pSceneObject SceneObject::AddChild(pSceneObject child, bool retainLocal) {
	Vec3 t;
	Quat r;
	if (!retainLocal) {
		t = child->transform._position;
		r = child->transform._rotation;
	}
	children.push_back(child);
	childCount++;
	child->parent(this);
	child->transform._UpdateWMatrix(transform._worldMatrix);
	if (!retainLocal) {
		child->transform.position(t);
		child->transform.rotation(r);
	}
	return get_shared<SceneObject>(this);
}

pComponent SceneObject::AddComponent(pComponent c) {
	c->object(this);
	int i = 0;
	for (COMPONENT_TYPE t : c->dependancies) {
		c->dependacyPointers[i] = GetComponent(t);
		if (!c->dependacyPointers[i]) {
			c->dependacyPointers[i](AddComponent(ComponentFromType(t)));
		}
		i++;
	}
	for (pComponent cc : _components)
	{
		if ((cc->componentType == c->componentType) && cc->componentType != COMP_SCR) {
			Debug::Warning("Add Component", "Same component already exists!");
			return cc;
		}
	}
	_components.push_back(c);
	_componentCount++;
	return c;
}

pComponent SceneObject::GetComponent(COMPONENT_TYPE type) {
	for (pComponent cc : _components)
	{
		if (cc->componentType == type) {
			return cc;
		}
	}
	return nullptr;
}

void SceneObject::RemoveComponent(pComponent c) {
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
			return;
		}
	}
	Debug::Warning("SceneObject", "component to delete is not found");
}

void SceneObject::Refresh() {
	for (pComponent c : _components) {
		c->Refresh();
	}
}