#ifndef SCENE_H
#define SCENE_H

#include "Engine.h"

class Object {
public:
	Object(string nm = "") : id(Engine::GetNewId()), name(nm) {}
	ulong id;
	string name;

	virtual bool ReferencingObject(Object* o) { return false; }
};

typedef unsigned char COMPONENT_TYPE;
typedef unsigned char DRAWORDER;
#define DRAWORDER_NOT 0x00
#define DRAWORDER_SOLID 0x01
#define DRAWORDER_TRANSPARENT 0x02
#define DRAWORDER_OVERLAY 0x04
class Component : public Object {
public:
	Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder = 0x00, vector<COMPONENT_TYPE> dep = {}) : Object(name), componentType(t), active(true), drawOrder(drawOrder), _expanded(true), dependancies(dep) {}
	virtual  ~Component() {}

	const COMPONENT_TYPE componentType = 0;
	const vector<COMPONENT_TYPE> dependancies;
	const DRAWORDER drawOrder;
	bool active;
	SceneObject* object;

	bool serializable; 
	vector<pair<void*, void*>> serializedValues;

	bool _expanded;

	virtual void LoadDefaultValues() {} //also loads assets
	virtual void DrawEditor() {} //trs matrix not applied, apply before calling
	virtual void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) = 0;
	virtual void DrawGameCamera() {}
	virtual void Serialize(Editor* e, ofstream* stream) {}

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, ofstream* stream);
	friend class Scene;
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

class AssetObject : public Object {
protected:
	AssetObject(ASSETTYPE t) : type(t), Object() {}
	virtual  ~AssetObject() {}

	const ASSETTYPE type = 0;

	//virtual void Load() = 0;
};

class Mesh : public AssetObject {
public:
	//Mesh(); //until i figure out normal recalc algorithm
	bool loaded;

	vector<Vec3> vertices;
	vector<Vec3> normals;
	vector<int> triangles;

	uint vertexCount, triangleCount;

	friend int main(int argc, char **argv);
	friend class Editor;
private:
	Mesh(Editor* e, int i);
	Mesh(string path);
	//void Load();
};

class Texture {
public:
	Texture(const string& path);
	Texture(const string& path, bool mipmap);
	Texture(const string& path, bool mipmap, bool nearest);
	Texture(ifstream& stream, long pos);
	~Texture(){ glDeleteTextures(1, &pointer); }
	bool loaded;
	unsigned int width, height;
	GLuint pointer;
	friend int main(int argc, char **argv);
private:
	Texture(int i);
	void Load();
};

class Background : public AssetObject {
public:
	//Background() : AssetObject(ASSETTYPE_HDRI){}
	Background(const string& path);

	bool loaded;
	unsigned int width, height;
	GLuint pointer;
	friend int main(int argc, char **argv);
	static bool Parse(string path);

private:
	Background(int i);
	//void Load() {}
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
	void DrawEditor() override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos);
	void Serialize(Editor* e, ofstream* stream) override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, ofstream* stream);
	friend void Deserialize(ifstream& stream, SceneObject* obj);
protected:
	Camera(ifstream& stream, long pos);
};

#define COMP_MFT 0x02
class MeshFilter : public Component {
public:
	MeshFilter();
	
	Mesh* mesh;
	//void LoadDefaultValues() override;

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos);
	void Serialize(Editor* e, ofstream* stream) override;
	
	friend class Editor;
	friend void LoadMeshMeta(vector<SceneObject*>& os, string& path);
protected:
	void SetMesh(int i);
	int _mesh;
};

#define COMP_MRD 0x10
class MeshRenderer : public Component {
public:
	MeshRenderer();

	void DrawEditor() override {}
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos);
};

#define COMP_TRD 0x10
class TextureRenderer : public Component {
public:
	TextureRenderer(): _texture(-1), Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY) {}


	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos);
	void Serialize(Editor* e, ofstream* stream) override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, ofstream* stream);
	friend void Deserialize(ifstream& stream, SceneObject* obj);
protected:
	int _texture;
	TextureRenderer(ifstream& stream, long pos);
};

#define COMP_SCR 0xff
class SceneScript : public Component {
public:
	//called in editor
	SceneScript(Editor* e, string name);

	virtual void Start() {}
	virtual void Update() {}
	virtual void LateUpdate() {}
	virtual void Paint() {}

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos); //we want c to be null if deleted
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
	void RemoveComponent(Component*& c);

	bool _expanded;
	int _componentCount;
	vector<Component*> _components;

};

#endif