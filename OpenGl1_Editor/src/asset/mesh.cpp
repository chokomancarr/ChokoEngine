#include "Engine.h"
#include "Editor.h"

#define F2ISTREAM(_varname, _pathname) std::ifstream _f2i_ifstream((_pathname).c_str(), std::ios::in | std::ios::binary); \
std::istream _varname(_f2i_ifstream.rdbuf());

Mesh::Mesh(const std::vector<Vec3>& verts, const std::vector<Vec3>& norms, const std::vector<int>& tris, std::vector<Vec2> uvs) : AssetObject(ASSETTYPE_MESH) {
	vertices = std::vector<Vec3>(verts);
	vertexCount = verts.size();
	normals = std::vector<Vec3>(norms);
	triangles = std::vector<int>(tris);
	triangleCount = tris.size() / 3;
	uv0 = std::vector<Vec2>(uvs);
	materialCount = 1;
	_matTriangles.push_back(std::vector<int>(tris));
	GLuint buf;
	glGenBuffers(1, &buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tris.size() * 4, &tris[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	_matIndicesBuffers.push_back(buf);
	loaded = (vertexCount > 0) && (normals.size() == vertexCount) && (triangleCount > 0);
	if (!loaded) return;
	CalcTangents();
	RecalculateBoundingBox();
	InitVao();
}

Mesh::Mesh(Editor* e, int i) : AssetObject(ASSETTYPE_MESH) {
	Mesh* m2 = _GetCache<Mesh>(type, i);
	vertices = m2->vertices;
	vertexCount = m2->vertexCount;
	normals = m2->normals;
	tangents = m2->tangents;
	//bitangents = m2->bitangents;
	triangles = m2->triangles;
	triangleCount = m2->triangleCount;
	boundingBox = m2->boundingBox;
	loaded = m2->loaded;
	if (loaded) InitVao();
}

Mesh::Mesh(std::istream& stream, uint offset) : AssetObject(ASSETTYPE_MESH), loaded(false), vertexCount(0), triangleCount(0), materialCount(0) {
	if (stream.good()) {
		stream.seekg(offset);

		char* c = new char[100];
		stream.read(c, 6);
		c[6] = char0;
		if (string(c) != "KTO123") {
			Debug::Error("Mesh importer", "file not supported");
			return;
		}
		stream.getline(c, 100, 0);
		name += string(c);
		delete[](c);

		char cc;
		cc = stream.get();

		while (cc != 0) {
			if (cc == 'V') {
				_Strm2Val(stream, vertexCount);
				for (uint vc = 0; vc < vertexCount; vc++) {
					Vec3 v;
					_Strm2Val(stream, v.x);
					_Strm2Val(stream, v.y);
					_Strm2Val(stream, v.z);
					vertices.push_back(v);
					_Strm2Val(stream, v.x);
					_Strm2Val(stream, v.y);
					_Strm2Val(stream, v.z);
					normals.push_back(v);
				}
			}
			else if (cc == 'F') {
				_Strm2Val(stream, triangleCount);
				for (uint fc = 0; fc < triangleCount; fc++) {
					byte m;
					_Strm2Val(stream, m);
					while (materialCount <= m) {
						_matTriangles.push_back(std::vector<int>());
						materialCount++;
					}
					uint i;
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
					triangles.push_back(i);
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
					triangles.push_back(i);
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
					triangles.push_back(i);
				}
			}
			else if (cc == 'U') {
				byte c;
				_Strm2Val(stream, c);
				for (uint vc = 0; vc < vertexCount; vc++) {
					Vec2 i;
					_Strm2Val(stream, i.x);
					_Strm2Val(stream, i.y);
					uv0.push_back(i);
				}
				if (c > 1) {
					for (uint vc = 0; vc < vertexCount; vc++) {
						Vec2 i;
						_Strm2Val(stream, i.x);
						_Strm2Val(stream, i.y);
						uv1.push_back(i);
					}
				}
			}
			else {
				Debug::Error("Mesh Importer", "Unknown char: " + std::to_string(cc));
			}
			cc = stream.get();
		}

		if (vertexCount > 0) {
			if (normals.size() != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (normals incomplete)!");
				return;
			}
			else if (uv0.size() == 0)
				uv0.resize(vertexCount);
			else if (uv0.size() != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (uv0 incomplete)!");
				return;
			}
			if (uv1.size() == 0)
				uv1.resize(vertexCount);
			else if (uv1.size() != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (uv1 incomplete)!");
				return;
			}
			GLuint buf;
			for (uint i = 0; i < materialCount; i++) {
				glGenBuffers(1, &buf);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, _matTriangles[i].size() * 4, &_matTriangles[i][0], GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				_matIndicesBuffers.push_back(buf);
			}
			CalcTangents();
			RecalculateBoundingBox();
			InitVao();
			loaded = true;
		}
		return;
	}
}

Mesh::Mesh(string p) : AssetObject(ASSETTYPE_MESH), loaded(false), vertexCount(0), triangleCount(0), materialCount(0) {
	F2ISTREAM(stream, p);
	if (!stream.good()) {
		std::cout << "mesh file not found!" << std::endl;
		return;
	}

	char* c = new char[100];
	stream.read(c, 6);
	c[6] = char0;
	if (string(c) != "KTO123") {
		Debug::Error("Mesh importer", "file not supported");
		return;
	}
	stream.getline(c, 100, 0);
	name += string(c);
	delete[](c);

	char cc;
	stream.read(&cc, 1);

	byte b;
	string s;
	Vec2 v2;
	Vec3 v3;

	//std::cout << path << std::endl;
	while (cc != 0 && !stream.eof()) {
		switch (cc) {
		case 'V':
			_Strm2Val(stream, vertexCount);
			vertices.reserve(vertexCount);
			normals.reserve(vertexCount);
			uv0.reserve(vertexCount);
			uv1.reserve(vertexCount);
			vertexGroups.reserve(vertexCount);
			for (uint vc = 0; vc < vertexCount; vc++) {
				_Strm2Val(stream, v3.x);
				_Strm2Val(stream, v3.y);
				_Strm2Val(stream, v3.z);
				vertices.push_back(v3);
				_Strm2Val(stream, v3.x);
				_Strm2Val(stream, v3.y);
				_Strm2Val(stream, v3.z);
				normals.push_back(v3);
			}
			break;
		case 'F':
			_Strm2Val(stream, triangleCount);
			triangles.reserve(triangleCount * 3);
			for (uint fc = 0; fc < triangleCount; fc++) {
				byte m;
				_Strm2Val(stream, m);
				while (materialCount <= m) {
					_matTriangles.push_back(std::vector<int>());
					materialCount++;
				}
				uint i;
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
				triangles.push_back(i);
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
				triangles.push_back(i);
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
				triangles.push_back(i);
			}
			break;
		case 'U':
			_Strm2Val(stream, b);
			for (uint vc = 0; vc < vertexCount; vc++) {
				_Strm2Val(stream, v2.x);
				_Strm2Val(stream, v2.y);
				uv0.push_back(v2);
			}
			if (b > 1) {
				for (uint vc = 0; vc < vertexCount; vc++) {
					_Strm2Val(stream, v2.x);
					_Strm2Val(stream, v2.y);
					uv1.push_back(v2);
				}
			}
			break;
		case 'G':
			_Strm2Val(stream, b);
			while (b > 0) {
				//char* cc = new char[100];
				//stream.getline(cc, 100, char0);
				//vertexGroups.push_back(string(cc));
				std::getline(stream, s, char0);
				vertexGroups.push_back(s);
				b--;
			}
			vertexGroupWeights.resize(vertexCount);
			for (uint vc = 0; vc < vertexCount; vc++) {
				_Strm2Val(stream, b);
				byte dd;
				float ff;
				vertexGroupWeights[vc].reserve(b);
				while (b > 0) {
					_Strm2Val(stream, dd);
					_Strm2Val(stream, ff);
					vertexGroupWeights[vc].push_back(std::pair<byte, float>(dd, ff));
					b--;
				}
			}
			break;
		case 'S':
			_Strm2Val(stream, b);
			shapekeys.resize(b);
			for (byte a = 0; a < b; a++) {
				std::getline(stream, s, char0);
				shapekeys[a].first = s;
				shapekeys[a].second.resize(vertexCount);
				for (uint i = 0; i < vertexCount; i++) {
					_Strm2Val(stream, v3.x);
					_Strm2Val(stream, v3.y);
					_Strm2Val(stream, v3.z);
					shapekeys[a].second[i] = v3;
				}
			}
			break;
		default:
			Debug::Error("Mesh Importer", "Unknown char: " + std::to_string(cc));
		}
		stream.read(&cc, 1);
	}
	if (vertexCount > 0) {
		if (normals.size() != vertexCount) {
			Debug::Error("Mesh Importer", "mesh metadata corrupted (normals incomplete)!");
			return;
		}
		else if (uv0.size() == 0)
			uv0.resize(vertexCount);
		else if (uv0.size() != vertexCount) {
			Debug::Error("Mesh Importer", "mesh metadata corrupted (uv0 incomplete)!");
			return;
		}
		if (uv1.size() == 0)
			uv1.resize(vertexCount);
		else if (uv1.size() != vertexCount) {
			Debug::Error("Mesh Importer", "mesh metadata corrupted (uv1 incomplete)!");
			return;
		}
		GLuint buf;
		for (uint i = 0; i < materialCount; i++) {
			glGenBuffers(1, &buf);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, _matTriangles[i].size() * 4, &_matTriangles[i][0], GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			_matIndicesBuffers.push_back(buf);
		}
		CalcTangents();
		RecalculateBoundingBox();
		InitVao();

		GenECache();
		loaded = true;
	}
}

void Mesh::RecalculateBoundingBox() {
	uint sz = vertices.size();
	if (sz == 0) {
		boundingBox = BBox();
		return;
	}
	boundingBox = BBox(vertices[0].x, vertices[0].x, vertices[0].x, vertices[0].x, vertices[0].x, vertices[0].x);
	for (uint i = 1; i < sz; i++) {
		Vec3& v = vertices[i];
		boundingBox.x0 = min(boundingBox.x0, v.x);
		boundingBox.x1 = max(boundingBox.x1, v.x);
		boundingBox.y0 = min(boundingBox.y0, v.y);
		boundingBox.y1 = max(boundingBox.y1, v.y);
		boundingBox.z0 = min(boundingBox.z0, v.z);
		boundingBox.z1 = max(boundingBox.z1, v.z);
	}
}

void Mesh::CalcTangents() {
	tangents = std::vector<Vec3>(vertices.size(), Vec3(0, 0, 0));
	std::vector<int> tC = std::vector<int>(vertices.size(), 0);

	if (!uv0.size()) return;

	Vec2 u0, u1, u2, r1, r2;
	Vec3 p0, p1, p2;
	float a, b, db;
	int n0, n1, n2;
	for (uint n = 0; n < triangleCount; n++) {
		n0 = triangles[n * 3];
		n1 = triangles[n * 3 + 1];
		n2 = triangles[n * 3 + 2];

		u0 = uv0[n0];
		u1 = uv0[n1];
		u2 = uv0[n2];
		p0 = vertices[n0];
		p1 = vertices[n1];
		p2 = vertices[n2];

		if ((u0 == u1) || (u1 == u2) || (u0 == u2)) {
			//Debug::Warning("Tangent calculator", "Triangle " + std::to_string(n) + " does not have well-defined UVs!");
			continue;
		}
		r1 = u1 - u0;
		r2 = u2 - u0;

		//t = ar + br';
		db = r1.y*r2.x - r1.x*r2.y;
		if (db == 0) {
			//Debug::Warning("Tangent calculator", "Triangle " + std::to_string(n) + " does not have well-defined UVs!");
			continue;
		}
		b = r1.y / db;
		if (r1.y == 0) a = (1 - b*r2.x) / r1.x;
		else a = -b*r2.y / r1.y;
		Vec3 t = (p1 - p0)*a + (p2 - p0)*b;
		tangents[n0] += t;
		tangents[n1] += t;
		tangents[n2] += t;
		tC[n0]++;
		tC[n1]++;
		tC[n2]++;
	}
	for (uint a = 0; a < vertexCount; a++) {
		if (tC[a] > 0) tangents[a] = Normalize(tangents[a] / ((float)tC[a]));
	}
}

Mesh::Mesh(byte* mem) : AssetObject(ASSETTYPE_MESH), triangleCount(0), materialCount(0) {
#ifndef IS_EDITOR
#define RD(tar, sz) memcpy(tar, mem, sz); mem += sz;
#define VCT(typ, sz) std::vector<typ>((typ*)mem, (typ*)(mem + sizeof(typ)*sz)); mem += sizeof(typ)*sz;
	RD(&vertexCount, sizeof(uint));
	RD(&materialCount, sizeof(byte));
	vertices = VCT(Vec3, vertexCount);
	normals = VCT(Vec3, vertexCount);
	tangents = VCT(Vec3, vertexCount);
	uv0 = VCT(Vec2, vertexCount);
	uv1 = VCT(Vec2, vertexCount);
	for (byte mt = 0; mt < materialCount; mt++) {
		uint ct = 0;
		RD(&ct, sizeof(uint));
		triangleCount += ct;
		_matTriangles.push_back(std::vector<int>((int*)mem, (int*)(mem + sizeof(int) * 3 * ct)));
		mem += sizeof(int) * 3 * ct;
	}
#undef RD
#undef VCT
	RecalculateBoundingBox();
	loaded = true;
#endif
}

void Mesh::GenECache() {
	/*
	vertexcount (uint)
	materialcount (byte)
	vertices[] (vec3)
	normals[] (vec3)
	tangents[] (vec3)
	uv0[] (vec2)
	uv1[] (vec2)
	for each material[
	trianglecount (uint)
	triangles[] (uint*3)
	]
	*/
#ifdef IS_EDITOR
	if (_eCache) return;
	_eCacheSz = sizeof(uint) + sizeof(byte) + sizeof(Vec3)*vertexCount * 3 + sizeof(Vec2)*vertexCount * 2 + sizeof(uint)*materialCount + sizeof(uint)*triangleCount * 3;
	uint off = 1;
	_eCache = (byte*)malloc(_eCacheSz + 1);
	*_eCache = 255;
#define CPY(pt, sz) memcpy(_eCache + off, pt, sz); off += sz;
	CPY(&vertexCount, sizeof(uint));
	CPY(&materialCount, sizeof(byte));
	CPY(&vertices[0], sizeof(Vec3)*vertexCount);
	CPY(&normals[0], sizeof(Vec3)*vertexCount);
	CPY(&tangents[0], sizeof(Vec3)*vertexCount);
	CPY(&uv0[0], sizeof(Vec2)*vertexCount);
	CPY(&uv1[0], sizeof(Vec2)*vertexCount);
	for (auto& aa : _matTriangles) {
		uint ct = aa.size() / 3;
		CPY(&ct, sizeof(uint));
		CPY(&aa[0], sizeof(int) * 3 * ct);
	}
	assert(off == _eCacheSz + 1);
#undef CPY
#endif
}

void Mesh::InitVao() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(4, vbos);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]); //pos
	glBufferStorage(GL_ARRAY_BUFFER, vertexCount * sizeof(Vec3), &vertices[0], GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[1]); //uv
	glBufferStorage(GL_ARRAY_BUFFER, vertexCount * sizeof(Vec2), &uv0[0], GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[2]); //norm
	glBufferStorage(GL_ARRAY_BUFFER, vertexCount * sizeof(Vec3), &normals[0], GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[3]); //tan
	glBufferStorage(GL_ARRAY_BUFFER, vertexCount * sizeof(Vec3), &tangents[0], GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

#ifdef IS_EDITOR
bool Mesh::ParseBlend(Editor* e, string s) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	HANDLE stdOutR, stdOutW, stdInR, stdInW;
	if (!CreatePipe(&stdInR, &stdInW, &sa, 0)) {
		std::cout << "failed to create pipe for stdin!";
		return false;
	}
	if (!SetHandleInformation(stdInW, HANDLE_FLAG_INHERIT, 0)) {
		std::cout << "failed to set handle for stdin!";
		return false;
	}
	if (!CreatePipe(&stdOutR, &stdOutW, &sa, 0)) {
		std::cout << "failed to create pipe for stdout!";
		return false;
	}
	if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0)) {
		std::cout << "failed to set handle for stdout!";
		return false;
	}
	STARTUPINFO startInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startInfo, sizeof(STARTUPINFO));
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
	startInfo.cb = sizeof(STARTUPINFO);
	startInfo.hStdInput = stdInR;
	startInfo.hStdOutput = stdOutW;
	startInfo.dwFlags |= STARTF_USESTDHANDLES;

	//create meta directory
	string ss = s.substr(0, s.find_last_of('.'));
	string sss = ss + "_blend";
	if (!CreateDirectory(sss.c_str(), NULL)) {
		for (string file : IO::GetFiles(sss))
			DeleteFile(file.c_str());
	}
	SetFileAttributes(sss.c_str(), FILE_ATTRIBUTE_HIDDEN);
	string ms(s + ".meta");
	DeleteFile(ms.c_str());

	bool failed = true;
	string cmd1(e->_blenderInstallationPath.substr(0, 2) + "\n"); //root
	string cmd2("cd " + e->_blenderInstallationPath.substr(0, e->_blenderInstallationPath.find_last_of("\\")) + "\n");
	string cmd3("blender \"" + s + "\" --background --python \"" + e->dataPath + "\\Python\\blend_exporter.py\" -- \"" + s.substr(0, s.find_last_of('\\')) + "?" + ss.substr(ss.find_last_of('\\') + 1, string::npos) + "\"\n");
	//outputs object list, and meshes in subdir
	if (CreateProcess("C:\\Windows\\System32\\cmd.exe", 0, NULL, NULL, true, CREATE_NO_WINDOW, NULL, "D:\\TestProject2\\", &startInfo, &processInfo) != 0) {
		std::cout << "executing Blender..." << std::endl;
		bool bSuccess = false;
		DWORD dwWrite;
		bSuccess = WriteFile(stdInW, cmd1.c_str(), cmd1.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			Debug::Error("Blender Parser", "can't get to drive root!");
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd2.c_str(), cmd2.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			Debug::Error("Blender Parser", "can't get to blender dir!");
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd3.c_str(), cmd3.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			Debug::Error("Blender Parser", "can't execute blender!");
			return false;
		}
		DWORD w;
		bool finish = false;
		do {
			w = WaitForSingleObject(processInfo.hProcess, DWORD(200));
			DWORD dwRead;
			CHAR chBuf[4096];
			string out = "";
			bSuccess = ReadFile(stdOutR, chBuf, 4096, &dwRead, NULL) != 0;
			if (bSuccess && dwRead > 0) {
				string s(chBuf, dwRead);
				out += s;
			}
			for (uint r = 0; r < out.size();) {
				int rr = out.find_first_of('\n', r);
				if (rr == string::npos)
					rr = out.size() - 1;
				string sss = out.substr(r, rr - r);
				std::cout << sss << std::endl;
				r = rr + 1;
				if (sss.size() > 1 && sss[0] == '!')
					e->_Message("Blender", sss.substr(1, string::npos));
				if (sss.size() > 12 && sss.substr(0, 12) == "Blender quit") {
					TerminateProcess(processInfo.hProcess, 0);
					failed = false;
					finish = true;
				}
			}
		} while (w == WAIT_TIMEOUT && !finish);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		if (failed)
			return false;
	}
	else {
		DWORD err = GetLastError();
		std::cout << "Cannot start Blender! (Error code " << std::to_string(err) << ")" << std::endl;
		CloseHandle(stdOutR);
		CloseHandle(stdOutW);
		CloseHandle(stdInR);
		CloseHandle(stdInW);
		return false;
	}
	CloseHandle(stdOutR);
	CloseHandle(stdOutW);
	SetFileAttributes(ms.c_str(), FILE_ATTRIBUTE_HIDDEN);
	return true;
}
#endif
