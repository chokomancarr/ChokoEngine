#include "Engine.h"
#include "Editor.h"

const Vec3 VoxelRenderer::cubeVerts[] = {
	Vec3(-1, -1, -1), Vec3(1, -1, -1), Vec3(-1, 1, -1), Vec3(1, 1, -1),
	Vec3(-1, -1, 1), Vec3(1, -1, 1), Vec3(-1, 1, 1), Vec3(1, 1, 1) };

const uint VoxelRenderer::cubeIndices[] = {
	0, 1, 2, 1, 3, 2,	4, 5, 6, 5, 7, 6,
	0, 2, 4, 2, 6, 4,	1, 3, 5, 3, 7, 5,
	0, 1, 4, 1, 5, 4,	2, 3, 6, 3, 7, 6 };

Shader* VoxelRenderer::_shader = 0;
int VoxelRenderer::_shaderLocs[] = {};

void VoxelRenderer::Init() {
	_shader = new Shader(DefaultResources::GetStr("voxelVert.txt"), DefaultResources::GetStr("voxelFrag.txt"));
	_shaderLocs[0] = glGetUniformLocation(_shader->pointer, "tex");
	_shaderLocs[1] = glGetUniformLocation(_shader->pointer, "size");
	_shaderLocs[2] = glGetUniformLocation(_shader->pointer, "_MV");
	_shaderLocs[3] = glGetUniformLocation(_shader->pointer, "_P");
	_shaderLocs[4] = glGetUniformLocation(_shader->pointer, "_IP");
	_shaderLocs[5] = glGetUniformLocation(_shader->pointer, "screenSize");
}

void VoxelRenderer::Draw() {
	if (!texture || !texture->loaded) return;

	//if (renderType == VOXEL_TYPE_ADDITIVE) {
	glEnable(GL_BLEND);
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glBlendFunc(GL_ONE, GL_ONE);
	//}

	UI::SetVao(8, (void*)cubeVerts);

	Mat4x4 m1 = MVP::modelview();
	Mat4x4 m2 = MVP::projection();
	Mat4x4 im2 = glm::inverse(m2);
	glUseProgram(_shader->pointer);

	glUniform1i(_shaderLocs[0], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, texture->pointer);
	glUniform1f(_shaderLocs[1], size);
	glUniformMatrix4fv(_shaderLocs[2], 1, GL_FALSE, glm::value_ptr(m1));
	glUniformMatrix4fv(_shaderLocs[3], 1, GL_FALSE, glm::value_ptr(m2));
	glUniformMatrix4fv(_shaderLocs[4], 1, GL_FALSE, glm::value_ptr(im2));
	glUniform2f(_shaderLocs[5], (float)Display::width, (float)Display::height);

	glBindVertexArray(UI::_vao);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cubeIndices);
	glBindVertexArray(0);
	glUseProgram(0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void VoxelRenderer::_UpdateTexture(void* i) {
#ifdef IS_EDITOR
	auto r = (VoxelRenderer*)i;
	r->texture = _GetCache<Texture3D>(ASSETTYPE_TEXCUBE, r->_texture);
#endif
}

#ifdef IS_EDITOR

void VoxelRenderer::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		auto vr = (VoxelRenderer*)c;

		UI::Label(v.r + 2, v.g + pos + 20, 12, "3D Texture", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXCUBE, 12, e->font, &_texture, &_UpdateTexture, this);
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "size", e->font, white());
		//size = std::stoi(UI::EditText(v.r + v.b * 0.3f, v.g + pos, v.b * 0.7f, 16, 12, grey2(), std::to_string(size), e->font, true, nullptr, white()));
		size = Engine::DrawSliderFill(v.r + v.b * 0.3f, v.g + pos, v.b * 0.7f, 16, 1, 10, size, grey2(), white());
		pos += 17;
	}
	else pos += 17;
}

#endif