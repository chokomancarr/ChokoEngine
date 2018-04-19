#pragma once
#include "SceneObjects.h"

class ArrayRenderer : public Component {
public:
	ArrayRenderer();
	rMaterial material;
	std::vector<Vec3> positions;

#ifdef IS_EDITOR
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;

	void Serialize(Editor* e, std::ofstream* stream) override {}
#endif

	friend class Camera;
	friend class Editor;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(ArrayRenderer);
protected:
	ArrayRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);

	void DrawDeferred(GLuint shader = 0);

	ASSETID _material;
	bool overwriteWriteMask = false;
	std::vector<bool> writeMask;
};
