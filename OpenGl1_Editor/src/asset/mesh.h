#pragma once
#include "AssetObjects.h"

class Mesh : public AssetObject {
public:
	//Mesh(); //until i figure out normal recalc algorithm
	bool loaded;
	Mesh(const std::vector<Vec3>& verts, const std::vector<Vec3>& norms, const std::vector<int>& tris, std::vector<Vec2> uvs = std::vector<Vec2>());

	std::vector<Vec3> vertices;
	std::vector<Vec3> normals, tangents;// , bitangents;
	std::vector<int> triangles;
	std::vector<Vec2> uv0, uv1;
	std::vector<std::vector<std::pair<byte, float>>> vertexGroupWeights;
	std::vector<string> vertexGroups;
	BBox boundingBox;

	uint vertexCount, triangleCount, materialCount;

	void RecalculateBoundingBox();

	friend int main(int argc, char **argv);
	friend class Engine;
	friend class Editor;
	friend class MeshFilter;
	friend class MeshRenderer;
	friend class SkinnedMeshRenderer;
	friend class AssetManager;
	_allowshared(Mesh);
protected:
	Mesh(Editor* e, int i);
	Mesh(std::istream& strm, uint offset = 0);
	Mesh(string path);
	Mesh(byte* mem);

	void CalcTangents();
	void GenECache() override;
	void InitVao();

	GLuint vao, vbos[4];

#ifdef IS_EDITOR
	static bool ParseBlend(Editor* e, string s);
#endif

	std::vector<std::vector<int>> _matTriangles;
	std::vector<GLuint> _matIndicesBuffers;
	//void Draw(Material* mat);
	//void Load();

};
