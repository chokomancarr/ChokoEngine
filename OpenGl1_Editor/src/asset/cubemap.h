#pragma once
#include "AssetObjects.h"

class CubeMap : public AssetObject {
public:
	CubeMap(Texture* px, Texture* nx, Texture* py, Texture* ny, Texture* pz, Texture* nz);
	CubeMap(Texture* tex);

	const ushort size;
	bool loaded;

	friend class Camera;
	friend class RenderCubeMap;
	friend class ReflectionProbe;
	_allowshared(CubeMap);
protected:
	CubeMap(ushort size, bool mips = false, GLenum type = GL_RGBA, byte dataSize = 4, GLenum format = GL_RGBA, GLenum dataType = GL_UNSIGNED_BYTE);

	uint pointer;

	static void _RenderCube(Vec3 pos, Vec3 xdir, GLuint fbos[], uint size, GLuint shader = 0);
	static void _DoRenderCubeFace(GLuint fbo);
};

class RenderCubeMap {
protected:
	RenderCubeMap();

	CubeMap map;
	std::vector<GLuint> fbos[6]; //[face][mip]
};
