#include "Engine.h"

bool Rect::Inside(const Vec2& v) {
	return ((w > 0) ? (v.x > x && v.x < (x + w)) : (v.x >(x + w) && v.x < x)) && ((h > 0) ? (v.y > y && v.y < (y + h)) : (v.y >(y + h) && v.y < y));
}

Rect Rect::Intersection(const Rect& r2) {
	float ox = max(x, r2.x);
	float oy = max(y, r2.y);
	float p2x = min(x + w, r2.x + r2.w);
	float p2y = min(y + h, r2.y + r2.h);
	return Rect(ox, oy, p2x - ox, p2y - oy);
}