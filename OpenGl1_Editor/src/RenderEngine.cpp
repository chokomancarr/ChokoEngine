#include "Engine.h"

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
#ifndef IS_EDITOR
	glGenFramebuffers(1, &d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);

	// Create the gbuffer textures
	glGenTextures(3, d_texs);
	glGenTextures(1, &d_depthTex);

	for (uint i = 0; i < 3; i++) {
		glBindTexture(GL_TEXTURE_2D, d_texs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Display::width, Display::height, 0, GL_RGB, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, d_texs[i], 0);
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
#endif
}

void Camera::Render(RenderTexture* target) {
	//enable deferred
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);
	//clear
	ApplyGL();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//render opaque
	DrawSceneObjectsOpaque(Scene::active->objects);

	RenderLights();
}
//setreadbuffer glReadBuffer(GL_COLOR_ATTACHMENT0 + TextureType);

void Camera::RenderLights() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);

	GLsizei HalfWidth = (GLsizei)(Display::width / 2.0f);
	GLsizei HalfHeight = (GLsizei)(Display::height / 2.0f);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_DIFFUSE);
	glBlitFramebuffer(0, 0, Display::width, Display::height, 0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_NORMAL);
	glBlitFramebuffer(0, 0, Display::width, Display::height, 0, HalfHeight, HalfWidth, Display::height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_SPEC_GLOSS);
	glBlitFramebuffer(0, 0, Display::width, Display::height, HalfWidth, HalfHeight, Display::width, Display::height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}