#pragma once
#include "Engine.h"

class MD {
public:
	static MD* me;
	MD(uint c, float d, float t);
	void InitProg(string path);
	void Update();
	void DrawUI();

	float dens, wall, mwall, temp, tempo;
	const uint particlecount, threads;
	GLuint forSSBO, posSSBO, velSSBO, prmSSBO;
	GLuint computeprog;
	Vec4* colors;
};