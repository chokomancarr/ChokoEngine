#ifndef SHAD_H
#define SHAD_H
#include <gl/glew.h>
#include <string>

using namespace std;

class Shader {
public:
	static GLuint pointer;
	static GLuint CreateProgram(string& path);
	static void SetUniform(GLuint program, GLchar* name, float val);
	//static bool SetTexture(GLuint program, GLchar* name, string& path, int w, int h, GLenum slot = GL_TEXTURE0);
	static void SetWindow(GLuint program, float w, float h);
	static GLuint LoadShader(GLenum shaderType, string source);
	static GLuint GetTexture(const string& path);
};

#endif