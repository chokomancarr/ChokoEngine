#pragma once
#include "Engine.h"

const float _si_l = 1e-9;
const float _si_m = 1.66e-27;
const float _si_t = 1e-12;
const float _si_q = 1.602e-19;

const float _ast = 0.1;

const float BOND_LENGTH = 1 * _ast;
const float BOND_ANGLE = 109.47 * deg2rad;
const float K_LINEAR = 345000; //kJ / mol nm^2
const float K_RADIAL = 383; //kJ / mol rad^2
const float dt = 1e-4; //0.1fs

const float MASS_H = 1.0;
const float MASS_O = 15.99;

/*
class WaterParticle {
public:
	bool is_oxygen;

	float charge;
	dVec3 position, velocity, force;
	uint p1, p2; //h1, h2 if oxygen, o, h2 if hydrogen
	double energy;

	void repos(), f_bond();
};
*/

class Water {
public:
	static Water* me;

	Water(uint cnt, float d, float t);

	//std::vector<WaterParticle*> particles;

	//static const WaterParticle* Get(uint i);
	void Update(), Draw();

	IComputeBuffer* frb, *psb, *vlb;
	ComputeShader* shader;

	float dens, temp, wall;

	const uint particlecount, threads;
	GLuint forSSBO, posSSBO, velSSBO, prmSSBO, rdfSSBO;
	GLuint computeprog;
	Vec4* colors;
};