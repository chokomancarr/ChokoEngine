#pragma once
#include "SceneObjects.h"

struct RSM_RANDOM_BUFFER {
	float xPos[1024];
	float yPos[1024];
	float size[1024];
};

enum LIGHTTYPE : byte {
	LIGHTTYPE_POINT,
	LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_SPOT,
};

enum LIGHT_FALLOFF : byte {
	LIGHT_FALLOFF_INVERSE,
	LIGHT_FALLOFF_INVSQUARE,
	LIGHT_FALLOFF_LINEAR,
	LIGHT_FALLOFF_CONSTANT
};
#define LIGHT_POINT_MINSTR 0.01f
#define BUFFERLOC_LIGHT_RSM 2
class Light : public Component {
public:
	Light();
	LIGHTTYPE lightType() { return _lightType; }

	float intensity = 1;
	Vec4 color = white();
	float angle = 60;
	float minDist = 0.01f, maxDist = 5;
	bool drawShadow = true;
	float shadowBias = 0.001f, shadowStrength = 1;
	bool contactShadows = false;
	float contactShadowDistance = 0.1f;
	uint contactShadowSamples = 20;
	Texture* cookie = 0;
	float cookieStrength = 1;
	bool square = false;
	LIGHT_FALLOFF falloff = LIGHT_FALLOFF_INVSQUARE;
	Texture* hsvMap = 0;

	void DrawShadowMap(GLuint tar = 0), BlitRSMFlux(), DrawRSM(Mat4x4& ip, Mat4x4& lp, float w, float h, GLuint gtexs[], GLuint gdepth);

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Camera;
	friend class Engine;
	friend class EB_Viewer;
	friend class EB_Previewer;
	_allowshared(Light);
protected:
	LIGHTTYPE _lightType;
	Light(std::ifstream& stream, SceneObject* o, long pos = -1);
	//Mat4x4 _shadowMatrix;
	ASSETID _cookie = -1, _hsvMap = -1;
	static void _SetCookie(void* v), _SetHsvMap(void* v);

#ifdef IS_EDITOR
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;
#endif

	static void InitShadow(), InitRSM();
	void CalcShadowMatrix();
	static GLuint _shadowFbo, _shadowGITexs[3], _shadowMap;
	static GLuint _shadowCubeFbos[6], _shadowCubeMap;
	static GLuint _fluxFbo, _fluxTex, _rsmFbo, _rsmTex, _rsmUBO;
	static RSM_RANDOM_BUFFER _rsmBuffer;

	static std::vector<GLint> paramLocs_Point, paramLocs_Spot, paramLocs_SpotCS, paramLocs_SpotFluxer, paramLocs_SpotRSM; //make writing faster
	static void ScanParams();
	//static CubeMap* _shadowCube;
};
