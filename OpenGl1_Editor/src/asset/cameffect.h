#pragma once
#include "AssetObjects.h"

class CameraEffect : public AssetObject {
public:
	CameraEffect(Material* mat);

	bool enabled = true;

	friend class Camera;
	friend class Engine;
	friend class Editor;
	friend class EB_Browser;
	friend class EB_Inspector;
	friend void EBI_DrawAss_Eff(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
	_allowshared(CameraEffect);
protected:
	Material* material;
	int _material = -1;
	bool expanded; //editor only
	string mainTex;

#ifdef IS_EDITOR
	void Save(string nm);
#endif

	CameraEffect(string s);
	//CameraEffect(std::ifstream& strm, uint offset);
	//void SetShader(ShaderBase* shad);
};
