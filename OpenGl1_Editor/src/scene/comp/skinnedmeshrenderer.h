#pragma once
#include "SceneObjects.h"

const uint SKINNED_MAX_VERTICES = 65535;
const uint SKINNED_THREADS_PER_GROUP = 32;

class SkinnedMeshRenderer : public Component {
public:
	SkinnedMeshRenderer(SceneObject* o);
	std::vector<Material*> materials;
	Mesh* mesh() { return _mesh; }
	void mesh(Mesh*);
	//void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
#ifdef IS_EDITOR
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override {}
	//void Refresh() override;
#endif
	friend class Engine;
	friend class Editor;
	friend class Camera;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend void LoadMeshMeta(std::vector<pSceneObject>& os, string& path);
	_allowshared(SkinnedMeshRenderer);
protected:
	SkinnedMeshRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);

	std::vector<std::array<std::pair<ArmatureBone*, float>, 4>> weights;

	void InitWeights();
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0);
	void DrawDeferred(GLuint shader = 0);

	Mesh* _mesh = 0;
	ASSETID _meshId = -1;
	Armature* armature;
	std::vector<ASSETID> _materials;
	bool overwriteWriteMask = false;
	std::vector<bool> writeMask;
	bool showBoundingBox = false;

	struct SkinDats {
		SkinDats() {
			mats[0] = 0;
			mats[1] = 0;
			mats[2] = 0;
			mats[3] = 0;
		}

		uint mats[4];
		Vec4 weights;
	};
	ComputeBuffer<Vec4>* skinBufPoss = 0, *skinBufNrms;
	ComputeBuffer<Vec4>*skinBufPossO, *skinBufNrmsO;
	ComputeBuffer<SkinDats>* skinBufDats;
	ComputeBuffer<Mat4x4>* skinBufMats;
	static ComputeShader* skinningProg;
	uint skinDispatchGroups;

	static void InitSkinning();
	void Skin();

	void SetMesh(int i);
	static void _UpdateMesh(void* i);
	static void _UpdateMat(void* i);
	static void _UpdateTex(void* i);
};
