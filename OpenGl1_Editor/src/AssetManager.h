#include "Engine.h"

class AssetItem {
public:
	ASSETTYPE type;
	bool loaded;
	void* obj;
	byte bundleId;
	long dataPos;
};

class AssetManager {
public:
	void GetAsset() {

	}
protected:
	vector<AssetItem> assets;
	vector<AssetItem> alwaysIncludedAssets;
};