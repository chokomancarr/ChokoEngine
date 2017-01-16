#include "KTMModel.h"
#include "Engine.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
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

string KTMModel::Parse(string s) {
	HKEY key;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft"), 0, KEY_READ, &key) == ERROR_SUCCESS) {
		TCHAR* achKey = IO::GetRegistryKeys(key);

	}

	RegCloseKey(key);
	return "";
}