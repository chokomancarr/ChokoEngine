#pragma once
#include "Engine.h"

class MD {
public:
	static MD* me;
	MD(const string& path, uint c, float d, float t);
	void Update();
	void DrawUI();

	std::vector<Vec3> pes;
	uint pei, pen;
	Vec2 per;
	float pe;

	IComputeBuffer* frb, *psb, *vlb, *prb, *rdb;
	ComputeShader* shader;

	Vec3 rdf[40];

	float dens, wall, mwall, temp, tempo;
	const uint particlecount, threads;
	GLuint forSSBO, posSSBO, velSSBO, prmSSBO, rdfSSBO;
	GLuint computeprog;
	Vec4* colors;
};