#pragma once
#include "AssetObjects.h"

class Background : public AssetObject {
public:
	//Background() : AssetObject(ASSETTYPE_HDRI){}
	Background(const string& path);

	bool loaded;
	unsigned int width, height;
	GLuint pointer;
	friend int main(int argc, char **argv);
	friend class Editor;
	friend class AssetManager;
	_allowshared(Background);
private:
	Background(int i, Editor* editor);
	Background(std::istream& strm, uint offset);
	Background(byte*);
	static bool Parse(string path);

	void GenECache(const std::vector<Vec2>& szs, const std::vector<RenderTexture*>& rts);
};
