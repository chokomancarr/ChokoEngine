#pragma once
#include "AssetObjects.h"

class RenderTexture : public Texture {
public:
	RenderTexture(uint w, uint h, RT_FLAGS flags = RT_FLAG_NONE, const GLvoid* pixels = NULL, GLenum pixelFormat = GL_RGBA, GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR, GLenum wrapS = GL_REPEAT, GLenum wrapT = GL_REPEAT);
	~RenderTexture();

	const bool depth, stencil, hdr;

	static void Blit(Texture* src, RenderTexture* dst, Shader* shd, string texName = "mainTex");
	static void Blit(Texture* src, RenderTexture* dst, Material* mat, string texName = "mainTex");
	static void Blit(GLuint src, RenderTexture* dst, GLuint shd, string texName = "mainTex");

	template <typename T>
	std::vector<T> pixels(bool alpha) {
		assert((typeid(byte).hash_code() == typeid(T).hash_code()) || (typeid(float).hash_code() == typeid(T).hash_code()));
		std::vector<T> v = std::vector<T>(width*height * (alpha ? 4 : 3));
		glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
		glReadPixels(0, 0, width, height, (alpha ? GL_RGBA : GL_RGB), (typeid(byte).hash_code() == typeid(T).hash_code()) ? GL_UNSIGNED_BYTE : GL_FLOAT, &v[0]);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		return v;
	}

	//void Resize(uint w, uint h);
	friend class Texture;
	friend class Editor;
	friend class Background;
	friend int main(int argc, char **argv);
	_allowshared(RenderTexture);
protected:
	GLuint d_fbo;
	void Load(string path);
	void Load(std::istream& strm);
	static bool Parse(string path); //just tell Texture to load as rendtex
};
