#pragma once
#include "SceneObjects.h"

class Component : public Object {
public:
	Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder = 0x00, SceneObject* o = nullptr, std::vector<COMPONENT_TYPE> dep = {});
	virtual  ~Component() {}

	const COMPONENT_TYPE componentType = COMP_UNDEF;
	const DRAWORDER drawOrder;
	bool active;
	rSceneObject object;

	virtual void OnPreUpdate() {}
	virtual void OnPreLUpdate() {}
	virtual void OnPreRender() {}

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend class Scene;
	friend class Editor;
	friend class EB_Viewer;
	friend class EB_Inspector;
	friend class SceneObject;
	friend bool DrawComponentHeader(Editor* e, Vec4 v, uint pos, Component* c);
	friend void DrawSceneObjectsOpaque(EB_Viewer* ebv, const std::vector<SceneObject*> &oo), DrawSceneObjectsGizmos(EB_Viewer* ebv, const std::vector<SceneObject*> &oo), DrawSceneObjectsTrans(EB_Viewer* ebv, std::vector<SceneObject*> oo);
protected:
	std::vector<COMPONENT_TYPE> dependancies;
	std::vector<rComponent> dependacyPointers;

	//bool serializable;
	//std::vector<pair<void*, void*>> serializedValues;

	bool _expanded;

	static COMPONENT_TYPE Name2Type(string nm);

	virtual void LoadDefaultValues() {} //also loads assets

#ifdef IS_EDITOR
	virtual void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) {} //trs matrix not applied, apply before calling
	virtual void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) = 0;
	//virtual void DrawGameCamera() {}
	virtual void Serialize(Editor* e, std::ofstream* stream) = 0;
#endif
	virtual void Refresh() {}
};
