#include "Engine.h"
#include "Editor.h"

MeshRenderer::MeshRenderer() : Component("Mesh Renderer", COMP_MRD, DRAWORDER_SOLID | DRAWORDER_TRANSPARENT, nullptr, { COMP_MFT }) {
	_materials.push_back(-1);
}

MeshRenderer::MeshRenderer(std::ifstream& stream, SceneObject* o, long pos) : Component("Mesh Renderer", COMP_MRD, DRAWORDER_SOLID | DRAWORDER_TRANSPARENT, o, { COMP_MFT }) {
	_UpdateMat(this);
	if (pos >= 0)
		stream.seekg(pos);
	ASSETTYPE t;
	int s;
	_Strm2Val(stream, s);
	for (int q = 0; q < s; q++) {
		Material* m;
		ASSETID i;
		_Strm2Asset(stream, Editor::instance, t, i);
		m = _GetCache<Material>(ASSETTYPE_MATERIAL, i);
		materials.push_back(rMaterial(pMaterial(m)));
		_materials.push_back(i);
	}
	//_Strm2Asset(stream, Editor::instance, t, _mat);
}

void MeshRenderer::DrawDeferred(GLuint shader) {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0].raw();
	if (!mf->mesh || !mf->mesh->loaded)
		return;
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
	Mat4x4 m1 = MVP::modelview();
	Mat4x4 m2 = MVP::projection();

	glBindVertexArray(mf->mesh->vao);

	for (uint m = 0; m < mf->mesh->materialCount; m++) {
		if (!materials[m])
			continue;
		if (shader == 0) materials[m]->ApplyGL(m1, m2);
		else glUseProgram(shader);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mf->mesh->_matIndicesBuffers[m]);
		glDrawElements(GL_TRIANGLES, mf->mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, 0);
		//glDrawElements(GL_TRIANGLES, mf->mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &mf->mesh->_matTriangles[m][0]);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glUseProgram(0);
	glBindVertexArray(0);
	glDisable(GL_CULL_FACE);
}

void MeshRenderer::_UpdateMat(void* i) {
	MeshRenderer* mf = (MeshRenderer*)i;
	for (int q = mf->_materials.size() - 1; q >= 0; q--) {
		mf->materials[q](_GetCache<Material>(ASSETTYPE_MATERIAL, mf->_materials[q]));
	}
}

void MeshRenderer::_UpdateTex(void* i) {
	MatVal_Tex* v = (MatVal_Tex*)i;
	v->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, v->id);
}

void MeshRenderer::Refresh() {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0].raw();
	if (!mf || !mf->mesh || !mf->mesh->loaded) {
		materials.clear();
		_materials.clear();
	}
	else {
		//Ref<Material> emt;
		materials.resize(mf->mesh->materialCount);// , emt);
		_materials.resize(mf->mesh->materialCount, -1);
	}
}

#ifdef IS_EDITOR

void MeshRenderer::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	MeshFilter* mf = (MeshFilter*)dependacyPointers[0].raw();
	if (!mf->mesh || !mf->mesh->loaded)
		return;
	bool isE = (ebv != nullptr);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, (!isE || ebv->selectedShading == 0) ? GL_FILL : GL_LINE);
	if (!isE || ebv->selectedShading == 0) glEnable(GL_CULL_FACE);
	glVertexPointer(3, GL_FLOAT, 0, &(mf->mesh->vertices[0]));
	glLineWidth(1);
	Mat4x4 m1 = MVP::modelview();
	Mat4x4 m2 = MVP::projection();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->vertices[0]));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->uv0[0]));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->normals[0]));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, 0, &(mf->mesh->tangents[0]));
	for (uint m = 0; m < mf->mesh->materialCount; m++) {
		if (!materials[m])
			continue;
		if (shader == 0) materials[m]->ApplyGL(m1, m2);
		else glUseProgram(shader);

		glDrawElements(GL_TRIANGLES, mf->mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &(mf->mesh->_matTriangles[m][0]));
	}
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_CULL_FACE);
	if (isE && mf->showBoundingBox) {
		BBox& b = mf->mesh->boundingBox;
		Engine::DrawCubeLinesW(b.x0, b.x1, b.y0, b.y1, b.z0, b.z1, 1, white(1, 0.5f));
	}
}

void MeshRenderer::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		MeshFilter* mft = (MeshFilter*)dependacyPointers[0].raw();
		if (!mft->mesh) {
			UI::Label(v.r + 2, v.g + pos + 20, 12, "No Mesh Assigned!", e->font, white());
			pos += 34;
		}
		else {
			UI::Label(v.r + 2, v.g + pos + 18, 12, "Materials: " + std::to_string(mft->mesh->materialCount), e->font, white());
			pos += 35;
			for (uint a = 0; a < mft->mesh->materialCount; a++) {
				UI::Label(v.r + 2, v.g + pos, 12, "Material " + std::to_string(a), e->font, white());
				e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_MATERIAL, 12, e->font, &_materials[a], &_UpdateMat, this);
				pos += 17;
				if (!materials[a])
					continue;
				if (overwriteWriteMask) {
					if (Engine::EButton(e->editorLayer == 0, v.r + 5, v.g + pos, 16, 16, e->tex_expand, white()) == MOUSE_RELEASE) {
						materials[a]->_maskExpanded = !materials[a]->_maskExpanded;
					}
					UI::Label(v.r + 22, v.g + pos, 12, "Write Mask", e->font, white());
					pos += 17;
					if (materials[a]->_maskExpanded) {
						for (uint ea = 0; ea < GBUFFER_NUM_TEXTURES - 1; ea++) {
							materials[a]->writeMask[ea] = Engine::Toggle(v.r + 22, v.g + pos, 16, e->tex_checkbox, materials[a]->writeMask[ea], white(), ORIENT_HORIZONTAL);
							UI::Label(v.r + 43, v.g + pos, 12, Camera::_gbufferNames[ea], e->font, white());
							pos += 17;
						}
					}
				}
				for (uint q = 0, qq = materials[a]->valOrders.size(); q < qq; q++) {
					UI::Texture(v.r + 8, v.g + pos, 16, 16, e->matVarTexs[materials[a]->valOrders[q]]);
					UI::Label(v.r + 25, v.g + pos, 12, materials[a]->valNames[materials[a]->valOrders[q]][materials[a]->valOrderIds[q]], e->font, white());
					void* bbs = materials[a]->vals[materials[a]->valOrders[q]][materials[a]->valOrderGLIds[q]];
					switch (materials[a]->valOrders[q]) {
					case SHADER_INT:
						*(int*)bbs = TryParse(UI::EditText(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.3f, 16, 12, grey1(), std::to_string(*(int*)bbs), e->font, true, nullptr, white()), *(int*)bbs);
						*(int*)bbs = (int)round(Engine::DrawSliderFill(v.r + v.b*0.6f + 23, v.g + pos, v.b*0.4f - 19, 16, 0, 1, (float)(*(int*)bbs), grey2(), white()));
						break;
					case SHADER_FLOAT:
						*(float*)bbs = TryParse(UI::EditText(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.3f, 16, 12, grey1(), std::to_string(*(float*)bbs), e->font, true, nullptr, white()), *(float*)bbs);
						*(float*)bbs = Engine::DrawSliderFill(v.r + v.b*0.6f + 23, v.g + pos, v.b*0.4f - 19, 16, 0, 1, *(float*)bbs, grey2(), white());
						break;
					case SHADER_SAMPLER:
						e->DrawAssetSelector(v.r + v.b * 0.3f + 22, v.g + pos, v.b*0.7f - 17, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &((MatVal_Tex*)bbs)->id, _UpdateTex, bbs);
						break;
					}
					pos += 17;
				}
				pos++;
			}
			UI::Label(v.r + 2, v.g + pos, 12, "Override Masks", e->font, white());
			overwriteWriteMask = Engine::Toggle(v.r + v.b*0.3f, v.g + pos, 12, e->tex_checkbox, overwriteWriteMask, white(), ORIENT_HORIZONTAL);
			pos += 17;
		}
	}
	else pos += 17;
}


void MeshRenderer::Serialize(Editor* e, std::ofstream* stream) {
	int s = _materials.size();
	_StreamWrite(&s, stream, 4);
	for (ASSETID i : _materials)
		_StreamWriteAsset(e, stream, ASSETTYPE_MATERIAL, i);
}

#endif