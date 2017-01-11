#ifndef SCENE_H
#define SCENE_H

#include "Engine.h"

class Object {
public:
	Object() : id(Engine::GetNewId()) {}
	ulong id;
	string name;

	virtual bool ReferencingObject(Object* o) { return false; }
};

typedef unsigned char COMPONENT_TYPE;
class Component : public Object {
public:
	Component(COMPONENT_TYPE t, bool draw) : componentType(t), active(true), drawable(draw), _expanded(true) {}
	virtual  ~Component() {}

	const COMPONENT_TYPE componentType = 0;
	const bool drawable;
	bool active;
	SceneObject* object;

	bool serializable; 
	vector<pair<void*, void*>> serializedValues;

	bool _expanded;

	virtual void LoadDefaultValues() {}
	virtual void DrawEditor() = 0; //trs matrix not applied, apply before calling
	virtual void DrawInspector(Editor* e, Component* c, Color v, uint& pos) = 0;
	virtual void Serialize(Editor* e, ofstream* stream) {}
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

#define COMP_UNDEF 0x00

#define COMP_CAM 0x01
class Camera : public Component {
public:
	Camera();


	bool ortographic;
	float fov, orthoSize;
	Rect screenPos;

	Vec3 camVerts[6];
	static int camVertsIds[19];
	
	void UpdateCamVerts();
	void DrawEditor();
	void DrawInspector(Editor* e, Component* c, Color v, uint& pos);
	void Serialize(Editor* e, ofstream* stream) override;
};

#define COMP_MFT 0x02
class MeshFilter : public Component {
public:
	MeshFilter();

	void DrawEditor() {}
	void DrawInspector(Editor* e, Component* c, Color v, uint& pos);
};

#define COMP_MRD 0x10
class MeshRenderer : public Component {
public:
	MeshRenderer();

	void DrawEditor() {}
	void DrawInspector(Editor* e, Component* c, Color v, uint& pos);
};

#define COMP_SCR 0xff
class SceneScript : public Component {
public:
	//called in editor
	SceneScript(Editor* e, string name);

	string name;


	virtual void Start() {}
	virtual void Update() {}
	virtual void LateUpdate() {}
	virtual void Paint() {}

	void DrawEditor() {} //nothing
	void DrawInspector(Editor* e, Component* c, Color v, uint& pos);
	void Serialize(Editor* e, ofstream* stream) override;

	//bool ReferencingObject(Object* o) override;
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