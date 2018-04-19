#pragma once
#include "SceneObjects.h"

class SceneSettings {
public:
	Background* sky;
	float skyStrength, skyBrightness;
	Color ambientCol;
	GITYPE GIType = GITYPE_RSM;
	float rsmRadius;

	bool useFog, sunFog;
	float fogDensity, fogSunSpread;
	Vec4 fogColor, fogSunColor;

	friend class Editor;
	friend class EB_Inspector;
	friend class Scene;
protected:
	SceneSettings() : sky(nullptr), skyId(-1), skyStrength(1), skyBrightness(1) {
		sky = 0;
	}

	SceneSettings(const SceneSettings&);
	SceneSettings& operator= (const SceneSettings&);

	int skyId;
};

/*! A scene contains everything in a level.
*/
class Scene {
public:
	Scene();

#ifndef CHOKO_LAIT
	//	Scene() : sceneName("newScene"), settings(), objects() {}
	Scene(std::ifstream& stream, long pos);
	~Scene() {}
	static bool loaded() {
		return active != nullptr;
	}
	int sceneId;

	static void Load(uint i), Load(string name);
#endif

	string sceneName;

	/*! The current active scene.
	In Lait versions, this value cannot be changed.
	*/
	static Scene* active;

	uint objectCount = 0;
	std::vector<pSceneObject> objects;
	SceneSettings settings;
	std::vector<Component*> _preUpdateComps;
	std::vector<Component*> _preLUpdateComps;
	std::vector<Component*> _preRenderComps;
	static void AddObject(pSceneObject object, pSceneObject parent = nullptr);
	static void DeleteObject(pSceneObject object);

	friend int main(int argc, char **argv);
	friend class Editor;
	friend struct Editor_PlaySyncer;
	friend class AssetManager;
	friend class Component;

protected:
	static std::ifstream* strm;
	//#ifndef IS_EDITOR
	static std::vector<string> sceneEPaths;
	//#endif
	static std::vector<string> sceneNames;
	static std::vector<long> scenePoss;

	static void ReadD0();
	static void Unload();
	static void CleanDeadObjects();
#ifdef IS_EDITOR
	void Save(Editor* e);
#endif
	static struct _offset_map {
		uint objCount = offsetof(Scene, objectCount),
			objs = offsetof(Scene, objects);
	} _offsets;
};
