#include "Engine.h"
#include "Editor.h"

const int Camera::camVertsIds[19] = { 0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 4, 4, 3, 3, 1, 1, 2, 5 };

Camera::Camera() : Component("Camera", COMP_CAM, DRAWORDER_NONE), ortographic(false), fov(60), orthoSize(10), screenPos(0.0f, 0.0f, 1.0f, 1.0f), clearType(CAM_CLEAR_COLOR), clearColor(black(1)), _tarRT(-1), nearClip(0.01f), farClip(500) {
#ifdef IS_EDITOR
	UpdateCamVerts();
#else
	//if (!d_fbo)
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
		glClearDepthf(1);
		glClear(GL_DEPTH_BUFFER_BIT);
		break;
	case CAM_CLEAR_DEPTH:
		glClearDepthf(1);
		glClear(GL_DEPTH_BUFFER_BIT);
		break;
	}

	MVP::Switch(true);
	MVP::Clear();
	Quat q = glm::inverse(object->transform.rotation());
	MVP::Mul(glm::perspectiveFov(fov * deg2rad, (float)Display::width, (float)Display::height, 0.01f, 500.0f));
	MVP::Scale(1, 1, -1);
	MVP::Mul(QuatFunc::ToMatrix(q));
	Vec3 pos = -object->transform.position();
	MVP::Translate(pos.x, pos.y, pos.z);
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

	d_skyProgramLocs[0] = glGetUniformLocation(d_skyProgram, "_IP");
	d_skyProgramLocs[1] = glGetUniformLocation(d_skyProgram, "inColor");
	d_skyProgramLocs[2] = glGetUniformLocation(d_skyProgram, "inNormal");
	d_skyProgramLocs[3] = glGetUniformLocation(d_skyProgram, "inSpec");
	d_skyProgramLocs[4] = glGetUniformLocation(d_skyProgram, "inDepth");
	d_skyProgramLocs[5] = glGetUniformLocation(d_skyProgram, "inSky");
	d_skyProgramLocs[6] = glGetUniformLocation(d_skyProgram, "skyStrength");
	d_skyProgramLocs[7] = glGetUniformLocation(d_skyProgram, "screenSize");
	d_skyProgramLocs[8] = glGetUniformLocation(d_skyProgram, "skyStrengthB");

	glGenBuffers(1, &fullscreenVbo);
	glBindBuffer(GL_ARRAY_BUFFER, fullscreenVbo);
	glBufferStorage(GL_ARRAY_BUFFER, 4 * sizeof(Vec2), &fullscreenVerts[0], GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenVertexArrays(1, &fullscreenVao);
	glBindVertexArray(fullscreenVao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, fullscreenVbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

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

#ifdef IS_EDITOR
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
		cam->fov = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f - 1, 16, 0.1f, 179.9f, cam->fov, grey1(), white());
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
#endif