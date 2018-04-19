#pragma once
#include "SceneObjects.h"

class SceneObject : public Object {
public:
	~SceneObject();
	static pSceneObject New(Vec3 pos, Quat rot = Quat(), Vec3 scale = Vec3(1, 1, 1)) {
		auto p = pSceneObject(new SceneObject(pos, rot, scale));
		p->transform.Init(p, pos, rot, scale);
		return p;
	}
	static pSceneObject New(string s = "New Object", Vec3 pos = Vec3(), Quat rot = Quat(), Vec3 scale = Vec3(1, 1, 1)) {
		auto p = pSceneObject(new SceneObject(s, pos, rot, scale));
		p->transform.Init(p, pos, rot, scale);
		return p;
	}

	bool active = true;
	Transform transform;

	void SetActive(bool active, bool enableAll = false);

	rSceneObject parent;
	uint childCount = 0;
	std::vector<pSceneObject> children;

	void SetParent(pSceneObject parent, bool retainLocal = false);
	/*! Add child to this SceneObject.
	*  @param child The child SceneObject to add.
	*  @param retainLocal If true, the child's local transform is retained. If false, the child's world transform is retained.
	*/
	pSceneObject AddChild(pSceneObject child, bool retainLocal = false);
	pSceneObject GetChild(int i) { return children[i]; }
	pComponent AddComponent(pComponent c);
	template <typename T, class ...Args> std::shared_ptr<T> AddComponent(Args&& ...args) {
		auto c = std::make_shared<T>(std::forward<Args>(args)...);
		AddComponent(std::static_pointer_cast<Component>(c));
		return c;
	}

	/*! you should probably use GetComponent<T>() instead.
	*/
	pComponent GetComponent(COMPONENT_TYPE type);
	template<class T> std::shared_ptr<T> GetComponent() {
		static_assert(std::is_base_of<Component, T>::value, "T is not a Component type!");
		for (pComponent cc : _components)
		{
			auto xx = std::dynamic_pointer_cast<T>(cc);
			if (xx != nullptr) {
				return xx;
			}
		}
		return nullptr;
	}
	void RemoveComponent(pComponent c);

	bool _expanded;
	int _componentCount;
	std::vector<pComponent> _components;

	friend class MeshFilter;
	friend class Scene;
	friend class Editor;
	friend struct Editor_PlaySyncer;

	_allowshared(SceneObject);
protected:
	//set transform.object after construction
	SceneObject(Vec3 pos, Quat rot = Quat(), Vec3 scale = Vec3(1, 1, 1));
	SceneObject(string s = "New Object", Vec3 pos = Vec3(), Quat rot = Quat(), Vec3 scale = Vec3(1, 1, 1));
	SceneObject(byte* data);
	static pSceneObject New(byte* data) {
		auto p = pSceneObject(new SceneObject(data));
		p->transform.Init(p, data + offsetof(SceneObject, transform));
		return p;
	}

	static pSceneObject _FromId(const std::vector<pSceneObject>& objs, ulong id);

	static struct _offset_map {
		uint name = offsetof(SceneObject, name),
			transform = offsetof(SceneObject, transform),
			components = offsetof(SceneObject, _components),
			children = offsetof(SceneObject, children);
	} _offsets;

	void Refresh();
};