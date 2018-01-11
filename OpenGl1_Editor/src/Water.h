#pragma once
#include "Engine.h"

const float _ast = 1e-10f;

const float BOND_LENGTH = 1 * _ast;
const float BOND_ANGLE = 109.47f * deg2rad;
const float K_LINEAR = 345000.0e18f;
const float K_RADIAL = 383.0e9f;
const float dt = 5e-16f;

const float avogadro = 6.02e23f;
const float MASS_H = 1.0e-3f;// / avogadro;
const float MASS_O = 16.0e-3f;// / avogadro;

class WaterParticle {
public:
	bool is_oxygen;

	float charge;
	Vec3 position, velocity, force;
	uint p1, p2; //h1, h2 if oxygen, o, h2 if hydrogen

	void repos(), f_bond();
};

class Water {
public:
	static Water* me;

	std::vector<WaterParticle*> particles;

	static const WaterParticle* Get(uint i);
	static void Init(), Update();
};