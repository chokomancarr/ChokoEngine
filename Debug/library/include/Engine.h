#ifndef ENGINE_H
#define ENGINE_H


/*
 Engine functions used by actual game code
*/

#include <gl/glew.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Windows.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "Defines.h"

#ifndef IS_EDITOR
extern bool _pipemode;
#endif

const float PI = 3.1415926535f;
const float rad2deg = 57.2958f;
const float deg2rad = 0.0174533f;
const char char0 = 0;

#define Lerp(a, b, c) ((a)*(1-(c)) + (b)*(c))
#define Normalize(a) glm::normalize(a)
#define Distance(a, b) glm::distance(a, b)

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

Vec3 to_vec3(Vec4 v);

class Mesh;

namespace ChokoEngine {
	class QuatFunc {
	public:
		static Quat Inverse(const Quat&);
		//static Vec3 Rotate(const Vec3&, const Quat&); //just use Q*V
		static Vec3 ToEuler(const Quat&);
		static Mat4x4 ToMatrix(const Quat&);
		static Quat FromAxisAngle(const Vec3&, float);
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
			return Vec4(r, g, b, a)*(1.0f/255);
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
}

class Rect {
public:
	Rect() : x(0), y(0), w(1), h(1) {}
	Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
	Rect(Vec4 v) : x(v.r), y(v.g), w(v.b), h(v.a) {}
	float x, y, w, h;

	bool Inside(const Vec2& v);
	Rect Intersection(const Rect& r2);
};

class Random {
public:
	static int seed;
	static float Value(), Range(float min, float max);
};

#define MOUSE_DOWN 0x01
#define MOUSE_HOLD 0x02
#define MOUSE_UP 0x03
#define MOUSE_HOVER_FLAG 0x10
#define MOUSE_CLICK 0x11 //use for buttons
#define MOUSE_PRESS 0x12 //use for buttons
#define MOUSE_RELEASE 0x13 //use for buttons

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

typedef unsigned char ASSETTYPE;
typedef int ASSETID;

//shorthands
Vec4 black(float f = 1);
Vec4 red(float f = 1, float i = 1), green(float f = 1, float i = 1), blue(float f = 1, float i = 1), cyan(float f = 1, float i = 1), yellow(float f = 1, float i = 1), white(float f = 1, float i = 1);
Vec4 LerpVec4(Vec4 a, Vec4 b, float f);
float clamp(float f, float a, float b);
float repeat(float f, float a, float b);
//Vec3 rotate(Vec3 v, Quat q);
void _StreamWrite(const void* val, std::ofstream* stream, int size);
void _StreamWriteAsset(Editor* e, std::ofstream* stream, ASSETTYPE t, ASSETID i);
//void _Strm2Int(std::ifstream& strm, int& i), _Strm2Float(std::ifstream& strm, float& f), _Strm2Short(std::ifstream& strm, short& i);

template<typename T> void _Strm2Val(std::istream& strm, T &val) {
	/*
	long long pos = strm.tellg();
	byte size = sizeof(T);
	char c[8];
	strm.read(c, size);
	if (strm.fail()) {
		Debug::Error("Strm2Val", "Fail bit raised! (probably eof reached) " + to_string(pos));
	}
	//int rr(*(T*)c);
	val = *(T*)c;
	*/
	long long pos = strm.tellg();
	strm.read((char*)&val, (byte)sizeof(T));
	if (strm.fail()) {
		Debug::Error("Strm2Val", "Fail bit raised! (probably eof reached) " + std::to_string(pos));
	}
}
ASSETID _Strm2H(std::istream& strm);

string _Strm2Asset(std::istream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int maxL = 100);

//void* __GetCacheE(ASSETTYPE t, ASSETID i);
template<typename T> T* _GetCache(ASSETTYPE t, ASSETID i) {
#ifdef IS_EDITOR
	return static_cast<T*>(Editor::instance->GetCache(t, i));
#else
	return static_cast<T*>(AssetManager::GetCache(t, i));
#endif
}
std::vector<float> hdr2float(byte imagergbe[], int w, int h);

//see SceneObjects.h
class Object;
class Component;
class Transform;
class Camera;
class MeshFilter;
class MeshRenderer;
class SkinnedMeshRenderer;
class Light;
class ParticleSystem;

//see Assetmanager
class AssetItem;
class AssetManager;

#define ASSETTYPE_UNDEF 0x00

#define ASSETTYPE_SCENE 0x90
#define ASSETTYPE_TEXTURE 0x01
#define ASSETTYPE_HDRI 0x02
#define ASSETTYPE_TEXCUBE 0x03
#define ASSETTYPE_SHADER 0x05
#define ASSETTYPE_MATERIAL 0x10
#define ASSETTYPE_BLEND 0x20
#define ASSETTYPE_MESH 0x21
#define ASSETTYPE_ANIMCLIP 0x30
#define ASSETTYPE_ANIMATOR 0x31
#define ASSETTYPE_CAMEFFECT 0x40
#define ASSETTYPE_SCRIPT_H 0x9e
#define ASSETTYPE_SCRIPT_CPP 0x9f
//derived types
#define ASSETTYPE_TEXTURE_REND 0xa0
class AssetObject;
class Mesh;
class Texture;
class Background;
class SceneScript;
class SceneObject;

class Int2 {
public:	
	Int2(): x(0), y(0){}
	Int2(int x, int y): x(x), y(y) {}

	int x, y;

	bool operator==(const Int2& rhs)
	{
		return (x == rhs.x) && (y == rhs.y);
	}
};

class Debug {
public:
	static void Message(string c, string s);
	static void Warning(string c, string s);
	static void Error(string c, string s);
	static void ObjectTree(std::vector<SceneObject*> o);

	friend int main(int argc, char **argv);
protected:
	static std::ofstream* stream;
	static void Init(string path);

private:
	static void DoDebugObjectTree(std::vector<SceneObject*> o, int i);
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

enum InputKey {
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
};

class Object {
public:
	Object(string nm = "");
	ulong id;
	string name;

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

	const ASSETTYPE type = 0;
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

class ShaderBase : public AssetObject {
public:
	ShaderBase(string path);
	ShaderBase(std::istream& stream, uint offset);
	ShaderBase(const string& vert, const string& frag);
	ShaderBase(GLuint p) : AssetObject(ASSETTYPE_SHADER), pointer(p), loaded(!!p), inherited(true) {}
	//ShaderBase(string vert, string frag);
	~ShaderBase() {
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
	Material(ShaderBase* shad);
	~Material();

	ShaderBase* Shader();
	void Shader(ShaderBase* shad);
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
	friend class Scene;
	friend class AssetManager;
	friend class ShaderBase;
	friend class RenderTexture;
	friend int main(int argc, char **argv);
	friend void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
protected:
	Material(string s);
	Material(std::istream& stream, uint offset = 0);
	void _ReloadParams();

	ShaderBase* shader;
	int _shader;
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

class Engine {
public:
	static void BeginStencil(float x, float y, float w, float h);
	static void EndStencil();
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, DrawTex_Scaling scl = DrawTex_Stretch, float miplevel = 0);
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, float alpha, DrawTex_Scaling scl = DrawTex_Stretch, float miplevel = 0);
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, Vec4 tint, DrawTex_Scaling scl = DrawTex_Stretch, float miplevel = 0);
	static void DrawLine(Vec2 v1, Vec2 v2, Vec4 col, float width);
	static void DrawLine(Vec3 v1, Vec3 v2, Vec4 col, float width);
	static void DrawLineW(Vec3 v1, Vec3 v2, Vec4 col, float width);
	static void DrawLineWDotted(Vec3 v1, Vec3 v2, Vec4 col, float width, float dotSz, bool app = false);
	static void DrawTriangle(Vec2 v1, Vec2 v2, Vec2 v3, Vec4 col, bool fill = true, float width = 1);
	static void DrawTriangle(Vec2 centre, Vec2 dir, Vec4 col, bool fill = true, float width = 1);
	static void DrawCircle(Vec2 c, float r, uint n, Vec4 col, float width);
	static void DrawCircleW(Vec3 c, Vec3 x, Vec3 y, float r, uint n, Vec4 col, float width, bool dotted = false);
	static void DrawCubeLinesW(float x0, float x1, float y0, float y1, float z0, float z1, float width, Vec4 col);
	static void Label(float x, float y, float s, string str, Font* font);
	static void Label(float x, float y, float s, string str, Font* font, Vec4 Vec4);
	static void Label(float x, float y, float s, string str, Font* font, Vec4 Vec4, float maxw);
	static byte Button(float x, float y, float w, float h);
	static byte Button(float x, float y, float w, float h, Vec4 normalVec4);
	static byte Button(float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4, bool labelCenter = false);
	static byte Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static byte Button(float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, float uvx = 0, float uvy = 0, float uvw = 1, float uvh = 1);
	static byte Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4, bool labelCenter = false);
	static byte EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Texture* texture, Vec4 Vec4);
	static byte EButton(bool a, float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4);
	static bool DrawToggle(float x, float y, float s, Vec4 col, bool t);
	static bool DrawToggle(float x, float y, float s, Texture* texture, bool t, Vec4 col=white(), ORIENTATION o = ORIENT_NONE);
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
	static void DrawQuad(float x, float y, float w, float h, uint texture, Vec4 Vec4);
	static void DrawQuad(float x, float y, float w, float h, Vec4 Vec4);
	static void DrawQuad(float x, float y, float w, float h, GLuint texture, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, bool single, Vec4 Vec4, float miplevel = 0);
	static void DrawCube(Vec3 pos, float dx, float dy, float dz, Vec4 Vec4);
	static void DrawIndices(const Vec3* poss, const int* is, int length, float r, float g, float b);
	static void DrawIndicesI(const Vec3* poss, const int* is, int length, float r, float g, float b);
	static ulong idCounter;
	static std::vector<Camera*> sceneCameras;
	
	static std::vector<std::ifstream*> assetStreams;
	static std::unordered_map<byte, std::vector<string>> assetData;
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
	ChokoEngine::Color ambientCol;
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
	static std::shared_ptr<Scene> active;
	static bool loaded() {
		return active != nullptr;
	}
	string sceneName;
	int sceneId;
	static void AddObject(SceneObject* object, SceneObject* parent = nullptr);
	static void DeleteObject(SceneObject* object);
	std::vector<SceneObject*> objects;
	SceneSettings settings;

	static void Load(uint i), Load(string name);

	friend int main(int argc, char **argv);
	friend class Editor;
	friend class AssetManager;
protected:
	static std::ifstream* strm;
#ifndef IS_EDITOR
	static std::vector<string> sceneEPaths;
#endif
	static std::vector<string> sceneNames;
	static std::vector<long> scenePoss;
	static void ReadD0();
	static void Unload();
	void Save(Editor* e);
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
#else
	static std::vector <std::pair<ASSETTYPE, ASSETID>> dataECacheIds;
#endif
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

#endif