#pragma once
#include "AssetObjects.h"

class IShaderBuffer {
public:
	IShaderBuffer(uint size, void* data, uint padding = 0, uint stride = 1);
	~IShaderBuffer();

	GLuint pointer;
	uint size;


	void Set(void* data, uint padding = 0, uint stride = 1);

	template <typename T> T* Get(T* target = nullptr) {
		byte* tar;
		if (!target) tar = new byte[size];
		else tar = (byte*)target;
		glBindBuffer(GL_UNIFORM_BUFFER, pointer);
		void* src = (void*)glMapBuffer(GL_UNIFORM_BUFFER, GL_READ_ONLY);
		if (!src) {
			Debug::Warning("ShaderBuffer", "Set: Unable to map buffer!");
		}
		memcpy(tar, src, size);
		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		return (T*)tar;
	}
};

template<typename T> class ShaderBuffer : public IShaderBuffer {
public:
	ShaderBuffer(uint num, T* data = nullptr, uint padding = 0) : IShaderBuffer(num * sizeof(T), data, padding) {}
};

class ShaderValue {
public:
	ShaderValue() : i(0), x(0), y(0), z(0), m() {}
	int i;
	float x, y, z;
	glm::quat m;
};

class ShaderTags {
public:
	ShaderTags() : zTest(SHADER_TEST_LEQUAL), aTest(SHADER_TEST_ALWAYS), culling(SHADER_CULL_BACK), zWrite() {}
	SHADER_TEST zTest, aTest;
	SHADER_CULL culling;
	bool zWrite;

	void ApplyGL();

	static void Reset();
};

class ShaderVariable {
public:
	//ShaderVariable() : type(0), name(""), val(), min(0), max(0) {}

	SHADER_VARTYPE type;
	string name;
	ShaderValue val, def;

	float min, max;
};

class Shader : public AssetObject {
public:
	Shader(string path);
	Shader(std::istream& stream, uint offset);
	Shader(const string& vert, const string& frag);
	Shader(GLuint p) : AssetObject(ASSETTYPE_SHADER), pointer(p), loaded(!!p), inherited(true) {}
	//ShaderBase(string vert, string frag);
	~Shader() {
		if (!inherited) glDeleteProgram(pointer);
		for (ShaderVariable* v : vars)
			delete(v);
	}

	bool loaded;
	GLuint pointer;
	ShaderTags tags;
	std::vector<ShaderVariable*> vars;

	//void Set(byte type, GLint id, void* val) { values[type][id] = val; }
	//void* Get(byte type, GLint id) { return values[type][id]; }

	//removes macros, insert include files
#ifdef IS_EDITOR
	static bool Parse(std::ifstream* text, string path);
#endif
	static bool LoadShader(GLenum shaderType, string source, GLuint& shader, string* err = nullptr);

	friend class Camera;
	friend class Engine;

protected:
	static GLuint FromVF(const string& vert, const string& frag);
	bool inherited = false;
};