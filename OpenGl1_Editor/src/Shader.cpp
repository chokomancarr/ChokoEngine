#include "Shader.h"
#include "Engine.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

bool LoadShader(GLenum shaderType, string source, GLuint& shader) {

	int compile_result = 0;

	shader = glCreateShader(shaderType);
	const char *shader_code_ptr = source.c_str();
	const int shader_code_size = source.size();

	glShaderSource(shader, 1, &shader_code_ptr, &shader_code_size);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_result);

	//check for errors
	if (compile_result == GL_FALSE)
	{
		int info_log_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
		vector<char> shader_log(info_log_length);
		glGetShaderInfoLog(shader, info_log_length, NULL, &shader_log[0]);
		cerr << "error compiling shader" << endl;
		glDeleteShader(shader);
		shader = 0;
		return false;
	}
	std::cout << "shader compiled" << endl;
	return true;
}

ShaderBase::ShaderBase(string path) {
	string vertex_shader_code = "";
	string fragment_shader_code = "";
	ifstream stream(path.c_str());
	if (!stream.good()) {
		cout << "not found!" << endl;
		return;
	}
	string a;
	bool hasData;
	int x;
	stream >> a;
	if (a != "KTS123") {
		cerr << "file not supported" << endl;
		return;
	}
	int readingType = 0;
	while (!stream.eof()) {
		getline(stream, a);
		if (readingType == 0) {
			if (a == "VERTEXBEGIN") {
				readingType = 1;
			}
			else if (a == "FRAGMENTBEGIN") {
				readingType = 2;
			}
		}
		else if (readingType == 1) {
			if (a == "VERTEXEND") {
				readingType = 0;
			}
			else if (a != ""){
				vertex_shader_code += a + "\n";
				hasData = true;
			}
		}
		else if (readingType == 2) {
			if (a == "FRAGMENTEND") {
				readingType = 0;
			}
			else if (a != ""){
				fragment_shader_code += a + "\n";
				hasData = true;
			}
		}
	}

	if (!hasData)
		return;

	GLuint vertex_shader, fragment_shader;
	if (vertex_shader_code != "") {
		cout << "Vertex Shader: " << endl << vertex_shader_code;
		if (!LoadShader(GL_VERTEX_SHADER, vertex_shader_code, vertex_shader))
			return;
	}
	else return;
	if (fragment_shader_code != "") {
		cout << "Fragment Shader: " << endl << fragment_shader_code;
		if (!LoadShader(GL_FRAGMENT_SHADER, fragment_shader_code, fragment_shader))
			return;
	}
	else return;

	pointer = glCreateProgram();
	glAttachShader(pointer, vertex_shader);
	glAttachShader(pointer, fragment_shader);

	int link_result = 0;

	glLinkProgram(pointer);
	glGetProgramiv(pointer, GL_LINK_STATUS, &link_result);
	if (link_result == GL_FALSE)
	{
		int info_log_length = 0;
		glGetProgramiv(pointer, GL_INFO_LOG_LENGTH, &info_log_length);
		vector<char> program_log(info_log_length);
		glGetProgramInfoLog(pointer, info_log_length, NULL, &program_log[0]);
		cout << "Shader link error" << endl << &program_log[0] << endl;
		glDeleteProgram(pointer);
		pointer = 0;
		return;
	}
	cout << "shader linked" << endl;

	glDetachShader(pointer, vertex_shader);
	glDeleteShader(vertex_shader);
	glDetachShader(pointer, fragment_shader);
	glDeleteShader(fragment_shader);
	loaded = true;
}

string ShaderBase::Parse(ifstream* stream) {
	return "foo";
}

//old shader class

GLuint Shader::pointer = 0;

GLuint Shader::LoadShader(GLenum shaderType, string source) {

	int compile_result = 0;

	GLuint shader = glCreateShader(shaderType);
	const char *shader_code_ptr = source.c_str();
	const int shader_code_size = source.size();

	glShaderSource(shader, 1, &shader_code_ptr, &shader_code_size);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_result);

	//check for errors
	if (compile_result == GL_FALSE)
	{

		int info_log_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
		vector<char> shader_log(info_log_length);
		glGetShaderInfoLog(shader, info_log_length, NULL, &shader_log[0]);
		cerr << "error compiling shader" << endl;
		return 0;
	}
	std::cout << "shader compiled" << endl;
	return shader;
}

GLuint Shader::CreateProgram(string& path){
	string vertex_shader_code = "";
	string fragment_shader_code = "";
	ifstream stream(path.c_str());
	if (!stream.good()) {
		cout << "not found!" << endl;
		return 0;
	}
	string a;
	bool hasData;
	int x;
	stream >> a;
	if (a != "KTS123") {
		cerr << "file not supported" << endl;
		return 0;
	}
	int readingType = 0;
	while (!stream.eof()) {
		getline(stream, a);
		if (readingType == 0) {
			if (a == "VERTEXBEGIN") {
				readingType = 1;
			}
			else if (a == "FRAGMENTBEGIN") {
				readingType = 2;
			}
		}
		else if (readingType == 1) {
			if (a == "VERTEXEND") {
				readingType = 0;
			}
			else if (a != ""){
				vertex_shader_code += a + "\n";
				hasData = true;
			}
		}
		else if (readingType == 2) {
			if (a == "FRAGMENTEND") {
				readingType = 0;
			}
			else if (a != ""){
				fragment_shader_code += a + "\n";
				hasData = true;
			}
		}
	}

	GLuint program = glCreateProgram();
	GLuint vertex_shader, fragment_shader;
	if (vertex_shader_code != "") {
		cout << "Vertex Shader: " << endl << vertex_shader_code;
		vertex_shader = LoadShader(GL_VERTEX_SHADER, vertex_shader_code);
		glAttachShader(program, vertex_shader);
	}
	else return 0;
	if (fragment_shader_code != "") {
		cout << "Fragment Shader: " << endl << fragment_shader_code;
		fragment_shader = LoadShader(GL_FRAGMENT_SHADER, fragment_shader_code);
		glAttachShader(program, fragment_shader);
	}
	else return 0;

	int link_result = 0;

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &link_result);
	if (link_result == GL_FALSE)
	{

		int info_log_length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
		vector<char> program_log(info_log_length);
		glGetProgramInfoLog(program, info_log_length, NULL, &program_log[0]);
		cout << "Shader link error" << endl << &program_log[0] << endl;
		return 0;
	}
	cout << "shader linked" << endl;

	glDetachShader(program, vertex_shader);
	glDeleteShader(vertex_shader);
	glDetachShader(program, fragment_shader);
	glDeleteShader(fragment_shader);
	
	pointer = program;
	return program;
}


GLuint Shader::GetTexture(const string& path) {
	unsigned char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int width, height;
	unsigned int imageSize;   // = width*height*3
	unsigned char *data;

	FILE *file;
	fopen_s(&file, path.c_str(), "rb");
	if (!file){ printf("Image could not be opened\n"); return 0; }
	if (fread(header, 1, 54, file) != 54){ // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return false;
	}
	if (header[0] != 'B' || header[1] != 'M'){
		printf("Not a correct BMP file\n");
		return 0;
	}
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);
	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	data = new unsigned char[imageSize];
	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);
	//Everything is in memory now, the file can be closed
	fclose(file);
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}
/*

bool Shader::SetTexture(GLuint program, GLchar* name, string& path, int w, int h, GLenum slot) {
	ifstream stream(path.c_str());
	if (!stream.good()) {
		cout << "not found!" << endl;
		return false;
	}
	unsigned char* tex = GetTexture(path, w, h);
	GLuint texi;
	glGenTextures(1, &texi);
	glUseProgram(program);
	glActiveTexture(slot);
	glBindTexture(GL_TEXTURE_2D, texi);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
	GLint loc = glGetUniformLocation(program, name);
	if (loc != -1)
	{
		glUniform1i(loc, 0);
		glUseProgram(0);
		return true;
	}
	else return false;
}
*/

void Shader::SetUniform(GLuint program, GLchar* name, float val) {
	glUseProgram(program);
	GLint loc = glGetUniformLocation(program, name);
	if (loc != -1)
	{
		glUniform1f(loc, val);
	}
	glUseProgram(0);
}

void Shader::SetWindow(GLuint program, float w, float h) {
	glUseProgram(program);
	GLint loc = glGetUniformLocation(program, "_windowSize");
	if (loc != -1)
	{
		glUniform2f(loc, w, h);
	}
	else
		cout << "program uniform not found";
	glUseProgram(0);
}