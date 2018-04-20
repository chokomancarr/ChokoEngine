#include "Engine.h"

Object::Object(string nm) : id(Engine::GetNewId()), name(nm) {}
Object::Object(ulong id, string nm) : id(id), name(nm) {}