#ifndef ENGINE_H
#define ENGINE_H


/*
 Engine functions used by actual game code
*/

#include <gl/glew.h>
#include <string>
#include "Shader.h"
#include "KTMModel.h"
#include <Windows.h>

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

//shorthands
Color red(), green(), blue(), cyan(), black(), white();
Color red(float f), green(float f), blue(float f), cyan(float f), black(float f), white(float f);
Color red(float f, float i), green(float f, float i), blue(float f, float i), cyan(float f, float i), white(float f, float i);
Color LerpColor(Color a, Color b, float f);
float clamp(float f, float a, float b);
Vec3 rotate(Vec3 v, Quat q);
glm::mat4 Quat2Mat(Quat q);

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

class Texture {
public:
	Texture() {}
	Texture(const string& path);
	Texture(const string& path, bool mipmap);
	Texture(const string& path, bool mipmap, bool nearest);
	Texture(HBITMAP bmp, int width, int height);
	~Texture(){ Delete(); }
	bool loaded;
	unsigned int width, height;
	GLuint pointer;

	Texture &operator=(const Texture&) = delete; //No copy-assignment.
	Texture &operator=(Texture &&other)
	{
		Delete();
		width = other.width;
		height = other.height;
		pointer = other.pointer;
		loaded = other.loaded;
	}

protected:
	void Delete() {
		glDeleteTextures(1, &pointer);
	}
};

class EB_Browser_File;

class IO {
public:
	static vector<EB_Browser_File> GetFiles(const string& path);
	static void GetFolders(const string& path, vector<string>* names);
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

class Material {
public:
	uint shader;

	Material(void);
	Material(Shader* shad);
	void SetSampler(string name, Texture* texture);
	void SetSampler(string name, Texture* texture, int id);

};

class Time {
public:
	static long long startMillis;
	static long long millis;
	static double time;
	static float delta;
};

//see SceneObjects.h
class Object;
class Component;
class Camera;
class SceneObject;

class Engine {
public:

	static void Init();
	static void DrawTexture(float x, float y, float w, float h, Texture* texture);
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, float alpha);
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, Color tint);
	static void DrawLine(Vec2 v1, Vec2 v2, Color col, float width);
	static void DrawLine(Vec3 v1, Vec3 v2, Color col, float width);
	static void Label(float x, float y, float s, string str, Font* texture);
	static void Label(float x, float y, float s, string str, Font* texture, Color color);
	static void Label(float x, float y, float s, string str, Font* texture, Color color, float maxw);
	static byte Button(float x, float y, float w, float h);
	static byte Button(float x, float y, float w, float h, Color normalColor);
	static byte Button(float x, float y, float w, float h, Color normalColor, string label, float labelSize, Font* labelFont, Color labelColor);
	static byte Button(float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor);
	static byte Button(float x, float y, float w, float h, Texture* texture, Color normalColor, Color highlightColor, Color pressColor);
	static byte Button(float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor, string label, float labelSize, Font* labelFont, Color labelColor);

	static void RotateUI(float a, Vec2 point);
	static void ResetUIMatrix();

	static uint unlitProgram, unlitProgramA, unlitProgramC;
	static Font* defaultFont;
	
	static ulong GetNewId();

//private: //only me using this project, fk users
	static void DrawQuad(float x, float y, float w, float h, uint texture);
	static void DrawQuad(float x, float y, float w, float h, Color color);
	static void DrawQuad(float x, float y, float w, float h, uint texture, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, bool single, Color color);
	static ulong idCounter;
	static vector<Camera*> sceneCameras;
};

#include "SceneObjects.h"

class Scene {
public:
	Scene() {}

	vector<SceneObject*> objects;
};

#endif