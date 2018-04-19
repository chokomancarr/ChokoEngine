#pragma once

/*! Base class of instantiatable object
[av] */
class Object : public std::enable_shared_from_this<Object> {
public:
	Object(string nm = "");
	Object(ulong id, string nm);
	ulong id;
	string name;
	bool dirty = false; //triggers a reload of internal variables
	bool dead = false; //will be cleaned up after this frame

	virtual bool ReferencingObject(Object* o) { return false; }
};