#pragma once
#include "Engine.h"

#pragma region enums

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

enum RT_FLAGS : byte {
	RT_FLAG_NONE = 0U,
	RT_FLAG_DEPTH = 1U,
	RT_FLAG_STENCIL = 2U, //doesn't do anything for now
	RT_FLAG_HDR = 4U
};

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

#pragma endregion

class AssetManager {
	friend int main(int argc, char **argv);
	friend class Engine;
	friend class Editor;
	friend class Scene;
	friend class SceneObject;
	friend class Material;
	friend struct Editor_PlaySyncer;
	template<typename T> friend T* _GetCache(ASSETTYPE t, ASSETID i, bool async);
	friend string _Strm2Asset(std::istream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int max);
protected:
	static std::unordered_map<ASSETTYPE, std::vector<string>> names;
#ifndef IS_EDITOR
	static string eBasePath;
	static std::unordered_map<ASSETTYPE, std::vector<string>> dataELocs;
	static std::unordered_map<ASSETTYPE, std::vector<std::pair<byte*, uint>>> dataECaches;
	static std::vector<uint> dataECacheLocs;
	static std::vector<uint> dataECacheSzLocs;
	static void AllocECache();
#endif
	static std::vector <std::pair<ASSETTYPE, ASSETID>> dataECacheIds;
	static std::unordered_map<ASSETTYPE, std::vector<std::pair<byte, std::pair<uint, uint>>>> dataLocs;
	static std::unordered_map<ASSETTYPE, std::vector<AssetObject*>> dataCaches;
	static std::vector<std::ifstream*> streams;
	static void Init(string dpath);
	static AssetObject* CacheFromName(string nm);
	static ASSETID GetAssetId(string nm, ASSETTYPE &t);
	static AssetObject* GetCache(ASSETTYPE t, ASSETID i);
	static AssetObject* GenCache(ASSETTYPE t, ASSETID i);
};

class DefaultResources { //loads binary created by DefaultResourcesBuild.exe
public:
	static void Init(string path);

	static string GetStr(string name);
	static std::vector<byte> GetBin(string name);
private:
	static std::vector<string> names;
	static std::vector<char*> datas;
	static std::vector<uint> sizes;
};

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


class ShaderValue {
public:
	ShaderValue() : i(0), x(0), y(0), z(0), m() {}
	int i;
	float x, y, z;
	glm::quat m;
};

class ShaderTags {
public:
	ShaderTags() : zTest(SHADER_TEST_LEQUAL), aTest(SHADER_TEST_ALWAYS), culling(SHADER_CULL_BACK), zWrite() {}
	SHADER_TEST zTest, aTest;
	SHADER_CULL culling;
	bool zWrite;

	void ApplyGL();

	static void Reset();
};

class ShaderVariable {
public:
	//ShaderVariable() : type(0), name(""), val(), min(0), max(0) {}

	SHADER_VARTYPE type;
	string name;
	ShaderValue val, def;

	float min, max;
};

class Shader : public AssetObject {
public:
	Shader(string path);
	Shader(std::istream& stream, uint offset);
	Shader(const string& vert, const string& frag);
	Shader(GLuint p) : AssetObject(ASSETTYPE_SHADER), pointer(p), loaded(!!p), inherited(true) {}
	//ShaderBase(string vert, string frag);
	~Shader() {
		if (!inherited) glDeleteProgram(pointer);
		for (ShaderVariable* v : vars)
			delete(v);
	}

	bool loaded;
	GLuint pointer;
	ShaderTags tags;
	std::vector<ShaderVariable*> vars;

	//void Set(byte type, GLint id, void* val) { values[type][id] = val; }
	//void* Get(byte type, GLint id) { return values[type][id]; }

	//removes macros, insert include files
	static bool Parse(std::ifstream* text, string path);
	static bool LoadShader(GLenum shaderType, string source, GLuint& shader, string* err = nullptr);

	friend class Camera;
	friend class Engine;

protected:
	static GLuint FromVF(const string& vert, const string& frag);
	bool inherited = false;
};

class MatVal_Tex {
	friend class Material;
	friend class MeshRenderer;
	friend class SkinnedMeshRenderer;
	friend void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
protected:
	MatVal_Tex() : id(-1), tex(nullptr), defTex(0) {}
	~MatVal_Tex() {}

	int id;
	Texture* tex;
	GLuint defTex;
};

/*! Material used by renderers.
[av] */
class Material : public AssetObject {
public:
	Material(void);
	Material(Shader* shad);
	~Material();

	Shader* shader();
	void shader(Shader* shad);
	//values applied to program on drawing stage
	std::unordered_map<SHADER_VARTYPE, std::unordered_map <GLint, void*>> vals;
	std::unordered_map<SHADER_VARTYPE, std::vector<string>> valNames;
	//std::unordered_map<GLint, ShaderVariable> vals;
	void SetBuffer(string name, IComputeBuffer* buffer);
	void SetBuffer(GLint id, IComputeBuffer* buffer);
	void SetTexture(string name, Texture* texture);
	void SetTexture(GLint id, Texture* texture);
	void SetFloat(string name, float val);
	void SetFloat(GLint id, float val);
	void SetInt(string name, int val);
	void SetInt(GLint id, int val);
	void SetVec2(string name, Vec2 val);
	void SetVec2(GLint id, Vec2 val);

	friend class Engine;
	friend class Editor;
	friend class EB_Browser;
	friend class EB_Inspector;
	friend class Mesh;
	friend class MeshRenderer;
	friend class SkinnedMeshRenderer;
	friend class Scene;
	friend class AssetManager;
	friend class Shader;
	friend class RenderTexture;
	friend int main(int argc, char **argv);
	friend void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
	_allowshared(Material);
protected:
	Material(string s);
	Material(std::istream& stream, uint offset = 0);
	void _ReloadParams();

	Shader* _shader;
	int _shaderId;
	std::vector<SHADER_VARTYPE> valOrders;
	std::vector<byte> valOrderIds;
	std::vector<byte> valOrderGLIds;
	std::vector<bool> writeMask;

	static void LoadOris();
	static std::array<GLuint, 7> defTexs;

	static void _UpdateTexCache(void*);

	void Save(string path);
	void ApplyGL(Mat4x4& _mv, Mat4x4& _p);
	void ResetVals();

	bool _maskExpanded;
};

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
	friend class Engine;
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

class AnimClip_Key {
public:
	AnimClip_Key(string name, AnimKeyType type, uint cnt = 0, float* loc = 0) : name(name), type(type),
		dim((type == AK_ShapeKey) ? 1 : ((type & 0x0f) == 0x01) ? 4 : 3) {
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

class Texture : public AssetObject {
public:
	Texture(const string& path, bool mipmap = true, TEX_FILTERING filter = TEX_FILTER_BILINEAR, byte aniso = 5, TEX_WARPING warp = TEX_WARP_REPEAT);
	//Texture(std::ifstream& stream, long pos);
	~Texture() { glDeleteTextures(1, &pointer); }
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
		std::vector<T> v = std::vector<T>(width*height * (alpha ? 4 : 3));
		glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
		glReadPixels(0, 0, width, height, (alpha ? GL_RGBA : GL_RGB), (typeid(byte).hash_code() == typeid(T).hash_code()) ? GL_UNSIGNED_BYTE : GL_FLOAT, &v[0]);
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