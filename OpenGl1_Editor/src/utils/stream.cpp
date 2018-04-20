#include "Engine.h"
#include "Editor.h"

void _StreamWrite(const void* val, std::ofstream* stream, int size) {
	stream->write(reinterpret_cast<char const *>(val), size);
}

void _StreamWriteAsset(Editor* e, std::ofstream* stream, ASSETTYPE t, ASSETID i) {
#ifdef IS_EDITOR
	if (i < 0) {
		(*stream) << (char)0;
		return;
	}
	string p;
	if (t == ASSETTYPE_SCRIPT_H)
		p = e->headerAssets[i];
	else
		p = e->normalAssets[t][i];
	(*stream) << p << char0;
#endif
}

ASSETID _Strm2H(std::istream& strm) {
	return -1;
}
string _Strm2Asset(std::istream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int max) {
	char* c = new char[max];
	strm.getline(c, max, (char)0);
	string s(c);
#ifdef IS_EDITOR
	e->GetAssetInfo(s, t, i);
#else
	i = AssetManager::GetAssetId(s, t);
#endif
	delete[](c);
	return s;
}