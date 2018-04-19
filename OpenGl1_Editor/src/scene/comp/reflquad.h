#pragma once
#include "SceneObjects.h"

class ReflectiveQuad : public Component {
public:
	ReflectiveQuad(Texture* tex = nullptr);

	Texture* texture;
	float intensity;
	Vec2 size, origin;
	bool invertX, invertY, invertDirection;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Camera;
	friend class Light;
	friend class Engine;
	_allowshared(ReflectiveQuad);
protected:
	ASSETID _texture;
	static std::vector<GLint> paramLocs;
	//static Vec3 _poss[4], _uvs[4];
	//static uint _ids[6];
	static void _SetTex(void* v);

	static void ScanParams();
#ifdef IS_EDITOR
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;
#endif
};
