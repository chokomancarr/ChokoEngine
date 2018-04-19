#include "Engine.h"

CameraEffect::CameraEffect(Material* mat) : AssetObject(ASSETTYPE_CAMEFFECT) {
	material = mat;
}

CameraEffect::CameraEffect(string p) : AssetObject(ASSETTYPE_CAMEFFECT) {
#ifdef IS_EDITOR
	//string p = Editor::instance->projectFolder + "Assets\\" + path;
	std::ifstream stream(p.c_str());
	if (!stream.good()) {
		std::cout << "cameffect not found!" << std::endl;
		return;
	}
	char* c = new char[4];
	stream.read(c, 3);
	c[3] = char0;
	string ss(c);
	if (ss != "KEF") {
		std::cerr << "file not supported" << std::endl;
		return;
	}
	delete[](c);
	char* nmm = new char[100];
	stream.getline(nmm, 100, char0);
	string shp(nmm);
	if (shp == "") {
		delete[](nmm);
		return;
	}
	ASSETTYPE t;
	Editor::instance->GetAssetInfo(shp, t, _material);
	material = _GetCache<Material>(ASSETTYPE_MATERIAL, _material);
	if (_material != -1) {
		stream.getline(nmm, 100, char0);
		mainTex = string(nmm);
	}
	delete[](nmm);
#endif
}

#ifdef IS_EDITOR
void CameraEffect::Save(string path) {
	std::ofstream strm(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	strm << "KEF";
	ASSETTYPE st;
	ASSETID si = Editor::instance->GetAssetId(material, st);
	_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_MATERIAL, si);
	if (si != -1) strm << mainTex << char0;
	strm.close();
}
#endif