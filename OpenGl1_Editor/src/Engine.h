#pragma once
#define _ITERATOR_DEBUG_LEVEL 0

#include "Defines.h"

#include <Windows.h>

#include <gl/glew.h>
#include <GLFW\glfw3.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <array>
#include <memory>
#include <jpeglib.h>
#include <jerror.h>
#include <lodepng.h>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
extern "C" {
//#include "libavcodec\avcodec.h"
//#include "libavutil\avutil.h"
}

#ifndef IS_EDITOR
extern bool _pipemode;
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define STRINGMRGDO(a,b) a ## b
#define STRINGMRG(a,b) STRINGMRGDO(a,b)

#pragma region asm_related_functions

#ifdef __MSVC_RUNTIME_CHECKS
#error don't do runtime checks, it will ruin the stack-tracer (for now)
#else
#define __asmloc_offset 0
#endif

#define __asmloc_label STRINGMRG(asmlocregion, __COUNTER__)

#define __asmloc(nm) __asmloc_do(nm, __asmloc_label)

#define __asmloc_do(nm,lbl) int nm; \
{__asm lbl: __asm mov eax, lbl __asm mov nm, eax}


//get the location after calling instruction : https://en.wikibooks.org/wiki/X86_Disassembly/Functions_and_Stack_Frames
#define __trace(nm) int nm; \
__asm mov ebx, [ebp + 4] __asm mov nm, ebx

//traces up cnt times. cnt must be more than 1.
#define __tracen(nm,cnt) int nm; {\
unsigned short num = cnt-1; \
assert(num); \
__asm mov ebx, ebp \
__asm mov cx, num \
__asm tracenregion: \
__asm mov ebx, [ebx] \
__asm dec cx \
__asm jnz tracenregion \
__asm mov ebx, [ebx + 4] \
__asm mov nm, ebx }

#pragma endregion

const float PI = 3.1415926535f;
const float rad2deg = 57.2958f;
const float deg2rad = 0.0174533f;
const char char0 = 0;

#define Lerp(a, b, c) ((a)*(1-(c)) + (b)*(c))
#define InverseLerp(a,b,c) (clamp( (c - a)/(b - a) , 0, 1))
#define Normalize(a) glm::normalize(a)
#define Distance(a, b) glm::distance(a, b)
template <typename T> T Repeat(T t, T a, T b) {
	while (t > b)
		t -= (b - a);
	while (t < a)
		t += (b - a);
	return t;
}

template <typename T> T min(const T& a, const T& b) {
	if (a > b) return b;
	return a;
}
template <typename T> T max(const T& a, const T& b) {
	if (a > b) return a;
	return b;
}

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef std::string string;
typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;
typedef glm::quat Quat;
typedef glm::mat4 Mat4x4;

string to_string(float f);
string to_string(double f);
string to_string(ulong f);
string to_string(long f);
string to_string(uint f);
string to_string(int f);
string to_string(Vec2 v), to_string(Vec3 v), to_string(Vec4 v), to_string(Quat v);
std::vector<string> string_split(string s, char c);

int TryParse(string str, int defVal);
uint TryParse(string str, uint defVal);
float TryParse(string str, float defVal);

class Mesh;
class MatFunc {
public:
	static Mat4x4 FromTRS(const Vec3& t, const Quat& r, const Vec3& s);
};
class QuatFunc {
public:
	static Quat Inverse(const Quat&);
	//static Vec3 Rotate(const Vec3&, const Quat&); //just use Q*V
	static Vec3 ToEuler(const Quat&);
	static Mat4x4 ToMatrix(const Quat&);
	static Quat FromAxisAngle(Vec3, float);
	static Quat LookAt(const Vec3&, const Vec3&);
};

struct BBox {
	BBox() {}
	BBox(float, float, float, float, float, float);

	float x0, x1, y0, y1, z0, z1;
};

class Color {
public:
	Color() : r(0), g(0), b(0), a(0), useA(true) {}
	Color(Vec4 v, bool hasA = true) : r((byte)round(v.r * 255)), g((byte)round(v.g * 255)), b((byte)round(v.b * 255)), a((byte)round(v.a * 255)), useA(hasA) {}

	bool useA;
	byte r, g, b, a;
	float h, s, v;

	Vec3 hsv() { return Vec3(h, s, v); }

	Vec4 vec4() {
		return Vec4(r, g, b, a)*(1.0f / 255);
	}

	static GLuint pickerProgH, pickerProgSV;

	string hex();

	static void Rgb2Hsv(byte r, byte g, byte b, float& h, float& s, float& v), Hsv2Rgb(float h, float s, float v, byte& r, byte& g, byte& b);
	static Vec3 Rgb2Hsv(Vec4 col);
	static string Col2Hex(Vec4 col);
	static void DrawPicker(float x, float y, Color& c);
	static Vec4 HueBaseCol(float hue);

protected:

	void RecalcRGB(), RecalcHSV();
	static void DrawSV(float x, float y, float w, float h, float hue);
	static void DrawH(float x, float y, float w, float h);
};

class Procedurals {
public:
	static Mesh* Plane(uint xCount, uint yCount);
	static Mesh* UVSphere(uint uCount, uint vCount);
};

class Rect {
public:
	Rect() : x(0), y(0), w(1), h(1) {}
	Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
	Rect(Vec4 v) : x(v.r), y(v.g), w(v.b), h(v.a) {}
	float x, y, w, h;

	/// Check if v is inside this rect.
	bool Inside(const Vec2& v);
	/// Returns a new Rect covered by both this rect and r2
	Rect Intersection(const Rect& r2);
};

class Int2 {
public:
	Int2() : x(0), y(0) {}
	Int2(int x, int y) : x(x), y(y) {}

	int x, y;

	bool operator==(const Int2& rhs)
	{
		return (x == rhs.x) && (y == rhs.y);
	}
};

class Random {
public:
	static int seed;
	static float Value(), Range(float min, float max);
};

enum MOUSE_STATUS : byte {
	MOUSE_NONE = 0x00,
	MOUSE_DOWN,
	MOUSE_HOLD,
	MOUSE_UP,
	MOUSE_HOVER_FLAG = 0x10,
	MOUSE_CLICK, //use for buttons
	MOUSE_PRESS, //use for buttons
	MOUSE_RELEASE  //use for buttons
};

enum ALIGNMENT : byte {
	ALIGN_BOTLEFT = 0x00,
	ALIGN_BOTCENTER = 0x01,
	ALIGN_BOTRIGHT = 0x02,
	ALIGN_MIDLEFT = 0x10,
	ALIGN_MIDCENTER = 0x11,
	ALIGN_MIDRIGHT = 0x12,
	ALIGN_TOPLEFT = 0x20,
	ALIGN_TOPCENTER = 0x21,
	ALIGN_TOPRIGHT = 0x22
};

enum ORIENTATION : byte {
	ORIENT_NONE,
	ORIENT_HORIZONTAL,
	ORIENT_VERTICAL
};

enum TransformSpace : byte {
	Space_Self = 0x00,
	Space_World
};

long long milliseconds();

class Editor;
class EditorBlock;
class EB_Inspector;

//typedef unsigned char ASSETTYPE;
enum ASSETTYPE : byte {
	ASSETTYPE_UNDEF = 0x00,

	ASSETTYPE_SCENE = 0x90,
	ASSETTYPE_TEXTURE = 0x01,
	ASSETTYPE_HDRI = 0x02,
	ASSETTYPE_TEXCUBE = 0x03,
	ASSETTYPE_SHADER = 0x05,
	ASSETTYPE_MATERIAL = 0x10,
	ASSETTYPE_BLEND = 0x20,
	ASSETTYPE_MESH = 0x21,
	ASSETTYPE_ANIMCLIP = 0x30,
	ASSETTYPE_ANIMATION = 0x31,
	ASSETTYPE_CAMEFFECT = 0x40,
	ASSETTYPE_SCRIPT_H = 0x9e,
	ASSETTYPE_SCRIPT_CPP = 0x9f,
	//derived types
	ASSETTYPE_TEXTURE_REND = 0xa0
};
typedef int ASSETID;

//shorthands
Vec4 black(float f = 1);
Vec4 red(float f = 1, float i = 1), green(float f = 1, float i = 1), blue(float f = 1, float i = 1), cyan(float f = 1, float i = 1), yellow(float f = 1, float i = 1), white(float f = 1, float i = 1);
Vec4 LerpVec4(Vec4 a, Vec4 b, float f);
float clamp(float f, float a, float b);
#define Clamp(f,a,b) min(b, max(f, a))
float repeat(float f, float a, float b);
//Vec3 rotate(Vec3 v, Quat q);
void _StreamWrite(const void* val, std::ofstream* stream, int size);
void _StreamWriteAsset(Editor* e, std::ofstream* stream, ASSETTYPE t, ASSETID i);
//void _Strm2Int(std::ifstream& strm, int& i), _Strm2Float(std::ifstream& strm, float& f), _Strm2Short(std::ifstream& strm, short& i);

template<typename T> void _Strm2Val(std::istream& strm, T &val) {
	long long pos = strm.tellg();
	strm.read((char*)&val, (byte)sizeof(T));
	if (strm.fail()) {
		Debug::Error("Strm2Val", "Fail bit raised! (probably eof reached) " + std::to_string(pos));
	}
}
ASSETID _Strm2H(std::istream& strm);

string _Strm2Asset(std::istream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int maxL = 100);

template<typename T> T* _GetCache(ASSETTYPE t, ASSETID i) {
#ifdef IS_EDITOR
	return static_cast<T*>(Editor::instance->GetCache(t, i));
#else
	return static_cast<T*>(AssetManager::GetCache(t, i));
#endif
}

class Object;
template <class T>
class Ref {
public:
	Ref(bool suppress = false) : _suppress(suppress) {
		static_assert(std::is_base_of<Object, T>::value, "Ref class must be derived from Object!");
	}

	std::shared_ptr<T> operator()() { //get
		if (_empty) {
			if (!_suppress) Debug::Error("Object Reference", "Reference is null!");
			return nullptr;
		}
		else if (_object.expired()) {
			if (!_suppress) Debug::Error("Object Reference", "Reference is deleted!");
			return nullptr;
		}
		else return _object.lock();
	}
	std::shared_ptr<T> operator->() { return this->operator()(); }

	void operator()(const std::shared_ptr<T>& ref) { //set
		_object = ref;
		_empty = false;
	}
	void operator()(const Ref<T>& ref) {
		_object = ref._object;
		_empty = false;
	}
	void operator()(const T* ref) {
		_object = ref->shared_from_this();
		_empty = false;
	}

	bool operator ==(const Ref<T>& rhs) const {
		return this->_object.lock() == rhs._object.lock();
	}

	bool operator !=(const Ref<T>& rhs) const {
		return this->_object.lock() != rhs._object.lock();
	}

	void clear() {
		_empty = true;
	}

	bool ok() {
		return !(_empty || _object.expired());
	}

	T* raw() {
		if (ok()) return _object.lock().get();
		else return nullptr;
	}

private:
	std::weak_ptr<T> _object;
	bool _empty = true, _suppress = false;
};

#define _canref(obj) class obj; \
typedef std::shared_ptr<obj> p ## obj; \
typedef Ref<obj> r ## obj;

//SceneObjects.h
_canref(Component);
_canref(Transform);
_canref(SceneObject);
_canref(Animator);
_canref(Armature);
_canref(Camera);
_canref(Light);
_canref(MeshFilter);
_canref(MeshRenderer);
_canref(ParticleSystem);
_canref(ReflectionProbe);
_canref(ReflectiveQuad);
_canref(SceneScript);
_canref(SkinnedMeshRenderer);
_canref(TextureRenderer);

//AssetObjects.h
class AssetItem;
class AssetManager;
class AssetObject;

_canref(Animation);
_canref(AnimClip);
_canref(Background);
_canref(CameraEffect);
_canref(CubeMap);
_canref(Material);
_canref(Mesh);
_canref(Shader);
_canref(Texture);

class Debug {
public:
	static void Message(string c, string s);
	static void Warning(string c, string s);
	static void Error(string c, string s);
	static void ObjectTree(const std::vector<pSceneObject>& o);

	friend int main(int argc, char **argv);
protected:
	static std::ofstream* stream;
	static void Init(string path);

private:
	static void DoDebugObjectTree(const std::vector<pSceneObject>& o, int i);
};

class EB_Browser_File;

class IO {
public:
	static std::vector<string> GetFiles(const string& path, string ext = "");
	static std::vector<EB_Browser_File> GetFilesE(Editor* e, const string& path);
	static void GetFolders(const string& path, std::vector<string>* names, bool hidden = false);
	static bool HasDirectory(string szPath);
	static bool HasFile(string szPath);
	static string ReadFile(const string& path);
	static std::vector<string> GetRegistryKeys(HKEY key);
	static string GetText(const string& path);
	static std::vector<byte> GetBytes(const string& path);
};

class WinFunc {
public:
	static HWND GetHwndFromProcessID(DWORD id);
};

class Font {
public:
	Font(const string& path, int size = 12);
	bool loaded = false;
	//bool useSubpixel; //glyphs are rgba if true, else r (does it look good in games?)
	GLuint glyph(uint size) { if (_glyphs.count(size) == 1) return _glyphs[size]; else return CreateGlyph(size); }

	ALIGNMENT alignment;

	Font* Align(ALIGNMENT a);

	friend class Engine;
	friend class UI;
protected:
	static FT_Library _ftlib;
	static void Init();
	FT_Face _face;
	static GLuint fontProgram;

	float w2h[256];
	float w2s[256];
	Vec2 off[256];

	uint vecSize;
	std::vector<Vec3> poss;
	std::vector<Vec2> uvs;
	std::vector<uint> ids;
	std::vector<float> cs;
	void SizeVec(uint sz);

	std::unordered_map<uint, GLuint> _glyphs; //each glyph size is fontSize*16
	GLuint CreateGlyph (uint size, bool recalcW2h = false);
};

enum InputKey : byte {
	Key_None = 0x00,
	Key_Backspace = 0x08,
	Key_Tab = 0x09,
	Key_Enter = 0x0D,
	Key_Shift = 0x10,
	Key_Control = 0x11,
	Key_Alt = 0x12,
	Key_CapsLock = 0x14,
	Key_Escape = 0x1B,
	Key_Space = 0x20,
	Key_LeftArrow = 0x25, Key_UpArrow, Key_RightArrow, Key_DownArrow,
	Key_Delete = 0x2E,
	Key_0 = 0x30, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9,
	Key_A = 0x41, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J, Key_K, Key_L, Key_M, Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T, Key_U, Key_V, Key_W, Key_X, Key_Y, Key_Z,
	Key_NumPad0 = 0x60, Key_NumPad1, Key_NumPad2, Key_NumPad3, Key_NumPad4, Key_NumPad5, Key_NumPad6, Key_NumPad7, Key_NumPad8, Key_NumPad9,
	Key_NumPadMultiply, Key_NumPadAdd, Key_NumPadSeparator, Key_NumPadMinus, Key_NumPadDot, Key_NumPadDivide,
	Key_Plus = 0xBB, Key_Comma, Key_Minus, Key_Dot
};

class Input {
public:
	static Vec2 mousePos, mousePosRelative, mouseDelta, mouseDownPos;
	static bool mouse0, mouse1, mouse2;
	static byte mouse0State, mouse1State, mouse2State;
	static string inputString;
	static void UpdateMouseNKeyboard(bool* src = nullptr);

	static bool KeyDown(InputKey key), KeyHold(InputKey key), KeyUp(InputKey key);

	Vec2 _mousePos, _mousePosRelative, _mouseDelta, _mouseDownPos;
	bool _mouse0, _mouse1, _mouse2;
	bool _keyStatuses[256];
	friend struct Editor_PlaySyncer;
protected:
	static bool keyStatusOld[256], keyStatusNew[256];
private:
	static Vec2 mousePosOld;
	//Input(Input const &); //deliberately not defined
	//Input& operator= (Input const &);
};

class Display {
public:
	static int width, height;
	const static uint dpi = 96;
	static glm::mat3 uiMatrix;

	static void Resize(int x, int y, bool maximize = false);

	friend int main(int argc, char **argv);
protected:
	static GLFWwindow* window;
};

enum OBJECT_STATUS {
	OBJECT_UNDEF,
	OBJECT_ALIVE,
	OBJECT_DEAD
};

class Object {
public:
	Object(string nm = "");
	Object(ulong id, string nm);
	ulong id;
	string name;
	bool dirty = false; //triggers a reload of internal variables
	bool dead = false; //will be cleaned up after this frame

	virtual bool ReferencingObject(Object* o) { return false; }
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

enum SHADER_VARTYPE : byte {
	SHADER_INT = 0x00,
	SHADER_FLOAT = 0x01,
	SHADER_VEC2 = 0x02,
	SHADER_VEC3 = 0x03,
	SHADER_SAMPLER = 0x10,
	SHADER_MATRIX = 0x20
};

class ShaderValue {
public:
	ShaderValue() : i(0), x(0), y(0), z(0), m() {}
	int i;
	float x, y, z;
	glm::quat m;
};

enum SHADER_TEST {
	SHADER_TEST_NEVER,
	SHADER_TEST_ALWAYS,
	SHADER_TEST_LESS,
	SHADER_TEST_LEQUAL,
	SHADER_TEST_EQUAL,
	SHADER_TEST_GEQUAL,
	SHADER_TEST_GREATER,
	SHADER_TEST_NOTEQUAL
};

enum SHADER_CULL {
	SHADER_CULL_BACK,
	SHADER_CULL_FRONT,
	SHADER_CULL_NONE
};

class ShaderTags {
public:
	ShaderTags(): zTest(SHADER_TEST_LEQUAL), aTest(SHADER_TEST_ALWAYS), culling(SHADER_CULL_BACK), zWrite() {}
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

class IComputeBuffer {
public:
	IComputeBuffer(uint size, void* data, uint padding = 0, uint stride = 1);
	~IComputeBuffer();

	GLuint pointer;
	uint size;


	void Set(void* data, uint padding = 0, uint stride = 1);
	
	/// Returns a copy of the compute buffer data.
	/// The lhs pointer will be overwritten if target is null. Use Get(T*) to prevent memory leaks.
	template <typename T> T* Get(T* target = nullptr) {
		byte* tar;
		if (!target) tar = new byte[size];
		else tar = (byte*)target;
		GLint bufmask = GL_MAP_READ_BIT;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointer);
		void* src = (void*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * 4, bufmask);
		if (!src) {
			Debug::Warning("ComputeBuffer", "Set: Unable to map buffer!");
		}
		memcpy(tar, src, size);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return (T*)tar;
	}
};

template<typename T> class ComputeBuffer : public IComputeBuffer {
public:
	ComputeBuffer(uint num, T* data = nullptr, uint padding = 0) : IComputeBuffer(num*sizeof(T), data, padding) {}
};

class ComputeShader {
public:
	ComputeShader(string str);
	~ComputeShader();

	static ComputeShader* FromPath(string path);

	void SetBuffer(uint binding, IComputeBuffer* buf);
	void SetFloat(string name, const int& val);
	void SetFloat(string name, const float& val);
	void SetMatrix(string name, const Mat4x4& val);
	void SetArray(string name, void* val, uint cnt);

	void Dispatch(uint cx, uint cy, uint xz);
protected:
	std::vector<std::pair<GLuint, IComputeBuffer*>> buffers;

	GLuint pointer;
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

enum DEFTEX : byte {
	DEFTEX_BLACK,
	DEFTEX_GREY,
	DEFTEX_WHITE,
	DEFTEX_RED,
	DEFTEX_GREEN,
	DEFTEX_BLUE,
	DEFTEX_NORMAL
};

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
	static std::vector<GLuint> defTexs;

	static void _UpdateTexCache(void*);

	void Save(string path);
	void ApplyGL(Mat4x4& _mv, Mat4x4& _p);
	void ResetVals();
	
	bool _maskExpanded;
};

class Time {
public:
	static long long startMillis;
	static long long millis;
	static double time;
	static float delta;
};

enum DrawTex_Scaling {
	DrawTex_Fit,
	DrawTex_Crop,
	DrawTex_Stretch
};

//#define EditText(x,y,w,h,s,str,font) _EditText(__COUNTER__, x,y,w,h,s,str,font)

#define UI_MAX_EDIT_TEXT_FRAMES 8

class UI {
public:
	static void Texture(float x, float y, float w, float h, ::Texture* texture, DrawTex_Scaling scl = DrawTex_Stretch, float miplevel = 0);
	static void Texture(float x, float y, float w, float h, ::Texture* texture, float alpha, DrawTex_Scaling scl = DrawTex_Stretch, float miplevel = 0);
	static void Texture(float x, float y, float w, float h, ::Texture* texture, Vec4 tint, DrawTex_Scaling scl = DrawTex_Stretch, float miplevel = 0);
	static void Label(float x, float y, float s, string str, Font* font, Vec4 col = black(), float maxw = -1);

	//Draws an editable text box. EditText does not work on recursive functions.
	static string EditText(float x, float y, float w, float h, float s, Vec4 bcol, const string& str, Font* font, bool delayed = false, bool* changed = nullptr, Vec4 fcol = black(), Vec4 hcol = Vec4(0, 120.0f / 255, 215.0f / 255, 1), Vec4 acol = white(), bool ser = true);
	
	static bool CanDraw();
//protected:
	static bool _isDrawingLoop;
	static void PreLoop();
	static uint _activeEditText[UI_MAX_EDIT_TEXT_FRAMES], _lastEditText[UI_MAX_EDIT_TEXT_FRAMES], _editingEditText[UI_MAX_EDIT_TEXT_FRAMES];
	static ushort _activeEditTextId, _editingEditTextId;
	static uint drawFuncLoc;

	static void GetEditTextId();
	static bool IsActiveEditText();

	struct StyleColor {
		Vec4 backColor, fontColor;
		//Texture* backTex;

		void Set(const Vec4 vb, const Vec4 vf) {
			backColor = vb;
			fontColor = vf;
		}
	};
	struct Style {
		StyleColor normal, mouseover, highlight, press;
		int fontSize;
	};

	friend class Engine;
protected:
	static uint _editTextCursorPos, _editTextCursorPos2;
	static string _editTextString;
	static float _editTextBlinkTime;

	static Style _defaultStyle;
	
	static void Init();
};

class Engine {
public:
	static void BeginStencil(float x, float y, float w, float h);
	static void EndStencil();
	static void DrawLine(Vec2 v1, Vec2 v2, Vec4 col, float width);
	static void DrawLine(Vec3 v1, Vec3 v2, Vec4 col, float width);
	static void DrawLineW(Vec3 v1, Vec3 v2, Vec4 col, float width);
	static void DrawLineWDotted(Vec3 v1, Vec3 v2, Vec4 col, float width, float dotSz, bool app = false);
	static void DrawTriangle(Vec2 v1, Vec2 v2, Vec2 v3, Vec4 col, bool fill = true, float width = 1);
	static void DrawTriangle(Vec2 centre, Vec2 dir, Vec4 col, bool fill = true, float width = 1);
	static void DrawCircle(Vec2 c, float r, uint n, Vec4 col, float width);
	static void DrawCircleW(Vec3 c, Vec3 x, Vec3 y, float r, uint n, Vec4 col, float width, bool dotted = false);
	static void DrawCubeLinesW(float x0, float x1, float y0, float y1, float z0, float z1, float width, Vec4 col);
	static MOUSE_STATUS Button(float x, float y, float w, float h);
	static MOUSE_STATUS Button(float x, float y, float w, float h, Vec4 normalVec4);
	static MOUSE_STATUS Button(float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4, bool labelCenter = false);
	static MOUSE_STATUS Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static MOUSE_STATUS Button(float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, float uvx = 0, float uvy = 0, float uvw = 1, float uvh = 1);
	static MOUSE_STATUS Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4, bool labelCenter = false);
	static MOUSE_STATUS EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4);
	static MOUSE_STATUS EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static MOUSE_STATUS EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4);
	static MOUSE_STATUS EButton(bool a, float x, float y, float w, float h, Texture* texture, Vec4 col);
	static MOUSE_STATUS EButton(bool a, float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static MOUSE_STATUS EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4);
	static bool Toggle(float x, float y, float s, Vec4 col, bool t);
	static bool Toggle(float x, float y, float s, Texture* texture, bool t, Vec4 col=white(), ORIENTATION o = ORIENT_NONE);
	static float DrawSliderFill(float x, float y, float w, float h, float min, float max, float val, Vec4 background, Vec4 foreground);
	static float DrawSliderFillY(float x, float y, float w, float h, float min, float max, float val, Vec4 background, Vec4 foreground);
	static Vec2 DrawSliderFill2D(float x, float y, float w, float h, Vec2 min, Vec2 max, Vec2 val, Vec4 background, Vec4 foreground);
	//scaleType: 0=scale, 1=clip, 2=tile
	static void DrawProgressBar(float x, float y, float w, float h, float progress, Vec4 background, Texture* foreground, Vec4 tint, int padding, byte scaleType);

	static void RotateUI(float a, Vec2 point);
	static void ResetUIMatrix();

	static uint unlitProgram, unlitProgramA, unlitProgramC, skyProgram;
	//static uint blurProgram, blurSBProgram;
	static Font* defaultFont;
	
	static ulong GetNewId();

	static Texture* fallbackTex;

	static std::vector<std::ifstream> dataFiles;
	static std::unordered_map<ASSETTYPE, std::vector<std::pair<byte, long>>> dataPoss;
	static std::unordered_map<ASSETTYPE, std::vector<void*>> dataPossCache;

//private: //fk users
	static GLint drawQuadLocs[3], drawQuadLocsA[3], drawQuadLocsC[1];
	static void ScanQuadParams();
	static void DrawQuad(float x, float y, float w, float h, uint texture, float miplevel = 0);
	static void DrawQuad(float x, float y, float w, float h, uint texture, Vec4 col);
	static void DrawQuad(float x, float y, float w, float h, Vec4 col);
	static void DrawQuad(float x, float y, float w, float h, GLuint texture, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, bool single, Vec4 col, float miplevel = 0);
	static void DrawCube(Vec3 pos, float dx, float dy, float dz, Vec4 col);
	static void DrawIndices(const Vec3* poss, const int* is, int length, float r, float g, float b);
	static void DrawIndicesI(const Vec3* poss, const int* is, int length, float r, float g, float b);
	static ulong idCounter;
	static std::vector<Camera*> sceneCameras;
	
	static std::vector<std::ifstream*> assetStreams;
	static std::unordered_map<byte, std::vector<string>> assetData;

	static size_t _mainThreadId;
	//static std::unordered_map<string, byte[]> assetDataLoaded;
	//byte GetAsset(string name);
	friend int main(int argc, char **argv);
protected:
	static void Init(string path = "");
	static bool LoadDatas(string path);

	static Rect* stencilRect;
};

enum GITYPE : byte {
	GITYPE_NONE,
	GITYPE_RSM
};

class SceneSettings {
public:
	SceneSettings(): sky(nullptr), skyId(-1), skyStrength(1) {}
	
	Background* sky;
	float skyStrength;
	Color ambientCol;
	GITYPE GIType = GITYPE_RSM;
	float rsmRadius;

	bool useFog, sunFog;
	float fogDensity, fogSunSpread;
	Vec4 fogColor, fogSunColor;

	friend class Editor;
	friend class EB_Inspector;
	friend class Scene;
protected:
	int skyId;
};

class Scene {
public:
	Scene() : sceneName("newScene") {}
	Scene(std::ifstream& stream, long pos);
	~Scene() {}
	static Scene* active;
	static bool loaded() {
		return active != nullptr;
	}
	string sceneName;
	int sceneId;
	uint objectCount = 0;
	std::vector<pSceneObject> objects;
	SceneSettings settings;
	std::vector<Component*> _preUpdateComps;
	std::vector<Component*> _preLUpdateComps;
	std::vector<Component*> _preRenderComps;

	static void Load(uint i), Load(string name);
	static void AddObject(pSceneObject object, pSceneObject parent = nullptr);
	static void DeleteObject(pSceneObject object);

	friend int main(int argc, char **argv);
	friend class Editor;
	friend struct Editor_PlaySyncer;
	friend class AssetManager;
	friend class Component;
protected:

	static std::ifstream* strm;
//#ifndef IS_EDITOR
	static std::vector<string> sceneEPaths;
//#endif
	static std::vector<string> sceneNames;
	static std::vector<long> scenePoss;

	static void ReadD0();
	static void Unload();
	void Save(Editor* e);

	static struct _offset_map {
		uint objCount = offsetof(Scene, objectCount),
			objs = offsetof(Scene, objects);
	} _offsets;
};

class AssetManager {
	friend int main(int argc, char **argv);
	friend class Engine;
	friend class Editor;
	friend class Scene;
	friend class SceneObject;
	friend class Material;
	friend struct Editor_PlaySyncer;
	template<typename T> friend T* _GetCache(ASSETTYPE t, ASSETID i);
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

#include "SceneObjects.h"

//#endif