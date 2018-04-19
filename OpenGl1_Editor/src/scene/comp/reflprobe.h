#pragma once
#include "SceneObjects.h"

enum ReflProbe_UpdateMode : byte {
	ReflProbe_UpdateMode_Start,
	ReflProbe_UpdateMode_ReKey_LeftAltime,
	ReflProbe_UpdateMode_Manual
};
enum ReflProbe_ClearType : byte {
	ReflProbe_Clear_Sky,
	ReflProbe_Clear_Color
};
class ReflectionProbe : public Component {
public:
	ReflectionProbe(ushort size = 256);
	~ReflectionProbe() { delete(map); }
	ushort size;
	ReflProbe_UpdateMode updateMode;
	float intensity;
	ReflProbe_ClearType clearType;
	Vec4 clearColor;
	Vec3 range;
	float softness;

	void Update() { _pendingUpdate = true; }

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Camera;
	_allowshared(ReflectionProbe);
protected:
	ReflectionProbe(std::ifstream& stream, SceneObject* o, long pos = -1);
	bool _pendingUpdate;
	CubeMap* map;
	GLuint mipFbos[7];

#ifdef IS_EDITOR
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;
#endif

	void _DoUpdate();
};
