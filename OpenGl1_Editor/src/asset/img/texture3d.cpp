#include "Engine.h"

Texture3D::Texture3D(const string& path, TEX_FILTERING filter) : Texture3D() {
	length = 64;

	byte* data = new byte[(uint)pow(length, 3) * 3]{};
	for (uint a = 0; a < length; a++) {
		for (uint b = 0; b < length; b++) {
			for (uint c = 0; c < length; c++) {
				//auto v = Vec3(a, b, c) - Vec3(1,1,1) * (0.5f * (length - 1));
				//auto l = glm::length(v);
				auto f = sinf((a + 2 * c) * 2 * PI / length);
				auto l = abs(f - ((b / (float)length) * 2 - 1));
				if (l < -0.2 || l > 0.2) {
					//if (l < length * 0.4f || l > length / 2) {
					auto pos = (a + b * length + c * length * length) * 3;
					data[pos] = (byte)(a * 256 / length);
					data[pos + 1] = (byte)(b * 256 / length);
					data[pos + 2] = (byte)(c * 256 / length);
				}
			}
		}
	}

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_3D, pointer);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, length, length, length, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, (filter == TEX_FILTER_TRILINEAR) ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, (filter == TEX_FILTER_TRILINEAR) ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_3D, 0);

	loaded = true;
	delete[](data);
}

bool Texture3D::Parse(Editor* e, const string& path) {
	return false;
}