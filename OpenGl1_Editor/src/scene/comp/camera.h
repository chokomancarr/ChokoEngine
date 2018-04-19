#pragma once
#include "SceneObjects.h"

class Camera : public Component {
public:
	Camera();

	bool ortographic;
	float fov, orthoSize;
	Rect screenPos;
	CAM_CLEARTYPE clearType;
	Vec4 clearColor;
	float nearClip;
	float farClip;
	rRenderTexture targetRT;
	std::vector<rCameraEffect> effects;

	void Render(RenderTexture* target = nullptr, renderFunc func = nullptr);

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class EB_Viewer;
	friend class EB_Inspector;
	friend class EB_Previewer;
	friend class Background;
	friend class MeshRenderer;
	friend class SkinnedMeshRenderer;
	friend class Engine;
	friend class Editor;
	friend class Light;
	friend class ReflectiveQuad;
	friend class Texture;
	friend class RenderTexture;
	friend class ReflectionProbe;
	friend class CubeMap;
	friend class Color;
	_allowshared(Camera);
protected:
	Camera(std::ifstream& stream, SceneObject* o, long pos = -1);

	std::vector<ASSETID> _effects;

	static void DrawSceneObjectsOpaque(std::vector<pSceneObject> oo, GLuint shader = 0);
	static void DrawSceneObjectsOverlay(std::vector<pSceneObject> oo, GLuint shader = 0);
	void RenderLights(GLuint targetFbo = 0);
	void DumpBuffers();

	void _RenderProbesMask(std::vector<pSceneObject>& objs, Mat4x4 mat, std::vector<ReflectionProbe*>& probes), _RenderProbes(std::vector<ReflectionProbe*>& probes, Mat4x4 mat);
	void _DoRenderProbeMask(ReflectionProbe* p, Mat4x4& ip), _DoRenderProbe(ReflectionProbe* p, Mat4x4& ip);
	static void _RenderSky(Mat4x4 ip, GLuint d_texs[], GLuint d_depthTex, float w = (float)Display::width, float h = (float)Display::height);
	void _DrawLights(std::vector<pSceneObject>& oo, Mat4x4& ip, GLuint targetFbo = 0);
	static void _ApplyEmission(GLuint d_fbo, GLuint d_texs[], float w = (float)Display::width, float h = (float)Display::height, GLuint targetFbo = 0);
	static void _DoDrawLight_Point(Light* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w = (float)Display::width, float h = (float)Display::height, GLuint targetFbo = 0);
	static void _DoDrawLight_Spot(Light* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w = (float)Display::width, float h = (float)Display::height, GLuint targetFbo = 0);
	static void _DoDrawLight_Spot_Contact(Light* l, Mat4x4& p, GLuint d_depthTex, float w, float h, GLuint src, GLuint tar);
	static void _DoDrawLight_ReflQuad(ReflectiveQuad* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w = Display::width, float h = Display::height, GLuint targetFbo = 0);

	static void GenShaderFromPath(const string& pathv, const string& pathf, GLuint* program);
	static void GenShaderFromPath(GLuint vertex_shader, const string& path, GLuint* program);

	Vec3 camVerts[6];
	static const int camVertsIds[19];
	GLuint d_fbo, d_texs[4], d_depthTex;
	uint d_w, d_h;
	static GLuint d_probeMaskProgram, d_probeProgram, d_blurProgram, d_blurSBProgram, d_skyProgram, d_pLightProgram, d_sLightProgram, d_sLightCSProgram, d_sLightRSMProgram, d_sLightRSMFluxProgram;
	static GLuint d_reflQuadProgram;
	static GLint d_skyProgramLocs[];

	static GLuint fullscreenVao, fullscreenVbo;
	static const float fullscreenVerts[];
	static const int fullscreenIndices[];

	int _tarRT;

	static std::unordered_map<string, GLuint> fetchTextures;
	static std::vector<string> fetchTexturesUpdated;
	static GLuint DoFetchTexture(string s);
	static void ClearFetchTextures();

	static const string _gbufferNames[];

	void ApplyGL();

	static void InitShaders();
	void UpdateCamVerts();
	void InitGBuffer();

#ifdef IS_EDITOR
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	static void _SetClear0(EditorBlock* b), _SetClear1(EditorBlock* b), _SetClear2(EditorBlock* b), _SetClear3(EditorBlock* b);
#endif
};
