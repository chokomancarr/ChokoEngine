#pragma once
#include "AssetObjects.h"

class Texture3D : public AssetObject {
public:
	Texture3D(const string& path, TEX_FILTERING filter = TEX_FILTER_TRILINEAR);

	bool loaded;
	uint length;
	GLuint pointer;

	friend class Editor;
protected:
	Texture3D() : AssetObject(ASSETTYPE_TEXCUBE) {}
	static bool Parse(Editor* e, const string& path);
};
