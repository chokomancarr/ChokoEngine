#pragma once
#include "AssetObjects.h"

class AssetObject : public Object {
	friend class Editor;
	friend struct Editor_PlaySyncer;
protected:
	AssetObject(ASSETTYPE t) : type(t), Object(), _changed(false)
#ifdef IS_EDITOR
		, _eCache(nullptr), _eCacheSz(0)
#endif
	{}
	virtual  ~AssetObject() {}

	const ASSETTYPE type = ASSETTYPE_UNDEF;
#ifdef IS_EDITOR
	byte* _eCache = nullptr; //first byte is always 255
	uint _eCacheSz = 0;
#endif
	bool _changed;
	virtual void GenECache() {}

	virtual bool DrawPreview(uint x, uint y, uint w, uint h) { return false; }

	//virtual void Load() = 0;
};