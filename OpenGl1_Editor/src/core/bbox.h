#pragma once
#include "Engine.h"

struct BBox {
	BBox() {}
	BBox(float, float, float, float, float, float);

	float x0, x1, y0, y1, z0, z1;
};