#include "Engine.h"

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

Quat QuatFunc::FromAxisAngle(const Vec3& axis, float angle) {
	float a = deg2rad*angle;
	float factor = (float)sin(a / 2.0);

	// Calculate the x, y and z of the quaternion
	float x = axis.x * factor;
	float y = axis.y * factor;
	float z = axis.z * factor;

	// Calcualte the w value by cos( theta / 2 )
	float w = (float)cos(a / 2.0);
	return Normalize(Quat(w, x, y, z));
}

BBox::BBox(float x0, float x1, float y0, float y1, float z0, float z1) : x0(x0), x1(x1), y0(y0), y1(y1), z0(z0), z1(z1) {}