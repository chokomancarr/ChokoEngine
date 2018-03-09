#pragma once

/*
Global stuff, normally not macro-protected

[av]: available in Lait version
*/

#define _ITERATOR_DEBUG_LEVEL 0

#include "Defines.h"

#ifdef CHOKO_LAIT
#define LAIT_STATIC static
#else
#define LAIT_STATIC
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define STRINGMRGDO(a,b) a ## b
#define STRINGMRG(a,b) STRINGMRGDO(a,b)

#pragma region includes
/* OS-specific headers */
#if defined(PLATFORM_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Dbghelp.h>
#include <WinSock2.h>
#include <signal.h>
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Dbghelp.lib")
#else //networking is identical on unix systems
#include <arpa/inet.h>
#include <sys/socket.h>
#ifdef PLATFORM_LNX
#include <unistd.h>
#include <execinfo.h>
#endif
#endif

/* gl (windows) */
#ifdef PLATFORM_WIN
#pragma comment(lib, "glfw_win.lib")
#pragma comment(lib, "glew_win.lib")
#include <glew.h>
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <glfw3native.h>
typedef GLFWwindow NativeWindow;
#define LOGI(...)

#elif defined(PLATFORM_ADR)
#ifdef FEATURE_COMPUTE_SHADERS
#include <GLES3\gl31.h>
#else
#include <GLES3\gl3.h>
#endif
//#include <GLES\gl.h>
#include <GLES\glext.h>
#include <GLES3\gl3ext.h>
#include <android/input.h>
#include <android/native_window.h>
typedef void GLFWwindow;
typedef unsigned int size_t;
typedef ANativeWindow NativeWindow;
#define NULL 0

#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ChokoLait", __VA_ARGS__))

//define some functions not available is OPENGL ES
extern void glPolygonMode(GLenum a, GLenum b);
#define GL_BGR GL_RGB
#define GL_BGRA GL_RGBA
#define GL_LINE 0U
#define GL_FILL 0U

#elif defined(PLATFORM_LNX)
#include "GL/glew.h"
#include "GLFW/glfw3.h"
//#define GLFW_EXPOSE_NATIVE_X11
//#define GLFW_EXPOSE_NATIVE_GLX
//#include "GLFW/glfw3native.h"
//typedef void* NativeWindow;
typedef GLFWwindow NativeWindow;
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <array>
#include <memory>
#include <thread>
#include "lodepng.h"
#include <math.h>
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

/* freetype */
#ifdef PLATFORM_WIN
#pragma comment(lib, "freetype_win.lib")
#endif
#include <ft2build.h>
#include FT_FREETYPE_H

/* jpeglib */
#ifdef PLATFORM_WIN
#pragma comment(lib, "jpeg_win.lib")
#endif
#include "jpeglib.h"
#include "jerror.h"

/* ffmpeg */
#ifdef FEATURE_AV_CODECS
#ifdef PLATFORM_WIN
#pragma comment(lib, "ffmpeg_win.lib")
#endif
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
extern std::string ffmpeg_getmsg(int i);
class ffmpeg_init {
public:
	ffmpeg_init() {
		av_register_all();
	};
};
#else
class ffmpeg_init {};
#endif
#pragma endregion

enum PLATFORM : unsigned char {
	PLATFORM_WINDOWS,
	PLATFORM_ANDROID,
	PLATFORM_LINUX
};
#if defined(PLATFORM_WIN)
const PLATFORM platform = PLATFORM_WINDOWS;
#elif defined(PLATFORM_ADR)
const PLATFORM platform = PLATFORM_ANDROID;
#elif defined(PLATFORM_LNX)
const PLATFORM platform = PLATFORM_LINUX;
#endif

#ifndef IS_EDITOR
extern bool _pipemode;
#endif

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

#pragma region Type Extensions

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

typedef int ASSETID;

const float PI = 3.1415926535f;
const float rad2deg = 57.2958f;
const float deg2rad = 0.0174533f;
const char char0 = 0;

#define UI_MAX_EDIT_TEXT_FRAMES 8

#define Normalize(a) glm::normalize(a)
#define Distance(a, b) glm::distance(a, b)

template <typename T> const T& min(const T& a, const T& b) {
	if (a > b) return b;
	else return a;
}
template <typename T> const T& max(const T& a, const T& b) {
	if (a > b) return a;
	else return b;
}
template <typename T> T Repeat(T t, T a, T b) {
	while (t > b)
		t -= (b - a);
	while (t < a)
		t += (b - a);
	return t;
}
template <typename T> T Clamp(T t, T a, T b) {
	return min(b, max(t, a));
}

template <typename T> T Lerp(T a, T b, float c) {
	if (c < 0) return a;
	else if (c > 1) return b;
	else return a*(1 - c) + b*c;
}
template <typename T> float InverseLerp(T a, T b, T c) {
	return Clamp((float)((c - a) / (b - a)), 0.0f, 1.0f);
}

#include "AudioEngine.h"
#include "Networking.h"

//shorthands
Vec4 black(float f = 1);
Vec4 red(float f = 1, float i = 1), green(float f = 1, float i = 1), blue(float f = 1, float i = 1), cyan(float f = 1, float i = 1), yellow(float f = 1, float i = 1), white(float f = 1, float i = 1);

#define push_val(var, nm, val) auto var = nm; nm = val;

/* cmake for msvc does not have to_string */
#if defined(PLATFORM_ADR)
namespace std {
	template <typename T> string to_string(T val) {
		std::ostringstream strm;
		strm << val;
		return strm.str();
	}

	int stoi(const string& s);
	float stof(const string& s);
	unsigned long stoul(const string& s);
}

#elif defined(PLATFORM_LNX)
#endif

#ifndef PLATFORM_WIN
void fopen_s(FILE** f, const char* c, const char* m);
#define sscanf_s sscanf
#endif

namespace std {
	string to_string(Vec2 v);
	string to_string(Vec3 v);
	string to_string(Vec4 v);
	string to_string(Quat v);
}

std::vector<string> string_split(string s, char c);

int TryParse(string str, int defVal);
uint TryParse(string str, uint defVal);
float TryParse(string str, float defVal);

class MatFunc {
public:
	static Mat4x4 FromTRS(const Vec3& t, const Quat& r, const Vec3& s);
};
class QuatFunc {
public:
	static Quat Inverse(const Quat&);
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

class Rect {
public:
	Rect() : x(0), y(0), w(1), h(1) {}
	Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
	Rect(Vec4 v) : x(v.r), y(v.g), w(v.b), h(v.a) {}
	float x, y, w, h;

	/*! Check if v is inside this rect.
	*/
	bool Inside(const Vec2& v);
	/*! Returns a new Rect covered by both this rect and r2
	*/
	Rect Intersection(const Rect& r2);
};

typedef glm::tvec2<int, glm::highp> Int2;

typedef unsigned char DRAWORDER;
#define DRAWORDER_NONE 0x00
#define DRAWORDER_SOLID 0x01
#define DRAWORDER_TRANSPARENT 0x02
#define DRAWORDER_OVERLAY 0x04
#define DRAWORDER_LIGHT 0x08

#define ECACHESZ_PADDING 1

#pragma endregion

#pragma region class names

//we really shouldn't be doing this
#if defined(PLATFORM_WIN)
#define _allowshared(T) friend class std::_Ref_count_obj<T>
#elif defined(PLATFORM_ADR)
#define _allowshared(T) friend class __gnu_cxx::new_allocator<T>
//#define _allowshared(T) friend class std::__libcpp_compressed_pair_imp<std::allocator<T>, T, 1>
#elif defined(PLATFORM_LNX)
#define _allowshared(T) friend class __gnu_cxx::new_allocator<T>
#endif

template <class T> class Ref;

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
_canref(AudioSource);
_canref(AudioListener);

//AssetObjects.h
class AssetItem;
class AssetManager;
_canref(AssetObject);

_canref(Animation);
_canref(AnimClip);
_canref(Background);
_canref(CameraEffect);
_canref(CubeMap);
_canref(Material);
_canref(Mesh);
_canref(Shader);
_canref(Texture);
_canref(RenderTexture);
_canref(AudioClip);

class Editor;
class EditorBlock;
class EB_Inspector;
class EB_Browser_File;
class EB_Viewer;

#pragma endregion

#pragma region enums

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
	ASSETTYPE_AUDIOCLIP = 0x50,
	ASSETTYPE_SCRIPT_H = 0x9e,
	ASSETTYPE_SCRIPT_CPP = 0x9f,
	//derived types
	ASSETTYPE_TEXTURE_REND = 0xa0
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

enum OBJECT_STATUS : byte {
	OBJECT_UNDEF,
	OBJECT_ALIVE,
	OBJECT_DEAD
};

enum SHADER_VARTYPE : byte {
	SHADER_INT = 0x00,
	SHADER_FLOAT = 0x01,
	SHADER_VEC2 = 0x02,
	SHADER_VEC3 = 0x03,
	SHADER_SAMPLER = 0x10,
	SHADER_MATRIX = 0x20,
	SHADER_BUFFER = 0x30
};

enum SHADER_TEST : byte {
	SHADER_TEST_NEVER,
	SHADER_TEST_ALWAYS,
	SHADER_TEST_LESS,
	SHADER_TEST_LEQUAL,
	SHADER_TEST_EQUAL,
	SHADER_TEST_GEQUAL,
	SHADER_TEST_GREATER,
	SHADER_TEST_NOTEQUAL
};

enum SHADER_CULL : byte {
	SHADER_CULL_BACK,
	SHADER_CULL_FRONT,
	SHADER_CULL_NONE
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

enum DRAWTEX_SCALING : byte {
	DRAWTEX_FIT,
	DRAWTEX_CROP,
	DRAWTEX_STRETCH
};

enum GITYPE : byte {
	GITYPE_NONE,
	GITYPE_RSM
};

enum COMPONENT_TYPE : byte {
	COMP_UNDEF = 0x00,
	COMP_CAM = 0x01, //camera
	COMP_MFT = 0x02, //mesh filter
	COMP_MRD = 0x10, //mesh renderer
	COMP_TRD = 0x11, //texture renderer
	COMP_SRD = 0x12, //skinned mesh renderer
	COMP_PST = 0x13, //particle system
	COMP_LHT = 0x20, //light
	COMP_RFQ = 0x22, //reflective quad
	COMP_RDP = 0x25, //render probe
	COMP_ARM = 0x30, //armature
	COMP_ANM = 0x31, //animator
	COMP_INK = 0x35, //inverse kinematics
	COMP_AUS = 0x40, //audio source
	COMP_AUL = 0x41, //audio listener
	COMP_SCR = 0xff //script
};

#pragma endregion

/*! Debugging functions. Output is saved in Log.txt beside the executable.
[av] */
class Debug {
public:
	static void Message(string c, string s);
	static void Warning(string c, string s);
	static void Error(string c, string s);
	static void ObjectTree(const std::vector<pSceneObject>& o);

	static void InitStackTrace();
	static void** StackTrace(uint* count = nullptr);
	static std::vector<string> StackTraceNames();
	static void DumpStackTrace();

	friend int main(int argc, char **argv);
	friend class ChokoLait;
protected:
	static std::ofstream* stream;
	static void Init(string path);

private:
	static void DoDebugObjectTree(const std::vector<pSceneObject>& o, int i);
	
#ifdef PLATFORM_WIN
	static HANDLE winHandle;
#endif
};

/*! Base class of instantiatable object
[av] */
class Object : public std::enable_shared_from_this<Object> {
public:
	Object(string nm = "");
	Object(ulong id, string nm);
	ulong id;
	string name;
	bool dirty = false; //triggers a reload of internal variables
	bool dead = false; //will be cleaned up after this frame

	virtual bool ReferencingObject(Object* o) { return false; }
};

#pragma region pointer extensions

template <class T> std::shared_ptr<T> get_shared(Object* ref) {
	return std::dynamic_pointer_cast<T> (ref->shared_from_this());
}

template <class T> class Ref {
public:
	Ref(bool suppress = false) : _suppress(suppress) {
		static_assert(std::is_base_of<Object, T>::value, "Ref class must be derived from Object!");
	}
	Ref(std::shared_ptr<T> ref, bool suppress = false) : _suppress(suppress), _object(ref), _empty(false) {
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
		if (!ref) clear();
		else {
			_object = get_shared<T>((Object*)ref);
			_empty = false;
		}
	}
	operator bool() const {
		return !(_empty || _object.expired());
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

#pragma endregion

/*! Run-time mesh generation.
[av] */
class Procedurals {
public:
	static Mesh* Plane(uint xCount, uint yCount);
	static Mesh* UVSphere(uint uCount, uint vCount);
};

/*! RNG System
[av] */
class Random {
public:
	static int seed;
	static float Value(), Range(float min, float max);
};

/*! Time information
[av] */
class Time {
public:
	static long long startMillis;
	static long long millis;
	static double time;
	static float delta;
};

long long milliseconds();

void _StreamWrite(const void* val, std::ofstream* stream, int size);
void _StreamWriteAsset(Editor* e, std::ofstream* stream, ASSETTYPE t, ASSETID i);

template<typename T> void _Strm2Val(std::istream& strm, T &val) {
	long long pos = strm.tellg();
	strm.read((char*)&val, (byte)sizeof(T));
	if (strm.fail()) {
		Debug::Error("Strm2Val", "Fail bit raised! (probably eof reached) " + std::to_string(pos));
	}
}
ASSETID _Strm2H(std::istream& strm);

string _Strm2Asset(std::istream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int maxL = 100);


/*! File/folder reading/writing functions.
[av] */
class IO {
public:
	static std::vector<string> GetFiles(const string& path, string ext = "");
#ifdef IS_EDITOR
	static std::vector<EB_Browser_File> GetFilesE(Editor* e, const string& path);
#endif
	static void GetFolders(const string& path, std::vector<string>* names, bool hidden = false);
	static bool HasDirectory(string szPath);
	static bool HasFile(string szPath);
	static string ReadFile(const string& path);
#if defined(IS_EDITOR) || defined(PLATFORM_WIN)
	static std::vector<string> GetRegistryKeys(HKEY key);
	static std::vector<std::pair<string, string>> GetRegistryKeyValues(HKEY hKey, DWORD numValues = 5);
#endif
	static string GetText(const string& path);
	static std::vector<byte> GetBytes(const string& path);

	static string path;

	friend class ChokoLait;
	friend class Engine;
protected:
	static const string& InitPath();
};

#ifdef PLATFORM_WIN
class SerialPort {
public:
	static std::vector<string> GetNames();
	static bool Connect(string port);
	static bool Disconnect();
	static bool Read(byte* data, uint count);
	static bool Write(byte* data, uint count);

protected:
	static HANDLE handle;
};
#endif

#ifdef PLATFORM_WIN
class WinFunc {
public:
	static HWND GetHwndFromProcessID(DWORD id);
};
#endif

/*! Dynamic fonts constructed with FreeType (.ttf files)
[av]*/
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
#ifdef IS_EDITOR
	friend class PopupSelector;
#endif
protected:
	static FT_Library _ftlib;
	static void Init(), InitVao(uint sz);
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

	static uint vaoSz;
	static GLuint vao, vbos[3];

	std::unordered_map<uint, GLuint> _glyphs; //each glyph size is fontSize*16
	GLuint CreateGlyph (uint size, bool recalcW2h = false);
};

/*! Mouse and keyboard input
[av] */
class Input {
public:
	static Vec2 mousePos, mousePosRelative, mouseDelta, mouseDownPos;
	static bool mouse0, mouse1, mouse2;
	static byte mouse0State, mouse1State, mouse2State;
	static string inputString;
	static void UpdateMouseNKeyboard(bool* src = nullptr);

	static bool KeyDown(InputKey key), KeyHold(InputKey key), KeyUp(InputKey key);

	static uint touchCount;
	static std::array<uint, 10> touchIds;
	static std::array<Vec2, 10> touchPoss;
	//static std::array<float, 10> touchForce;
	static std::array<byte, 10> touchStates;
	class Motion {
	public:
		static Vec2 Pan();
		static Vec2 Zoom();
		static Vec3 Rotate();
	};
#ifdef PLATFORM_ADR
	static void UpdateAdr(AInputEvent* e);
#endif

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

/*! Display window properties
[av] */
class Display {
public:
	static int width, height;
	const static uint dpi = 96;
	static glm::mat3 uiMatrix;

	static void Resize(int x, int y, bool maximize = false);

	friend int main(int argc, char **argv);
	//move all functions in here please
	friend void MouseGL(GLFWwindow* window, int button, int state, int mods);
	friend void MotionGL(GLFWwindow* window, double x, double y);
	friend void FocusGL(GLFWwindow* window, int focus);
	friend class PopupSelector;
	friend class ChokoLait;
//protected:
	static NativeWindow* window;
};

class Audio {
public:
	class Playback {
	public:
		uint pos;
		AudioClip* clip;

		friend class Audio;
	protected:
		Playback(AudioClip* clip, float pos);

		bool Gen(float* data, uint count);
	};

	/*!
	Plays an audio clip directly into stream, ignoring effects
	[av] */
	static Playback* Play(AudioClip* clip, float pos = 0);

//protected:
	static std::vector<Playback*> sources;

	static bool Gen(byte* data, uint count);
};

#ifdef FEATURE_COMPUTE_SHADERS
/*! generic class of ComputeBuffer
[av] */
class IComputeBuffer {
public:
	IComputeBuffer(uint size, void* data, uint padding = 0, uint stride = 1);
	~IComputeBuffer();

	GLuint pointer;
	uint size;


	void Set(void* data, uint padding = 0, uint stride = 1);
	
	/*! Returns a copy of the compute buffer data.
	 * The lhs pointer will be overwritten if target is null. Use Get(T*) to use a preallocated buffer.
	 */
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

/*! Data buffer to be used by ComputeShader.
[av] */
template<typename T> class ComputeBuffer : public IComputeBuffer {
public:
	ComputeBuffer(uint num, T* data = nullptr, uint padding = 0) : IComputeBuffer(num*sizeof(T), data, padding) {}
};

/*! GPGPU Shader. 
[av] */
class ComputeShader {
public:
	/*! Creates a ComputeShader from the shader source.
	Example usage: ComputeShader(IO::GetText("C:\\source.compute"));
	*/
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
#endif

//#define EditText(x,y,w,h,s,str,font) _EditText(__COUNTER__, x,y,w,h,s,str,font)


/*! 2D drawing to the screen.
[av] */
class UI {
public:
	static void Texture(float x, float y, float w, float h, ::Texture* texture, DRAWTEX_SCALING scl = DRAWTEX_STRETCH, float miplevel = 0);
	static void Texture(float x, float y, float w, float h, ::Texture* texture, float alpha, DRAWTEX_SCALING scl = DRAWTEX_STRETCH, float miplevel = 0);
	static void Texture(float x, float y, float w, float h, ::Texture* texture, Vec4 tint, DRAWTEX_SCALING scl = DRAWTEX_STRETCH, float miplevel = 0);
	static void Label(float x, float y, float s, string str, Font* font, Vec4 col = black(), float maxw = -1);

	//Draws an editable text box. EditText does not work on recursive functions.
	static string EditText(float x, float y, float w, float h, float s, Vec4 bcol, const string& str, Font* font, bool delayed = false, bool* changed = nullptr, Vec4 fcol = black(), Vec4 hcol = Vec4(0, 120.0f / 255, 215.0f / 255, 1), Vec4 acol = white(), bool ser = true);
	
	static bool CanDraw();

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
	friend void FocusGL(GLFWwindow* window, int focus);
	friend class PopupSelector;
	friend class RenderTexture;
protected:

	static bool focused;
	static uint _editTextCursorPos, _editTextCursorPos2;
	static string _editTextString;
	static float _editTextBlinkTime;

	static Style _defaultStyle;

	static void InitVao(), SetVao(uint sz, void* verts, void* uvs = nullptr);
	static uint _vboSz;
	static GLuint _vao, _vboV, _vboU;
	
	static void Init();
};

class Engine { //why do I have this class again?
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

	static GLuint defProgram, unlitProgram, unlitProgramA, unlitProgramC, skyProgram;
	static GLint defColLoc;
	//static uint blurProgram, blurSBProgram;
	static Font* defaultFont;
	
	static ulong GetNewId();

	static Texture* fallbackTex;

	static std::vector<std::ifstream> dataFiles;
	static std::unordered_map<ASSETTYPE, std::vector<std::pair<byte, long>>, std::hash<byte>> dataPoss;
	static std::unordered_map<ASSETTYPE, std::vector<void*>, std::hash<byte>> dataPossCache;

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
	static void DrawMeshInstanced(Mesh* mesh, uint matId, Material* mat, uint count);

	static ulong idCounter;
	static std::vector<Camera*> sceneCameras;
	
	static std::vector<std::ifstream*> assetStreams;
	static std::unordered_map<byte, std::vector<string>> assetData;

	static std::thread::id _mainThreadId;
	//static std::unordered_map<string, byte[]> assetDataLoaded;
	//byte GetAsset(string name);
	friend int main(int argc, char **argv);
	friend class ChokoLait;
	friend void Start();
protected:
	static void Init(string path = "");
	static bool LoadDatas(string path);

	static Rect* stencilRect;
};

#include "AudioEngine.h"
#include "AssetObjects.h"
#include "SceneObjects.h"
