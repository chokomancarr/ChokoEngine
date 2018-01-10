#pragma once

#include "Engine.h"

enum COMPONENT_TYPE : byte {
	COMP_UNDEF = 0x00,
	COMP_CAM = 0x01,
	COMP_MFT = 0x02,
	COMP_MRD = 0x10,
	COMP_TRD = 0x11,
	COMP_SRD = 0x12,
	COMP_PST = 0x13,
	COMP_LHT = 0x20,
	COMP_RFQ = 0x22,
	COMP_RDP = 0x25,
	COMP_ARM = 0x30,
	COMP_ANM = 0x31,
	COMP_INK = 0x35,
	COMP_SCR = 0xff
};
typedef unsigned char DRAWORDER;
#define DRAWORDER_NONE 0x00
#define DRAWORDER_SOLID 0x01
#define DRAWORDER_TRANSPARENT 0x02
#define DRAWORDER_OVERLAY 0x04
#define DRAWORDER_LIGHT 0x08

#define ECACHESZ_PADDING 1
class Component: public Object {
public:
	Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder = 0x00, SceneObject* o = nullptr, std::vector<COMPONENT_TYPE> dep = {});
	virtual  ~Component() {}

	const COMPONENT_TYPE componentType = COMP_UNDEF;
	const DRAWORDER drawOrder;
	bool active;
	rSceneObject object;

	virtual void OnPreUpdate() {}
	virtual void OnPreLUpdate() {}
	virtual void OnPreRender() {}

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend class Scene;
	friend class Editor;
	friend class EB_Viewer;
	friend class EB_Inspector;
	friend class SceneObject;
	friend bool DrawComponentHeader(Editor* e, Vec4 v, uint pos, Component* c);
	friend void DrawSceneObjectsOpaque(EB_Viewer* ebv, const std::vector<SceneObject*> &oo), DrawSceneObjectsGizmos(EB_Viewer* ebv, const std::vector<SceneObject*> &oo), DrawSceneObjectsTrans(EB_Viewer* ebv, std::vector<SceneObject*> oo);
protected:
	std::vector<COMPONENT_TYPE> dependancies;
	std::vector<rComponent> dependacyPointers;

	//bool serializable;
	//std::vector<pair<void*, void*>> serializedValues;

	bool _expanded;

	static COMPONENT_TYPE Name2Type(string nm);

	virtual void LoadDefaultValues() {} //also loads assets
	virtual void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) {} //trs matrix not applied, apply before calling
	virtual void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) = 0;
	//virtual void DrawGameCamera() {}
	virtual void Serialize(Editor* e, std::ofstream* stream) = 0;
	virtual void Refresh() {}
};

class Transform: public Object {
public:
	rSceneObject object;

	const Vec3& position() { return _position; }
	const Vec3& localPosition() { return _localPosition; }
	const Quat& rotation() { return _rotation; }
	const Quat& localRotation() { return _localRotation; }
	const Vec3& localScale () { return _localScale; }
	const Vec3& eulerRotation() { return _eulerRotation; }
	const Vec3& localEulerRotation() { return _localEulerRotation;  }
	void position(const Vec3& r);
	void localPosition(const Vec3& r);
	void rotation(const Quat& r);
	void localRotation(const Quat& r);
	void localScale(const Vec3& r);
	void eulerRotation(const Vec3& r);
	void localEulerRotation(const Vec3& r);
	const Mat4x4 localMatrix() { return _localMatrix; }
	const Mat4x4 worldMatrix() { return _worldMatrix; }

	Vec3 forward(), right(), up(), Local2World(Vec3);

	Transform& Translate(float x, float y, float z, TransformSpace sp = Space_Self) { return Translate(Vec3(x, y, z), sp); }
	Transform& Rotate(float x, float y, float z, TransformSpace sp = Space_Self) { return Rotate(Vec3(x, y, z), sp); }
	Transform& Scale(float x, float y, float z) { return Scale(Vec3(x, y, z)); }
	Transform& Translate(Vec3 v, TransformSpace sp = Space_Self), &Rotate(Vec3 v, TransformSpace sp = Space_Self), &Scale(Vec3 v);
	Transform& RotateAround(Vec3 a, float f);

	friend class SceneObject;
	friend class EB_Viewer;
	friend class EB_Inspector;
	friend struct Editor_PlaySyncer;
	friend class Armature;
	_allowshared(Transform);
private:
	Transform() {}
	void Init(pSceneObject& sc, Vec3 pos, Quat rot, Vec3 scl);
	void Init(pSceneObject& sc, byte* data);

	Vec3 _position, _localPosition;
	Quat _rotation, _localRotation;
	Vec3 _eulerRotation, _localEulerRotation;
	Vec3 _localScale;
	Mat4x4 _localMatrix, _worldMatrix;

	static struct _offset_map {
		uint position = offsetof(Transform, _position),
			rotation = offsetof(Transform, _rotation),
			scale = offsetof(Transform, _localScale);
	} _offsets;

	void _W2LPos(), _L2WPos();
	void _UpdateWQuat(), _UpdateLQuat(), _L2WQuat(), _W2LQuat();
	void _UpdateWEuler(), _UpdateLEuler();
	void _UpdateLMatrix();
	void _UpdateWMatrix(const Mat4x4& mat);

	//never call Transform constructor directly.
	Transform(const Transform& rhs);
};

#pragma region AssetObjects

class Mesh : public AssetObject {
public:
	//Mesh(); //until i figure out normal recalc algorithm
	bool loaded;
	Mesh(const std::vector<Vec3>& verts, const std::vector<Vec3>& norms, const std::vector<int>& tris, std::vector<Vec2> uvs = std::vector<Vec2>());

	std::vector<Vec3> vertices;
	std::vector<Vec3> normals, tangents;// , bitangents;
	std::vector<int> triangles;
	std::vector<Vec2> uv0, uv1;
	std::vector<std::vector<std::pair<byte, float>>> vertexGroupWeights;
	std::vector<string> vertexGroups;
	BBox boundingBox;
	
	uint vertexCount, triangleCount, materialCount;
	
	void RecalculateBoundingBox();
	
	friend int main(int argc, char **argv);
	friend class Editor;
	friend class MeshFilter;
	friend class MeshRenderer;
	friend class SkinnedMeshRenderer;
	friend class AssetManager;
	_allowshared(Mesh);
protected:
	Mesh(Editor* e, int i);
	Mesh(std::istream& strm, uint offset = 0);
	Mesh(string path);
	Mesh(byte* mem);

	void CalcTangents();
	void GenECache() override;

	static bool ParseBlend(Editor* e, string s);
	std::vector<std::vector<int>> _matTriangles;
	//void Draw(Material* mat);
	//void Load();
};

enum Interpolation : byte {
	Interpolation_Ease,
	Interpolation_Linear,
	Interpolation_Before,
	Interpolation_Center,
	Interpolation_After
};

enum AnimKeyType : byte {
	AK_Undef = 0x00,
	AK_BoneLoc = 0x10,
	AK_BoneRot,
	AK_BoneScl,
	AK_Location = 0x20,
	AK_Rotation,
	AK_Scale,
	AK_ShapeKey = 0x30,
};

class AnimClip_Key {
public:
	AnimClip_Key(string name, AnimKeyType type, uint cnt = 0, float* loc = 0) : name(name), type(type),
		dim((type == AK_ShapeKey) ? 1 : ((type & 0x0f) == 0x01)? 4 : 3) {
		if (!!cnt) AddFrames(cnt, loc);
	}
	
	std::vector<std::pair<int, Vec4>> frames;
	uint frameCount;

	string name;
	const AnimKeyType type;
	const byte dim;

	void AddFrames(uint cnt, float* loc);
	Vec4 Eval(float t);
};

class AnimClip : public AssetObject {
public:
	std::vector<AnimClip_Key*> keys;
	ushort keyCount;
	ushort frameStart, frameEnd;
	bool repeat;

	std::vector<string> keynames;
	std::vector<Vec4> keyvals;

	Interpolation interpolation;

	void Eval(float t);

	friend class Editor;
	friend class AssetManager;
	_allowshared(AnimClip);
protected:
	AnimClip(Editor* e, int i);
	AnimClip(std::ifstream& strm, uint offset);
	AnimClip(string path);
};

class AnimFrameItem {
public:
	AnimFrameItem();
	friend class Animation;
	friend class Anim_State;
protected:
	byte type;
	string name;
	Vec3 transform, scale;
	Quat rotation;
};

class Anim_State {
public:
	Anim_State(bool blend = false) : Anim_State(Vec2()) {}
	
	string name;
	void SetClip(AnimClip* clip);

	friend class Animation;
	friend class Animator;
	friend class EB_AnimEditor;
protected:
	Anim_State(Vec2 pos, bool blend = false) : name(blend ? "newBlendState" : "newState"), isBlend(blend), speed(1), length(0), time(0), _clip(-1), editorPos(pos), editorExpand(true) {}
	bool isBlend;
	float speed, length, time;

	std::vector<string> keynames;
	std::vector<Vec4> keyvals;

	AnimClip* clip;
	ASSETID _clip;
	Vec2 editorPos;
	bool editorExpand, editorShowGraph;

	std::vector<AnimClip*> blendClips;
	std::vector<ASSETID> _blendClips;
	std::vector<float> blendOffsets;

	void Eval(float dt);
};

class Anim_Transition {
public:
	Anim_Transition();
	friend class Animation;
protected:
	bool canInterrupt;
	float length, time;

};

class Animation : public AssetObject {

	friend class EB_AnimEditor;
	friend class Editor;
	friend class SkinnedMeshRenderer;
	friend class Animator;
	_allowshared(Animation);
protected:
	Animation();
	Animation(string s);
	Animation(std::ifstream& stream, uint offset);
	uint activeState, nextState;
	float stateTransition;

	std::vector<Anim_State*> states;
	std::vector<Anim_Transition*> transitions;

	std::vector<string> keynames;
	std::vector<Vec4> keyvals;
	
	int IdOf(const string& s);

	Vec4 Get(const string& s);
	Vec4 Get(uint id);

	void Save(string s);
};

enum TEX_FILTERING : byte {
	TEX_FILTER_POINT = 0x00,
	TEX_FILTER_BILINEAR,
	TEX_FILTER_TRILINEAR
};

enum TEX_WARPING : byte {
	TEX_WARP_CLAMP,
	TEX_WARP_REPEAT
};

enum TEX_TYPE : byte {
	TEX_TYPE_NORMAL = 0x00,
	TEX_TYPE_RENDERTEXTURE,
	TEX_TYPE_READWRITE,
	TEX_TYPE_UNDEF
};

class Texture : public AssetObject {
public:
	Texture(const string& path, bool mipmap = true, TEX_FILTERING filter = TEX_FILTER_BILINEAR, byte aniso = 5, TEX_WARPING warp = TEX_WARP_REPEAT);
	//Texture(std::ifstream& stream, long pos);
	~Texture(){ glDeleteTextures(1, &pointer); }
	bool loaded;
	uint width, height;
	GLuint pointer;
	TEX_TYPE texType() { return _texType; }
	
	static byte* LoadPixels(const string& path, byte& chn, uint& w, uint& h);

	friend int main(int argc, char **argv);
	friend class Editor;
	friend class EB_Inspector;
	friend class AssetManager;
	friend class RenderTexture;
	friend void EBI_DrawAss_Tex(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
	_allowshared(Texture);
protected:
	Texture() : AssetObject(ASSETTYPE_TEXTURE) {}
	Texture(int i, Editor* e); //for caches
	Texture(std::istream& strm, uint offset = 0);
	Texture(byte* b);
	static TEX_TYPE _ReadStrm(Texture* tex, std::istream& strm, byte& chn, GLenum& rgb, GLenum& rgba);
	byte _aniso = 0;
	TEX_FILTERING _filter = TEX_FILTER_POINT;
	TEX_TYPE _texType = TEX_TYPE_NORMAL;
	bool _mipmap = true, _repeat = false, _blurmips = false;
	static bool Parse(Editor* e, string path);
	void _ApplyPrefs(const string& p);
	bool DrawPreview(uint x, uint y, uint w, uint h) override;
	
	void GenECache(byte* dat, byte chn, bool isrgb, std::vector<RenderTexture*>* rts);
};
/*
class VideoTexture : public Texture {
public:
	VideoTexture(const string& path);

	void Play(), Pause(), Stop();
//protected:
	GLuint d_fbo;
	std::vector<byte> buffer;

	std::istream* strm;
	AVCodec* codec;
	AVCodecContext* codecContext;
	AVCodecParserContext* parser;
	AVFrame* picture;
	int frame;

	static void Init();
	void GetFrame();
};
*/
enum RT_FLAGS : byte {
	RT_FLAG_NONE = 0U,
	RT_FLAG_DEPTH = 1U,
	RT_FLAG_STENCIL = 2U, //doesn't do anything for now
	RT_FLAG_HDR = 4U
};

class RenderTexture : public Texture {
public:
	RenderTexture(uint w, uint h, RT_FLAGS flags = RT_FLAG_NONE, const GLvoid* pixels = NULL, GLenum pixelFormat = GL_RGBA, GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR, GLenum wrapS = GL_REPEAT, GLenum wrapT = GL_REPEAT);
	~RenderTexture();

	const bool depth, stencil, hdr;

	static void Blit(Texture* src, RenderTexture* dst, Shader* shd, string texName = "mainTex");
	static void Blit(Texture* src, RenderTexture* dst, Material* mat, string texName = "mainTex");
	static void Blit(GLuint src, RenderTexture* dst, GLuint shd, string texName = "mainTex");

	template <typename T>
	std::vector<T> pixels(bool alpha) {
		assert((typeid(byte).hash_code() == typeid(T).hash_code()) || (typeid(float).hash_code() == typeid(T).hash_code()));
		std::vector<T> v = std::vector<T>(width*height * (alpha? 4 : 3));
		glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
		glReadPixels(0, 0, width, height, (alpha? GL_RGBA : GL_RGB), (typeid(byte).hash_code() == typeid(T).hash_code())? GL_UNSIGNED_BYTE : GL_FLOAT, &v[0]);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		return v;
	}

	//void Resize(uint w, uint h);
	friend class Texture;
	friend class Editor;
	friend class Background;
	friend int main(int argc, char **argv);
	_allowshared(RenderTexture);
protected:
	GLuint d_fbo;
	void Load(string path);
	void Load(std::istream& strm);
	static bool Parse(string path); //just tell Texture to load as rendtex
};

class Background : public AssetObject {
public:
	//Background() : AssetObject(ASSETTYPE_HDRI){}
	Background(const string& path);

	bool loaded;
	unsigned int width, height;
	GLuint pointer;
	friend int main(int argc, char **argv);
	friend class Editor;
	friend class AssetManager;
	_allowshared(Background);
private:
	Background(int i, Editor* editor);
	Background(std::istream& strm, uint offset);
	Background(byte*);
	static bool Parse(string path);

	void GenECache(const std::vector<Vec2>& szs, const std::vector<RenderTexture*>& rts);
};

class CubeMap : public AssetObject {
public:
	CubeMap(Texture* px, Texture* nx, Texture* py, Texture* ny, Texture* pz, Texture* nz);
	CubeMap(Texture* tex);

	const ushort size;
	bool loaded;
	
	friend class Camera;
	friend class RenderCubeMap;
	friend class ReflectionProbe;
	_allowshared(CubeMap);
protected:
	CubeMap(ushort size, bool mips = false, GLenum type = GL_RGBA, byte dataSize = 4, GLenum format = GL_RGBA, GLenum dataType = GL_UNSIGNED_BYTE);
	
	uint pointer;

	static void _RenderCube(Vec3 pos, Vec3 xdir, GLuint fbos[], uint size, GLuint shader = 0);
	static void _DoRenderCubeFace(GLuint fbo);
};

class RenderCubeMap {
protected:
	RenderCubeMap();

	CubeMap map;
	std::vector<GLuint> fbos[6]; //[face][mip]
};


class ReflectionProbe;

enum CAM_CLEARTYPE : byte {
	CAM_CLEAR_NONE,
	CAM_CLEAR_COLOR,
	CAM_CLEAR_DEPTH,
	CAM_CLEAR_SKY
};

enum GBUFFERS {
	GBUFFER_DIFFUSE,
	GBUFFER_NORMAL,
	GBUFFER_SPEC_GLOSS,
	GBUFFER_EMISSION_AO,
	GBUFFER_Z,
	GBUFFER_NUM_TEXTURES
};

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

	void Save(string nm);

	CameraEffect(string s);
	//CameraEffect(std::ifstream& strm, uint offset);
	//void SetShader(ShaderBase* shad);
};

#pragma endregion

#pragma region Components

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
	
	void Render(RenderTexture* target = nullptr);

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
	static GLuint d_probeMaskProgram, d_probeProgram, d_blurProgram, d_blurSBProgram, d_skyProgram, d_pLightProgram, d_sLightProgram, d_sLightCSProgram, d_sLightRSMProgram, d_sLightRSMFluxProgram;
	static GLuint d_reflQuadProgram;

	static Vec2 screenRectVerts[];
	static const int screenRectIndices[];

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
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	static void _SetClear0(EditorBlock* b), _SetClear1(EditorBlock* b), _SetClear2(EditorBlock* b), _SetClear3(EditorBlock* b);
};

class MeshFilter : public Component {
public:
	MeshFilter();
	
	rMesh mesh = 0;
	//void LoadDefaultValues() override;

	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	friend class MeshRenderer;
	friend class Editor;
	friend void LoadMeshMeta(std::vector<pSceneObject>& os, string& path);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(MeshFilter);
protected:
	MeshFilter(std::ifstream& stream, SceneObject* o, long pos = -1);

	bool showBoundingBox = false;
	ASSETID _mesh = -1;

	void SetMesh(int i);
	static void _UpdateMesh(void* i);
};

class MeshRenderer : public Component {
public:
	MeshRenderer();
	std::vector<rMaterial> materials;

	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;

	void Serialize(Editor* e, std::ofstream* stream) override;
	void Refresh() override;

	friend class Camera;
	friend class Editor;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(MeshRenderer);
protected:
	MeshRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);

	void DrawDeferred(GLuint shader = 0);

	std::vector<ASSETID> _materials;
	bool overwriteWriteMask = false;
	std::vector<bool> writeMask;
	static void _UpdateMat(void* i);
	static void _UpdateTex(void* i);
};

class TextureRenderer : public Component {
public:
	TextureRenderer(): _texture(-1), Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY) {}
	
	Texture* texture;

	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(TextureRenderer);
protected:
	TextureRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);
	int _texture;
};

#define SKINNED_MAX_VERTICES 65535
#define SKINNED_THREADS_PER_GROUP 32

class Armature;
class ArmatureBone;
class SkinnedMeshRenderer : public Component {
public:
	SkinnedMeshRenderer(SceneObject* o);
	std::vector<Material*> materials;
	Mesh* mesh() { return _mesh; }
	void mesh(Mesh*);
	//void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override {}
	//void Refresh() override;

	friend class Engine;
	friend class Editor;
	friend class Camera;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend void LoadMeshMeta(std::vector<pSceneObject>& os, string& path);
	_allowshared(SkinnedMeshRenderer);
protected:
	SkinnedMeshRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);

	std::vector<std::array<std::pair<ArmatureBone*, float>, 4>> weights;

	void InitWeights();
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0);
	void DrawDeferred(GLuint shader = 0);

	Mesh* _mesh = 0;
	ASSETID _meshId = -1;
	Armature* armature;
	std::vector<ASSETID> _materials;
	bool overwriteWriteMask = false;
	std::vector<bool> writeMask;
	bool showBoundingBox = false;

	struct SkinDats {
		SkinDats() {
			mats[0] = 0;
			mats[1] = 0;
			mats[2] = 0;
			mats[3] = 0;
		}

		uint mats[4];
		Vec4 weights;
	};
	ComputeBuffer<Vec4>* skinBufPoss = 0, *skinBufNrms;
	ComputeBuffer<Vec4>*skinBufPossO, *skinBufNrmsO;
	ComputeBuffer<SkinDats>* skinBufDats;
	ComputeBuffer<Mat4x4>* skinBufMats;
	static ComputeShader* skinningProg;
	uint skinDispatchGroups;

	static void InitSkinning();
	void Skin();

	void SetMesh(int i);
	static void _UpdateMesh(void* i);
	static void _UpdateMat(void* i);
	static void _UpdateTex(void* i);
};

enum ParticleEmissionType {
	ParticleEmission_Cone
};
struct Particle {
	Vec3 position, direction;
	float size, sizeFactor, life;
};
enum ParticleSystemValueType {
	ParticleSystemValue_Constant,
	ParticleSystemValue_Random,
	ParticleSystemValue_Curve,
	ParticleSystemValue_RandomCurve,
};
struct ParticleSystemValue {
	ParticleSystemValueType type;
	float val1, val2;

	float Eval();
};
class ParticleSystem : public Component {
public:
	int particleCount, maxParticles();
	void maxParticles(int);
	ParticleEmissionType emissionType;
	float emissionSize;
	ParticleSystemValue particleLifetime, startingSize, scalingFactor;
	bool worldSpace;

	void Emit(int count);

protected:
	std::vector<Particle> particles;
	int _maxParticles;
	void Update();
	const int _clock = 100;
	const float _dclock = 0.01f;
	double clockOffset;
};

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
	Texture* cookie;
	float cookieStrength = 1;
	bool square = false;
	LIGHT_FALLOFF falloff;
	Texture* hsvMap;

	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawShadowMap(GLuint tar = 0), BlitRSMFlux(), DrawRSM(Mat4x4& ip, Mat4x4& lp, float w, float h, GLuint gtexs[], GLuint gdepth);

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Camera;
	friend class Engine;
	friend class EB_Previewer;
	_allowshared(Light);
protected:
	LIGHTTYPE _lightType;
	Light(std::ifstream& stream, SceneObject* o, long pos = -1);
	//Mat4x4 _shadowMatrix;
	ASSETID _cookie = -1, _hsvMap = -1;
	static void _SetCookie(void* v), _SetHsvMap(void* v);

	void Serialize(Editor* e, std::ofstream* stream) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;

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

class ReflectiveQuad : public Component {
public:
	ReflectiveQuad(Texture* tex = nullptr);

	Texture* texture;
	float intensity;
	Vec2 size, origin;
	bool invertX, invertY, invertDirection;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Camera;
	friend class Light;
	friend class Engine;
	_allowshared(ReflectiveQuad);
protected:
	ASSETID _texture;
	static std::vector<GLint> paramLocs;
	//static Vec3 _poss[4], _uvs[4];
	//static uint _ids[6];
	static void _SetTex(void* v);

	static void ScanParams();
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;
};

enum ReflProbe_UpdateMode : byte {
	ReflProbe_UpdateMode_Start,
	ReflProbe_UpdateMode_Realtime,
	ReflProbe_UpdateMode_Manual
};
enum ReflProbe_ClearType : byte {
	ReflProbe_Clear_Sky,
	ReflProbe_Clear_Color
};
class ReflectionProbe : public Component {
public:
	ReflectionProbe(ushort size = 256);
	~ReflectionProbe() { delete(map); }
	ushort size;
	ReflProbe_UpdateMode updateMode;
	float intensity;
	ReflProbe_ClearType clearType;
	Vec4 clearColor;
	Vec3 range;
	float softness;

	void Update() { _pendingUpdate = true; }

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Camera;
	_allowshared(ReflectionProbe);
protected:
	ReflectionProbe(std::ifstream& stream, SceneObject* o, long pos = -1);
	bool _pendingUpdate;
	CubeMap* map;
	GLuint mipFbos[7];

	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	void _DoUpdate();
};

class InverseKinematics : public Component {
public:
	InverseKinematics();

	rSceneObject target;
	byte length = 1, iterations = 10;

	void Apply();

	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(InverseKinematics);
protected:
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override {}
};

class Animator : public Component {
public:
	Animator(Animation* anim = nullptr);
	~Animator();

	Animation* animation;

	int IdOf(const string& s) {
		return animation ? animation->IdOf(s) : -1;
	}

	Vec4 Get(const string& name) {
		return animation ? animation->Get(name) : Vec4();
	}
	Vec4 Get(uint id) {
		return animation ? animation->Get(id) : Vec4();
	}

	float fps = 24;

	void OnPreLUpdate() override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Engine;
	_allowshared(Animator);
protected:
	ASSETID _animation = -1;

	static void _SetAnim(void* v);
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override {}
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override {}
};

#define ARMATURE_MAX_BONES 256
class ArmatureBone {
public:

	Transform* const tr;
	Vec3 const restPosition;
	Quat const restRotation;
	Vec3 const restScale;
	Vec3 tailPos() { return tr->position() + tr->forward()*length*tr->localScale().z; }
	float const length;
	bool const connected;
	const Mat4x4 restMatrix, restMatrixInv;
	const Mat4x4 restMatrixAInv;
	Mat4x4 newMatrix, animMatrix;
	string const name, fullName; // parent2/parent/me/
	uint const id;
	const std::vector<ArmatureBone*>& children() { return _children; }
	const ArmatureBone* parent;

	friend class Armature;
protected:
	ArmatureBone(uint id, Vec3 pos, Quat rot, Vec3 scl, float lgh, bool conn, Transform* tr, ArmatureBone* par);
	
	static const Vec3 boneVecs[6];
	static const uint boneIndices[24];
	static const Vec3 boneCol, boneSelCol;
	std::vector<ArmatureBone*> _children;

	void Draw(EB_Viewer* ebv);
};
class Armature : public Component {
public:
	//Armature() : _anim(-1), Component("Armature", COMP_ARM, DRAWORDER_OVERLAY) {}
	~Armature();

	bool overridePos;
	Vec3 restPosition;
	Quat restRotation;
	Vec3 restScale;
	float animationScale = 1;
	const std::vector<ArmatureBone*>& bones() { return _bones; }

	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override;
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override {}

	virtual void OnPreRender() override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend void LoadMeshMeta(std::vector<SceneObject*>& os, string& path);
	friend class Editor;
	friend class EB_Viewer;
	friend class EB_Previewer;
	friend class SkinnedMeshRenderer;
	_allowshared(Armature);
protected:
	Armature(string s, SceneObject* o);
	Armature(std::ifstream& stream, SceneObject* o, long pos = -1);

	Animator* _anim;

	bool xray;
	std::vector<ArmatureBone*> _bones;
	std::vector<string> _allbonenames;
	std::vector<ArmatureBone*> _allbones;
	std::vector<int> _boneAnimIds;
	Mat4x4 _animMatrices[ARMATURE_MAX_BONES];
	ArmatureBone* MapBone(string nm);
	
	static void AddBone(std::ifstream&, std::vector<ArmatureBone*>&, std::vector<ArmatureBone*>&, SceneObject*, uint&);
	void GenMap(ArmatureBone* b = nullptr);
	void UpdateAnimIds();
	void Animate();
	void UpdateMats(ArmatureBone* b = nullptr);
};

enum SCR_VARTYPE : byte {
	SCR_VAR_UNDEF = 0U,
	SCR_VAR_INT, SCR_VAR_UINT,
	SCR_VAR_FLOAT,
	SCR_VAR_V2, SCR_VAR_V3,
	SCR_VAR_STRING = 16U,
	SCR_VAR_TEXTURE,
	SCR_VAR_COMPREF,
	SCR_VAR_COMMENT = 255U
};
class SceneScript : public Component {
public:
	~SceneScript();

	virtual void Start() {}
	virtual void Update() {}
	virtual void LateUpdate() {}
	virtual void Paint() {}

	static void Parse(string s, Editor* e);

	//bool ReferencingObject(Object* o) override;
	friend class Editor;
	friend class EB_Viewer;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(SceneScript);
protected:
	SceneScript(Editor* e, ASSETID id);
	SceneScript(std::ifstream& strm, SceneObject* o);
	SceneScript() : Component("", COMP_SCR, DRAWORDER_NONE) {}
	
	ASSETID _script;
	std::vector<std::pair<string, std::pair<SCR_VARTYPE, void*>>> _vals;

	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override; //we want c to be null if deleted
	void Serialize(Editor* e, std::ofstream* stream) override;
};

#pragma endregion

class SceneObject: public Object {
public:
	~SceneObject();
	static pSceneObject New(Vec3 pos, Quat rot = Quat(), Vec3 scale = Vec3(1, 1, 1)) {
		auto p = pSceneObject(new SceneObject(pos, rot, scale));
		p->transform.Init(p, pos, rot, scale);
		return p;
	}
	static pSceneObject New(string s = "New Object", Vec3 pos = Vec3(), Quat rot = Quat(), Vec3 scale = Vec3(1, 1, 1)) {
		auto p = pSceneObject(new SceneObject(s, pos, rot, scale));
		p->transform.Init(p, pos, rot, scale);
		return p;
	}

	bool active = true;
	Transform transform;

	void SetActive(bool active, bool enableAll = false);

	rSceneObject parent;
	uint childCount = 0;
	std::vector<pSceneObject> children;

	void SetParent(pSceneObject parent, bool retainLocal = false);
	/*! Add child to this SceneObject.
	 *  @param child The child SceneObject to add.
	 *  @param retainLocal If true, the child's local transform is retained. If false, the child's world transform is retained.
	 */
	pSceneObject AddChild(pSceneObject child, bool retainLocal = false);
	pSceneObject GetChild(int i) { return children[i]; }
	pComponent AddComponent(pComponent c);
	template <typename T, class ...Args> std::shared_ptr<T> AddComponent(Args&& ...args) {
		auto c = std::make_shared<T>(std::forward<Args>(args)...);
		AddComponent(std::static_pointer_cast<Component>(c));
		return c;
	}

	/*! you should probably use GetComponent<T>() instead.
	 */
	pComponent GetComponent(COMPONENT_TYPE type);
	template<class T> std::shared_ptr<T> GetComponent() {
		static_assert(std::is_base_of<Component, T>::value, "T is not a Component type!");
		for (pComponent cc : _components)
		{
			auto xx = std::dynamic_pointer_cast<T>(cc);
			if (xx != nullptr) {
				return xx;
			}
		}
		return nullptr;
	}
	void RemoveComponent(pComponent c);

	bool _expanded;
	int _componentCount;
	std::vector<pComponent> _components;

	friend class MeshFilter;
	friend class Scene;
	friend class Editor;
	friend struct Editor_PlaySyncer;
protected:
	//set transform.object after construction
	SceneObject(Vec3 pos, Quat rot = Quat(), Vec3 scale = Vec3(1, 1, 1));
	SceneObject(string s = "New Object", Vec3 pos = Vec3(), Quat rot = Quat(), Vec3 scale = Vec3(1, 1, 1));
	SceneObject(byte* data);
	static pSceneObject New(byte* data) {
		auto p = pSceneObject(new SceneObject(data));
		p->transform.Init(p, data + offsetof(SceneObject, transform));
		return p;
	}

	static pSceneObject _FromId(const std::vector<pSceneObject>& objs, ulong id);

	static struct _offset_map {
		uint name = offsetof(SceneObject, name),
			transform = offsetof(SceneObject, transform),
			components = offsetof(SceneObject, _components),
			children = offsetof(SceneObject, children);
	} _offsets;

	void Refresh();

	bool _pendingDelete;
};

#undef _shareable