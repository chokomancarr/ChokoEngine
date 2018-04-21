#include "Engine.h"
#include "Editor.h"

SceneObject::_offset_map SceneObject::_offsets = {};

SceneObject::SceneObject(Vec3 pos, Quat rot, Vec3 scale) : SceneObject("New Object", pos, rot, scale) {}
SceneObject::SceneObject(string s, Vec3 pos, Quat rot, Vec3 scale) : _expanded(true), Object(s) {
	id = Engine::GetNewId();
}
SceneObject::SceneObject(byte* data) : _expanded(true), Object(*((uint*)data + 1), string((char*)data + SceneObject::_offsets.name)) {}

SceneObject::~SceneObject() {

}

void SceneObject::SetActive(bool a, bool enableAll) {
	active = a;
}

bool SO_DoFromId(const std::vector<pSceneObject>& objs, uint id, pSceneObject& res) {
	for (auto a : objs) {
		if (a->id == id) {
			res = a;
			return true;
		}
		if (SO_DoFromId(a->children, id, res)) return true;
	}
	return false;
}

pSceneObject SceneObject::_FromId(const std::vector<pSceneObject>& objs, ulong id) {
	if (!id) return nullptr;
	pSceneObject res = 0;
	SO_DoFromId(objs, id, res);
	return res;
}


pComponent ComponentFromType(COMPONENT_TYPE t) {
	switch (t) {
	case COMP_CAM:
		return std::make_shared<Camera>();
	case COMP_MFT:
		return std::make_shared<MeshFilter>();
	default:
		return nullptr;
	}
}

void SceneObject::SetParent(pSceneObject nparent, bool retainLocal) {
	if (parent()) parent->children.erase(std::find_if(parent->children.begin(), parent->children.end(), [this](pSceneObject o)-> bool {return o.get() == this; }));
	parent->childCount--;
	if (nparent) parent->AddChild(pSceneObject(this), retainLocal);
}

pSceneObject SceneObject::AddChild(pSceneObject child, bool retainLocal) {
	Vec3 t;
	Quat r;
	if (!retainLocal) {
		t = child->transform._position;
		r = child->transform._rotation;
	}
	children.push_back(child);
	childCount++;
	child->parent(this);
	child->transform._UpdateWMatrix(transform._worldMatrix);
	if (!retainLocal) {
		child->transform.position(t);
		child->transform.rotation(r);
	}
	return get_shared<SceneObject>(this);
}

pComponent SceneObject::AddComponent(pComponent c) {
	c->object(this);
	int i = 0;
	for (COMPONENT_TYPE t : c->dependancies) {
		c->dependacyPointers[i] = GetComponent(t);
		if (!c->dependacyPointers[i]) {
			c->dependacyPointers[i](AddComponent(ComponentFromType(t)));
		}
		i++;
	}
	for (pComponent cc : _components)
	{
		if ((cc->componentType == c->componentType) && cc->componentType != COMP_SCR) {
			Debug::Warning("Add Component", "Same component already exists!");
			return cc;
		}
	}
	_components.push_back(c);
	_componentCount++;
	return c;
}

pComponent SceneObject::GetComponent(COMPONENT_TYPE type) {
	for (pComponent cc : _components)
	{
		if (cc->componentType == type) {
			return cc;
		}
	}
	return nullptr;
}

void SceneObject::RemoveComponent(pComponent c) {
	for (int a = _components.size() - 1; a >= 0; a--) {
		if (_components[a] == c) {
			for (int aa = _components.size() - 1; aa >= 0; aa--) {
				for (COMPONENT_TYPE t : _components[aa]->dependancies) {
					if (t == c->componentType) {
#ifdef IS_EDITOR
						Editor::instance->_Warning("Component Deleter", "Cannot delete " + c->name + " because other components depend on it!");
#else
						Debug::Warning("SceneObject", "Cannot delete " + c->name + " because other components depend on it!");
#endif
						return;
					}
				}
			}
			_components.erase(_components.begin() + a);
			return;
		}
	}
	Debug::Warning("SceneObject", "component to delete is not found");
}

void SceneObject::Refresh() {
	for (pComponent c : _components) {
		c->Refresh();
	}
}