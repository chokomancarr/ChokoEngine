#pragma once
#include "AssetObjects.h"

class MatVal_Tex {
	friend class Material;
	friend class MeshRenderer;
	friend class SkinnedMeshRenderer;
	friend void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
protected:
	MatVal_Tex() : id(-1), tex(nullptr), defTex(0) {}
	~MatVal_Tex() {}

	int id;
	Texture* tex;
	GLuint defTex;
};

/*! Material used by renderers.
[av] */
class Material : public AssetObject {
public:
	Material(void);
	Material(Shader* shad);
	~Material();

	Shader* shader();
	void shader(Shader* shad);
	//values applied to program on drawing stage
	std::unordered_map<SHADER_VARTYPE, std::unordered_map <GLint, void*>, std::hash<byte>> vals;
	std::unordered_map<SHADER_VARTYPE, std::vector<string>, std::hash<byte>> valNames;
	//std::unordered_map<GLint, ShaderVariable> vals;
	void SetBuffer(uint location, IComputeBuffer* buffer);
	void SetTexture(string name, Texture* texture);
	void SetTexture(GLint id, Texture* texture);
	void SetFloat(string name, float val);
	void SetFloat(GLint id, float val);
	void SetInt(string name, int val);
	void SetInt(GLint id, int val);
	void SetVec2(string name, Vec2 val);
	void SetVec2(GLint id, Vec2 val);

	friend class Engine;
	friend class Editor;
	friend class EB_Browser;
	friend class EB_Inspector;
	friend class Mesh;
	friend class MeshRenderer;
	friend class SkinnedMeshRenderer;
	friend class Scene;
	friend class AssetManager;
	friend class Shader;
	friend class RenderTexture;
	friend int main(int argc, char **argv);
	friend void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
	_allowshared(Material);
protected:
	Material(string s);
	Material(std::istream& stream, uint offset = 0);
	void _ReloadParams();

	Shader* _shader;
	int _shaderId;
	std::vector<SHADER_VARTYPE> valOrders;
	std::vector<byte> valOrderIds;
	std::vector<byte> valOrderGLIds;
	std::vector<bool> writeMask;

	static void LoadOris();
	static std::array<GLuint, 7> defTexs;

	static void _UpdateTexCache(void*);

#ifdef IS_EDITOR
	void Save(string path);
#endif
	void ApplyGL(Mat4x4& _mv, Mat4x4& _p);
	void ResetVals();

	bool _maskExpanded;
};
