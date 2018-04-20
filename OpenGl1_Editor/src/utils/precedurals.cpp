#include "Engine.h"

Mesh* Procedurals::Plane(uint xCount, uint yCount) {
	std::vector<Vec3> norms = std::vector<Vec3>();
	std::vector<Vec3> verts = std::vector<Vec3>();
	std::vector<Vec2> uvs = std::vector<Vec2>();
	std::vector<int> tris = std::vector<int>();
	for (uint a = 0; a <= xCount; a++) {
		for (uint b = 0; b <= yCount; b++) {
			verts.push_back(Vec3(xCount * (a*1.0f / xCount - 0.5f), 0, xCount * (b*1.0f / yCount - 0.5f)));
			norms.push_back(Vec3(0, 1, 0));
			uvs.push_back(Vec2(a*1.0f / xCount, b*1.0f / yCount));
		}
	}
	int va1, va2, vb1, vb2;
	for (uint a = 0; a < xCount; a++) {
		for (uint b = 0; b < yCount; b++) {
			va1 = (yCount + 1)*a + b;
			va2 = va1 + 1;
			vb1 = (yCount + 1)*(a + 1) + b;
			vb2 = vb1 + 1;
			/*
			if (b == uCount - 1) {
			va2 -= uCount;
			vb2 -= uCount;
			}
			*/
			tris.push_back(va1);
			tris.push_back(vb2);
			tris.push_back(vb1);
			tris.push_back(va1);
			tris.push_back(va2);
			tris.push_back(vb2);
		}
	}
	return new Mesh(verts, norms, tris, uvs);
}

Mesh* Procedurals::UVSphere(uint uCount, uint vCount) {
	std::vector<Vec3> verts, norms;
	std::vector<Vec2> uvs = std::vector<Vec2>();
	std::vector<int> tris = std::vector<int>();
	verts = std::vector<Vec3>();
	for (uint a = 0; a <= vCount; a++) {
		double theta1 = 0.5f * PI - PI*a / (vCount - 1);
		for (uint b = 0; b <= uCount; b++) {
			double theta2 = 2 * PI * b / uCount;
			verts.push_back(Vec3(cos(theta2)*cos(theta1), sin(theta1), sin(theta2)*cos(theta1)));
			uvs.push_back(Vec2(b * 1.0f / (uCount), a * 1.0f / (vCount)));
		}
	}
	norms = std::vector<Vec3>(verts);
	int va1, va2, vb1, vb2;
	for (uint a = 0; a < vCount - 1; a++) {
		for (uint b = 0; b < uCount; b++) {
			va1 = (uCount + 1)*a + b;
			va2 = va1 + 1;
			vb1 = (uCount + 1)*(a + 1) + b;
			vb2 = vb1 + 1;
			/*
			if (b == uCount - 1) {
			va2 -= uCount;
			vb2 -= uCount;
			}
			*/
			tris.push_back(va1);
			tris.push_back(vb2);
			tris.push_back(vb1);
			tris.push_back(va1);
			tris.push_back(va2);
			tris.push_back(vb2);
		}
	}
	return new Mesh(verts, norms, tris, uvs);
}