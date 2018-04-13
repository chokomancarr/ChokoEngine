#pragma once

#include "Engine.h"

typedef SceneScript*(*sceneScriptInstantiator)();
typedef void (*sceneScriptAssigner)(SceneScript*, std::ifstream&);

class SceneScriptResolver {
public:
	SceneScriptResolver(); 
	static SceneScriptResolver* instance; 
	std::vector<string> names; 
	std::vector<sceneScriptInstantiator> map;
	std::vector<sceneScriptAssigner> ass;
	SceneScript* Resolve(std::ifstream& strm);
};