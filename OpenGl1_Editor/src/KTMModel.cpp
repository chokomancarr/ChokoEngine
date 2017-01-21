#include "KTMModel.h"
#include "Engine.h"
#include "Editor.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <shellapi.h>
#include <Windows.h>
using namespace std;

KTMModel::KTMModel() {
	vertCount = 0;
	faceCount = 0;
	cout << "reading model..." << endl;
}

bool KTMModel::LoadModel(const string& path) {
	ifstream stream(path.c_str());
	if (!stream.good()) {
		cout << "not found!" << endl;
		return false;
	}
	string a;
	string junk;
	int x;
	stream >> a;
	if (a != "KTM123") {
		cerr << "file not supported" << endl;
		return false;
	}
	while (!stream.eof()) {
		stream >> a;
		if (a == "obj") {
			stream >> mesh.name >> junk;
		}
		else if (a == "pos") {
			stream >> mesh.position.x >> mesh.position.y >> mesh.position.z;
			cout << "position " << s(mesh.position) << endl;
		}
		else if (a == "rot") {
			stream >> mesh.rotation.w >> mesh.rotation.x >> mesh.rotation.y >> mesh.rotation.z;
			cout << "rotation " << s(mesh.rotation) << endl;
		}
		else if (a == "scl") {
			stream >> mesh.scale.x >> mesh.scale.y >> mesh.scale.z;
			cout << "scale " << s(mesh.scale) << endl;
		}
		else if (a == "vrt") {
			Vertex v;
			stream >> x;
			if (x != vertCount) {
				cerr << "vertex index is wrong! x=" << x << "c=" << vertCount << endl;
				return false;
			}
			vertCount++;
			stream >> v.position.x >> v.position.y >> v.position.z;
			mesh.vertices.push_back(v);
			mesh.vertices[x].Vec4.x = 1;
			mesh.vertices[x].Vec4.y = 1;
			mesh.vertices[x].Vec4.z = 1;
			//cout << "v" << vertCount - 1 << " " << s(v.position) << endl;
		}
		else if (a == "nrm") {
			stream >> x;
			if (x >= vertCount) {
				cerr << "normal index is wrong! x=" << x << "c=" << vertCount << endl;
				return false;
			}
			stream >> mesh.vertices[x].normal.x >> mesh.vertices[x].normal.y >> mesh.vertices[x].normal.z;
			//cout << "vn" << x << " " << s(mesh.vertices[x].normal) << endl;
		}
		else if (a == "vcl") {
			stream >> x;
			if (x >= vertCount) {
				cerr << "vert Vec4 index is wrong! x=" << x << "c=" << vertCount << endl;
				return false;
			}
			stream >> mesh.vertices[x].Vec4.x >> mesh.vertices[x].Vec4.y >> mesh.vertices[x].Vec4.z;
			//cout << "vc" << x << " " << s(mesh.vertices[x].normal) << endl;
		}
		else if (a == "tri") {
			Triangle t;
			faceCount++;
			stream >> t.vertices[0] >> t.vertices[1] >> t.vertices[2];
			mesh.triangles.push_back(t);
			//cout << "tri" << faceCount - 1 << endl;
		}
		else if (a == "]")
			break;
		else
			cout << "what is this? " << a << endl;

		
	}
	mesh.m_PositionBuffer.clear();
	mesh.m_NormalBuffer.clear();
	mesh.m_VertVec4Buffer.clear();
	for (Vertex v : mesh.vertices) {
		mesh.m_PositionBuffer.push_back(v.position);
		mesh.m_NormalBuffer.push_back(v.normal);
		mesh.m_VertVec4Buffer.push_back(v.Vec4);
	}
	for (Triangle t : mesh.triangles) {
		mesh.m_IndexBuffer.push_back(t.vertices[0]);
		mesh.m_IndexBuffer.push_back(t.vertices[1]);
		mesh.m_IndexBuffer.push_back(t.vertices[2]);
	}
	cout << "total verts: " << mesh.vertices.size() << " total tris: " << mesh.triangles.size() << endl;
	m_model2world = glm::translate(mesh.position) * glm::scale(mesh.scale);
	return true;
}

void KTMModel::RenderModel() {
	//cout << mesh.m_PositionBuffer.size() << " : " << mesh.m_NormalBuffer.size() << " : " << mesh.m_IndexBuffer.size() << endl;
	glPushMatrix();
	glMultMatrixf(glm::value_ptr(m_model2world));
	glTranslatef(0, 0, -1.5f);
	
	glColor3f(1.0f, 1.0f, 1.0f);

	//glVec43f(1.0f, 1.0f, 0.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glEnable(GL_DEPTH_TEST);

	//glBindTexture(GL_TEXTURE_2D, mesh.texID);
	glVertexPointer(3, GL_FLOAT, 0, &(mesh.m_PositionBuffer[0]));
	glNormalPointer(GL_FLOAT, 0, &(mesh.m_NormalBuffer[0]));
	glColorPointer(3, GL_FLOAT, 0, &(mesh.m_VertVec4Buffer[0]));
	//glTexCoordPointer(2, GL_FLOAT, 0, &(mesh.m_Tex2DBuffer[0]));
	glUseProgram(shader);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, mesh.m_IndexBuffer.size(), GL_UNSIGNED_INT, &(mesh.m_IndexBuffer[0]));
	glDisableClientState(GL_COLOR_ARRAY);
	glUseProgram(0);
	if (useLine) {
		glColor3f(0.0f, 0.0f, 0.0f);
		glLineWidth(1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, mesh.m_IndexBuffer.size(), GL_UNSIGNED_INT, &(mesh.m_IndexBuffer[0]));
	}

	glDisableClientState(GL_NORMAL_ARRAY);
	//glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_DEPTH_TEST);
	//glBindTexture(GL_TEXTURE_2D, 0);

	glPopMatrix();
}

bool KTMModel::Parse(Editor* e, string s) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	HANDLE stdOutR, stdOutW, stdInR, stdInW;
	if (!CreatePipe(&stdInR, &stdInW, &sa, 0)) {
		cout << "failed to create pipe for stdin!";
		return false;
	}
	if (!SetHandleInformation(stdInW, HANDLE_FLAG_INHERIT, 0)){
		cout << "failed to set handle for stdin!";
		return false;
	}
	if (!CreatePipe(&stdOutR, &stdOutW, &sa, 0)) {
		cout << "failed to create pipe for stdout!";
		return false;
	}
	if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0)){
		cout << "failed to set handle for stdout!";
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

	bool failed = true;
	string cmd1(e->_blenderInstallationPath.substr(0, 2) + "\n"); //root
	string cmd2("cd " + e->_blenderInstallationPath.substr(0, e->_blenderInstallationPath.find_last_of("\\")) + "\n");
	string cmd3("blender \"" + s + "\" --background --python \"" + e->dataPath + "\\Python\\blend_exporter.py\" -- \"" + s.substr(0, s.find_last_of("\\")) + "?" + s.substr(s.find_last_of("\\") + 1, string::npos) + ".meta\"\n");
	string cmdDie("exit\n");
	if (CreateProcess("C:\\Windows\\System32\\cmd.exe", 0, NULL, NULL, true, CREATE_NO_WINDOW, NULL, "F:\\TestProject\\", &startInfo, &processInfo) != 0) {
		cout << "executing Blender..." << endl;
		bool bSuccess = false;
		DWORD dwWrite;
		bSuccess = WriteFile(stdInW, cmd1.c_str(), cmd1.size(), &dwWrite, NULL);
		if (!bSuccess || dwWrite == 0) {
			cout << "can't get to root!" << endl;
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd2.c_str(), cmd2.size(), &dwWrite, NULL);
		if (!bSuccess || dwWrite == 0) {
			cout << "can't navigate to blender dir!" << endl;
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd3.c_str(), cmd3.size(), &dwWrite, NULL);
		if (!bSuccess || dwWrite == 0) {
			cout << "can't execute blender!" << endl;
			return false;
		}
		DWORD w;
		bool finish = false;
		do {
			w = WaitForSingleObject(processInfo.hProcess, 0.5f);
			DWORD dwRead;
			CHAR chBuf[4096];
			string out = "";
			bSuccess = ReadFile(stdOutR, chBuf, 4096, &dwRead, NULL);
			if (bSuccess && dwRead > 0) {
				string s(chBuf, dwRead);
				out += s;
			}
			for (int r = 0; r < out.size();) {
				int rr = out.find_first_of('\n', r);
				if (rr == string::npos)
					rr = out.size() - 1;
				string sss = out.substr(r, rr - r);
				cout << sss << endl;
				r = rr + 1;
				if (sss.size() > 1 && sss[0] == '!')
					e->_Message("Blender", sss.substr(1, string::npos));
				if (sss.size() > 12 && sss.substr(0, 12) == "Blender quit"){
					TerminateProcess(processInfo.hProcess, 0);
					failed = false;
					finish = true;
				}
			}
		} while (w == WAIT_TIMEOUT && !finish);
		return (!failed);
	}
	else {
		cout << "Cannot start Blender!" << endl;
		CloseHandle(stdOutR);
		CloseHandle(stdOutW);
		CloseHandle(stdInR);
		CloseHandle(stdInW);
		return false;
	}
	CloseHandle(stdOutR);
	CloseHandle(stdOutW);
	return true;
}