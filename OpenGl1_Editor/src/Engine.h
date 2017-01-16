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
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Windows.h>
#include <jpeglib.h>
#include <png.h>

using namespace std;

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::vec4 Color;
typedef glm::quat Quat;

struct Rect {
public:
	Rect();
	Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
	float x, y, w, h;

	bool Inside(Vec2 v);
};

#define MOUSE_DOWN 0x01
#define MOUSE_HOLD 0x02
#define MOUSE_UP 0x03
#define MOUSE_HOVER_FLAG 0x10
#define MOUSE_CLICK 0x11 //use for buttons
#define MOUSE_PRESS 0x12 //use for buttons
#define MOUSE_RELEASE 0x13 //use for buttons

typedef unsigned char ALIGNMENT;
#define ALIGN_BOTLEFT 0x00
#define ALIGN_BOTCENTER 0x01
#define ALIGN_BOTRIGHT 0x02
#define ALIGN_MIDLEFT 0x10
#define ALIGN_MIDCENTER 0x11
#define ALIGN_MIDRIGHT 0x12
#define ALIGN_TOPLEFT 0x20
#define ALIGN_TOPCENTER 0x21
#define ALIGN_TOPRIGHT 0x22

long long milliseconds();

class Editor;

//shorthands
Color red(), green(), blue(), cyan(), black(), white();
Color red(float f), green(float f), blue(float f), cyan(float f), black(float f), white(float f);
Color red(float f, float i), green(float f, float i), blue(float f, float i), cyan(float f, float i), white(float f, float i);
Color LerpColor(Color a, Color b, float f);
float clamp(float f, float a, float b);
Vec3 rotate(Vec3 v, Quat q);
glm::mat4 Quat2Mat(Quat q);
void _StreamWrite(void* val, ofstream* stream, int size);
float* hdr2float(byte imagergbe[], int w, int h);

//see SceneObjects.h
class Object;
class Component;
class Transform;
class Camera;
class MeshFilter;
class MeshRenderer;

//see Assetmanager
class AssetItem;
class AssetManager;

typedef unsigned char ASSETTYPE;

#define ASSETTYPE_UNDEF 0x00

#define ASSETTYPE_MESH 0x20
#define ASSETTYPE_TEXTURE 0x01
#define ASSETTYPE_HDRI 0x02
#define ASSETTYPE_SCRIPT_H 0xfe
#define ASSETTYPE_SCRIPT_CPP 0xff
class AssetObject;
class Mesh;
class Texture;
class Background;
class SceneScript;
class SceneObject;

struct Int2 {
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
};

class EB_Browser_File;

class IO {
public:
	static vector<string> GetFiles(const string& path);
	static vector<EB_Browser_File> GetFilesE(Editor* e, const string& path);
	static void GetFolders(const string& path, vector<string>* names);
	static bool HasDirectory(LPCTSTR szPath);
	static bool HasFile(LPCTSTR szPath);
	static string ReadFile(const string& path);
};

class Font {
public:
	Font(const string& path);
	Font(const string& paths, const string& pathb, int size);
	bool loaded;
	uint width, padding, height, chars;
	float w2h;
	uint width2, padding2, height2, chars2;
	uint gwidth(float s) { return (s > sizeToggle) ? width2 : width; }
	uint gheight(float s) { return (s > sizeToggle) ? height2 : height; }
	uint gpadding(float s) { return (s > sizeToggle) ? padding2 : padding; }
	uint gchars(float s) { return (s > sizeToggle) ? chars2 : chars; }
	float w2h2;
	GLuint pointer;
	GLuint pointer2;
	int sizeToggle;
	GLuint getpointer(float size);
	float gw2h(float s) { return (s > sizeToggle) ? w2h2 : w2h; }
	ALIGNMENT alignment;

	Font* Align(ALIGNMENT a);
};

class Input {
public:
	static Vec2 mousePos, mousePosRelative;
	static bool mouse0, mouse1, mouse2;
	static byte mouse0State, mouse1State, mouse2State;
	static void UpdateMouse();
};

class Display {
public:
	static int width, height;
	static glm::mat3 uiMatrix;
};

#define ASSETTYPE_SHADER 0x05
#define SHADER_INT 0x00
#define SHADER_FLOAT 0x01
#define SHADER_SAMPLER 0x10
#define SHADER_MATRIX 0x20
class ShaderBase {
public:
	ShaderBase(string path);
	ShaderBase(string vert, string frag);
	~ShaderBase() {
		glDeleteProgram(pointer);
	}

	bool loaded;
	GLuint pointer;
	//values applied to program on drawing stage
	unordered_map<byte, unordered_map <GLint, void*>> values;

	void Set(byte type, GLint id, void* val) { values[type][id] = val; }
	void* Get(byte type, GLint id) { return values[type][id]; }
	
	//removes macros, insert include files
	static string Parse(ifstream* text);
	static bool LoadShader(GLenum shaderType, string source, GLuint& shader);
};

#define ASSETTYPE_MATERIAL 0x10
class Material {
public:
	ShaderBase* shader;

	Material(void);
	Material(ShaderBase* shad);
	void SetTexture(string name, Texture* texture);
	void SetTexture(GLint id, Texture* texture);
	void SetFloat(string name, float val);
	void SetFloat(GLint id, float val);
	void SetInt(string name, int val);
	void SetInt(GLint id, int val);
};

class Time {
public:
	static long long startMillis;
	static long long millis;
	static double time;
	static float delta;
};

class Engine {
public:
	static void Init(string path);

	static void BeginStencil(float x, float y, float w, float h);
	static void EndStencil();
	static void DrawTexture(float x, float y, float w, float h, Texture* texture);
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, float alpha);
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, Color tint);
	static void DrawLine(Vec2 v1, Vec2 v2, Color col, float width);
	static void DrawLine(Vec3 v1, Vec3 v2, Color col, float width);
	static void DrawLineW(Vec3 v1, Vec3 v2, Color col, float width);
	static void Label(float x, float y, float s, string str, Font* texture);
	static void Label(float x, float y, float s, string str, Font* texture, Color color);
	static void Label(float x, float y, float s, string str, Font* texture, Color color, float maxw);
	static byte Button(float x, float y, float w, float h);
	static byte Button(float x, float y, float w, float h, Color normalColor);
	static byte Button(float x, float y, float w, float h, Color normalColor, string label, float labelSize, Font* labelFont, Color labelColor);
	static byte Button(float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor);
	static byte Button(float x, float y, float w, float h, Texture* texture, Color normalColor, Color highlightColor, Color pressColor);
	static byte Button(float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor, string label, float labelSize, Font* labelFont, Color labelColor);
	static byte EButton(bool a, float x, float y, float w, float h, Color normalColor);
	static byte EButton(bool a, float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor);
	static byte EButton(bool a, float x, float y, float w, float h, Color normalColor, string label, float labelSize, Font* labelFont, Color labelColor);
	static byte EButton(bool a, float x, float y, float w, float h, Texture* texture, Color color);
	static byte EButton(bool a, float x, float y, float w, float h, Texture* texture, Color normalColor, Color highlightColor, Color pressColor);
	static byte EButton(bool a, float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor, string label, float labelSize, Font* labelFont, Color labelColor);
	static bool DrawToggle(float x, float y, float s, Color col, bool t);
	static bool DrawToggle(float x, float y, float s, Texture* texture, bool t);
	static bool DrawToggle(float x, float y, float s, Texture* texture, Color col, bool t);
	//scaleType: 0=scale, 1=clip, 2=tile
	static void DrawProgressBar(float x, float y, float w, float h, float progress, Color background, Texture* foreground, Color tint, int padding, byte scaleType);
	static void DrawSky(Background* sky, float forX, float forY, float angleW, float rot);
	static void DrawSky(float x, float y, float w, float h, Background* sky, float forX, float forY, float angleW, float rot);

	static void RotateUI(float a, Vec2 point);
	static void ResetUIMatrix();

	static uint unlitProgram, unlitProgramA, unlitProgramC, skyProgram;
	static Font* defaultFont;
	
	static ulong GetNewId();

	static Texture* fallbackTex;

//private: //fk users
	static void DrawQuad(float x, float y, float w, float h, uint texture);
	static void DrawQuad(float x, float y, float w, float h, uint texture, Color color);
	static void DrawQuad(float x, float y, float w, float h, Color color);
	static void DrawQuad(float x, float y, float w, float h, uint texture, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, bool single, Color color);
	static void DrawCube(Vec3 pos, float dx, float dy, float dz, Color color);
	static void DrawIndices(const Vec3* poss, const int* is, int length, float r, float g, float b);
	static void DrawIndicesI(const Vec3* poss, const int* is, int length, float r, float g, float b);
	static ulong idCounter;
	static vector<Camera*> sceneCameras;
	
	unordered_map<byte, vector<string>> assetData;
	unordered_map<string, byte[]> assetDataLoaded;
	//byte GetAsset(string name);
	

	void Render();
};

class Scene {
public:
	Scene() : sceneName(""), sky(nullptr) {}

	string sceneName;
	//global parameters
	Background* sky;
	vector<SceneObject*> objects;

	void Save(Editor* e);
};

#include "SceneObjects.h"

#endif