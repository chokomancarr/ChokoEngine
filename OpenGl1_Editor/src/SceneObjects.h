#ifndef SCENE_H
#define SCENE_H

#include "Engine.h"

typedef unsigned char COMPONENT_TYPE;
typedef unsigned char DRAWORDER;
#define DRAWORDER_NONE 0x00
#define DRAWORDER_SOLID 0x01
#define DRAWORDER_TRANSPARENT 0x02
#define DRAWORDER_OVERLAY 0x04
#define DRAWORDER_LIGHT 0x08
class Component : public Object {
public:
	Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder = 0x00, SceneObject* o = nullptr, std::vector<COMPONENT_TYPE> dep = {});
	virtual  ~Component() {}

	const COMPONENT_TYPE componentType = 0;
	const DRAWORDER drawOrder;
	bool active;
	SceneObject* object;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend class Scene;
	friend class Editor;
	friend class EB_Viewer;
	friend class EB_Inspector;
	friend class SceneObject;
	friend void EBI_DrawObj(Vec4 v, Editor* editor, EB_Inspector* b, SceneObject* o);
	friend bool DrawComponentHeader(Editor* e, Vec4 v, uint pos, Component* c);
	friend void DrawSceneObjectsOpaque(EB_Viewer* ebv, const std::vector<SceneObject*> &oo), DrawSceneObjectsGizmos(EB_Viewer* ebv, const std::vector<SceneObject*> &oo), DrawSceneObjectsTrans(EB_Viewer* ebv, std::vector<SceneObject*> oo);

protected:
	std::vector<COMPONENT_TYPE> dependancies;
	std::vector<Component*> dependacyPointers;

	//bool serializable;
	//std::vector<pair<void*, void*>> serializedValues;

	bool _expanded;

	static COMPONENT_TYPE Name2Type(string nm);

	virtual void LoadDefaultValues() {} //also loads assets
	virtual void DrawEditor(EB_Viewer* ebv) {} //trs matrix not applied, apply before calling
	virtual void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) = 0;
	//virtual void DrawGameCamera() {}
	virtual void Serialize(Editor* e, std::ofstream* stream) = 0;
	virtual void Refresh() {}
};

class Transform : public Object {
public:

	SceneObject* object;
	Vec3 position;
	const Vec3 worldPosition(), forward(), right(), up();
	const Quat rotation(), worldRotation();
	const Vec3& eulerRotation() { return _eulerRotation;  }
	void eulerRotation(Vec3 r);
	Vec3 scale;

	Transform* Translate(float x, float y, float z) { return Translate(Vec3(x, y, z)); }
	Transform* Rotate(float x, float y, float z, TransformSpace sp = Space_Self) { return Rotate(Vec3(x, y, z), sp); }
	Transform* Scale(float x, float y, float z) { return Scale(Vec3(x, y, z)); }
	Transform* Translate(Vec3 v), *Rotate(Vec3 v, TransformSpace sp = Space_Self), *Scale(Vec3 v);
	Transform* RotateAround(Vec3 a, float f);

	friend class SceneObject;
	friend class EB_Viewer;
	friend void DrawSceneObjectsOpaque(EB_Viewer*, const std::vector<SceneObject*>&);
	friend void DrawSceneObjectsGizmos(EB_Viewer*, const std::vector<SceneObject*>&);
private:
	//never call Transform constructor directly. Call SceneObject().transform instead.
	Transform(SceneObject* sc, Vec3 pos, Quat rot, Vec3 scl);
	Quat _rotation;
	Vec3 _eulerRotation;
	Mat4x4 _localMatrix, _worldMatrix;

	void _UpdateQuat();
	void _UpdateEuler();
	void _UpdateLMatrix();
	void _UpdateWMatrix(const Mat4x4& mat);
};

class Mesh : public AssetObject {
public:
	//Mesh(); //until i figure out normal recalc algorithm
	bool loaded;

	std::vector<Vec3> vertices;
	std::vector<Vec3> normals, tangents, bitangents;
	std::vector<int> triangles;
	std::vector<Vec2> uv0, uv1;
	BBox boundingBox;
	
	uint vertexCount, triangleCount, materialCount;
	
	void RecalculateBoundingBox();
	
	friend int main(int argc, char **argv);
	friend class Editor;
	friend class MeshFilter;
	friend class MeshRenderer;
	friend class AssetManager;
protected:
	Mesh(Editor* e, int i);
	Mesh(std::ifstream& strm, uint offset);
	Mesh(string path);

	void CalcTangents();

	static bool ParseBlend(Editor* e, string s);
	std::vector<std::vector<int>> _matTriangles;
	//void Draw(Material* mat);
	//void Load();
};

class FCurve_Key {
public:
	FCurve_Key(Vec2 a, Vec2 b, Vec2 c) : point(a), left(b), right(c) {}
	Vec2 point, left, right;
};

enum FCurveType : byte {
	FC_Undef = 0x00,
	FC_BoneLoc_X = 0x10,
	FC_BoneLoc_Y, FC_BoneLoc_Z,
	FC_BoneQuat_W, FC_BoneQuat_X, FC_BoneQuat_Y, FC_BoneQuat_Z,
	FC_BoneScl_X, FC_BoneScl_Y, FC_BoneScl_Z,
	FC_ShapeKey = 0x20,
	FC_Location_X = 0xf0,
	FC_Location_Y, FC_Location_Z,
	FC_Rotation_X, FC_Rotation_Y, FC_Rotation_Z,
	FC_Scale_X = 0xf7,
	FC_Scale_Y, FC_Scale_Z,
};
class FCurve {
public:
	FCurve() : type(FC_Undef), name(""), keys(), keyCount(0), startTime(0), endTime(0) {}
	FCurveType type;
	string name;
	std::vector<FCurve_Key> keys;
	byte keyCount;
	float startTime, endTime;
	float Eval(float t, float repeat = false);
};

class AnimClip : public AssetObject {
	std::vector<FCurve> curves;
	uint curveLength;

	friend class Editor;
	friend class AssetManager;
protected:
	AnimClip(Editor* e, int i);
	AnimClip(std::ifstream& strm, uint offset);
	AnimClip(string path);
};

class AnimFrameItem {
public:
	AnimFrameItem();
	friend class Animator;
	friend class Anim_State;
protected:
	byte type;
	string name;
	Vec3 transform, scale;
	Quat rotation;
};

class Anim_State {
public:
	Anim_State(bool blend = false) : name(blend? "newBlendState" : "newState"), isBlend(blend), speed(1), length(0), time(0), _clip(-1), editorPos(Vec2()), editorExpand(true) {}
	friend class Animator;
	friend class EB_AnimEditor;
protected:
	Anim_State(Vec2 pos, bool blend = false) : name(blend ? "newBlendState" : "newState"), isBlend(blend), speed(1), length(0), time(0), _clip(-1), editorPos(pos), editorExpand(true) {}
	string name;
	bool isBlend;
	float speed, length, time;
	float Eval(); //increments time automatically

	AnimClip* clip;
	ASSETID _clip;
	Vec2 editorPos;
	bool editorExpand, editorShowGraph;

	std::vector<AnimClip*> blendClips;
	std::vector<ASSETID> _blendClips;
	std::vector<float> blendOffsets;
};

class Anim_Transition {
public:
	Anim_Transition();
	friend class Animator;
protected:
	bool canInterrupt;
	float length, time;

};

class AnimValue {
	string name;
	Vec3 pos, scl;
	Quat rot;
};

class Animator : public AssetObject {

	friend class EB_AnimEditor;
	friend class Editor;
	friend class SkinnedMeshRenderer;
protected:
	Animator();
	Animator(string s);
	Animator(std::ifstream& stream, uint offset);
	uint activeState, nextState;
	float stateTransition;

	std::vector<string> names; //Bones must have unique names
	std::vector<Anim_State*> states;
	std::vector<Anim_Transition*> transitions;

	int IdOfName(string n) {
		for (int a = names.size() - 1; a >= 0; a--)
			if (names[a] == n)
				return a;
		return -1;
	}
	void Get(uint id);

	void Save(string s);
};

enum TEX_FILTERING : byte {
	TEX_FILTER_POINT = 0x00,
	TEX_FILTER_BILINEAR,
	TEX_FILTER_TRILINEAR
};

enum TEX_TYPE : byte {
	TEX_TYPE_NORMAL = 0x00,
	TEX_TYPE_RENDERTEXTURE,
	TEX_TYPE_READWRITE
};

class Texture : public AssetObject {
public:
	Texture(const string& path);
	Texture(const string& path, bool mipmap);
	Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso);
	//Texture(std::ifstream& stream, long pos);
	~Texture(){ glDeleteTextures(1, &pointer); }
	bool loaded;
	uint width, height;
	GLuint pointer;
	TEX_TYPE texType() { return _texType; }
	friend int main(int argc, char **argv);
	friend class Editor;
	friend class EB_Inspector;
	friend class AssetManager;
	friend class RenderTexture;
	friend void EBI_DrawAss_Tex(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
protected:
	Texture() : AssetObject(ASSETTYPE_TEXTURE) {}
	Texture(int i, Editor* e); //for caches
	Texture(std::ifstream& strm, uint offset);
	static TEX_TYPE _ReadStrm(Texture* tex, std::ifstream& strm, byte& chn, GLenum& rgb, GLenum& rgba);
	byte _aniso = 0;
	TEX_FILTERING _filter = TEX_FILTER_POINT;
	TEX_TYPE _texType = TEX_TYPE_NORMAL;
	bool _mipmap = true, _repeat = false;
	static bool Parse(Editor* e, string path);
	void _ApplyPrefs(const string& p);
	bool DrawPreview(uint x, uint y, uint w, uint h) override;
};

class RenderTexture : public Texture {
public:
	RenderTexture(uint w, uint h, bool depth);
	~RenderTexture();

	static void Blit(Texture* src, RenderTexture* dst, ShaderBase* shd, string texName = "mainTex");
	static void Blit(Texture* src, RenderTexture* dst, Material* mat, string texName = "mainTex");

	std::vector<float> pixels();

	//void Resize(uint w, uint h);
	friend class Texture;
	friend class Editor;
	friend class Background;
protected:
	static void Blit(GLuint src, RenderTexture* dst, GLuint shd, string texName = "mainTex");
	GLuint d_fbo, d_depthTex;
	void Load(string path);
	void Load(std::ifstream& strm);
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
private:
	Background(int i, Editor* editor);
	Background(std::ifstream& strm, uint offset);
	static bool Parse(string path);
	static void B_DS(Background* b, std::vector<float> data);
	static std::vector<float> Downsample(std::vector<float>&, uint, uint, uint&, uint&);
};

class CubeMap : public AssetObject {
public:
	const ushort size;
	bool loaded;
	CubeMap(ushort size, bool mips = false, GLenum type = GL_RGBA, byte dataSize = 4, GLenum format = GL_RGBA, GLenum dataType = GL_UNSIGNED_BYTE);
	friend class ReflectionProbe;
protected:
	uint pointer;
	uint facePointers[6];
	std::vector<uint> facePointerMips[6]; //[face id] [mip level]
};

#define COMP_UNDEF 0x00

class ReflectionProbe;

#define COMP_CAM 0x01
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
	GBUFFER_EXTRA, //temporary one lights and stuff
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
	RenderTexture* targetRT;
	std::vector<CameraEffect*> effects;
	
	void Render(RenderTexture* target = nullptr);

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class EB_Viewer;
	friend class EB_Inspector;
	friend class EB_Previewer;
	friend class Background;
	friend class Engine;
	friend class Light;
protected:
	Camera(std::ifstream& stream, SceneObject* o, long pos = -1);

	std::vector<ASSETID> _effects;

	void RenderLights(GLuint targetFbo = 0);
	void DumpBuffers();
	void _RenderProbesMask(std::vector<SceneObject*>& objs, Mat4x4 mat, std::vector<ReflectionProbe*>& probes), _RenderProbes(std::vector<ReflectionProbe*>& probes, Mat4x4 mat);
	void _DoRenderProbeMask(ReflectionProbe* p, Mat4x4& ip), _DoRenderProbe(ReflectionProbe* p, Mat4x4& ip);
	static void _RenderSky(Mat4x4 ip, GLuint d_texs[], GLuint d_depthTex, float w = Display::width, float h = Display::height);
	void _DrawLights(std::vector<SceneObject*>& oo, Mat4x4& ip, GLuint targetFbo = 0);
	static void _DoDrawLight_Point(Light* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w = Display::width, float h = Display::height, GLuint targetFbo = 0);
	static void _DoDrawLight_Spot(Light* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w = Display::width, float h = Display::height, GLuint targetFbo = 0);
	static void _DoDrawLight_Spot_Contact(Light* l, Mat4x4& p, GLuint d_depthTex, float w, float h, GLuint src, GLuint tar);

	Vec3 camVerts[6];
	static int camVertsIds[19];
	GLuint d_fbo, d_texs[3], d_depthTex;
	static GLuint d_probeMaskProgram, d_probeProgram, d_skyProgram, d_pLightProgram, d_sLightProgram, d_sLightCSProgram;

	static Vec2 screenRectVerts[];
	static const int screenRectIndices[];

	int _tarRT;

	static std::unordered_map<string, GLuint> fetchTextures;
	static std::vector<string> fetchTexturesUpdated;
	static GLuint DoFetchTexture(string s);
	static void ClearFetchTextures();
	
	void ApplyGL();

	static void InitShaders();
	void UpdateCamVerts();
	void InitGBuffer();
	void DrawEditor(EB_Viewer* ebv) override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	static void _SetClear0(EditorBlock* b), _SetClear1(EditorBlock* b), _SetClear2(EditorBlock* b), _SetClear3(EditorBlock* b);
};

#define COMP_MFT 0x02
class MeshFilter : public Component {
public:
	MeshFilter();
	
	Mesh* mesh;
	//void LoadDefaultValues() override;

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	friend class MeshRenderer;
	friend class Editor;
	friend void LoadMeshMeta(std::vector<SceneObject*>& os, string& path);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
protected:
	bool showBoundingBox;

	MeshFilter(std::ifstream& stream, SceneObject* o, long pos = -1);

	void SetMesh(int i);
	static void _UpdateMesh(void* i);
	ASSETID _mesh;
};

#define COMP_MRD 0x10
class MeshRenderer : public Component {
public:
	MeshRenderer();
	std::vector<Material*> materials;

	void DrawEditor(EB_Viewer* ebv) override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;

	void Serialize(Editor* e, std::ofstream* stream) override;
	void Refresh() override;

	friend class Editor;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend void DrawSceneObjectsOpaque(std::vector<SceneObject*> oo);
protected:
	MeshRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);

	void DrawDeferred();

	std::vector<ASSETID> _materials;
	static void _UpdateMat(void* i);
	static void _UpdateTex(void* i);
};

#define COMP_TRD 0x11
class TextureRenderer : public Component {
public:
	TextureRenderer(): _texture(-1), Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY) {}
	
	Texture* texture;

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
protected:
	TextureRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);
	int _texture;
};

struct SRD_ArmatureData { //bind(-1) -> animate -> bind
	Mat4x4 bind[512];
	Mat4x4 animate[512];
};

#define COMP_SRD 0x12
class SkinnedMeshRenderer : public Component {
public:
	SkinnedMeshRenderer();

	std::vector<Material*> materials;
	Animator* anim;

	//void DrawEditor(EB_Viewer* ebv) override;
	//void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;

	//void Serialize(Editor* e, std::ofstream* stream) override;
	//void Refresh() override;

	friend class Editor;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend void DrawSceneObjectsOpaque(std::vector<SceneObject*> oo);
protected:
	SkinnedMeshRenderer(std::ifstream& stream, SceneObject* o, long pos = -1);

	SceneObject* baseBone;
	SRD_ArmatureData animData;
	std::vector<std::pair<uint, SceneObject*>> boneObjs;

	void ApplyAnim();
	void DrawDeferred();

	std::vector<ASSETID> _materials;
	static void _UpdateMat(void* i);
	static void _UpdateTex(void* i);
};

enum LIGHTTYPE : byte {
	LIGHTTYPE_POINT,
	LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_SPOT,
};
#define COMP_LHT 0x20
#define LIGHT_POINT_MINSTR 0.01f
class Light : public Component {
public:
	Light();
	LIGHTTYPE lightType() { return _lightType; }

	float intensity = 1;
	Vec4 color = white();
	float angle = 60;
	float minDist = 0.01f, maxDist = 5;
	bool drawShadow = true;
	float shadowBias = 0.01f, shadowStrength = 1;
	bool contactShadows = true;
	float contactShadowDistance = 0.1f;
	uint contactShadowSamples = 20;
	Texture* cookie;
	float cookieStrength = 1;
	bool square = false;

	void DrawEditor(EB_Viewer* ebv) override;
	void DrawShadowMap(GLuint tar = 0);

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Camera;
	friend class Engine;
	friend class EB_Previewer;
protected:
	LIGHTTYPE _lightType;
	Light(std::ifstream& stream, SceneObject* o, long pos = -1);
	//Mat4x4 _shadowMatrix;
	ASSETID _cookie;
	static void _SetCookie(void* v);

	void Serialize(Editor* e, std::ofstream* stream) override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;

	static void InitShadow();
	void CalcShadowMatrix();
	static GLuint _shadowFbo, _shadowMap;

	static std::vector<GLint> paramLocs_Spot, paramLocs_SpotCS; //make writing faster
	static void ScanParams();
	//static CubeMap* _shadowCube;
};

#define COMP_RDP 0x25
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
protected:
	ReflectionProbe(std::ifstream& stream, SceneObject* o, long pos = -1);
	bool _pendingUpdate;
	CubeMap* map;
	GLuint mipFbos[7];

	void DrawEditor(EB_Viewer* ebv) override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override;

	void _DoUpdate();
};

#define COMP_ARM 0x30
class ArmatureBone {
public:
	SceneObject* obj;
	//transform is local
	Vec3 position;
	Vec3 const restPosition;
	Quat rotation;
	Quat const restRotation;
	Vec3 scale;
	Vec3 const restScale;
	bool const connected;
	std::vector<ArmatureBone*> children() { return _children; }

	friend class Armature;
protected:
	ArmatureBone(Vec3 pos, Quat rot, Vec3 scl, bool conn) : position(pos), restPosition(pos), rotation(rot), restRotation(rot), scale(scl), restScale(scl), connected(conn) {}
	std::vector<ArmatureBone*> _children;
};
class Armature : public Component {
public:
	//Armature() : _anim(-1), Component("Armature", COMP_ARM, DRAWORDER_OVERLAY) {}

	Animator* anim;

	bool overridePos;
	Vec3 restPosition;
	Quat restRotation;
	Vec3 restScale;
	std::vector<ArmatureBone*> bones() { return _bones; }

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override {}
	void Serialize(Editor* e, std::ofstream* stream) override {}

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend void LoadMeshMeta(std::vector<SceneObject*>& os, string& path);
protected:
	Armature(string s, SceneObject* o);
	Armature(std::ifstream& stream, SceneObject* o, long pos = -1);
	static void AddBone(std::ifstream& stream, std::vector<ArmatureBone*>& bones);
	ASSETID _anim;
	std::vector<ArmatureBone*> _bones;
};

#define COMP_SCR 0xff
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
protected:
	SceneScript(Editor* e, ASSETID id);
	SceneScript(std::ifstream& strm, SceneObject* o);
	SceneScript() : Component("", COMP_SCR, DRAWORDER_NONE) {}
	
	ASSETID _script;
	std::vector<std::pair<string, std::pair<SCR_VARTYPE, void*>>> _vals;

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override; //we want c to be null if deleted
	void Serialize(Editor* e, std::ofstream* stream) override;
};

class SceneObject : public Object {
public:
	SceneObject();
	SceneObject(string s);
	SceneObject(Vec3 pos, Quat rot, Vec3 scale);
	SceneObject(string s, Vec3 pos, Quat rot, Vec3 scale);
	~SceneObject();
	bool active;
	uint childCount;
	Transform transform;

	void SetActive(bool active, bool enableAll = false);

	SceneObject* parent;
	std::vector<SceneObject*> children;

	SceneObject* AddChild(SceneObject* child);
	SceneObject* GetChild(int i) { return children[i]; }
	void RemoveChild(SceneObject* o);
	Component* AddComponent(Component* c);
	
	Component* GetComponent(COMPONENT_TYPE type);
	template<class T> T* GetComponent() {
		//static_assert(is_base_of(Component, T), "GetComponent requires a component type!");
		(void)static_cast<Component*>((T*)0);
		for (Component* cc : _components)
		{
			T* xx = dynamic_cast<T*>(cc);
			if (xx != nullptr) {
			//if (typeid(&cc) == typeid(T)) {
				return xx;
			}
		}
		return nullptr;
	}
	void RemoveComponent(Component*& c);

	bool _expanded;
	int _componentCount;
	std::vector<Component*> _components;

	friend class MeshFilter;
	friend class Scene;
protected:
	void Refresh();
};

#endif