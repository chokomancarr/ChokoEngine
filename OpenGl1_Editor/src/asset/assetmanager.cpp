#include "Engine.h"
#include "Editor.h"
#include "hdr.h"
#include <iomanip>
#include <algorithm>
#include <climits>
#include <thread>
#include "Defines.h"
#include "SceneScriptResolver.h"
#ifdef IS_EDITOR
#include <shellapi.h>
#endif

//#define F2ISTREAM(_varname, _pathname) std::ifstream _f2i_ifstream((_pathname).c_str(), std::ios::in | std::ios::binary); \
//std::istream _varname(_f2i_ifstream.rdbuf());

std::unordered_map<ASSETTYPE, std::vector<string>, std::hash<byte>> AssetManager::names = std::unordered_map<ASSETTYPE, std::vector<string>, std::hash<byte>>();
std::unordered_map<ASSETTYPE, std::vector<std::pair<byte, std::pair<uint, uint>>>, std::hash<byte>> AssetManager::dataLocs = std::unordered_map<ASSETTYPE, std::vector<std::pair<byte, std::pair<uint, uint>>>, std::hash<byte>>();
std::unordered_map<ASSETTYPE, std::vector<AssetObject*>, std::hash<byte>> AssetManager::dataCaches = std::unordered_map<ASSETTYPE, std::vector<AssetObject*>, std::hash<byte>>();
std::vector<std::ifstream*> AssetManager::streams = {};
#ifndef IS_EDITOR
string AssetManager::eBasePath = "";
std::unordered_map<ASSETTYPE, std::vector<string>, std::hash<byte>> AssetManager::dataELocs = std::unordered_map<ASSETTYPE, std::vector<string>, std::hash<byte>>();
std::unordered_map<ASSETTYPE, std::vector<std::pair<byte*, uint>>, std::hash<byte>> AssetManager::dataECaches = std::unordered_map<ASSETTYPE, std::vector<std::pair<byte*, uint>>, std::hash<byte>>();
std::vector<size_t> AssetManager::dataECacheLocs = {};
std::vector<size_t> AssetManager::dataECacheSzLocs = {};
#endif
std::vector<std::pair<ASSETTYPE, ASSETID>> AssetManager::dataECacheIds = {};

void AssetManager::Init(string dpath) {
#if !defined(IS_EDITOR) && !defined(CHOKO_LAIT)
	names.clear();
	dataLocs.clear();
	std::vector<std::ifstream*>().swap(streams);
	if (_pipemode) {
		dataECaches.emplace(ASSETTYPE_TEXTURE, std::vector<std::pair<byte*, uint>>());
		dataECaches.emplace(ASSETTYPE_HDRI, std::vector<std::pair<byte*, uint>>());
		dataECaches.emplace(ASSETTYPE_MESH, std::vector<std::pair<byte*, uint>>());
		dataECaches.emplace(ASSETTYPE_MATERIAL, std::vector<std::pair<byte*, uint>>());
		dataECaches.emplace(ASSETTYPE_SHADER, std::vector<std::pair<byte*, uint>>());

		string pp = dpath + "0_";
		Scene::strm = new std::ifstream(pp.c_str(), std::ios::in | std::ios::binary);
		std::ifstream* strm = Scene::strm;
		if (!strm->is_open()) {
			Debug::Error("AssetManager", "Fatal: cannot open data file 0!");
			abort();
		}

		ushort num;
		ASSETTYPE type;
		ushort id;
		char c[100];
		*strm >> c[0] >> c[1];
		if (c[0] != 'D' || c[1] != '0') {
			Debug::Error("AssetManager", "Fatal: file 0 has wrong signature!");
			abort();
		}
		strm->getline(c, 100, char0);
		string path(c);
		_Strm2Val(*strm, num);
		for (ushort a = 0; a < num; a++) {
			_Strm2Val(*strm, type);
			_Strm2Val(*strm, id);
			strm->getline(c, 100, (char)0);
			string nm = path + string(c);
			strm->getline(c, 100, (char)0);
			//std::cout << (int)type << ": " << nm << std::endl;
			while (dataCaches[type].size() < id) {
				dataCaches[type].push_back(nullptr);
				names[type].push_back("");
				dataELocs[type].push_back("");
			}
			dataCaches[type].push_back(nullptr);
			names[type].push_back(c);
			dataELocs[type].push_back(nm);

			dataECacheIds.push_back(std::pair<ASSETTYPE, ASSETID>(type, id));
			dataECaches[type].push_back(std::pair<byte*, uint>(nullptr, 0));
			dataECacheLocs.push_back(0);
			dataECacheSzLocs.push_back((size_t)(new uint(0)));
		}
	}
	else {
		string pp = dpath + "0";
		Scene::strm = new std::ifstream(pp.c_str(), std::ios::in | std::ios::binary);
		std::ifstream* strm = Scene::strm;
		if (!strm->is_open()) {
			Debug::Error("AssetManager", "Fatal: cannot open data file 0!");
			abort();
		}

		byte numDat = 0, id;
		ushort num;
		ASSETTYPE type;
		uint pos;
		char c[100];
		*strm >> c[0] >> c[1];
		if (c[0] != 'D' || c[1] != '0') {
			Debug::Error("AssetManager", "Fatal: file 0 has wrong signature!");
			abort();
		}
		_Strm2Val(*strm, num);
		for (ushort a = 0; a < num; a++) {
			_Strm2Val(*strm, type);
			_Strm2Val(*strm, id);
			_Strm2Val(*strm, pos);
			strm->getline(c, 100, (char)0);
			//std::cout << (int)type << " " << (int)id << " " << pos << ": " << string(c) << std::endl;
			numDat = max(numDat, id);
			dataLocs[type].push_back(std::pair<byte, std::pair<uint, uint>>(id - 1, std::pair<uint, uint>(pos, 0)));
			dataCaches[type].push_back(nullptr);
			names[type].push_back(c);
		}

		for (int a = 0; a < numDat; a++) {
			string pp = dpath + std::to_string(a + 1);
			streams.push_back(new std::ifstream(pp.c_str(), std::ios::in | std::ios::binary));
			if (streams[a]->is_open())
				Debug::Message("AssetManager", "Streaming data" + std::to_string(a + 1));
			else {
				Debug::Error("AssetManager", "Fatal: Failed to open data" + std::to_string(a + 1) + "!");
				abort();
			}
		}

		for (auto& aa : dataLocs) {
			ushort s = 0;
			for (auto& bb : aa.second) {
				streams[bb.first]->seekg(bb.second.first);
				_Strm2Val(*streams[bb.first], bb.second.second);
				Debug::Message("AssetManager", "Registered asset " + names[aa.first][s] + " (" + std::to_string(bb.second.second) + " bytes)");
				streams[bb.first]->seekg(0);
				s++;
			}
		}
		//long long ppos = strm->tellg();
	}
	Scene::ReadD0();
#endif
}

#ifndef IS_EDITOR
void AssetManager::AllocECache() {
	uint c = 0;
	for (auto& a : dataECacheIds) {
		auto& b = dataECaches[a.first][a.second];
		b.second = *(uint*)dataECacheSzLocs[c];
		b.first = (byte*)malloc(b.second + 1);
		*b.first = 0;
		//if (!!b.second) std::cout << "Allocating: " << b.second << "B" << std::endl;
		dataECacheLocs[c] = (size_t)b.first;
#ifdef VERBOSE
		std::cout << "Allocated " << (int)a.first << "." << a.second << ": " << b.second;
		std::cout << " (" << (uint)&b.second << ")";
		std::cout << " -> " << (uint)b.first << std::endl;
#endif
		c++;
	}
}
#endif

AssetObject* AssetManager::CacheFromName(string nm) {
	//ASSETTYPE t;
	for (auto& t : names) {
		for (uint a = names[t.first].size(); a > 0; a--) {
			if (names[t.first][a - 1] == nm) {
				return GetCache(t.first, a - 1);
			}
		}
	}
	Debug::Warning("Asset Finder", "Asset not found for " + nm + "!");
	return nullptr;
}

ASSETID AssetManager::GetAssetId(string nm, ASSETTYPE &t) {
	for (auto& q : names) {
		for (uint a = q.second.size(); a > 0; a--) {
			if (q.second[a - 1] == nm) {
				t = q.first;
				return a - 1;
			}
		}
	}
	t = ASSETTYPE_UNDEF;
	return -1;
}

AssetObject* AssetManager::GetCache(ASSETTYPE t, ASSETID i) {
	if (i == -1)
		return nullptr;
	if (dataCaches[t][i] != nullptr)
		return dataCaches[t][i];
	else
		return GenCache(t, i);
}
AssetObject* AssetManager::GenCache(ASSETTYPE t, ASSETID i) {
#if !defined(IS_EDITOR) && !defined(CHOKO_LAIT)
	std::ifstream* fstrm = nullptr;
	std::stringstream sstrm;
	uint off = 0, cnt = -1;
	byte* cache = 0;
	if (_pipemode) {
		if (!!(*dataECaches[t][i].first)) {
			cache = dataECaches[t][i].first + 1;
		}
		else {
			fstrm = new std::ifstream(dataELocs[t][i], std::ios::in | std::ios::binary);
			sstrm << fstrm->rdbuf();
		}
	}
	else {
		fstrm = streams[dataLocs[t][i].first];
		off = dataLocs[t][i].second.first + 4;
		cnt = dataLocs[t][i].second.second;
		char *data = new char[cnt];
		fstrm->read(data, cnt);
		sstrm.write(data, cnt);
	}
	std::istream strm(sstrm.rdbuf());
	//strm->seekg(off);
	//uint sz;
	//_Strm2Val(*strm, sz);
	//off += 4;
	switch (t) {
	case ASSETTYPE_TEXTURE:
		dataCaches[t][i] = new Texture(strm, off);
		break;
	case ASSETTYPE_HDRI:
		dataCaches[t][i] = cache ? new Background(cache) : new Background(strm, off);
		break;
	case ASSETTYPE_MESH:
		dataCaches[t][i] = cache ? new Mesh(cache) : new Mesh(strm, off);
		break;
	case ASSETTYPE_MATERIAL:
		dataCaches[t][i] = new Material(strm, off);
		break;
	case ASSETTYPE_SHADER:
		dataCaches[t][i] = new Shader(strm, off);
		break;
	default:
		Debug::Error("AssetManager", "No operation suits asset type " + std::to_string(t) + "!");
		return nullptr;
	}
	if (_pipemode && !cache) delete(fstrm);
	if (cache) std::cout << "Using cache for " << (int)t << "." << i << std::endl;
	//std::cout << "Loaded " << (int)t << "." << i << "in " << (clock() - time) << "ms" << std::endl;
	return dataCaches[t][i];
#else
	return nullptr;
#endif
}
