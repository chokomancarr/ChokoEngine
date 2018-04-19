#pragma once
#include "Engine.h"

class MVP {
public:
	static void Reset();
	static void Switch(bool isProj);
	static void Push(), Pop(), Clear();
	static void Mul(const Mat4x4& mat);
	static void Translate(const Vec3& v), Translate(float x, float y, float z);
	static void Scale(const Vec3& v), Scale(float x, float y, float z);

	static Mat4x4 modelview(), projection();

protected:
	class stack : public std::stack<Mat4x4> {
	public:
		using std::stack<Mat4x4>::c;
	};

	static stack MV, P;
	static Mat4x4 _mv, _p;
	static bool changedMv, changedP;
	static Mat4x4 identity;
	static bool isProj;

};