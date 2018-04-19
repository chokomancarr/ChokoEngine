#include "mvp.h"

MVP::stack MVP::MV = MVP::stack();
MVP::stack MVP::P = MVP::stack();
Mat4x4 MVP::_mv, MVP::_p;
bool MVP::changedMv = true, MVP::changedP = true;
Mat4x4 MVP::identity = Mat4x4();
bool MVP::isProj = false;

void MVP::Reset() {
	P = stack();
	P.push(identity);
	MV = stack();
	MV.push(identity);
	changedMv = true;
	changedP = true;
}
void MVP::Switch(bool proj) {
	isProj = proj;
}
void MVP::Push() {
	if (isProj) P.push(identity);
	else MV.push(identity);
}
void MVP::Pop() {
	if (isProj) {
		P.pop();
		changedP = true;
	}
	else {
		MV.pop();
		changedMv = true;
	}
}
void MVP::Clear() {
	if (isProj) {
		P = stack();
		P.push(identity);
		changedP = true;
	}
	else {
		MV = stack();
		MV.push(identity);
		changedMv = true;
	}
}
void MVP::Mul(const Mat4x4& mat) {
	if (isProj) {
		P.top() *= mat;
		changedP = true;
	}
	else {
		MV.top() *= mat;
		changedMv = true;
	}
}
void MVP::Translate(const Vec3& v) {
	Translate(v.x, v.y, v.z);
}
void MVP::Translate(float x, float y, float z) {
	//Mul(Mat4x4(1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1));
	Mul(Mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1));
}
void MVP::Scale(const Vec3& v) {
	Scale(v.x, v.y, v.z);
}
void MVP::Scale(float x, float y, float z) {
	Mul(Mat4x4(x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1));
}

Mat4x4 MVP::modelview() {
	if (changedMv) {
		changedMv = false;
		_mv = identity;
		for (uint i = 0; i < MV.size(); i++) {
			_mv *= MV.c[i];// * m;
		}
	}
	return _mv;
}
Mat4x4 MVP::projection() {
	if (changedP) {
		changedP = false;
		_p = identity;
		for (uint i = 0; i < P.size(); i++) {
			_p *= P.c[i];// * m;
		}
	}
	return _p;
}