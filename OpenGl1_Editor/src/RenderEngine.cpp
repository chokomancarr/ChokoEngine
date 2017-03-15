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

void Camera::Render(RenderTexture* target) {
	//enable deferred
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);
	//clear
	ApplyGL();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//render opaque
	DrawSceneObjectsOpaque(Scene::active->objects);
}
//setreadbuffer glReadBuffer(GL_COLOR_ATTACHMENT0 + TextureType);