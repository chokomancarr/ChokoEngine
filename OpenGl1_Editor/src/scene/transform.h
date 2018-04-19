#pragma once
#include "SceneObjects.h"

class Transform : public Object {
public:
	rSceneObject object;

	const Vec3& position() { return _position; }
	const Vec3& localPosition() { return _localPosition; }
	const Quat& rotation() { return _rotation; }
	const Quat& localRotation() { return _localRotation; }
	const Vec3& localScale() { return _localScale; }
	const Vec3& eulerRotation() { return _eulerRotation; }
	const Vec3& localEulerRotation() { return _localEulerRotation; }
	void position(const Vec3& r);
	void localPosition(const Vec3& r);
	void rotation(const Quat& r);
	void localRotation(const Quat& r);
	void localScale(const Vec3& r);
	void eulerRotation(const Vec3& r);
	void localEulerRotation(const Vec3& r);
	const Mat4x4 localMatrix() { return _localMatrix; }
	const Mat4x4 worldMatrix() { return _worldMatrix; }

	Vec3 forward(), right(), up(), Local2World(Vec3);

	Transform& Translate(float x, float y, float z, TransformSpace sp = Space_Self) { return Translate(Vec3(x, y, z), sp); }
	Transform& Rotate(float x, float y, float z, TransformSpace sp = Space_Self) { return Rotate(Vec3(x, y, z), sp); }
	Transform& Scale(float x, float y, float z) { return Scale(Vec3(x, y, z)); }
	Transform& Translate(Vec3 v, TransformSpace sp = Space_Self), &Rotate(Vec3 v, TransformSpace sp = Space_Self), &Scale(Vec3 v);
	Transform& RotateAround(Vec3 a, float f);

	friend class SceneObject;
	friend class EB_Viewer;
	friend class EB_Inspector;
	friend struct Editor_PlaySyncer;
	friend class Armature;
	_allowshared(Transform);
private:
	Transform() {}
	void Init(pSceneObject& sc, Vec3 pos, Quat rot, Vec3 scl);
	void Init(pSceneObject& sc, byte* data);

	Vec3 _position, _localPosition;
	Quat _rotation, _localRotation;
	Vec3 _eulerRotation, _localEulerRotation;
	Vec3 _localScale;
	Mat4x4 _localMatrix, _worldMatrix;

	static struct _offset_map {
		uint position = offsetof(Transform, _position),
			rotation = offsetof(Transform, _rotation),
			scale = offsetof(Transform, _localScale);
	} _offsets;

	void _W2LPos(), _L2WPos();
	void _UpdateWQuat(), _UpdateLQuat(), _L2WQuat(), _W2LQuat();
	void _UpdateWEuler(), _UpdateLEuler();
	void _UpdateLMatrix();
	void _UpdateWMatrix(const Mat4x4& mat);

	//never call Transform constructor directly.
	Transform(const Transform& rhs);
};
