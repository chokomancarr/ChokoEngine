#pragma once
#include "SceneObjects.h"

class TextureRenderer : public Component {
public:
	TextureRenderer() : _texture(-1), Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY) {}

	Texture* texture;

#ifdef IS_EDITOR
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;
#endif

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(TextureRenderer);
protected:
	TextureRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);
	int _texture;
};
