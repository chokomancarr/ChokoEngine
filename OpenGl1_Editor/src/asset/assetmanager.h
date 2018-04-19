#pragma once
#include "AssetObjects.h"

class AssetManager {
public:
	/*
	friend int main(int argc, char **argv);
	friend class Engine;
	friend class Editor;
	friend class Scene;
	friend class SceneObject;
	friend class Material;
	friend struct Editor_PlaySyncer;
	template<typename T> friend T* _GetCache(ASSETTYPE t, ASSETID i, bool async = false);
	friend string _Strm2Asset(std::istream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int max);
	protected:
	*/
	static std::unordered_map<ASSETTYPE, std::vector<string>, std::hash<byte>> names;
#ifndef IS_EDITOR
	static string eBasePath;
	static std::unordered_map<ASSETTYPE, std::vector<string>, std::hash<byte>> dataELocs;
	static std::unordered_map<ASSETTYPE, std::vector<std::pair<byte*, uint>>, std::hash<byte>> dataECaches;
	static std::vector<size_t> dataECacheLocs;
	static std::vector<size_t> dataECacheSzLocs;
	static void AllocECache();
#endif
	static std::vector <std::pair<ASSETTYPE, ASSETID>> dataECacheIds;
	static std::unordered_map<ASSETTYPE, std::vector<std::pair<byte, std::pair<uint, uint>>>, std::hash<byte>> dataLocs;
	static std::unordered_map<ASSETTYPE, std::vector<AssetObject*>, std::hash<byte>> dataCaches;
	static std::vector<std::ifstream*> streams;
	static void Init(string dpath);
	static AssetObject* CacheFromName(string nm);
	static ASSETID GetAssetId(string nm, ASSETTYPE &t);
	static AssetObject* GetCache(ASSETTYPE t, ASSETID i);
	static AssetObject* GenCache(ASSETTYPE t, ASSETID i);
};

#ifndef IS_EDITOR
template<typename T> T* _GetCache(ASSETTYPE t, ASSETID i, bool async = false) {
	return static_cast<T*>(AssetManager::GetCache(t, i));
}
#endif