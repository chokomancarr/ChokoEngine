#include "Engine.h"
#include "Editor.h"

ComputeShader* SkinnedMeshRenderer::skinningProg = 0;

SkinnedMeshRenderer::SkinnedMeshRenderer(SceneObject* o) : Component("Skinned Mesh Renderer", COMP_SRD, DRAWORDER_SOLID, o) {
	if (!o) {
		Debug::Error("SMR", "SceneObject cannot be null!");
	}
	rSceneObject& par = object->parent;
	while (par) {
		armature = par->GetComponent<Armature>().get();
		if (armature) break;
		else par = par->parent;
	}
	if (!armature) {
		Debug::Error("SkinnedMeshRenderer", "Cannot add Skin to object without armature!");
		dead = true;
	}
}

SkinnedMeshRenderer::SkinnedMeshRenderer(std::ifstream& stream, SceneObject* o, long pos) : Component("Skinned Mesh Renderer", COMP_SRD, DRAWORDER_SOLID, o) {

}

void SkinnedMeshRenderer::mesh(Mesh* m) {
	_mesh = m;
	InitWeights();
}

void SkinnedMeshRenderer::InitWeights() {
	std::vector<uint> noweights;
	weights.clear();
	auto bsz = armature->_allbones.size();
	weights.resize(_mesh->vertexCount);
	std::vector<SkinDats> dats(_mesh->vertexCount);
	for (uint i = 0; i < _mesh->vertexCount; i++) {
		byte a = 0;
		float tot = 0;
		for (auto& g : _mesh->vertexGroupWeights[i]) {
			auto bn = armature->MapBone(_mesh->vertexGroups[g.first]);
			if (!bn) continue;
			weights[i][a].first = bn;
			weights[i][a].second = g.second;
			dats[i].mats[a] = bn->id;
			tot += g.second;
			if (++a == 4) break;
		}
		for (byte b = a; b < 4; b++) {
			weights[i][b].first = armature->_bones[0];
		}
		if (a == 0) {
			noweights.push_back(i);
			weights[i][0].second = 1;
			dats[i].weights[0] = 1;
		}
		else {
			while (a > 0) {
				weights[i][a - 1].second /= tot;
				dats[i].weights[a - 1] = weights[i][a - 1].second;
				a--;
			}
		}
	}

	if (skinBufPoss) {
		delete(skinBufPoss);
		delete(skinBufNrms);
		delete(skinBufPossO);
		delete(skinBufNrmsO);
		delete(skinBufDats);
		delete(skinBufMats);
	}
	skinBufPoss = new ComputeBuffer<Vec4>(_mesh->vertexCount);
	skinBufNrms = new ComputeBuffer<Vec4>(_mesh->vertexCount);
	skinBufPossO = new ComputeBuffer<Vec4>(_mesh->vertexCount);
	skinBufNrmsO = new ComputeBuffer<Vec4>(_mesh->vertexCount);
	skinBufDats = new ComputeBuffer<SkinDats>(_mesh->vertexCount);
	skinBufMats = new ComputeBuffer<Mat4x4>(ARMATURE_MAX_BONES);
	skinBufPoss->Set(&_mesh->vertices[0].x, sizeof(Vec4), sizeof(Vec3));
	skinBufNrms->Set(&_mesh->normals[0].x, sizeof(Vec4), sizeof(Vec3));
	skinBufDats->Set(&dats[0]);
	skinDispatchGroups = (uint)ceilf(((float)_mesh->vertexCount) / SKINNED_THREADS_PER_GROUP);

	if (!!noweights.size())
		Debug::Warning("SMR", std::to_string(noweights.size()) + " vertices in \"" + _mesh->name + "\" have no weights assigned!");
}

void SkinnedMeshRenderer::DrawDeferred(GLuint shader) {
	if (!_mesh || !_mesh->loaded)
		return;
	Skin();

	glEnableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
	glVertexPointer(3, GL_FLOAT, 0, &(_mesh->vertices[0]));
	glLineWidth(1);
	Mat4x4 m1 = MVP::modelview();
	Mat4x4 m2 = MVP::projection();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, skinBufPossO->pointer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, sizeof(Vec4), 0);
	glBindBuffer(GL_ARRAY_BUFFER, skinBufNrmsO->pointer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(Vec4), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, 0, &(_mesh->tangents[0]));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &(_mesh->uv0[0]));
	for (uint m = 0; m < _mesh->materialCount; m++) {
		if (!materials[m])
			continue;
		if (shader == 0) materials[m]->ApplyGL(m1, m2);
		else glUseProgram(shader);
		glDrawElements(GL_TRIANGLES, _mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &(_mesh->_matTriangles[m][0]));
	}
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_CULL_FACE);
}

void SkinnedMeshRenderer::InitSkinning() {
	skinningProg = new ComputeShader(DefaultResources::GetStr("gpuskin.txt"));
}

void SkinnedMeshRenderer::Skin() {
	skinBufMats->Set(&armature->_animMatrices[0][0].x);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, skinBufPoss->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, skinBufNrms->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, skinBufDats->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, skinBufMats->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, skinBufPossO->pointer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, skinBufNrmsO->pointer);
	skinningProg->Dispatch(skinDispatchGroups, 1, 1);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, 0);

}

void SkinnedMeshRenderer::SetMesh(int i) {
	_meshId = i;
	if (i >= 0) {
		_mesh = _GetCache<Mesh>(ASSETTYPE_MESH, i);
		InitWeights();
	}
	else
		_mesh = nullptr;
	if (!_mesh || !_mesh->loaded) {
		materials.clear();
		_materials.clear();
	}
	else {
		materials.resize(_mesh->materialCount, nullptr);
		_materials.resize(_mesh->materialCount, -1);
	}
}

void SkinnedMeshRenderer::_UpdateMesh(void* i) {
	SkinnedMeshRenderer* mf = (SkinnedMeshRenderer*)i;
	if (mf->_meshId != -1) {
		mf->_mesh = _GetCache<Mesh>(ASSETTYPE_MESH, mf->_meshId);
	}
	else
		mf->_mesh = nullptr;
}
void SkinnedMeshRenderer::_UpdateMat(void* i) {
	SkinnedMeshRenderer* mf = (SkinnedMeshRenderer*)i;
	for (int q = mf->_materials.size() - 1; q >= 0; q--) {
		mf->materials[q] = _GetCache<Material>(ASSETTYPE_MATERIAL, mf->_materials[q]);
	}
}
void SkinnedMeshRenderer::_UpdateTex(void* i) {
	MatVal_Tex* v = (MatVal_Tex*)i;
	v->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, v->id);
}

#ifdef IS_EDITOR

void SkinnedMeshRenderer::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		UI::Label(v.r + 2, v.g + pos + 20, 12, "Mesh", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_MESH, 12, e->font, &_meshId, &_UpdateMesh, this);
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "Show Bounding Box", e->font, white());
		showBoundingBox = Engine::Toggle(v.r + v.b*0.3f, v.g + pos, 12, e->tex_checkbox, showBoundingBox, white(), ORIENT_HORIZONTAL);
		pos += 17;

		if (!_mesh) {
			UI::Label(v.r + 2, v.g + pos + 20, 12, "No Mesh Assigned!", e->font, white());
			pos += 34;
		}
		else {
			UI::Label(v.r + 2, v.g + pos, 12, "Materials: " + std::to_string(_mesh->materialCount), e->font, white());
			pos += 17;
			for (uint a = 0; a < _mesh->materialCount; a++) {
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

void SkinnedMeshRenderer::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	if (!_mesh || !_mesh->loaded)
		return;
	bool isE = !!ebv;

	Skin();

	glEnableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, (!isE || ebv->selectedShading == 0) ? GL_FILL : GL_LINE);
	if (!isE || ebv->selectedShading == 0) glEnable(GL_CULL_FACE);
	glVertexPointer(3, GL_FLOAT, 0, &(_mesh->vertices[0]));
	glLineWidth(1);
	Mat4x4 m1 = MVP::modelview();
	Mat4x4 m2 = MVP::projection();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, skinBufPossO->pointer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, sizeof(Vec4), 0);
	glBindBuffer(GL_ARRAY_BUFFER, skinBufNrmsO->pointer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(Vec4), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, 0, &(_mesh->tangents[0]));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &(_mesh->uv0[0]));
	for (uint m = 0; m < _mesh->materialCount; m++) {
		if (!materials[m])
			continue;
		if (shader == 0) materials[m]->ApplyGL(m1, m2);
		else glUseProgram(shader);
		glDrawElements(GL_TRIANGLES, _mesh->_matTriangles[m].size(), GL_UNSIGNED_INT, &(_mesh->_matTriangles[m][0]));
	}
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_CULL_FACE);
	if (isE && showBoundingBox) {
		BBox& b = _mesh->boundingBox;
		Engine::DrawCubeLinesW(b.x0, b.x1, b.y0, b.y1, b.z0, b.z1, 1, white(1, 0.5f));
	}
}

#endif