#pragma once
#include "Engine.h"

/*! generic class of ComputeBuffer
[av] */
class IComputeBuffer {
public:
	IComputeBuffer(uint size, void* data, uint padding = 0, uint stride = 1);
	~IComputeBuffer();

	GLuint pointer;
	uint size;


	void Set(void* data, uint padding = 0, uint stride = 1);

	/*! Returns a copy of the compute buffer data.
	* The lhs pointer will be overwritten if target is null. Use Get(T*) to use a preallocated buffer.
	*/
	template <typename T> T* Get(T* target = nullptr) {
		byte* tar;
		if (!target) tar = new byte[size];
		else tar = (byte*)target;
		GLint bufmask = GL_MAP_READ_BIT;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointer);
		void* src = (void*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * 4, bufmask);
		if (!src) {
			Debug::Warning("ComputeBuffer", "Set: Unable to map buffer!");
		}
		memcpy(tar, src, size);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return (T*)tar;
	}
};

/*! Data buffer to be used by ComputeShader.
[av] */
template<typename T> class ComputeBuffer : public IComputeBuffer {
public:
	ComputeBuffer(uint num, T* data = nullptr, uint padding = 0) : IComputeBuffer(num * sizeof(T), data, padding) {}
};

/*! GPGPU Shader.
[av] */
class ComputeShader {
public:
	/*! Creates a ComputeShader from the shader source.
	Example usage: ComputeShader(IO::GetText("C:\\source.compute"));
	*/
	ComputeShader(string str);
	~ComputeShader();

	static ComputeShader* FromPath(string path);

	void SetBuffer(uint binding, IComputeBuffer* buf);
	void SetFloat(string name, const int& val);
	void SetFloat(string name, const float& val);
	void SetMatrix(string name, const Mat4x4& val);
	void SetArray(string name, void* val, uint cnt);

	void Dispatch(uint cx, uint cy, uint xz);

	friend class SkinnedMeshRenderer;
protected:
	std::vector<std::pair<GLuint, IComputeBuffer*>> buffers;

	GLuint pointer;
};