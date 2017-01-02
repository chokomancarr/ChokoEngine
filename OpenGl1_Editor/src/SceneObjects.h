#ifndef SCENE_H
#define SCENE_H

#include "Engine.h"

class Object {
public:
	Object() : id(Engine::GetNewId()) {}
	ulong id;
	string name;
};

typedef unsigned char COMPONENT_TYPE;
class Component : public Object {
public:
	const COMPONENT_TYPE componentType = 0;
	const bool drawable;
	bool active;
	SceneObject* object;

	bool _expanded;

	virtual void DrawEditor() = 0; //trs matrix not applied, apply before calling
	virtual void DrawInspector(Editor* e, Component* c, Color v, uint& pos) = 0;
	Component(COMPONENT_TYPE t, bool draw) : componentType(t), active(true), drawable(draw), _expanded(true) {}
};

class Transform : public Object {
public:
	//never call Transform constructor directly. Call SceneObject().transform instead.
	Transform(SceneObject* sc, Vec3 pos, Quat rot, Vec3 scl) : object(sc), position(pos), rotation(rot), scale(scl) {}

	SceneObject* object;
	Vec3 position;
	Quat rotation;
	Vec3 eulerRotation;
	Vec3 scale;

	Transform* Translate(float x, float y, float z) { return Translate(Vec3(x, y, z)); }
	Transform* Translate(Vec3 v) { position += v; return this; }
};

#define COMP_CAM 0x01
class Camera : public Component {
public:
	Camera();


	bool ortographic;
	float fov, orthoSize;

	Vec3 camVerts[6];
	static int camVertsIds[19];
	
	void UpdateCamVerts();
	void DrawEditor();
	void DrawInspector(Editor* e, Component* c, Color v, uint& pos);
};

#define COMP_MFT 0x02
class MeshFilter : public Component {
public:
	MeshFilter();

	void DrawEditor() {}
	void DrawInspector(Editor* e, Component* c, Color v, uint& pos) {}
};

#define COMP_SCR 0xff
class SceneScript : public Component {
public:
	SceneScript() : Component(COMP_SCR, false) {}
	virtual void Start() {}
	virtual void Update() {}
	virtual void LateUpdate() {}
	virtual void Paint() {}

	void DrawEditor() {} //nothing
	void DrawInspector(Editor* e, Component* c, Color v, uint& pos) {}
};

class SceneObject : public Object {
public:
	SceneObject();
	SceneObject(string s);
	SceneObject(Vec3 pos, Quat rot, Vec3 scale);
	SceneObject(string s, Vec3 pos, Quat rot, Vec3 scale);
	bool active;
	int childCount;
	string name;
	Transform transform;

	void Enable(), Disable();
	void Enable(bool enableAll), Disable(bool disableAll);

	vector<SceneObject*> children;

	SceneObject* AddChild(SceneObject* child) { childCount++; children.push_back(child); return this; }
	SceneObject* GetChild(int i) { return children[i]; }
	Component* AddComponent(Component* c);
	Component* GetComponent(COMPONENT_TYPE type);
	void RemoveComponent(Component* c);

	bool _expanded;
	int _componentCount;
	vector<Component*> _components;

};

#endif