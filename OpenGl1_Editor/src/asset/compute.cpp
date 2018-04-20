#include "Engine.h"

IComputeBuffer::IComputeBuffer(uint size, void* data, uint padding, uint stride) : size(size) {
	glGenBuffers(1, &pointer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, (!!padding) ? nullptr : data, GL_DYNAMIC_READ);
	if (!!padding) Set(data, padding, stride);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
IComputeBuffer::~IComputeBuffer() {
	glDeleteBuffers(1, &pointer);
}

void IComputeBuffer::Set(void* data, uint padding, uint stride) {
	if (!data) {
		Debug::Warning("ComputeBuffer", "Set: Buffer is null!");
	}
	GLint bufmask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointer);
	void* tar = (void*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, bufmask);
	if (!tar) {
		Debug::Warning("ComputeBuffer", "Set: Unable to map buffer!");
	}
	if (!padding) memcpy(tar, data, size);
	else {
		for (uint a = 0; a*padding < size; a++) {
			memcpy((void*)((char*)tar + a*padding), (void*)((char*)data + a*stride), stride);
		}
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

ComputeShader::ComputeShader(string str) {
	pointer = glCreateProgram();
	GLuint mComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	auto c_str = str.c_str();
	char err[500];
	glShaderSource(mComputeShader, 1, &c_str, NULL);
	glCompileShader(mComputeShader);
	int rvalue, length;
	glGetShaderiv(mComputeShader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue)
	{
		glGetShaderInfoLog(mComputeShader, 500, &length, err);
		Debug::Error("ComputeShader", string(err));
		glDeleteShader(mComputeShader);
		return;
	}
	glAttachShader(pointer, mComputeShader);
	glLinkProgram(pointer);
	glGetProgramiv(pointer, GL_LINK_STATUS, &rvalue);
	if (!rvalue)
	{
		glGetProgramInfoLog(pointer, 500, &length, err);
		Debug::Error("ComputeShader", string(err));
	}
	glDetachShader(pointer, mComputeShader);
	glDeleteShader(mComputeShader);
}

ComputeShader::~ComputeShader() {
	glDeleteProgram(pointer);
}

ComputeShader* ComputeShader::FromPath(string path) {
	return new ComputeShader(IO::GetText(path));
}

void ComputeShader::SetBuffer(uint binding, IComputeBuffer* buf) {
	buffers.push_back(std::pair<uint, IComputeBuffer*>(binding, buf));
}

void ComputeShader::Dispatch(uint cx, uint cy, uint cz) {
	for (auto& a : buffers)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, a.first, a.second->pointer);
	glUseProgram(pointer);
	glDispatchCompute(cx, cy, cz);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glUseProgram(0);
	for (auto& a : buffers)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, a.first, 0);
}