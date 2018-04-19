#pragma once
#include "SceneObjects.h"

enum IK_TYPE : byte {
	IK_TARGET_ONLY,
	IK_JOINT_ONLY,
	IK_GENERIC
};

enum IK_AXIAL_CONSTRAINT : byte {
	IK_AXIAL_FREE,
	IK_AXIAL_ONLY,
	IK_NON_AXIAL_ONLY
};

class InverseKinematics : public Component {
public:
	InverseKinematics();

	IK_TYPE type;

	rSceneObject target;
	byte length = 1, iterations = 20;

	bool jointIsAxial;
	bool allowRotation[3];
	IK_AXIAL_CONSTRAINT axiKey_LeftAltype;

	void Apply();

	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(InverseKinematics);
protected:
#ifdef IS_EDITOR
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override {}
#endif
};
