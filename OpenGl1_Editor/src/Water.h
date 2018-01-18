#pragma once
#include "Engine.h"

typedef glm::highp_dvec3 dVec3;

const double _si_l = 1e-9;
const double _si_m = 1.66e-27;
const double _si_t = 1e-12;
const double _si_q = 1.602e-19;

const double _ast = 0.1;

const double BOND_LENGTH = 1 * _ast;
const double BOND_ANGLE = 109.47 * deg2rad;
const double K_LINEAR = 345000; //kJ / mol nm^2
const double K_RADIAL = 383; //kJ / mol rad^2
const double dt = 1e-4; //0.1fs

const double MASS_H = 1.0;
const double MASS_O = 15.99;

class WaterParticle {
public:
	bool is_oxygen;

	float charge;
	dVec3 position, velocity, force;
	uint p1, p2; //h1, h2 if oxygen, o, h2 if hydrogen
	double energy;

	void repos(), f_bond();
};

class Water {
public:
	static Water* me;

	std::vector<WaterParticle*> particles;

	static const WaterParticle* Get(uint i);
	static void Init(), Update();
};