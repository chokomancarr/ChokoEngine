#include "Engine.h"
#include "Editor.h"

Material::Material() : _shaderId(-1), AssetObject(ASSETTYPE_MATERIAL), writeMask(4, true) {
	_shader = nullptr;// Engine::unlitProgram;
}

Material::Material(Shader * shad) : _shaderId(-1), AssetObject(ASSETTYPE_MATERIAL), writeMask(4, true) {
	_shader = shad;
	if (shad == nullptr)
		return;
#ifdef IS_EDITOR
	_shaderId = Editor::instance->GetAssetId(_shader);
#endif
	_ReloadParams();
}

Shader* Material::shader() {
	return _shader;
}
void Material::shader(Shader* shad) {
	_shader = shad;
	_ReloadParams();
}

void Material::_ReloadParams() {
	ResetVals();
	for (ShaderVariable* v : _shader->vars) {
		void* l = nullptr;
		if (v->type == SHADER_INT)
			l = new int();
		else if (v->type == SHADER_FLOAT)
			l = new float();
		else if (v->type == SHADER_SAMPLER) {
			l = new MatVal_Tex();
			((MatVal_Tex*)l)->defTex = defTexs[v->def.i];
		}
		valNames[v->type].push_back(v->name);
		GLint ii = glGetUniformLocation(_shader->pointer, v->name.c_str());
		if (ii > -1) {
			vals[v->type].emplace(ii, l);
			valOrders.push_back(v->type);
			valOrderIds.push_back((byte)(valNames[v->type].size() - 1));
			valOrderGLIds.push_back(ii);
		}
		else {
#ifdef IS_EDITOR
			Editor::instance->_Warning("Material", "Shader parameter " + v->name + " not used!");
#endif
		}
	}
}

Material::Material(string p) : AssetObject(ASSETTYPE_MATERIAL), writeMask(4, true) {
#ifdef IS_EDITOR
	//string p = Editor::instance->projectFolder + "Assets\\" + path;
	//std::ifstream stream(p.c_str());
	F2ISTREAM(stream, p);
	if (!stream.good()) {
		std::cout << "material not found!" << std::endl;
		return;
	}
	char* c = new char[4];
	stream.read(c, 3);
	c[3] = (char)0;
	string ss(c);
	if (ss != "KTC") {
		std::cerr << "file not supported" << std::endl;
		return;
	}
	delete[](c);
	char* nmm = new char[100];
	stream.getline(nmm, 100, (char)0);
	string shp(nmm);
	if (shp == "") {
		delete[](nmm);
		return;
	}
	ASSETTYPE t;
	Editor::instance->GetAssetInfo(shp, t, _shaderId);
	_shader = _GetCache<Shader>(ASSETTYPE_SHADER, _shaderId);

	if (!_shader) {
		delete[](nmm);
		return;
	}
	ResetVals();
	std::unordered_map<string, GLint> nMap;
	for (ShaderVariable* v : _shader->vars) {
		void* l = nullptr;
		if (v->type == SHADER_INT)
			l = new int();
		else if (v->type == SHADER_FLOAT)
			l = new float();
		else if (v->type == SHADER_SAMPLER) {
			l = new MatVal_Tex();
			((MatVal_Tex*)l)->defTex = defTexs[v->def.i];
		}
		valNames[v->type].push_back(v->name);
		GLint loc = glGetUniformLocation(_shader->pointer, v->name.c_str());
		if (loc > -1) {
			vals[v->type].emplace(loc, l);
			nMap.emplace(v->name, loc);
			valOrders.push_back(v->type);
			valOrderIds.push_back((byte)(valNames[v->type].size() - 1));
			valOrderGLIds.push_back(loc);
		}
		else
			Editor::instance->_Warning("Material", "Shader parameter " + v->name + " not used!");
	}

	int vs;
	_Strm2Val(stream, vs);
	for (int r = 0; r < vs; r++) {
		char ii;
		stream >> ii;
		nmm = new char[100];
		stream.getline(nmm, 100, (char)0);
		string nm(nmm);
		switch (ii) {
		case SHADER_FLOAT:
			for (int x = vals[SHADER_FLOAT].size() - 1; x >= 0; x--) {
				if (valNames[SHADER_FLOAT][x] == nm) {
					float f;
					_Strm2Val(stream, f);
					(*(float*)vals[SHADER_FLOAT][nMap[nm]]) += f;
					break;
				}
			}
			break;
		case SHADER_INT:
			for (int x = vals[SHADER_INT].size() - 1; x >= 0; x--) {
				if (valNames[SHADER_INT][x] == nm) {
					int f;
					_Strm2Val(stream, f);
					(*(int*)vals[SHADER_INT][nMap[nm]]) += f;
					break;
				}
			}
			break;
		case SHADER_SAMPLER:
			for (int x = vals[SHADER_SAMPLER].size() - 1; x >= 0; x--) {
				if (valNames[SHADER_SAMPLER][x] == nm) {
					char* nmm2 = new char[100];
					stream.getline(nmm2, 100, (char)0);
					string nm2(nmm2);
					ASSETTYPE t;
					if (vals[SHADER_SAMPLER][nMap[nm]] == nullptr) {
						delete[](nmm2);
						break;
					}
					Editor::instance->GetAssetInfo(nm2, t, ((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id);
					((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, ((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id);
					delete[](nmm2);
					break;
				}
			}
			break;
		}
	}
	delete[](nmm);
	//stream.close();
#endif
}

Material::Material(std::istream& stream, uint offset) : AssetObject(ASSETTYPE_MATERIAL), writeMask(4, true) {
	if (stream.good()) {
		stream.seekg(offset);
		char* c = new char[4];
		stream.read(c, 3);
		c[3] = (char)0;
		string ss(c);
		if (ss != "KTC") {
			std::cerr << "file not supported" << std::endl;
			return;
		}
		delete[](c);
		char* nmm = new char[100];
		stream.getline(nmm, 100, (char)0);
		string shp(nmm);
		if (shp == "") {
			delete[](nmm);
			return;
		}
		offset = (uint)stream.tellg();
		ASSETTYPE t;
		_shaderId = AssetManager::GetAssetId(shp, t);
		_shader = _GetCache<Shader>(ASSETTYPE_SHADER, _shaderId);
		stream.seekg(offset);

		if (_shader == nullptr) {
			delete[](nmm);
			return;
		}
		ResetVals();
		std::unordered_map<string, GLint> nMap;
		for (ShaderVariable* v : _shader->vars) {
			void* l = nullptr;
			if (v->type == SHADER_INT)
				l = new int();
			else if (v->type == SHADER_FLOAT)
				l = new float();
			else if (v->type == SHADER_SAMPLER)
				l = new MatVal_Tex();
			valNames[v->type].push_back(v->name);
			GLint loc = glGetUniformLocation(_shader->pointer, v->name.c_str());
			if (loc > -1) {
				vals[v->type].emplace(loc, l);
				nMap.emplace(v->name, loc);
				valOrders.push_back(v->type);
				valOrderIds.push_back((byte)(valNames[v->type].size() - 1));
				valOrderGLIds.push_back(loc);
			}
			else
				Debug::Warning("Material", "Shader parameter " + v->name + " not used!");
		}
		int vs;
		_Strm2Val(stream, vs);
		for (int r = 0; r < vs; r++) {
			char ii;
			stream >> ii;
			nmm = new char[100];
			stream.getline(nmm, 100, (char)0);
			string nm(nmm);
			switch (ii) {
			case SHADER_FLOAT:
				for (int x = vals[SHADER_FLOAT].size() - 1; x >= 0; x--) {
					if (valNames[SHADER_FLOAT][x] == nm) {
						float f;
						_Strm2Val(stream, f);
						(*(float*)vals[SHADER_FLOAT][nMap[nm]]) += f;
						break;
					}
				}
				break;
			case SHADER_INT:
				for (int x = vals[SHADER_INT].size() - 1; x >= 0; x--) {
					if (valNames[SHADER_INT][x] == nm) {
						int f;
						_Strm2Val(stream, f);
						(*(int*)vals[SHADER_INT][nMap[nm]]) += f;
						break;
					}
				}
				break;
			case SHADER_SAMPLER:
				for (int x = vals[SHADER_SAMPLER].size() - 1; x >= 0; x--) {
					if (valNames[SHADER_SAMPLER][x] == nm) {
						char* nmm2 = new char[100];
						stream.getline(nmm2, 100, (char)0);
						string nm2(nmm2);
						ASSETTYPE t;
						if (vals[SHADER_SAMPLER][nMap[nm]] == nullptr) {
							delete[](nmm2);
							break;
						}
						offset = (uint)stream.tellg();
						((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id = AssetManager::GetAssetId(nm2, t);
						((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, ((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id);
						stream.seekg(offset);
						delete[](nmm2);
						break;
					}
				}
				break;
			}
		}
		delete[](nmm);
	}
}

Material::~Material() {
	for (auto& a : vals) {
		for (auto& b : a.second)
			delete(b.second);
	}
}

void Material::ResetVals() {
	vals[SHADER_INT] = std::unordered_map <GLint, void*>();
	vals[SHADER_FLOAT] = std::unordered_map <GLint, void*>();
	vals[SHADER_VEC2] = std::unordered_map <GLint, void*>();
	vals[SHADER_VEC3] = std::unordered_map <GLint, void*>();
	vals[SHADER_SAMPLER] = std::unordered_map <GLint, void*>();
	vals[SHADER_MATRIX] = std::unordered_map <GLint, void*>();
	vals[SHADER_BUFFER] = std::unordered_map <GLint, void*>();
	valNames[SHADER_INT] = std::vector<string>();
	valNames[SHADER_FLOAT] = std::vector<string>();
	valNames[SHADER_VEC2] = std::vector<string>();
	valNames[SHADER_VEC3] = std::vector<string>();
	valNames[SHADER_SAMPLER] = std::vector<string>();
	valNames[SHADER_MATRIX] = std::vector<string>();
	valNames[SHADER_BUFFER] = std::vector<string>();
	valOrders = std::vector<SHADER_VARTYPE>();
}

void Material::SetBuffer(uint location, IComputeBuffer* buffer) {
	vals[SHADER_BUFFER][location] = buffer;
}

void Material::SetTexture(string name, Texture* texture) {
	SetTexture(glGetUniformLocation(_shader->pointer, name.c_str()), texture);
}
void Material::SetTexture(GLint id, Texture* texture) {
	if (id > -1) {
		if (vals[SHADER_SAMPLER].find(id) == vals[SHADER_SAMPLER].end()) vals[SHADER_SAMPLER][id] = new MatVal_Tex();
		((MatVal_Tex*)vals[SHADER_SAMPLER][id])->tex = texture;
	}
}

void Material::SetFloat(string name, float val) {
	SetFloat(glGetUniformLocation(_shader->pointer, name.c_str()), val);
}
void Material::SetFloat(GLint id, float val) {
	if (id > -1)
		vals[SHADER_FLOAT][id] = new float(val);
}

void Material::SetInt(string name, int val) {
	SetInt(glGetUniformLocation(_shader->pointer, name.c_str()), val);
}
void Material::SetInt(GLint id, int val) {
	if (id > -1)
		vals[SHADER_INT][id] = new int(val);
}

void Material::SetVec2(string name, Vec2 val) {
	SetVec2(glGetUniformLocation(_shader->pointer, name.c_str()), val);
}
void Material::SetVec2(GLint id, Vec2 val) {
	if (id > -1)
		vals[SHADER_VEC2][id] = new Vec2(val);
}

std::array<GLuint, 7> Material::defTexs = std::array<GLuint, 7>();

void Material::LoadOris() {
	std::vector<byte> data(12, 0x00);

	glGenTextures(1, &defTexs[0]);
	glBindTexture(GL_TEXTURE_2D, defTexs[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = 0x80;
	glGenTextures(1, &defTexs[1]);
	glBindTexture(GL_TEXTURE_2D, defTexs[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = 0xFF;
	glGenTextures(1, &defTexs[2]);
	glBindTexture(GL_TEXTURE_2D, defTexs[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = (a / 3 == 0) ? 0xFF : 0x00;
	glGenTextures(1, &defTexs[3]);
	glBindTexture(GL_TEXTURE_2D, defTexs[3]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = ((a - 1) / 3 == 0) ? 0xFF : 0x00;
	glGenTextures(1, &defTexs[4]);
	glBindTexture(GL_TEXTURE_2D, defTexs[4]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = ((a + 1) / 3 == 0) ? 0xFF : 0x00;
	glGenTextures(1, &defTexs[5]);
	glBindTexture(GL_TEXTURE_2D, defTexs[5]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = ((a + 1) / 3 == 0) ? 0xFF : 0x80;
	glGenTextures(1, &defTexs[6]);
	glBindTexture(GL_TEXTURE_2D, defTexs[6]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Material::_UpdateTexCache(void* v) {
	Material* mat = (Material*)v;
	for (auto& a : mat->vals[SHADER_SAMPLER]) {
		if (a.second == nullptr)
			continue;
		MatVal_Tex* tx = (MatVal_Tex*)a.second;
		tx->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, tx->id);
	}
}

#ifdef IS_EDITOR
void Material::Save(string path) {
	std::ofstream strm(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	strm << "KTC";
	ASSETTYPE st;
	ASSETID si = Editor::instance->GetAssetId(_shader, st);
	_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_SHADER, si);
	int i = 0, j = 0;
	SHADER_VARTYPE t = SHADER_INT;
	long long p1 = strm.tellp();
	strm << "0000";
	//_StreamWrite(&i, &strm, 4);
	for (auto& v : vals[SHADER_INT]) {
		t = SHADER_INT;
		strm << (char)t;
		strm << valNames[SHADER_INT][j] << (char)0;
		int ii(*(int*)v.second);
		_StreamWrite(&ii, &strm, 4);
		i++;
		j++;
	}
	j = 0;
	for (auto& v : vals[SHADER_FLOAT]) {
		t = SHADER_FLOAT;
		strm << (char)t;
		strm << valNames[SHADER_FLOAT][j] << (char)0;
		float ff(*(float*)v.second);
		_StreamWrite(&ff, &strm, 4);
		i++;
		j++;
	}
	j = 0;
	for (auto& v : vals[SHADER_SAMPLER]) {
		if (v.second == nullptr)
			continue;
		t = SHADER_SAMPLER;
		strm << (char)t;
		strm << valNames[SHADER_SAMPLER][j] << (char)0;
		_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_TEXTURE, ((MatVal_Tex*)v.second)->id);
		i++;
		j++;
	}
	strm.seekp(p1);
	_StreamWrite(&i, &strm, 4);
	strm.close();
}
#endif

void Material::ApplyGL(Mat4x4& _mv, Mat4x4& _p) {
	if (_shader == nullptr || !_shader->loaded) {
		//Debug::Warning("mat", "no shader " + string((shader) ? "()" : ""));
		glUseProgram(0);
		return;
	}
	else {
		glUseProgram(_shader->pointer);
		GLint mv = glGetUniformLocation(_shader->pointer, "_M");
		GLint p = glGetUniformLocation(_shader->pointer, "_VP");
		GLint mvp = glGetUniformLocation(_shader->pointer, "_MVP");
		glUniformMatrix4fv(mv, 1, GL_FALSE, glm::value_ptr(_mv));
		glUniformMatrix4fv(p, 1, GL_FALSE, glm::value_ptr(_p));
		glUniformMatrix4fv(mvp, 1, GL_FALSE, glm::value_ptr(_p*_mv));
		//glUniformMatrix4fv(p, 1, GL_FALSE, matrix2);
		for (auto& a : vals[SHADER_INT])
			if (a.second != nullptr)
				glUniform1i(a.first, *(int*)a.second);
		for (auto& a : vals[SHADER_FLOAT])
			if (a.second != nullptr)
				glUniform1f(a.first, *(float*)a.second);
		for (auto& a : vals[SHADER_VEC2]) {
			if (a.second == nullptr)
				continue;
			Vec2* v2 = (Vec2*)a.second;
			glUniform2f(a.first, v2->x, v2->y);
		}
		int ti = -1;
		for (auto& a : vals[SHADER_SAMPLER]) {
			ti++;
			if (a.second == nullptr)
				continue;
			MatVal_Tex* tx = (MatVal_Tex*)a.second;
			glUniform1i(a.first, ti);
			glActiveTexture(GL_TEXTURE0 + ti);
			if (tx->tex == nullptr)
				glBindTexture(GL_TEXTURE_2D, tx->defTex);
			else
				glBindTexture(GL_TEXTURE_2D, tx->tex->pointer);
		}
		for (auto&a : vals[SHADER_BUFFER]) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, a.first, ((IComputeBuffer*)a.second)->pointer);
		}
		bool wm;
		for (uint m = 0; m < GBUFFER_NUM_TEXTURES - 1; m++) {
			wm = writeMask[m];
			//glColorMaski(m, wm, wm, wm, wm);
		}
	}
}
