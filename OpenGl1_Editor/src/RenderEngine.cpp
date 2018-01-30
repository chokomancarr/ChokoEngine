#include "Engine.h"
#include "Editor.h"
#include <random>

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

void Camera::DrawSceneObjectsOpaque(std::vector<pSceneObject> oo, GLuint shader) {
	for (auto& sc : oo)
	{
		glPushMatrix();
		glMultMatrixf(glm::value_ptr(sc->transform.localMatrix()));
		auto& mrd = sc->GetComponent<MeshRenderer>();
		if (mrd) {
			//Debug::Message("Cam", "Drawing " + sc->name);
			mrd->DrawDeferred(shader);
		}
		auto& smd = sc->GetComponent<SkinnedMeshRenderer>();
		if (smd) {
			//Debug::Message("Cam", "Drawing " + sc->name);
			smd->DrawDeferred(shader);
		}
		Camera::DrawSceneObjectsOpaque(sc->children, shader);
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
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, DrawBuffers);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}


RenderTexture::RenderTexture(uint w, uint h, RT_FLAGS flags, const GLvoid* pixels, GLenum pixelFormat, GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT) : Texture(), depth(!!(flags & RT_FLAG_DEPTH)), stencil(!!(flags & RT_FLAG_STENCIL)), hdr(!!(flags & RT_FLAG_HDR)) {
	width = w;
	height = h;
	_texType = TEX_TYPE_RENDERTEXTURE;

	glGenFramebuffers(1, &d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);

	glGenTextures(1, &pointer);

	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, hdr? GL_RGBA32F : GL_RGBA, w, h, 0, pixelFormat, hdr? GL_FLOAT : GL_UNSIGNED_BYTE, pixels);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pointer, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("RenderTexture", "FB error:" + std::to_string(Status));
	}
	else loaded = true;
}

RenderTexture::~RenderTexture() {
	glDeleteFramebuffers(1, &d_fbo);
}

void RenderTexture::Blit(Texture* src, RenderTexture* dst, Shader* shd, string texName) {
	if (src == nullptr || dst == nullptr || shd == nullptr) {
		Debug::Warning("Blit", "Parameter is null!");
		return;
	}
	Blit(src->pointer, dst, shd->pointer, texName);
}

void RenderTexture::Blit(GLuint src, RenderTexture* dst, GLuint shd, string texName) {
	glViewport(0, 0, dst->width, dst->height);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->d_fbo);
	float zero[] = { 0,0,1,1 };
	glClearBufferfv(GL_COLOR, 0, zero);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glDisable(GL_BLEND);

	glUseProgram(shd);
	GLint loc = glGetUniformLocation(shd, "mainTex");
	glUniform1i(loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, src);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, &Camera::screenRectVerts[0]);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, &Camera::screenRectVerts[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &Camera::screenRectIndices[0]);

	glDisableVertexAttribArray(0);
	glUseProgram(0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void RenderTexture::Blit(Texture* src, RenderTexture* dst, Material* mat, string texName) {
	glViewport(0, 0, dst->width, dst->height);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->d_fbo);
	float zero[] = { 0,0,0,0 };
	glClearBufferfv(GL_COLOR, 0, zero);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glDisable(GL_BLEND);

	mat->SetTexture(texName, src);
	mat->ApplyGL(Mat4x4(), Mat4x4());

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, &Camera::screenRectVerts[0]);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, &Camera::screenRectVerts[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &Camera::screenRectIndices[0]);

	glDisableVertexAttribArray(0);
	glUseProgram(0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

/*
std::vector<float> RenderTexture::pixels() {
	std::vector<float> v = std::vector<float>(width*height * 3);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, &v[0]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	return v;
}
*/

void RenderTexture::Load(string path) {
	throw std::runtime_error("RT Load (s) not implemented");
}
void RenderTexture::Load(std::istream& strm) {
	throw std::runtime_error("RT Load (i) not implemented");
}

bool RenderTexture::Parse(string path) {
	string ss(path + ".meta");
	std::ofstream str(ss, std::ios::out | std::ios::trunc | std::ios::binary);
	str << "IMR";
	return true;
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
		if (!Shader::LoadShader(GL_VERTEX_SHADER, vert.str(), vs, &error)) {
			Debug::Error("Previewer", "Cannot init lumi shader(v)! " + error);
		}
		strm.close();
		strm = std::ifstream("D:\\expVisFrag.txt");
		frag << strm.rdbuf();
		if (!Shader::LoadShader(GL_FRAGMENT_SHADER, frag.str(), fs, &error)) {
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
		Debug::Error("Previewer", "Fatal FB (MRT) error:" + std::to_string(Status));
	}
	_InitDummyBBuffer();
	Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Previewer", "Fatal FB (back) error:" + std::to_string(Status));
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
	Camera::DrawSceneObjectsOpaque(editor->activeScene->objects);

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

void EB_Previewer::_DrawLights(std::vector<pSceneObject> oo, Mat4x4 ip) {
	for (auto& o : oo) {
		if (!o->active)
			continue;
		for (auto& c : o->_components) {
			if (c->componentType == COMP_LHT) {
				Light* l = (Light*)c.get();
				switch (l->_lightType) {
				case LIGHTTYPE_POINT:
					Camera::_DoDrawLight_Point(l, ip, d_fbo, d_texs, d_depthTex, bb_fbo, bb_tex, previewWidth, previewHeight, b_fbo);
					break;
				case LIGHTTYPE_SPOT:
					Camera::_DoDrawLight_Spot(l, ip, d_fbo, d_texs, d_depthTex, bb_fbo, bb_tex, previewWidth, previewHeight, b_fbo);
					break;
				}
			}
			else if (c->componentType == COMP_RFQ) {
				Camera::_DoDrawLight_ReflQuad((ReflectiveQuad*)c.get(), ip, d_fbo, d_texs, d_depthTex, bb_fbo, bb_tex, previewWidth, previewHeight, b_fbo);
			}
		}

		_DrawLights(o->children, ip);
	}
}

Vec2 Camera::screenRectVerts[] = { Vec2(-1, -1), Vec2(-1, 1), Vec2(1, -1), Vec2(1, 1) };
const int Camera::screenRectIndices[] = { 0, 1, 2, 1, 3, 2 };
GLuint Camera::d_probeMaskProgram = 0;
GLuint Camera::d_probeProgram = 0;
GLuint Camera::d_blurProgram = 0;
GLuint Camera::d_blurSBProgram = 0;
GLuint Camera::d_skyProgram = 0;
GLuint Camera::d_pLightProgram = 0;
GLuint Camera::d_sLightProgram = 0;
GLuint Camera::d_sLightCSProgram = 0;
GLuint Camera::d_sLightRSMProgram = 0;
GLuint Camera::d_sLightRSMFluxProgram = 0;
GLuint Camera::d_reflQuadProgram = 0;
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
	float zero[] = { 0,0,0,0 };
	float one = 1;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);
	glClearBufferfv(GL_COLOR, 0, zero);
	glClearBufferfv(GL_DEPTH, 0, &one);
	glClearBufferfv(GL_COLOR, 1, zero);
	glClearBufferfv(GL_COLOR, 2, zero);
	glClearBufferfv(GL_COLOR, 3, zero);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);
	ApplyGL();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	DrawSceneObjectsOpaque(Scene::active->objects);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

//#ifdef SHOW_GBUFFERS
	//DumpBuffers();
//#else
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	RenderLights();
//#endif
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
	Vec3 wpos = p->object->transform.position();
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
void Camera::_RenderProbesMask(std::vector<pSceneObject>& objs, Mat4x4 mat, std::vector<ReflectionProbe*>& probes) {
	for (auto& o : objs) {
		if (!o->active)
			continue;
		for (auto& c : o->_components) {
			if (c->componentType == COMP_RDP) {
				ReflectionProbe* cc = (ReflectionProbe*)c.get();
				probes.push_back(cc);
				_DoRenderProbeMask(cc, mat);
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
	paramLocs_Point.clear();
#define PBSL paramLocs_Point.push_back(glGetUniformLocation(Camera::d_pLightProgram,
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
	PBSL "lightMinDist"));
	PBSL "lightMaxDist"));
	PBSL "lightCookie"));
	PBSL "lightCookieStrength"));
	PBSL "lightDepth"));
	PBSL "lightDepthBias"));
	PBSL "lightDepthStrength"));
	PBSL "lightFalloffType"));
	PBSL "useHsvMap"));
	PBSL "hsvMap"));
#undef PBSL
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
	PBSL "lightFalloffType"));
	PBSL "useHsvMap"));
	PBSL "hsvMap"));
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
	paramLocs_SpotFluxer.clear();
#define PBSL paramLocs_SpotFluxer.push_back(glGetUniformLocation(Camera::d_sLightRSMFluxProgram,
	PBSL "screenSize"));
	PBSL "inColor"));
	PBSL "inNormal"));
	PBSL "lightDir"));
	PBSL "lightColor"));
	PBSL "lightStrength"));
	PBSL "lightIsSquare"));
#undef PBSL
	paramLocs_SpotRSM.clear();
	paramLocs_SpotRSM.push_back((GLint)glGetUniformBlockIndex(Camera::d_sLightRSMProgram, "SampleBuffer"));
#define PBSL paramLocs_SpotRSM.push_back(glGetUniformLocation(Camera::d_sLightRSMProgram,
	PBSL "_IP"));
	PBSL "_LP"));
	PBSL "_ILP"));
	PBSL "screenSize"));
	PBSL "inColor"));
	PBSL "inNormal"));
	PBSL "inSpec"));
	PBSL "inDepth"));
	PBSL "lightFlux"));
	PBSL "lightNormal"));
	PBSL "lightDepth"));
	PBSL "bufferRadius"));
#undef PBSL
}

void ReflectiveQuad::ScanParams() {
	paramLocs.clear();
#define PBSL paramLocs.push_back(glGetUniformLocation(Camera::d_reflQuadProgram,
	PBSL "_IP"));
	PBSL "inColor"));
	PBSL "inNormal"));
	PBSL "inSpec"));
	PBSL "inDepth"));
	PBSL "screenSize"));
	PBSL "mesh_u"));
	PBSL "mesh_v"));
	PBSL "meshPos"));
	PBSL "meshTex"));
	PBSL "intensity"));
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
	if (l->maxDist <= 0 || l->angle <= 0 || l->intensity <= 0) return;
	if (l->minDist <= 0) l->minDist = 0.00001f;

	if (d_pLightProgram == 0) {
		Debug::Error("SpotLightPass", "Fatal: Shader not initialized!");
		abort();
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
#define sloc l->paramLocs_Point
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, screenRectVerts);
	glUseProgram(d_pLightProgram);

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
	Vec3 wpos = l->object->transform.position();
	glUniform3f(sloc[6], wpos.x, wpos.y, wpos.z);
	Vec3 dir = l->object->transform.forward();
	glUniform3f(sloc[7], dir.x, dir.y, dir.z);
	glUniform3f(sloc[8], l->color.x, l->color.y, l->color.z);
	glUniform1f(sloc[9], l->intensity);
	glUniform1f(sloc[10], l->minDist);
	glUniform1f(sloc[11], l->maxDist);
	/*
	if (l->cookie) {
		glUniform1i(sloc[16], 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, l->cookie->pointer);
	}
	glUniform1f(sloc[17], l->cookie ? l->cookieStrength : 0);
	*/
	glUniform1i(sloc[17], (int)l->falloff);

	glUniform1f(sloc[18], l->hsvMap? 1.0f : 0.0f);
	if (l->hsvMap) {
		glUniform1i(sloc[19], 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, l->hsvMap->pointer);
	}
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
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
	Vec3 wpos = l->object->transform.position();
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
	l->CalcShadowMatrix();
	if (l->drawShadow) {
		l->DrawShadowMap(tar);
		glViewport(0, 0, (int)w, (int)h); //shadow map modifies viewport
	}
	Mat4x4 lp = GetMatrix(GL_PROJECTION_MATRIX);
	
	if (d_sLightProgram == 0) {
		Debug::Error("SpotLightPass", "Fatal: Shader not initialized!");
		abort();
	}

	/*
	glBindFramebuffer(GL_READ_FRAMEBUFFER, l->_fluxFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, 512, 512, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	return;
	*/

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
	Vec3 wpos = l->object->transform.position();
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

	glUniform1f(sloc[24], l->hsvMap ? 1.0f : 0.0f);
	if (l->hsvMap) {
		glUniform1i(sloc[25], 8);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, l->hsvMap->pointer);
	}
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenRectIndices);

	if (Scene::active->settings.GIType == GITYPE_RSM && Scene::active->settings.rsmRadius > 0)
		l->DrawRSM(ip, lp, w, h, d_texs, d_depthTex);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Camera::_DoDrawLight_ReflQuad(ReflectiveQuad* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w, float h, GLuint tar) {
	if (l->intensity <= 0 || l->texture == nullptr) return;

	if (d_reflQuadProgram == 0) {
		Debug::Error("ReflQuadPass", "Fatal: Shader not initialized!");
		abort();
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
#define sloc l->paramLocs
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, screenRectVerts);
	glUseProgram(d_reflQuadProgram);

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
	Vec3 wp = l->object->transform.position();
	Vec3 wf = l->object->transform.right() * l->object->transform.localScale().x;
	Vec3 wu = l->object->transform.up() * l->object->transform.localScale().y;
	Vec3 pos = wp + wf*l->origin.x + wu*l->origin.y;
	wf *= l->size.x;
	wu *= l->size.y;
	glUniform3f(sloc[6], wf.x, wf.y, wf.z);
	glUniform3f(sloc[7], wu.x, wu.y, wu.z);
	glUniform3f(sloc[8], pos.x, pos.y, pos.z);
	glUniform1i(sloc[9], 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, l->texture->pointer);
	glUniform1f(sloc[10], l->intensity);
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Camera::_DrawLights(std::vector<pSceneObject>& oo, Mat4x4& ip, GLuint targetFbo) {
	for (auto& o : oo) {
		if (!o->active)
			continue;
		for (auto c : o->_components) {
			if (c->componentType == COMP_LHT) {
				Light* l = (Light*)c.get();
				switch (l->_lightType) {
				case LIGHTTYPE_POINT:
					_DoDrawLight_Point(l, ip, d_fbo, d_texs, d_depthTex, 0, 0, (float)Display::width, (float)Display::height, targetFbo);
					break;
				case LIGHTTYPE_SPOT:
					_DoDrawLight_Spot(l, ip, d_fbo, d_texs, d_depthTex, 0, 0, (float)Display::width, (float)Display::height, targetFbo);
					break;
				}
			}
			else if (c->componentType == COMP_RFQ) {
				_DoDrawLight_ReflQuad((ReflectiveQuad*)c.get(), ip, d_fbo, d_texs, d_depthTex, 0, 0, (float)Display::width, (float)Display::height, targetFbo);
			}
		}

		_DrawLights(o->children, ip);
	}
}

void Camera::RenderLights(GLuint targetFbo) {
	/*
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, matrix);
	Mat4x4 mat = glm::inverse(Mat4x4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));

	//clear alpha here
	//std::vector<ReflectionProbe*> probes;
	//_RenderProbesMask(Scene::active->objects, mat, probes);
	//_RenderProbes(probes, mat);
	_RenderSky(mat, d_texs, d_depthTex);
	_DrawLights(Scene::active->objects, mat, targetFbo);
	/*/
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, targetFbo);

	_ApplyEmission(d_fbo, d_texs, (float)Display::width, (float)Display::height, targetFbo);
	Mat4x4 mat = glm::inverse(GetMatrix(GL_PROJECTION_MATRIX));
	_RenderSky(mat, d_texs, d_depthTex); //wont work well on ortho, will it?
	//glViewport(v.r, Display::height - v.g - v.a, v.b, v.a - EB_HEADER_SIZE - 2);
	_DrawLights(Scene::active->objects, mat);
	//glViewport(0, 0, Display::width, Display::height);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//*/
}

void Camera::DumpBuffers() {
	glViewport(0, 0, Display::width, Display::height);

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
	
	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_EMISSION_AO);
	glBlitFramebuffer(0, 0, Display::width, Display::height, HalfWidth, 0, Display::width, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

GLuint Light::_shadowFbo = 0;
GLuint Light::_shadowGITexs[] = {0, 0, 0};
GLuint Light::_shadowMap = 0;
GLuint Light::_shadowCubeFbos[] = { 0, 0, 0, 0, 0, 0 };
GLuint Light::_shadowCubeMap = 0;
GLuint Light::_fluxFbo = 0;
GLuint Light::_fluxTex = 0;
GLuint Light::_rsmFbo = 0;
GLuint Light::_rsmTex = 0;
GLuint Light::_rsmUBO = 0;
RSM_RANDOM_BUFFER Light::_rsmBuffer = RSM_RANDOM_BUFFER();
std::vector<GLint> Light::paramLocs_Point = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_Spot = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_SpotCS = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_SpotFluxer = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_SpotRSM = std::vector<GLint>();

void Light::InitShadow() {
	glGenFramebuffers(1, &_shadowFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shadowFbo);

	glGenTextures(3, _shadowGITexs);
	glGenTextures(1, &_shadowMap);

	for (uint i = 0; i < 3; i++) {
		glBindTexture(GL_TEXTURE_2D, _shadowGITexs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _shadowGITexs[i], 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// depth
	glBindTexture(GL_TEXTURE_2D, _shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowMap, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, DrawBuffers);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("ShadowMap", "FB error:" + Status);
		abort();
	}
	else {
		Debug::Message("ShadowMap", "FB ok");
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	GLenum e;
	//* depth cube
	glGenFramebuffers(6, _shadowCubeFbos);
	glGenTextures(1, &_shadowCubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowCubeMap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	e = glGetError();
	for (uint a = 0; a < 6; a++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + a, 0, GL_DEPTH_COMPONENT32F, 512, 512, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	e = glGetError();

	for (uint a = 0; a < 6; a++) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shadowCubeFbos[a]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + a, _shadowCubeMap, 0);

		Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			Debug::Error("ShadowMap", "FB cube error: " + Status);
			abort();
		}
	}

	Debug::Message("ShadowMap", "FB cube ok");
	//*/
	InitRSM();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Light::InitRSM() {
	glGenFramebuffers(1, &_rsmFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _rsmFbo);

	glGenTextures(1, &_rsmTex);

	glBindTexture(GL_TEXTURE_2D, _rsmTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 512, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _rsmTex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	GLint Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("GI_RsmMap", "FB error:" + Status);
		abort();
	}
	else {
		Debug::Message("GI_RsmMap", "FB ok");
	}

	glGenFramebuffers(1, &_fluxFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fluxFbo);

	glGenTextures(1, &_fluxTex);

	glBindTexture(GL_TEXTURE_2D, _fluxTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 512, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fluxTex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum DrawBuffers2[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers2);

	Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("GI_FluxMap", "FB error:" + Status);
		abort();
	}
	else {
		Debug::Message("GI_FluxMap", "FB ok");
	}

	std::default_random_engine generator;
	std::normal_distribution<double> distri(5, 2);
	for (uint a = 0; a < 1024; a++) {
		_rsmBuffer.xPos[a] = (float)(distri(generator)*10.0);
		_rsmBuffer.yPos[a] = (float)(distri(generator)*10.0);
		float sz = sqrt(pow(_rsmBuffer.xPos[a], 2) + pow(_rsmBuffer.yPos[a], 2))*0.001f;
		_rsmBuffer.size[a] = 0.5f;// sz;
	}
	glGenBuffers(1, &_rsmUBO);
	//glBindBufferBase(GL_UNIFORM_BUFFER, BUFFERLOC_LIGHT_RSM, _rsmUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, _rsmUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(_rsmBuffer), &_rsmBuffer, GL_STATIC_DRAW);
	//glBindBuffer(GL_UNIFORM_BUFFER, 0);
	//glBindBuffer(GL_UNIFORM_BUFFER, gbo);
	GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p, &_rsmBuffer, sizeof(_rsmBuffer));
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
	//CalcShadowMatrix();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	Camera::DrawSceneObjectsOpaque(Scene::active->objects);
	//	break;           
	//}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

	if (Scene::active->settings.GIType == GITYPE_RSM) {
		BlitRSMFlux();
	}

	glEnable(GL_BLEND);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tar);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
	//glViewport(0, 0, Display::width, Display::height);
}

void Light::BlitRSMFlux() {
	glViewport(0, 0, 512, 512);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _shadowFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fluxFbo);
#define sloc paramLocs_SpotFluxer
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, Camera::screenRectVerts);
	glUseProgram(Camera::d_sLightRSMFluxProgram);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, Camera::screenRectVerts);

	glUniform2f(sloc[0], 512.0f, 512.0f);
	glUniform1i(sloc[1], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _shadowGITexs[0]);
	glUniform1i(sloc[2], 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _shadowGITexs[1]);
	Vec3 fwd = object->transform.forward();
	glUniform3f(sloc[3], fwd.x, fwd.y, fwd.z);
	glUniform4f(sloc[4], color.r, color.g, color.b, color.a);
	glUniform1f(sloc[5], intensity);
	glUniform1f(sloc[6], square? 1.0f : 0.0f);
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Light::DrawRSM(Mat4x4& ip, Mat4x4& lp, float w, float h, GLuint gtexs[], GLuint gdepth) {
	glUseProgram(Camera::d_sLightRSMProgram);
#define sloc paramLocs_SpotRSM
	//glUniformBlockBinding(Camera::d_sLightRSMProgram, (GLuint)sloc[0], _rsmUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, (GLuint)sloc[0], _rsmUBO);
	glUniformMatrix4fv(sloc[1], 1, GL_FALSE, glm::value_ptr(ip));
	glUniformMatrix4fv(sloc[2], 1, GL_FALSE, glm::value_ptr(lp));
	glUniformMatrix4fv(sloc[3], 1, GL_FALSE, glm::value_ptr(glm::inverse(lp)));
	glUniform2f(sloc[4], w, h);
	glUniform1i(sloc[5], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gtexs[0]);
	glUniform1i(sloc[6], 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gtexs[1]);
	glUniform1i(sloc[7], 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gtexs[2]);
	glUniform1i(sloc[8], 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gdepth);
	glUniform1i(sloc[9], 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, _fluxTex);
	glUniform1i(sloc[10], 5);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, _shadowGITexs[1]);
	glUniform1i(sloc[11], 6);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, _shadowMap);
	glUniform1f(sloc[12], Scene::active->settings.rsmRadius);
#undef sloc
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::screenRectIndices);
}


void ReflectionProbe::_DoUpdate() {
	
}


CubeMap::CubeMap(ushort size, bool mips, GLenum type, byte dataSize, GLenum format, GLenum dataType) : size(size), AssetObject(ASSETTYPE_HDRI), loaded(false) {
	if (size != 64 && size != 128 && size != 256 && size != 512 && size != 1024 && size != 2048) {
		Debug::Error("Cubemap", "CubeMaps must be POT-sized between 64 and 2048! (" + std::to_string(size) + ")");
		abort();
	}
	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pointer);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, mips? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (uint a = 0; a < 6; a++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + a, 0, type, size, size, 0, format, dataType, NULL);
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	loaded = true;
}

void CubeMap::_RenderCube(Vec3 pos, Vec3 xdir, GLuint fbos[], uint size, GLuint shader) {
	glViewport(0, 0, size, size);
	glUseProgram(shader);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixf(glm::value_ptr(glm::perspectiveFov(PI, (float)size, (float)size, 0.001f, 1000.0f)));
	glScalef(1, -1, -1);
	//glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 1, 0), PI))));
	glTranslatef(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[4]);
	glTranslatef(-pos.x, -pos.y, -pos.z);
	glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 1, 0), PI))));
	glTranslatef(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[0]);
	glTranslatef(-pos.x, -pos.y, -pos.z);
	glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 1, 0), PI))));
	glTranslatef(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[5]);
	glTranslatef(-pos.x, -pos.y, -pos.z);
	glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 1, 0), PI))));
	glTranslatef(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[1]);
	glTranslatef(-pos.x, -pos.y, -pos.z);
	glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 0, 1), PI))));
	glTranslatef(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[2]);
	glTranslatef(-pos.x, -pos.y, -pos.z);
	glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 0, 1), 2*PI))));
	glTranslatef(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[3]);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void CubeMap::_DoRenderCubeFace(GLuint fbo) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	float zero[] = { 0,0,0,0 };
	float one = 1;
	glClearBufferfv(GL_COLOR, 0, zero);
	glClearBufferfv(GL_DEPTH, 0, &one);
	glClearBufferfv(GL_COLOR, 1, zero);
	glClearBufferfv(GL_COLOR, 2, zero);
	glClearBufferfv(GL_COLOR, 3, zero);
	glMatrixMode(GL_MODELVIEW);
	Camera::DrawSceneObjectsOpaque(Scene::active->objects);
}

RenderCubeMap::RenderCubeMap() : map(256, true) {
	for (uint a = 0; a < 6; a++) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[a][0]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + a, map.pointer, 0);

		GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers);
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			Debug::Error("CubeMap", "FBO error: " + Status);
			abort();

		}
	}
}


void Editor::InitMaterialPreviewer() {
	_InitGBuffer(&matPrev_fbo, matPrev_texs, &matPrev_depthTex, 256, 256);
	matPreviewerSphere = Procedurals::UVSphere(32, 16);
}

void Editor::DrawMaterialPreviewer(float x, float y, float w, float h, float& rx, float& rz, Material* mat) {
	Engine::DrawQuad(x, y, w, h, black());

}