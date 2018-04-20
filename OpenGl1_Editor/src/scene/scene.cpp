#include "Engine.h"
#include "Editor.h"

void Serialize(Editor* e, SceneObject* o, std::ofstream* stream) {
#ifdef IS_EDITOR
	/*
	Object data
	O[tx][ty][tz][rx][ry][rz][sx][sy][sz] (trs=float32)
	(components)
	C[type][...]c (type=byte)
	(child objects...)
	o
	*/
	*stream << "O";
	//_StreamWrite(&o->name, stream, o->name.size());
	*stream << o->name;
	*stream << (char)0;
	auto& pos = o->transform.localPosition();
	_StreamWrite(&pos, stream, 12);
	auto& rot = o->transform.localEulerRotation();
	_StreamWrite(&rot, stream, 12);
	auto& scl = o->transform.localScale();
	_StreamWrite(&scl, stream, 12);
	for (auto& c : o->_components)
	{
		stream->write("C", 1);
		byte t = c->componentType;
		_StreamWrite(&t, stream, 1);
		c->Serialize(e, stream);
		stream->write("c", 1);
	}
	for (auto& oo : o->children)
	{
		Serialize(e, oo.get(), stream);
	}
	*stream << "o";
#endif
}

void Deserialize(std::ifstream& stream, SceneObject* obj) {
#ifndef CHOKO_LAIT
	char cc[100];
	stream.getline(cc, 100, 0);
	obj->name = string(cc);
	Debug::Message("Object Deserializer", "Deserializing object " + obj->name);
	char c;
	Vec3 pos, rot, scl;
	_Strm2Val(stream, pos);
	_Strm2Val(stream, rot);
	_Strm2Val(stream, scl);
	auto& tr = obj->transform;
	tr.localPosition(pos);
	tr.localEulerRotation(rot);
	tr.localScale(scl);

	c = stream.get();
	while (c != 'o') {
		//Debug::Message("Object Deserializer", std::to_string(c) + " " + std::to_string(stream.tellg()));
		if (c == 'O') {
			auto sc = SceneObject::New();
			obj->AddChild(sc);
			Deserialize(stream, sc.get());
		}
		else if (c == 'C') {
			c = stream.get(); //component type
			Debug::Message("Object Deserializer", "component " + std::to_string(c) + " " + std::to_string((int)c));
			switch (c) {
			case COMP_CAM:
				obj->AddComponent<Camera>(stream, obj);
				break;
			case COMP_MFT:
				obj->AddComponent<MeshFilter>(stream, obj);
				break;
			case COMP_MRD:
				obj->AddComponent<MeshRenderer>(stream, obj);
				break;
			case COMP_TRD:
				obj->AddComponent<TextureRenderer>(stream, obj);
				break;
			case (char)COMP_LHT:
				obj->AddComponent<Light>(stream, obj);
				break;
			case (char)COMP_RDP:
				obj->AddComponent<ReflectionProbe>(stream, obj);
				break;
			case (char)COMP_SCR:
#ifdef IS_EDITOR
				obj->AddComponent<SceneScript>(stream, obj);
#else
				obj->AddComponent(pComponent(SceneScriptResolver::instance->Resolve(stream, obj)));
#endif
				break;
			default:
				std::cout << "unknown component " << (int)c << "!" << std::endl;
				char cc;
				cc = stream.get();
				while (cc != 'c')
					stream >> cc;
				long long ss = stream.tellg();
				stream.seekg(ss - 1);
				break;
			}
			//Debug::Message("Object Deserializer", "2 " + std::to_string(stream.tellg()));
			c = stream.get();
			if (c != 'c') {
				Debug::Error("Object Deserializer", "scene data corrupted(component)");
				abort();
			}
		}
		else {
			Debug::Error("Object Deserializer", "scene data corrupted(2)");
			abort();
		}
		c = stream.get();
	}
	Debug::Message("Object Deserializer", "-- End --");
#endif
}




Scene::_offset_map Scene::_offsets = {};

Scene* Scene::active = nullptr;
std::ifstream* Scene::strm = nullptr;
#ifndef IS_EDITOR
std::vector<string> Scene::sceneEPaths = {};
#endif
std::vector<string> Scene::sceneNames = {};
std::vector<long> Scene::scenePoss = {};

Scene::Scene() : sceneName("newScene"), settings() {
	std::vector<pSceneObject>().swap(objects);
}

#ifndef CHOKO_LAIT
Scene::Scene(std::ifstream& stream, long pos) : sceneName("") {
	Debug::Message("SceneLoader", "Begin Loading Scene...");
	std::vector<pSceneObject>().swap(objects);
	stream.seekg(pos);
	char h1, h2;
	stream >> h1 >> h2;
	if (h1 != 'S' || h2 != 'N')
		Debug::Error("Scene Loader", "scene data corrupted");
	char* cc = new char[100];
	stream.getline(cc, 100, 0);
	sceneName += string(cc);
	ASSETTYPE t;

	_Strm2Asset(stream, Editor::instance, t, settings.skyId);
	if (settings.skyId >= 0 && t != ASSETTYPE_HDRI) {
		Debug::Error("Scene", "Sky asset invalid!");
		return;
	}

	Debug::Message("SceneLoader", "Loading SceneObjects...");
	char o;
	stream >> o;
	while (!stream.eof() && o == 'O') {
		auto sc = SceneObject::New();
		objects.push_back(sc);
		objectCount++;
		Deserialize(stream, sc.get());
		sc->Refresh();
		stream >> o;
	}

	Debug::Message("SceneLoader", "Loading Background...");
	settings.sky = _GetCache<Background>(t, settings.skyId);
	Debug::Message("SceneLoader", "Scene Loaded.");
}

void Scene::Load(uint i) {
	if (i >= sceneNames.size()) {
		Debug::Error("Scene Loader", "Scene ID (" + std::to_string(i) + ") out of range!");
		return;
	}
	Debug::Message("Scene Loader", "Loading scene " + std::to_string(i) + "...");
#ifndef IS_EDITOR
	if (_pipemode) {
		std::ifstream strm2(sceneEPaths[i], std::ios::binary);
		active = new Scene(strm2, 0);
	}
	else {
#else
		{
#endif
			if (active) delete(active);
			active = new Scene(*strm, scenePoss[i]);
		}
		active->sceneId = i;
		Debug::Message("Scene Loader", "Loaded scene " + std::to_string(i) + "(" + sceneNames[i] + ")");
	}

void Scene::Load(string name) {
	for (uint a = sceneNames.size(); a > 0; a--) {
		if (sceneNames[a - 1] == name) {
			Load(a - 1);
			return;
		}
	}
}

#endif

void Scene::AddObject(pSceneObject object, pSceneObject parent) {
	if (!active)
		return;
	if (!parent) {
		active->objects.push_back(object);
		active->objectCount++;
	}
	else
		parent->AddChild(object);
}

void Scene::DeleteObject(pSceneObject o) {
	if (!active)
		return;
	o->dead = true;
}

void Scene::ReadD0() {
#ifndef IS_EDITOR
	ushort num;
	_Strm2Val(*strm, num);
	for (ushort a = 0; a < num; a++) {
		uint sz;
		char c[100];
		if (_pipemode) {
			strm->getline(c, 100);
			sceneNames.push_back(c);
			sceneEPaths.push_back(AssetManager::eBasePath + sceneNames[a]);
			Debug::Message("AssetManager", "Registered scene " + sceneNames[a]);
		}
		else {
			_Strm2Val(*strm, sz);
			*strm >> c[0];
			long p1 = (long)strm->tellg();
			scenePoss.push_back(p1);
			*strm >> c[0] >> c[1];
			if (c[0] != 'S' || c[1] != 'N') {
				Debug::Error("Scene Importer", "fatal: Scene Signature wrong!");
				return;
			}
			strm->getline(c, 100, 0);
			sceneNames.push_back(c);
			strm->seekg(p1 + sz + 1);
			Debug::Message("AssetManager", "Registered scene " + sceneNames[a] + " (" + std::to_string(sz) + " bytes)");
		}
	}
#endif
}

void Scene::Unload() {

}

void Scene::CleanDeadObjects() {
	for (int a = active->objectCount - 1; a >= 0; a--) {
		if (active->objects[a]->dead) {
			active->objects.erase(active->objects.begin() + a);
			active->objectCount--;
		}
	}
}

#ifdef IS_EDITOR

void Scene::Save(Editor* e) {
	std::ofstream sw(e->projectFolder + "Assets\\" + sceneName + ".scene", std::ios::out | std::ios::binary);
	sw << "SN";
	sw << sceneName << (char)0;
	_StreamWriteAsset(e, &sw, ASSETTYPE_HDRI, settings.skyId);
	for (auto& sc : objects) {
		Serialize(e, sc.get(), &sw);
	}
	sw.close();
	int a = 0;
	for (auto& v : e->normalAssetCaches[ASSETTYPE_MATERIAL]) {
		if (v.get()) {
			Material* m = (Material*)v.get();
			//if (m->_changed) {
			m->Save(e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_MATERIAL][a]);
			//}
		}
		a++;
	}
	//
	//e->includedScenes.clear();
	//e->includedScenes.push_back(e->projectFolder + "Assets\\test.scene");
}
#endif