#include "Engine.h"

Transform::_offset_map Transform::_offsets = {};

void Transform::Init(pSceneObject& sc, Vec3 pos, Quat rot, Vec3 scl) {
	object(sc);
	_rotation = rot;
	Translate(pos).Scale(scl);
	_UpdateWEuler();
	_W2LQuat();
	_UpdateLMatrix();
}

void Transform::Init(pSceneObject& sc, byte* data) {
	Transform* tr = (Transform*)data;
	Init(sc, tr->_position, tr->_rotation, tr->_localScale);
}

void Transform::position(const Vec3& r) {
	_position = r;
	_W2LPos();
	_UpdateLMatrix();
}

void Transform::localPosition(const Vec3& r) {
	_localPosition = r;
	_L2WPos();
	_UpdateLMatrix();
}

void Transform::rotation(const Quat& r) {
	_rotation = r;
	_UpdateWEuler();
	_W2LQuat();
	_UpdateLMatrix();
}

void Transform::localRotation(const Quat& r) {
	_localRotation = r;
	_UpdateLEuler();
	_L2WQuat();
	_UpdateLMatrix();
}

void Transform::localScale(const Vec3& r) {
	_localScale = r;
	_UpdateLMatrix();
}

void Transform::eulerRotation(const Vec3& r) {
	_eulerRotation.x = Repeat(r.x, 0.0f, 360.0f);
	_eulerRotation.y = Repeat(r.y, 0.0f, 360.0f);
	_eulerRotation.z = Repeat(r.z, 0.0f, 360.0f);
	_UpdateWQuat();
	_UpdateLMatrix();
}

void Transform::localEulerRotation(const Vec3& r) {
	_localEulerRotation.x = Repeat(r.x, 0.0f, 360.0f);
	_localEulerRotation.y = Repeat(r.y, 0.0f, 360.0f);
	_localEulerRotation.z = Repeat(r.z, 0.0f, 360.0f);
	_UpdateLQuat();
	_UpdateLMatrix();
}

Vec3 Transform::forward() {
	auto v = _worldMatrix*Vec4(0, 0, 1, 0);
	return Vec3(v);
}
Vec3 Transform::right() {
	auto v = _worldMatrix*Vec4(1, 0, 0, 0);
	return Vec3(v);
}
Vec3 Transform::up() {
	auto v = _worldMatrix*Vec4(0, 1, 0, 0);
	return Vec3(v);
}

Vec3 Transform::Local2World(Vec3 vec) {
	auto v = _worldMatrix*Vec4(vec, 1);
	return Vec3(v);
}

Transform& Transform::Translate(Vec3 v, TransformSpace sp) {
	if (sp == Space_Self) {
		_localPosition += v;
		_L2WPos();
	}
	else {
		_position += v;
		_W2LPos();
	}
	_UpdateLMatrix();
	return *this;
}
Transform& Transform::Rotate(Vec3 v, TransformSpace sp) {
	if ((sp == Space_Self) || (!object->parent)) {
		Quat qx = QuatFunc::FromAxisAngle(Vec3(1, 0, 0), v.x);
		Quat qy = QuatFunc::FromAxisAngle(Vec3(0, 1, 0), v.y);
		Quat qz = QuatFunc::FromAxisAngle(Vec3(0, 0, 1), v.z);
		_localRotation = _localRotation * qx * qy * qz;
		_UpdateLEuler();
		_L2WQuat();
	}
	else {
		Mat4x4 im = glm::inverse(object->parent->transform._worldMatrix);
		Vec4 vx = im*Vec4(1, 0, 0, 0);
		Vec4 vy = im*Vec4(0, 1, 0, 0);
		Vec4 vz = im*Vec4(0, 0, 1, 0);
		Quat qx = QuatFunc::FromAxisAngle(vx, v.x);
		Quat qy = QuatFunc::FromAxisAngle(vy, v.y);
		Quat qz = QuatFunc::FromAxisAngle(vz, v.z);
		_rotation = qx * qy * qz * _rotation;
		_UpdateWEuler();
		_W2LQuat();
	}

	_UpdateLMatrix();
	return *this;
}

Transform& Transform::Scale(Vec3 v) {
	_localScale += v;
	_UpdateLMatrix();
	return *this;
}

void Transform::_UpdateWQuat() {
	_rotation = Quat(deg2rad*_eulerRotation);
	_W2LQuat();
}

void Transform::_UpdateLQuat() {
	_localRotation = Quat(deg2rad*_localEulerRotation);
	_L2WQuat();
}

void Transform::_UpdateWEuler() {
	_eulerRotation = QuatFunc::ToEuler(_rotation);
}
void Transform::_UpdateLEuler() {
	_localEulerRotation = QuatFunc::ToEuler(_localRotation);
}

void Transform::_L2WPos() {
	if (!object->parent) _position = _localPosition;
	else {
		auto pos = object->parent->transform._worldMatrix*Vec4(_localPosition, 1);
		_position = Vec3(pos.x, pos.y, pos.z);
	}
}
void Transform::_W2LPos() {
	if (!object->parent) _localPosition = _position;
	else {
		auto pos = glm::inverse(object->parent->transform._worldMatrix)*Vec4(_position, 1);
		_localPosition = Vec3(pos.x, pos.y, pos.z);
	}
}

void Transform::_L2WQuat() {
	if (!object->parent) _rotation = _localRotation;
	else {
		_rotation = object->parent->transform._rotation*_localRotation;
	}
	_UpdateWEuler();
}
void Transform::_W2LQuat() {
	if (!object->parent) _localRotation = _rotation;
	else {
		_localRotation = glm::inverse(object->parent->transform._rotation)*_rotation;
	}
	_UpdateLEuler();
}

void Transform::_UpdateLMatrix() {
	_localMatrix = MatFunc::FromTRS(_localPosition, _localRotation, _localScale);
	_UpdateWMatrix(object->parent ? object->parent->transform._worldMatrix : Mat4x4());
}

void Transform::_UpdateWMatrix(const Mat4x4& mat) {
	_worldMatrix = mat*_localMatrix;

	_L2WPos();
	_L2WQuat();

	for (uint a = 0; a < object->children.size(); a++) //using iterators cause error
		object->children[a]->transform._UpdateWMatrix(_worldMatrix);
}