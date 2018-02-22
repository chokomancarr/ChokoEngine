#include "Engine.h"
typedef SceneScript*(*sceneScriptInstantiator)();
class SceneScriptResolver {
public:
	SceneScriptResolver();
	static SceneScriptResolver* instance;
	std::vector<sceneScriptInstantiator> map;
	SceneScript* Resolve(std::ifstream& strm, SceneObject* o);
};