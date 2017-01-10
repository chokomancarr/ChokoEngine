#include <unordered_map>
#include "Engine.h"

typedef SceneScript (*sceneScriptInstantiator)();
class SceneScriptResolver {
public:
	SceneScriptResolver();
	std::unordered_map<int, sceneScriptInstantiator> map;
};