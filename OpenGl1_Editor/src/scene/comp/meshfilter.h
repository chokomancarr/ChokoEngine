#pragma once
#include "SceneObjects.h"

class MeshFilter : public Component {
public:
	MeshFilter();

	rMesh mesh = 0;
	//void LoadDefaultValues() override;

#ifdef IS_EDITOR
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;
#endif

	friend class MeshRenderer;
	friend class Editor;
	friend void LoadMeshMeta(std::vector<pSceneObject>& os, string& path);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(MeshFilter);
protected:
	MeshFilter(std::ifstream& stream, SceneObject* o, long pos = -1);

	bool showBoundingBox = false;
	ASSETID _mesh = -1;

#ifdef IS_EDITOR
	void SetMesh(int i);
	static void _UpdateMesh(void* i);
#endif
};
