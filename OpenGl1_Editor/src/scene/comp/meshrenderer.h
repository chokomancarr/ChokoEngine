#pragma once
#include "SceneObjects.h"

class MeshRenderer : public Component {
public:
	MeshRenderer();
	std::vector<rMaterial> materials;

#ifdef IS_EDITOR
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;

	void Serialize(Editor* e, std::ofstream* stream) override;
#endif
	void Refresh() override;

	friend class Camera;
	friend class Editor;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(MeshRenderer);
protected:
	MeshRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);

	void DrawDeferred(GLuint shader = 0);

	std::vector<ASSETID> _materials;
	bool overwriteWriteMask = false;
	std::vector<bool> writeMask;
	static void _UpdateMat(void* i);
	static void _UpdateTex(void* i);
};
