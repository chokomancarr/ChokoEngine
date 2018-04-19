#pragma once
#include "SceneObjects.h"

enum VOXEL_TYPE : byte {
	VOXEL_TYPE_ADDITIVE,
	VOXEL_TYPE_BLOCKS
};

class VoxelRenderer : public Component {
public:
	VoxelRenderer() : _texture(-1), size(1), Component("Voxel Renderer", COMP_VRD, DRAWORDER_OVERLAY) {}

	Texture3D* texture;
	float size;

	VOXEL_TYPE renderType;

#ifdef IS_EDITOR
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override {}
#endif

	void Draw();

	friend class Engine;
	friend class Editor;
protected:
	static void Init();

	static Shader* _shader;
	static int _shaderLocs[6];

	static const Vec3 cubeVerts[8];
	static const uint cubeIndices[36];

	int _texture;
	static void _UpdateTexture(void* i);
};
