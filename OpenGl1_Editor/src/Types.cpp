#include "Engine.h"

Mat4x4 MatFunc::FromTRS(const Vec3& t, const Quat& r, const Vec3& s) {
	return glm::translate(t) * QuatFunc::ToMatrix(r) * glm::scale(s);
}

Quat QuatFunc::Inverse(const Quat& q) {
	return Quat(-q.x, -q.y, -q.z, q.w);
}

Vec3 QuatFunc::ToEuler(const Quat& q) {
	Vec3 out;
	double ysqr = q.y * q.y;

	// roll (x-axis rotation)
	double t0 = +2.0 * (q.w * q.x + q.y * q.z);
	double t1 = +1.0 - 2.0 * (q.x * q.x + ysqr);
	out.x = (float)atan2(t0, t1);

	// pitch (y-axis rotation)
	double t2 = +2.0 * (q.w * q.y - q.z * q.x);
	t2 = ((t2 > 1.0) ? 1.0 : t2);
	t2 = ((t2 < -1.0) ? -1.0 : t2);
	out.y = (float)asin(t2);

	// yaw (z-axis rotation)
	double t3 = +2.0 * (q.w * q.z + q.x * q.y);
	double t4 = +1.0 - 2.0 * (ysqr + q.z * q.z);
	out.z = (float)atan2(t3, t4);
	
	return out*rad2deg;
}

Mat4x4 QuatFunc::ToMatrix(const Quat& q) {
	return glm::mat4_cast(q);
}

Quat QuatFunc::FromAxisAngle(Vec3 axis, float angle) {
	axis = Normalize(axis);
	float a = deg2rad*angle;
	float factor = (float)sin(a / 2.0);
	// Calculate the x, y and z of the quaternion
	float x = axis.x * factor;
	float y = axis.y * factor;
	float z = axis.z * factor;

	// CalcuKey_LeftAlte the w value by cos( theta / 2 )
	float w = (float)cos(a / 2.0);
	return Normalize(Quat(w, x, y, z));
}

//https://gamedev.stackexchange.com/questions/53129/quaternion-look-at-with-up-vector
Quat QuatFunc::LookAt(const Vec3& tarr, const Vec3& up) {
	Vec3 tar = Normalize(tarr);
	Vec3 fw = Vec3(0,0,1);
	Vec3 axis = cross(tar, fw);
	float angle = rad2deg*acos(Clamp(dot(tar,fw), -1.0f, 1.0f));
	Vec3 tr = cross(axis, fw);
	if (dot(tr, tar) < 0) angle *= -1;
	Quat q1 = FromAxisAngle(axis, angle);
	if (abs(angle) < 0.000001f) q1 = Quat();

	Vec3 mup = q1*Vec3(0, 1, 0);//QuatFunc::ToMatrix(q1)*Vec4(0, 1, 0, 0);
	Vec3 mrt = q1*Vec3(1, 0, 0);
	Vec3 rt = Normalize(cross(up, tar));
	float angle2 = rad2deg*acos(Clamp(dot(mrt, rt), -1.0f, 1.0f));
	if (dot(mup, rt) < 0) angle2 *= -1;
	Quat q2 = FromAxisAngle(tar, angle2);
	if (abs(angle2) < 0.000001f) q2 = Quat();

	return q2 * q1;
}

BBox::BBox(float x0, float x1, float y0, float y1, float z0, float z1) : x0(x0), x1(x1), y0(y0), y1(y1), z0(z0), z1(z1) {}