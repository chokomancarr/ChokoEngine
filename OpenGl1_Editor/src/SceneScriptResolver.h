#include <vector>
#include "Engine.h"
typedef SceneScript*(*sceneScriptInstantiator)();
class SceneScriptResolver {
public:
	SceneScriptResolver();
	std::vector<sceneScriptInstantiator> map;
};