#include "Engine.h"

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

void Camera::InitGBuffer() {
	glGenFramebuffers(1, &d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);

	// Create the gbuffer textures
	glGenTextures(3, d_texs);
	glGenTextures(1, &d_depthTex);

	for (uint i = 0; i < 3; i++) {
		glBindTexture(GL_TEXTURE_2D, d_texs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Display::width, Display::height, 0, GL_RGBA, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, d_texs[i], 0);
#ifdef SHOW_GBUFFERS
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#else
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
	}

	// depth
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, Display::width, Display::height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, d_depthTex, 0);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, DrawBuffers);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Camera (" + name + ")", "FB error:" + Status);
	}
	else {
		Debug::Message("Camera (" + name + ")", "FB ok");
	}

	// restore default FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
	RenderLights();
#endif
}

void Camera::RenderLights() {
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



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

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_SPEC_GLOSS);
	glBlitFramebuffer(0, 0, Display::width, Display::height, HalfWidth, HalfHeight, Display::width, Display::height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_NORMAL);
	glBlitFramebuffer(0, 0, Display::width, Display::height, 0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}