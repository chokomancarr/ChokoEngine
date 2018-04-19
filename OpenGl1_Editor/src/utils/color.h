#pragma once
#include "Engine.h"

//shorthands
Vec4 black(float f = 1);
Vec4 red(float f = 1, float i = 1), green(float f = 1, float i = 1), blue(float f = 1, float i = 1), cyan(float f = 1, float i = 1), yellow(float f = 1, float i = 1), white(float f = 1, float i = 1);

class Color {
public:
	Color() : r(0), g(0), b(0), a(0), useA(true) {}
	Color(Vec4 v, bool hasA = true) : r((byte)round(v.r * 255)), g((byte)round(v.g * 255)), b((byte)round(v.b * 255)), a((byte)round(v.a * 255)), useA(hasA) {}

	bool useA;
	byte r, g, b, a;
	float h, s, v;

	Vec3 hsv() { return Vec3(h, s, v); }

	Vec4 vec4() {
		return Vec4(r, g, b, a)*(1.0f / 255);
	}

	static GLuint pickerProgH, pickerProgSV;

	string hex();

	static void Rgb2Hsv(byte r, byte g, byte b, float& h, float& s, float& v), Hsv2Rgb(float h, float s, float v, byte& r, byte& g, byte& b);
	static Vec3 Rgb2Hsv(Vec4 col);
	static string Col2Hex(Vec4 col);
	static void DrawPicker(float x, float y, Color& c);
	static Vec4 HueBaseCol(float hue);

protected:

	void RecalcRGB(), RecalcHSV();
	static void DrawSV(float x, float y, float w, float h, float hue);
	static void DrawH(float x, float y, float w, float h);
};