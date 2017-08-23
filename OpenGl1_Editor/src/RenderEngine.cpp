#include "Engine.h"
#include "Editor.h"
#include <sstream>

void CheckGLOK() {
	int err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cout << std::hex << err << std::endl;
		abort();
	}
}

Mat4x4 GetMatrix(GLenum type) {
	GLfloat matrix[16];
	glGetFloatv(type, matrix);
	return Mat4x4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]);
}

void DrawSceneObjectsOpaque(std::vector<SceneObject*> oo) {
	for (SceneObject* sc : oo)
	{
		glPushMatrix();
		glMultMatrixf(glm::value_ptr(sc->transform.localMatrix()));
		MeshRenderer* mrd = sc->GetComponent<MeshRenderer>();
		if (mrd != nullptr)
			mrd->DrawDeferred();
		DrawSceneObjectsOpaque(sc->children);
		glPopMatrix();
	}
}

void _InitGBuffer(GLuint* d_fbo, GLuint* d_texs, GLuint* d_depthTex, float w = Display::width, float h = Display::height) {
	glGenFramebuffers(1, d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *d_fbo);

	// Create the gbuffer textures
	glGenTextures(4, d_texs);
	glGenTextures(1, d_depthTex);

	for (uint i = 0; i < 4; i++) {
		glBindTexture(GL_TEXTURE_2D, d_texs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (int)w, (int)h, 0, GL_RGBA, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, d_texs[i], 0);
#ifdef SHOW_GBUFFERS
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#else
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
	}
	// depth
	glBindTexture(GL_TEXTURE_2D, *d_depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (int)w, (int)h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *d_depthTex, 0);

#ifdef SHOW_GBUFFERS
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#else
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
	glBindTexture(GL_TEXTURE_2D, 0);
	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, DrawBuffers);
}

void Camera::InitGBuffer() {
	_InitGBuffer(&d_fbo, d_texs, &d_depthTex);
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Camera (" + name + ")", "FB error:" + std::to_string(Status));
	}
	else {
		Debug::Message("Camera (" + name + ")", "FB ok");
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void EB_Previewer::_InitDummyBBuffer() {
	glGenFramebuffers(1, &b_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, b_fbo);

	glGenTextures(2, b_texs);

	glBindTexture(GL_TEXTURE_2D, b_texs[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (int)previewWidth, (int)previewHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, b_texs[0], 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, b_texs[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (int)previewWidth, (int)previewHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, b_texs[1], 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, DrawBuffers);


	glGenFramebuffers(1, &bb_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bb_fbo);

	glGenTextures(1, &bb_tex);
	glBindTexture(GL_TEXTURE_2D, bb_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (int)previewWidth, (int)previewHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bb_tex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum DrawBuffers2[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers2);
}

void EB_Previewer::_InitDebugPrograms() {
	if (lumiProgram == 0) {
		string error;
		GLuint vs, fs;
		std::ifstream strm("D:\\lightPassVert.txt");
		std::stringstream vert, frag;
		vert << strm.rdbuf();
		if (!ShaderBase::LoadShader(GL_VERTEX_SHADER, vert.str(), vs, &error)) {
			Debug::Error("Previewer", "Cannot init lumi shader(v)! " + error);
		}
		strm.close();
		strm = std::ifstream("D:\\expVisFrag.txt");
		frag << strm.rdbuf();
		if (!ShaderBase::LoadShader(GL_FRAGMENT_SHADER, frag.str(), fs, &error)) {
			Debug::Error("Previewer", "Cannot init lumi shader(f)! " + error);
		}
		lumiProgram = glCreateProgram();
		glAttachShader(lumiProgram, vs);
		glAttachShader(lumiProgram, fs);

		int link_result = 0;
		glLinkProgram(lumiProgram);
		glGetProgramiv(lumiProgram, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			int info_log_length = 0;
			glGetProgramiv(lumiProgram, GL_INFO_LOG_LENGTH, &info_log_length);
			std::vector<char> program_log(info_log_length);
			glGetProgramInfoLog(lumiProgram, info_log_length, NULL, &program_log[0]);
			std::cerr << "Lumi shader link error" << std::endl << &program_log[0] << std::endl;
			abort();
		}
		glDetachShader(lumiProgram, vs);
		glDetachShader(lumiProgram, fs);
		glDeleteShader(vs);
		glDeleteShader(fs);
	}
}

void EB_Previewer::InitGBuffer() {
	if (previewWidth < 1 || previewHeight < 1) return;
	if (d_fbo != 0) {
		glDeleteTextures(4, d_texs);
		glDeleteTextures(1, &d_depthTex);
		glDeleteFramebuffers(1, &d_fbo);
		glDeleteTextures(2, b_texs);
		glDeleteFramebuffers(1, &b_fbo);
		glDeleteTextures(1, &bb_tex);
		glDeleteFramebuffers(1, &bb_fbo);
	}
	_InitGBuffer(&d_fbo, d_texs, &d_depthTex, previewWidth, previewHeight);
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Previewer", "Fatal FB (MRT) error:" + to_string(Status));
	}
	_InitDummyBBuffer();
	Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Previewer", "Fatal FB (back) error:" + to_string(Status));
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	_InitDebugPrograms();
}

void EB_Previewer::Blit(GLuint prog, uint w, uint h) {
	glViewport(0, 0, (int)previewWidth, (int)previewHeight);
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blitting2? b_fbo : bb_fbo);
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, blitting2 ? bb_fbo : b_fbo);

	//glReadBuffer(GL_COLOR_ATTACHMENT0);
	//glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blitting2 ? b_fbo : bb_fbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, blitting2 ? bb_fbo : b_fbo);

	GLint inTexLoc = glGetUniformLocation(prog, "inColor");
	GLint scrSzLoc = glGetUniformLocation(prog, "screenSize");

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, Camera::screenRectVerts);
	glUseProgram(prog);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, Camera::screenRectVerts);

	glUniform2f(scrSzLoc, (float)w, (float)h);
	glUniform1i(inTexLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blitting2 ? bb_tex : b_texs[0]);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glViewport(0, 0, Display::width, Display::height);
	blitting2 = !blitting2;
}

void EB_Previewer::DrawPreview(Vec4 v) {
	float zero[] = { 0,0,0,0 };
	float one = 1;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, b_fbo);
	glClearBufferfv(GL_COLOR, 0, zero);
	glClearBufferfv(GL_DEPTH, 0, &one);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);
	glClearBufferfv(GL_COLOR, 0, zero);
	glClearBufferfv(GL_DEPTH, 0, &one);
	glClearBufferfv(GL_COLOR, 1, zero);
	glClearBufferfv(GL_COLOR, 2, zero);
	glClearBufferfv(GL_COLOR, 3, zero);
	//glDrawBuffer(0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//render opaque

	glViewport(0, 0, (int)previewWidth, (int)previewHeight);
	DrawSceneObjectsOpaque(editor->activeScene->objects);

	//*glDisable(GL_DEPTH_TEST);
	//glDepthMask(false);
	if (showBuffers) {
		glViewport(0, 0, Display::width, Display::height);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);

		glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_DIFFUSE);
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, (int)v.r, (int)(Display::height - v.g - v.a*0.5f), (int)(v.b*0.5f + v.r), (int)(Display::height - v.g - EB_HEADER_SIZE - 2), GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_SPEC_GLOSS);
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, (int)(v.r + v.b*0.5f + 1), (int)(Display::height - v.g - v.a*0.5f), (int)(v.b + v.r), (int)(Display::height - v.g - EB_HEADER_SIZE - 2), GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_NORMAL);
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, (int)v.r, (int)(Display::height - v.g - v.a), (int)(v.b*0.5f + v.r), (int)(Display::height - v.g - v.a*0.5f - 1), GL_COLOR_BUFFER_BIT, GL_LINEAR);
		
		glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_EMISSION_AO);
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, (int)(v.r + v.b*0.5f + 1), (int)(Display::height - v.g - v.a), (int)(v.b + v.r), (int)(Display::height - v.g - v.a*0.5f - 1), GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
	else {
		_RenderLights(v);
		glDisable(GL_BLEND);
		blitting2 = false;
		if (showLumi) Blit(lumiProgram, (uint)previewWidth, (uint)previewHeight);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, blitting2? bb_fbo : b_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, Display::width, Display::height);
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, (int)v.r, (int)(Display::height - v.g - v.a), (int)(v.b + v.r), (int)(Display::height - v.g - EB_HEADER_SIZE - 2), GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
}

void EB_Previewer::_RenderLights(Vec4 v) {
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, b_fbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, b_fbo);

	Camera::_ApplyEmission(d_fbo, d_texs, previewWidth, previewHeight, b_fbo);
	Mat4x4 mat = glm::inverse(GetMatrix(GL_PROJECTION_MATRIX));
	Camera::_RenderSky(mat, d_texs, d_depthTex, previewWidth, previewHeight); //wont work well on ortho, will it?
	//glViewport(v.r, Display::height - v.g - v.a, v.b, v.a - EB_HEADER_SIZE - 2);
	_DrawLights(Scene::active->objects, mat);
	//glViewport(0, 0, Display::width, Display::height);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EB_Previewer::_DrawLights(std::vector<SceneObject*> oo, Mat4x4 ip) {
	for (SceneObject* o : oo) {
		if (!o->active)
			continue;
		for (Component* c : o->_components) {
			if (c->componentType == COMP_LHT) {
				Light* l = (Light*)c;
				switch (l->_lightType) {
				case LIGHTTYPE_POINT:
					Camera::_DoDrawLight_Point(l, ip, d_fbo, d_texs, d_depthTex, bb_fbo, bb_tex, previewWidth, previewHeight, b_fbo);
					break;
				case LIGHTTYPE_SPOT:
					Camera::_DoDrawLight_Spot(l, ip, d_fbo, d_texs, d_depthTex, bb_fbo, bb_tex, previewWidth, previewHeight, b_fbo);
					break;
				}
			}
		}

		_DrawLights(o->children, ip);
	}
}

Vec2 Camera::screenRectVerts[] = { Vec2(-1, -1), Vec2(-1, 1), Vec2(1, -1), Vec2(1, 1) };
const int Camera::screenRectIndices[] = { 0, 1, 2, 1, 3, 2 };
GLuint Camera::d_probeMaskProgram = 0;
GLuint Camera::d_probeProgram = 0;
GLuint Camera::d_skyProgram = 0;
GLuint Camera::d_pLightProgram = 0;
GLuint Camera::d_sLightProgram = 0;
GLuint Camera::d_sLightCSProgram = 0;
std::unordered_map<string, GLuint> Camera::fetchTextures = std::unordered_map<string, GLuint>();
std::vector<string> Camera::fetchTexturesUpdated = std::vector<string>();
const string Camera::_gbufferNames[4] = {"Diffuse", "Normal", "Specular-Gloss", "Emission"};

GLuint Camera::DoFetchTexture(string s) {
	if (s == "") {
		ushort a = 0;
		s = "temp_fetch_" + a;
		while (fetchTextures[s] != 0)
			s = "temp_fetch_" + (++a);
	}
	if (fetchTextures[s] == 0) {
		glGenTextures(1, &fetchTextures[s]);
		glBindTexture(GL_TEXTURE_2D, fetchTextures[s]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Display::width, Display::height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	return 0;
}

void Camera::Render(RenderTexture* target) {
	//enable deferred
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);
	//clear
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);
	ApplyGL();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//render opaque
	DrawSceneObjectsOpaque(Scene::active->objects);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

#ifdef SHOW_GBUFFERS
	DumpBuffers();
#else
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	RenderLights();
#endif
}

void Camera::_DoRenderProbeMask(ReflectionProbe* p, Mat4x4& ip) {
	GLint _IP = glGetUniformLocation(d_probeMaskProgram, "_IP");
	GLint scrSzLoc = glGetUniformLocation(d_probeMaskProgram, "screenSize");
	GLint prbPosLoc = glGetUniformLocation(d_probeMaskProgram, "probePos");
	GLint prbRngLoc = glGetUniformLocation(d_probeMaskProgram, "range");
	GLint prbSftLoc = glGetUniformLocation(d_probeMaskProgram, "softness");
	GLint depthLoc = glGetUniformLocation(d_probeMaskProgram, "inDepth");

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, screenRectVerts);
	glUseProgram(d_probeMaskProgram);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, screenRectVerts);

	glUniformMatrix4fv(_IP, 1, GL_FALSE, glm::value_ptr(ip));

	glUniform2f(scrSzLoc, (GLfloat)Display::width, (GLfloat)Display::height);
	Vec3 wpos = p->object->transform.worldPosition();
	glUniform3f(prbPosLoc, wpos.x, wpos.y, wpos.z);
	glUniform3f(prbRngLoc, p->range.x, p->range.y, p->range.z);
	glUniform1f(prbSftLoc, p->softness);

	glUniform1i(depthLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

//mask shaded parts with alpha += influence/4
void Camera::_RenderProbesMask(std::vector<SceneObject*>& objs, Mat4x4 mat, std::vector<ReflectionProbe*>& probes) {
	for (SceneObject* o : objs) {
		if (!o->active)
			continue;
		for (Component* c : o->_components) {
			if (c->componentType == COMP_RDP) {
				probes.push_back((ReflectionProbe*)c);
				_DoRenderProbeMask((ReflectionProbe*)c, mat);
			}
		}
		_RenderProbesMask(o->children, mat, probes);
	}
}

//strength decided from influence / (alpha*4)
void Camera::_RenderProbes(std::vector<ReflectionProbe*>& probes, Mat4x4 mat) {
	for (ReflectionProbe* p : probes) {
		//_DoRenderProbe(p, mat);
	}
}

void Camera::_RenderSky(Mat4x4 ip, GLuint d_texs[], GLuint d_depthTex, float w, float h) {
	if (Scene::active->settings.sky == nullptr || !Scene::active->settings.sky->loaded) return;
	if (d_skyProgram == 0) {
		Debug::Error("SkyLightPass", "Fatal: Shader not initialized!");
		abort();
	}
	GLint _IP = glGetUniformLocation(d_skyProgram, "_IP");
	GLint diffLoc = glGetUniformLocation(d_skyProgram, "inColor");
	GLint specLoc = glGetUniformLocation(d_skyProgram, "inNormal");
	GLint normLoc = glGetUniformLocation(d_skyProgram, "inSpec");
	GLint depthLoc = glGetUniformLocation(d_skyProgram, "inDepth");
	GLint skyLoc = glGetUniformLocation(d_skyProgram, "inSky");
	GLint skyStrLoc = glGetUniformLocation(d_skyProgram, "skyStrength");
	GLint scrSzLoc = glGetUniformLocation(d_skyProgram, "screenSize");

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, screenRectVerts);
	glUseProgram(d_skyProgram);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, screenRectVerts);

	glUniformMatrix4fv(_IP, 1, GL_FALSE, glm::value_ptr(ip));

	glUniform2f(scrSzLoc, w, h);
	glUniform1i(diffLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_texs[0]);
	glUniform1i(specLoc, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d_texs[1]);
	glUniform1i(normLoc, 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, d_texs[2]);
	glUniform1i(depthLoc, 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glUniform1i(skyLoc, 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, Scene::active->settings.sky->pointer);
	glUniform1f(skyStrLoc, Scene::active->settings.skyStrength);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Light::ScanParams() {
	paramLocs_Spot.clear();
#define PBSL paramLocs_Spot.push_back(glGetUniformLocation(Camera::d_sLightProgram,
	PBSL "_IP"));
	PBSL "inColor"));
	PBSL "inNormal"));
	PBSL "inSpec"));
	PBSL "inDepth"));
	PBSL "screenSize"));
	PBSL "lightPos"));
	PBSL "lightDir"));
	PBSL "lightColor"));
	PBSL "lightStrength"));
	PBSL "lightCosAngle"));
	PBSL "lightMinDist"));
	PBSL "lightMaxDist"));
	PBSL "lightDepth"));
	PBSL "lightDepthBias"));
	PBSL "lightDepthStrength"));
	PBSL "lightCookie"));
	PBSL "lightCookieStrength"));
	PBSL "lightIsSquare"));
	PBSL "_LD"));
	PBSL "lightContShad"));
	PBSL "lightContShadStrength"));
	PBSL "inEmi"));
#undef PBSL
	paramLocs_SpotCS.clear();
#define PBSL paramLocs_SpotCS.push_back(glGetUniformLocation(Camera::d_sLightCSProgram,
	PBSL "_P"));
	PBSL "screenSize"));
	PBSL "inDepth"));
	PBSL "sampleCount"));
	PBSL "sampleLength"));
	PBSL "lightPos"));
#undef PBSL
}

void Camera::_ApplyEmission(GLuint d_fbo, GLuint d_texs[], float w, float h, GLuint targetFbo) {
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_EMISSION_AO);
	glBlitFramebuffer(0, 0, (int)w, (int)h, 0, 0, (int)w, (int)h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}


void Camera::_DoDrawLight_Point(Light* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w, float h, GLuint tar) {
	//draw 6 spotlights
	return;
#define DSL _DoDrawLight_Spot(l, ip, d_fbo, d_texs, d_depthTex, ctar, c_tex, w, h, tar);
#define RTT l->object->transform.Rotate(0, 0, 90);
	l->angle = 45;
	DSL RTT
	DSL RTT
	DSL RTT
	DSL RTT
	DSL RTT
	DSL RTT
#undef DSL
#undef RTT
}

void Camera::_DoDrawLight_Spot_Contact(Light* l, Mat4x4& p, GLuint d_depthTex, float w, float h, GLuint src, GLuint tar) {
	if (d_sLightCSProgram == 0) {
		Debug::Error("SpotLightCSPass", "Fatal: Shader not initialized!");
		abort();
	}

	//glDisable(GL_BLEND);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);

	float zero[] = { 0, 0, 0, 0 };
	glClearBufferfv(GL_COLOR, 0, zero);

#define sloc l->paramLocs_SpotCS
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, screenRectVerts);
	glUseProgram(d_sLightCSProgram);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, screenRectVerts);

	glUniformMatrix4fv(sloc[0], 1, GL_FALSE, glm::value_ptr(p));

	glUniform2f(sloc[1], w, h);
	glUniform1i(sloc[2], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glUniform1i(sloc[3], (GLint)l->contactShadowSamples);
	glUniform1f(sloc[4], l->contactShadowDistance);
	Vec3 wpos = l->object->transform.worldPosition();
	Vec4 wpos2 = p*Vec4(wpos.x, wpos.y, wpos.z, 1);
	wpos2 /= wpos2.w;
	glUniform3f(sloc[5], wpos2.x, wpos2.y, wpos2.z);
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenRectIndices);

	//glEnable(GL_BLEND);
	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Camera::_DoDrawLight_Spot(Light* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w, float h, GLuint tar) {
	if (l->maxDist <= 0 || l->angle <= 0 || l->intensity <= 0) return;
	if (l->minDist <= 0) l->minDist = 0.00001f;
	if (l->drawShadow) {
		l->DrawShadowMap(tar);
		glViewport(0, 0, (int)w, (int)h); //shadow map modifies viewport
	}
	Mat4x4 lp = GetMatrix(GL_PROJECTION_MATRIX);
	
	if (d_sLightProgram == 0) {
		Debug::Error("SpotLightPass", "Fatal: Shader not initialized!");
		abort();
	}

	if (l->drawShadow && l->contactShadows) _DoDrawLight_Spot_Contact(l, glm::inverse(ip), d_depthTex, w, h, d_fbo, ctar);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
#define sloc l->paramLocs_Spot
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, screenRectVerts);
	glUseProgram(d_sLightProgram);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, screenRectVerts);

	glUniformMatrix4fv(sloc[0], 1, GL_FALSE, glm::value_ptr(ip));

	glUniform1i(sloc[1], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_texs[0]);
	glUniform1i(sloc[2], 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d_texs[1]);
	glUniform1i(sloc[3], 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, d_texs[2]);
	glUniform1i(sloc[4], 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glUniform2f(sloc[5], w, h);
	Vec3 wpos = l->object->transform.worldPosition();
	glUniform3f(sloc[6], wpos.x, wpos.y, wpos.z);
	Vec3 dir = l->object->transform.forward();
	glUniform3f(sloc[7], dir.x, dir.y, dir.z);
	glUniform3f(sloc[8], l->color.x, l->color.y, l->color.z);
	glUniform1f(sloc[9], l->intensity);
	glUniform1f(sloc[10], cos(deg2rad*0.5f*l->angle));
	glUniform1f(sloc[11], l->minDist);
	glUniform1f(sloc[12], l->maxDist);
	if (l->drawShadow) {
		glUniform1i(sloc[13], 4);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, l->_shadowMap);
		glUniform1f(sloc[14], l->shadowBias);
	}
	glUniform1f(sloc[15], l->drawShadow? l->shadowStrength : 0);
	if (l->cookie) {
		glUniform1i(sloc[16], 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, l->cookie->pointer);
	}
	glUniform1f(sloc[17], l->cookie ? l->cookieStrength : 0);
	glUniform1f(sloc[18], l->square ? 1.0f : 0.0f);
	glUniformMatrix4fv(sloc[19], 1, GL_FALSE, glm::value_ptr(lp));

	if (l->drawShadow && l->contactShadows) {
		glUniform1i(sloc[20], 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, c_tex);
	}
	glUniform1f(sloc[21], (l->drawShadow && l->contactShadows) ? 1.0f : 0.0f);
	glUniform1i(sloc[22], 7);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, d_texs[3]);
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Camera::_DrawLights(std::vector<SceneObject*>& oo, Mat4x4& ip, GLuint targetFbo) {
	for (SceneObject* o : oo) {
		if (!o->active)
			continue;
		for (Component* c : o->_components) {
			if (c->componentType == COMP_LHT) {
				Light* l = (Light*)c;
				switch (l->_lightType) {
				case LIGHTTYPE_POINT:
					_DoDrawLight_Point(l, ip, d_fbo, d_texs, d_depthTex, 0, 0, (float)Display::width, (float)Display::height, targetFbo);
					break;
				case LIGHTTYPE_SPOT:
					_DoDrawLight_Spot(l, ip, d_fbo, d_texs, d_depthTex, 0, 0, (float)Display::width, (float)Display::height, targetFbo);
					break;
				}
			}
		}

		_DrawLights(o->children, ip);
	}
}

void Camera::RenderLights(GLuint targetFbo) {
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, matrix);
	Mat4x4 mat = glm::inverse(Mat4x4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));

	//clear alpha here
	std::vector<ReflectionProbe*> probes;
	_RenderProbesMask(Scene::active->objects, mat, probes);
	_RenderProbes(probes, mat);
	_RenderSky(mat, d_texs, d_depthTex);
	_DrawLights(Scene::active->objects, mat, targetFbo);
}

void Camera::DumpBuffers() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLsizei HalfWidth = (GLsizei)(Display::width / 2.0f);
	GLsizei HalfHeight = (GLsizei)(Display::height / 2.0f);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_DIFFUSE);
	glBlitFramebuffer(0, 0, Display::width, Display::height, 0, HalfHeight, HalfWidth, Display::height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_NORMAL);
	glBlitFramebuffer(0, 0, Display::width, Display::height, HalfWidth, HalfHeight, Display::width, Display::height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_SPEC_GLOSS);
	glBlitFramebuffer(0, 0, Display::width, Display::height, 0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

GLuint Light::_shadowFbo = 0;
GLuint Light::_shadowMap = 0;
std::vector<GLint> Light::paramLocs_Spot = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_SpotCS = std::vector<GLint>();

void Light::InitShadow() {
	glGenFramebuffers(1, &_shadowFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shadowFbo);

	glGenTextures(1, &_shadowMap);

	// depth
	glBindTexture(GL_TEXTURE_2D, _shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowMap, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("ShadowMap", "FB error:" + Status);
		abort();
	}
	else {
		Debug::Message("ShadowMap", "FB ok");
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Light::DrawShadowMap(GLuint tar) {
	if (_shadowFbo == 0) {
		Debug::Error("RenderShadow", "Fatal: Fbo not set up!");
		abort();
	}
	glViewport(0, 0, 1024, 1024);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tar);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shadowFbo);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);
	//glClearColor(1, 0, 0, 0);
	//glClear(GL_COLOR_BUFFER_BIT);
	//glClearDepth(1);
	//glClear(GL_DEPTH_BUFFER_BIT);
	float zero[] = { 0,0,0,0 };
	float one = 1;
	glClearBufferfv(GL_COLOR, 0, zero);
	glClearBufferfv(GL_DEPTH, 0, &one);
	//switch (_lightType) {
	//case LIGHTTYPE_SPOT:
	//case LIGHTTYPE_DIRECTIONAL:
		CalcShadowMatrix();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		DrawSceneObjectsOpaque(Scene::active->objects);
	//	break;
	//}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
	//glViewport(0, 0, Display::width, Display::height);
}

void ReflectionProbe::_DoUpdate() {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mipFbos[0]);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);
	for (byte a = 0; a < 6; a++) {
		glReadBuffer(GL_COLOR_ATTACHMENT0 + a);
		//clear
		//ApplyGL();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		//render opaque
		DrawSceneObjectsOpaque(Scene::active->objects);
	}
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
}