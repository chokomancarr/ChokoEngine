#ifndef KTM_H
#define KTM_H
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

using namespace std;

class KTMModel {
public:
	KTMModel();

	int vertCount;
	int faceCount;

	bool useLine;

	GLuint shader;

	bool LoadModel(const string& path);
	void RenderModel();
	void ClearModel();

	static string Parse(string path);

protected:
	typedef std::vector<glm::vec2> Vec2Buffer;
	typedef std::vector<glm::vec3> Vec3Buffer;
	typedef std::vector<glm::vec4> Vec4Buffer;
	typedef std::vector<GLuint> IndexBuffer;

	struct Vertex
	{
		glm::vec3   position;
		glm::vec3   normal;
		glm::vec3	Vec4;
		glm::vec2   tex0;
	};
	typedef std::vector<Vertex> VertexList;

	struct Triangle
	{
		int             vertices[3];
	};
	typedef std::vector<Triangle> TriangleList;

	struct Mesh
	{
		string			name;
		string			shader;
		glm::vec3		position;
		glm::vec4		rotation;
		glm::vec3		scale;
		glm::mat4		matrix;
		VertexList      vertices;
		TriangleList    triangles;

		GLuint          texId;

		Vec3Buffer		m_PositionBuffer;
		Vec3Buffer		m_NormalBuffer;
		Vec2Buffer		m_Tex2DBuffer;
		IndexBuffer     m_IndexBuffer;
		Vec3Buffer		m_VertVec4Buffer;
	};
	typedef std::vector<Mesh> MeshList;


	Mesh mesh;

private:
	glm::mat4x4 m_model2world;

	inline string s(glm::vec3& v) {
		return "(" + to_string(v.x) + ", " + to_string(v.y) + ", " + to_string(v.z) + ")";
	}
	inline string s(glm::vec4& v) {
		return "(" + to_string(v.w) + ", " + to_string(v.x) + ", " + to_string(v.y) + ", " + to_string(v.z) + ")";
	}
};

#endif