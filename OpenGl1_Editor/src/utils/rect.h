#pragma once
#include "Engine.h"

class Rect {
public:
	Rect() : x(0), y(0), w(1), h(1) {}
	Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
	Rect(Vec4 v) : x(v.r), y(v.g), w(v.b), h(v.a) {}
	float x, y, w, h;

	/*! Check if v is inside this rect.
	*/
	bool Inside(const Vec2& v);
	/*! Returns a new Rect covered by both this rect and r2
	*/
	Rect Intersection(const Rect& r2);
};