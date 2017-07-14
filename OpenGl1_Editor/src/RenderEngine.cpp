#include "Engine.h"
#include "Editor.h"
#include <sstream>

void CheckGLOK() {
	int err = glGetError();
	if (err != GL_NO_ERROR) {
		cout << hex << err << endl;
		abort();
	}
}

void DrawSceneObjectsOpaque(vector<SceneObject*> oo) {
	for (SceneObject* sc : oo)
	{
		glPushMatrix();
		Vec3 v = sc->transform.position;
		Vec3 vv = sc->transform.scale;
		Quat vvv = sc->transform.rotation;
		glTranslatef(v.x, v.y, v.z);
		glScalef(vv.x, vv.y, vv.z);
		glMultMatrixf(glm::value_ptr(Quat2Mat(vvv)));
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
	glGenTextures(3, d_texs);
	glGenTextures(1, d_depthTex);

	for (uint i = 0; i < 3; i++) {
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

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, DrawBuffers);
}

void Camera::InitGBuffer() {
	_InitGBuffer(&d_fbo, d_texs, &d_depthTex);
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Camera (" + name + ")", "FB error:" + Status);
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
	string error;
	GLuint vs, fs;
	ifstream strm("D:\\lightPassVert.txt");
	stringstream vert, frag;
	vert << strm.rdbuf();
	if (!ShaderBase::LoadShader(GL_VERTEX_SHADER, vert.str(), vs, &error)) {
		Debug::Error("Previewer", "Cannot init lumi shader(v)! " + error);
	}
	strm.close();
	strm = ifstream("D:\\expVisFrag.txt");
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
		vector<char> program_log(info_log_length);
		glGetProgramInfoLog(lumiProgram, info_log_length, NULL, &program_log[0]);
		cerr << "Lumi shader link error" << endl << &program_log[0] << endl;
		abort();
	}
	glDetachShader(lumiProgram, vs);
	glDetachShader(lumiProgram, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);
}

void EB_Previewer::InitGBuffer() {
	if (d_fbo != 0) {
		glDeleteTextures(3, d_texs);
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
		Debug::Error("Previewer", "Fatal FB (MRT) error:" + Status);
	}
	_InitDummyBBuffer();
	Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Previewer", "Fatal FB (back) error:" + Status);
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	_InitDebugPrograms();
}

void EB_Previewer::Blit(GLuint prog, uint w, uint h) {
	glViewport(0, 0, (int)previewWidth, (int)previewHeight);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bb_fbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, b_fbo);

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, b_fbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, bb_fbo);

	GLint inTexLoc = glGetUniformLocation(prog, "inColor");
	GLint scrSzLoc = glGetUniformLocation(prog, "screenSize");

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, Camera::screenRectVerts);
	glUseProgram(prog);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, Camera::screenRectVerts);

	glUniform2f(scrSzLoc, w, h);
	glUniform1i(inTexLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bb_tex);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glViewport(0, 0, Display::width, Display::height);
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
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, v.r, Display::height - v.g - v.a*0.5f, v.b*0.5f + v.r, Display::height - v.g - EB_HEADER_SIZE - 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_SPEC_GLOSS);
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, v.r + v.b*0.5f + 1, Display::height - v.g - v.a*0.5f, v.b + v.r, Display::height - v.g - EB_HEADER_SIZE - 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_NORMAL);
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, v.r, Display::height - v.g - v.a, v.b + v.r*0.5f, Display::height - v.g - v.a*0.5f - 1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
	else {
		_RenderLights(v);
		glDisable(GL_BLEND);
		if (showLumi) Blit(lumiProgram, (uint)previewWidth, (uint)previewHeight);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, b_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, Display::width, Display::height);
		glBlitFramebuffer(0, 0, (int)previewWidth, (int)previewHeight, v.r, Display::height - v.g - v.a, v.b + v.r, Display::height - v.g - EB_HEADER_SIZE - 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
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

	GLfloat matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, matrix);
	glm::mat4 mat = glm::inverse(glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));

	//_RenderSky(mat); //wont work on ortho, TODO
	//glViewport(v.r, Display::height - v.g - v.a, v.b, v.a - EB_HEADER_SIZE - 2);
	_DrawLights(Scene::active->objects, mat);
	//glViewport(0, 0, Display::width, Display::height);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EB_Previewer::_DrawLights(vector<SceneObject*> oo, glm::mat4 ip) {
	for (SceneObject* o : oo) {
		if (!o->active)
			continue;
		for (Component* c : o->_components) {
			if (c->componentType == COMP_LHT) {
				Light* l = (Light*)c;
				switch (l->_lightType) {
				case LIGHTTYPE_POINT:
					Camera::_DoDrawLight_Point(l, ip, d_texs, d_depthTex, previewWidth, previewHeight, b_fbo);
					break;
				case LIGHTTYPE_SPOT:
					Camera::_DoDrawLight_Spot(l, ip, d_texs, d_depthTex, previewWidth, previewHeight, b_fbo);
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
unordered_map<string, GLuint> Camera::fetchTextures = unordered_map<string, GLuint>();
vector<string> Camera::fetchTexturesUpdated = vector<string>();

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

void Camera::_DoRenderProbeMask(ReflectionProbe* p, glm::mat4& ip) {
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
void Camera::_RenderProbesMask(vector<SceneObject*>& objs, glm::mat4 mat, vector<ReflectionProbe*>& probes) {
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
void Camera::_RenderProbes(vector<ReflectionProbe*>& probes, glm::mat4 mat) {
	for (ReflectionProbe* p : probes) {
		//_DoRenderProbe(p, mat);
	}
}

void Camera::_RenderSky(glm::mat4 ip) {
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

	glUniform2f(scrSzLoc, (GLfloat)Display::width, (GLfloat)Display::height);
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

void Camera::_DoDrawLight_Point(Light* l, glm::mat4& ip, GLuint d_texs[], GLuint d_depthTex, float w, float h, GLuint tar) {
	//draw 6 spotlights
	l->angle = 45;
	_DoDrawLight_Spot(l, ip, d_texs, d_depthTex, w, h, tar);
	l->object->transform.Rotate(0, 0, 90);
	_DoDrawLight_Spot(l, ip, d_texs, d_depthTex, w, h, tar);
	l->object->transform.Rotate(0, 0, 90);
	_DoDrawLight_Spot(l, ip, d_texs, d_depthTex, w, h, tar);
	l->object->transform.Rotate(0, 0, 90);
	_DoDrawLight_Spot(l, ip, d_texs, d_depthTex, w, h, tar);
	l->object->transform.Rotate(0, 90, 90);
	_DoDrawLight_Spot(l, ip, d_texs, d_depthTex, w, h, tar);
	l->object->transform.Rotate(0, 180, 0);
	_DoDrawLight_Spot(l, ip, d_texs, d_depthTex, w, h, tar);
	l->object->transform.Rotate(0, 90, 0);
}

void Camera::_DoDrawLight_Spot(Light* l, glm::mat4& ip, GLuint d_texs[], GLuint d_depthTex, float w, float h, GLuint tar) {
	glm::mat4 lp = glm::mat4();
	if (l->drawShadow) {
		l->DrawShadowMap(tar);
		glViewport(0, 0, (int)w, (int)h); //shadow map modifies viewport
		GLfloat matrix[16];
		glGetFloatv(GL_PROJECTION_MATRIX, matrix);
		lp *= glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]);
	}

	if (d_sLightProgram == 0) {
		Debug::Error("SpotLightPass", "Fatal: Shader not initialized!");
		abort();
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
	GLint _IP = glGetUniformLocation(d_sLightProgram, "_IP");
	GLint diffLoc = glGetUniformLocation(d_sLightProgram, "inColor");
	GLint normLoc = glGetUniformLocation(d_sLightProgram, "inNormal");
	GLint specLoc = glGetUniformLocation(d_sLightProgram, "inSpec");
	GLint depthLoc = glGetUniformLocation(d_sLightProgram, "inDepth");
	GLint scrSzLoc = glGetUniformLocation(d_sLightProgram, "screenSize");
	GLint lPosLoc = glGetUniformLocation(d_sLightProgram, "lightPos");
	GLint lDirLoc = glGetUniformLocation(d_sLightProgram, "lightDir");
	GLint lColLoc = glGetUniformLocation(d_sLightProgram, "lightColor");
	GLint lStrLoc = glGetUniformLocation(d_sLightProgram, "lightStrength");
	GLint lCosLoc = glGetUniformLocation(d_sLightProgram, "lightCosAngle");
	GLint lMinLoc = glGetUniformLocation(d_sLightProgram, "lightMinDist");
	GLint lMaxLoc = glGetUniformLocation(d_sLightProgram, "lightMaxDist");
	GLint lDepLoc = glGetUniformLocation(d_sLightProgram, "lightDepth");
	GLint lDepBaiLoc = glGetUniformLocation(d_sLightProgram, "lightDepthBias");
	GLint lDepStrLoc = glGetUniformLocation(d_sLightProgram, "lightDepthStrength");
	GLint lCookie = glGetUniformLocation(d_sLightProgram, "lightCookie");
	GLint lCookieStr = glGetUniformLocation(d_sLightProgram, "lightCookieStrength");
	GLint lDepMatLoc = glGetUniformLocation(d_sLightProgram, "_LD");

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, screenRectVerts);
	glUseProgram(d_sLightProgram);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, screenRectVerts);

	glUniformMatrix4fv(_IP, 1, GL_FALSE, glm::value_ptr(ip));

	glUniform2f(scrSzLoc, w, h);
	glUniform1i(diffLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_texs[0]);
	glUniform1i(normLoc, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d_texs[1]);
	glUniform1i(specLoc, 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, d_texs[2]);
	glUniform1i(depthLoc, 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glUniform1i(lDepLoc, 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, l->_shadowMap);
	if (l->cookie) {
		glUniform1i(lCookie, 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, l->cookie->pointer);
	}
	Vec3 wpos = l->object->transform.worldPosition();
	glUniform3f(lPosLoc, wpos.x, wpos.y, wpos.z);
	Vec3 dir = l->object->transform.forward();
	glUniform3f(lDirLoc, dir.x, dir.y, dir.z);
	glUniform3f(lColLoc, l->color.x, l->color.y, l->color.z);
	glUniform1f(lStrLoc, l->intensity);
	glUniform1f(lCosLoc, cos(deg2rad*0.5f*l->angle));
	glUniform1f(lMinLoc, l->minDist);
	glUniform1f(lMaxLoc, l->maxDist);
	glUniformMatrix4fv(lDepMatLoc, 1, GL_FALSE, glm::value_ptr(lp));
	glUniform1f(lDepBaiLoc, l->shadowBias);
	glUniform1f(lDepStrLoc, l->shadowStrength);
	glUniform1f(lCookieStr, l->cookie? 1 : 0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Camera::_DrawLights(vector<SceneObject*>& oo, glm::mat4& ip, GLuint targetFbo) {
	for (SceneObject* o : oo) {
		if (!o->active)
			continue;
		for (Component* c : o->_components) {
			if (c->componentType == COMP_LHT) {
				Light* l = (Light*)c;
				switch (l->_lightType) {
				case LIGHTTYPE_POINT:
					_DoDrawLight_Point(l, ip, d_texs, d_depthTex, (float)Display::width, (float)Display::height, targetFbo);
					break;
				case LIGHTTYPE_SPOT:
					_DoDrawLight_Spot(l, ip, d_texs, d_depthTex, (float)Display::width, (float)Display::height, targetFbo);
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
	glm::mat4 mat = glm::inverse(glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));

	//clear alpha here
	vector<ReflectionProbe*> probes;
	_RenderProbesMask(Scene::active->objects, mat, probes);
	_RenderProbes(probes, mat);
	_RenderSky(mat);
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