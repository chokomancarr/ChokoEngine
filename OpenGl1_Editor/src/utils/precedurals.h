#pragma once
#include "Engine.h"

/*! Run-time mesh generation.
[av] */
class Procedurals {
public:
	static Mesh* Plane(uint xCount, uint yCount);
	static Mesh* UVSphere(uint uCount, uint vCount);
};