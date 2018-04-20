#pragma once

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

#ifndef IS_EDITOR
//disables windows.h features
#define NOGDICAPMASKS     ;// CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES ;// VK_*
#define NOWINMESSAGES     ;// WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       ;// WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      ;// SM_*
#define NOMENUS           ;// MF_*
#define NOICONS           ;// IDI_*
#define NOKEYSTATES       ;// MK_*
#define NOSYSCOMMANDS     ;// SC_*
#define NORASTEROPS       ;// Binary and Tertiary raster ops
//#define NOSHOWWINDOW      ;// SW_*
#define OEMRESOURCE       ;// OEM Resource values
#define NOATOM            ;// Atom Manager routines
#define NOCLIPBOARD       ;// Clipboard routines
#define NOCOLOR           ;// Screen colors
#define NOCTLMGR          ;// Control and Dialog routines
#define NODRAWTEXT        ;// DrawText() and DT_*
#define NOGDI             ;// All GDI defines and routines
#define NOKERNEL          ;// All KERNEL defines and routines
//#define NOUSER            ;// All USER defines and routines
#define NONLS             ;// All NLS defines and routines
//#define NOMB              ;// MB_* and MessageBox()
#define NOMEMMGR          ;// GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        ;// typedef METAFILEPICT
#define NOMINMAX          ;// Macros min(a,b) and max(a,b)
//#define NOMSG             ;// typedef MSG and associated routines
#define NOOPENFILE        ;// OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          ;// SB_* and scrolling routines
#define NOSERVICE         ;// All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           ;// Sound driver routines
#define NOTEXTMETRIC      ;// typedef TEXTMETRIC and associated routines
#define NOWH              ;// SetWindowsHook and WH_*
#define NOWINOFFSETS      ;// GWL_*, GCL_*, associated routines
#define NOCOMM            ;// COMM driver routines
#define NOKANJI           ;// Kanji support stuff.
#define NOHELP            ;// Help engine interface.
#define NOPROFILER        ;// Profiler interface.
#define NODEFERWINDOWPOS  ;// DeferWindowPos routines
#define NOMCX             ;// Modem Configuration Extensions

#define NOBITMAP
#endif

#include <Windows.h>
#include <WinSock2.h>
#include <signal.h>
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
//#pragma comment(lib, "Dbghelp.lib")
#else //networking is identical on *nix systems
#include <arpa/inet.h>
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

#include <complex>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <unordered_map>
#include <array>
#include <memory>
#include <thread>
#include <math.h>
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"


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

#pragma region Type Extensions

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef std::string string;

typedef glm::tvec2<int, glm::highp> Int2;
typedef glm::vec2 Vec2;
typedef glm::tvec2<int, glm::highp> Int3;
typedef glm::vec3 Vec3;
typedef glm::tvec3<int, glm::highp> Int4;
typedef glm::vec4 Vec4;
typedef glm::quat Quat;
typedef glm::mat4 Mat4x4;

typedef int ASSETID;

const float PI = 3.1415926535f;
const float rad2deg = 57.2958f;
const float deg2rad = 0.0174533f;
const char char0 = 0;

const uint UI_MAX_EDIT_TEXT_FRAMES = 8;

#define Normalize(a) glm::normalize(a)
#define Distance(a, b) glm::distance(a, b)

float Dw(float), Dh(float);
Vec3 Ds(Vec3);

#include "core/math.h"

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

#define push_val(var, nm, val) auto var = nm; nm = val;

#ifndef PLATFORM_WIN
void fopen_s(FILE** f, const char* c, const char* m);
#define sscanf_s sscanf
#endif

#define F2ISTREAM(_varname, _pathname) std::ifstream _f2i_ifstream((_pathname).c_str(), std::ios::in | std::ios::binary); \
std::istream _varname(_f2i_ifstream.rdbuf());

#ifdef PLATFORM_WIN
class WinFunc {
public:
	static HWND GetHwndFromProcessID(DWORD id);
};
#endif

#include "utils/net.h"
#include "utils/xml.h"
#include "utils/strext.h"

#include "core/mvp.h"
#include "core/bbox.h"

#include "utils/color.h"
#include "utils/rect.h"

typedef byte DRAWORDER; //because we use this as flags
const DRAWORDER DRAWORDER_NONE = 0x00;
const DRAWORDER DRAWORDER_SOLID = 0x01;
const DRAWORDER DRAWORDER_TRANSPARENT = 0x02;
const DRAWORDER DRAWORDER_OVERLAY = 0x04;
const DRAWORDER DRAWORDER_LIGHT = 0x08;

const uint ECACHESZ_PADDING = 1;

#pragma endregion

#pragma region class names

//we really shouldn't be doing this
#ifdef PLATFORM_WIN
#define _allowshared(T) friend class std::_Ref_count_obj<T>
#else
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
_canref(VoxelRenderer);
_canref(InverseKinematics);
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

/*
enum InputKey : byte {
	Key_None = 0x00,
	Key_Backspace = 0x08,
	Key_Tab = 0x09,
	Key_Enter = 0x0D,
	Key_LeftShift = 0x10,
	Key_LeftControl = 0x11,
	Key_LeftAlt = 0x12,
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
*/
enum InputKey {
	Key_None = 0,
	Key_Space = 32,
	Key_Apostrophe = 39,
	Key_Comma = 44, Key_Minus, Key_Dot, Key_Slash,
	Key_0 = 48, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9,
	Key_Semicolon = 59, Key_Equal = 61,
	Key_A = 65, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J, Key_K, Key_L, Key_M, Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T, Key_U, Key_V, Key_W, Key_X, Key_Y, Key_Z,
	Key_LeftBracket = 91, Key_BackSlash, Key_RightBracket, Key_Accent = 96,
	Key_Escape = 256, Key_Enter, Key_Tab, Key_Backspace, Key_Insert, Key_Delete,
	Key_RightArrow, Key_LeftArrow, Key_DownArrow, Key_UpArrow, Key_PageUp, Key_PageDown, Key_Home, Key_End, Key_CapsLock, Key_ScrollLock, Key_NumLock,
	Key_F1 = 290, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6, Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12,
	Key_NumPad0 = 320, Key_NumPad1, Key_NumPad2, Key_NumPad3, Key_NumPad4, Key_NumPad5, Key_NumPad6, Key_NumPad7, Key_NumPad8, Key_NumPad9,
	Key_NumPadDot, Key_NumPadDivide, Key_NumPadMultiply, Key_NumPadMinus, Key_NumPadAdd, Key_NumPadEnter, Key_NumPadEqual,
	Key_LeftShift = 340, Key_LeftControl, Key_LeftAlt, Key_RightShift = 344, Key_RightControl, Key_RightAlt
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
	COMP_VRD = 0x13, //voxel renderer
	COMP_LHT = 0x20, //light
	COMP_RFQ = 0x22, //reflective quad
	COMP_RDP = 0x25, //render probe
	COMP_ARM = 0x30, //armature
	COMP_ANM = 0x31, //animator
	COMP_INK = 0x35, //inverse kinematics
	COMP_AUS = 0x40, //audio source
	COMP_AUL = 0x41, //audio listener
	COMP_PST = 0x50, //particle system
	COMP_SCR = 0xff //script
};


enum FFT_WINDOW : byte {
	FFT_WINDOW_RECTANGLE,
	FFT_WINDOW_HAMMING,
	FFT_WINDOW_BLACKMAN
};

#pragma endregion

#include "scene/object.h"

#include "core/debug.h"
#include "core/time.h"
#include "core/io.h"

#include "utils/random.h"
#include "utils/ptrext.h"
#include "utils/precedurals.h"
#include "utils/stream.h"
#include "utils/serial.h"

#include "core/font.h"
#include "core/input.h"
#include "core/display.h"
#include "core/audio.h"
#include "core/ui.h"

#include "utils/fft.h"
#ifdef FEATURE_COMPUTE_SHADERS
#include "asset/compute.h"
#endif


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

	static GLuint defProgram, defProgramW, unlitProgram, unlitProgramA, unlitProgramC, skyProgram;
	static GLint defColLoc, defWColLoc, defWMVPLoc;
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
#include "SceneScriptResolver.h"