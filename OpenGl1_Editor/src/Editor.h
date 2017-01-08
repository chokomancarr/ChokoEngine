#ifndef EDITOR_H
#define EDITOR_H
#include "Engine.h"

/*
Editor functions
*/

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

#define EB_HEADER_SIZE 35
#define EB_HEADER_PADDING 16

//class Editor;
class EditorBlock;

typedef unsigned char byte;
typedef void(*shortcutFunc)(EditorBlock*);
typedef void(*shortcutFuncGlobal)(Editor*);
typedef pair<string, shortcutFunc> funcMap;
typedef pair<string, shortcutFunc> funcMapGlobal;
typedef unordered_map<int, shortcutFunc> ShortcutMap;
typedef unordered_map<int, shortcutFuncGlobal> ShortcutMapGlobal;
typedef unordered_map<int, funcMap[]> CommandsMap;
int GetShortcutInt(byte c, int m);

Color grey1(), grey2(), accent();

class EditorBlock {
public:

	virtual ~EditorBlock() {}

	int x1, y1, x2, y2;
	byte editorType;
	Editor* editor;
	ShortcutMap shortcuts;

	//bool clipM0, clipM1, clipM2;

	byte headerStatus;

	virtual void Draw() = 0;
	virtual void Refresh() = 0;
	virtual void OnMouseM(Vec2 d) = 0;
	virtual void OnMouseScr(bool up) = 0;
protected:
	void DrawControlButtons();
	//EditorBlock(byte t, float x1, float y1, float x2, float y2) : type(t), x1(x1), x2(x2), y1(y1), y2(y2) {}
};

class EB_Empty : public EditorBlock { //for test, does nothing
public:
	EB_Empty(Editor* e, int x1, int y1, int x2, int y2) {
		editorType = 0;
		editor = e;
		this->x1 = x1;
		this->y1 = y1;
		this->x2 = x2;
		this->y2 = y2;
	}

	void Draw();
	void Refresh(){} //nothing
	void OnMouseM(Vec2 d) {}
	void OnMouseScr(bool up) {}
};

class EB_Hierarchy: public EditorBlock {
public:
	EB_Hierarchy(Editor* e, int x1, int y1, int x2, int y2) {
		editorType = 4;
		editor = e;
		this->x1 = x1;
		this->y1 = y1;
		this->x2 = x2;
		this->y2 = y2;
	}

	void Draw();
	void Refresh(){} //nothing
	void OnMouseM(Vec2 d) {}
	void OnMouseScr(bool up) {}
};

class EB_Browser_File {
public:
	EB_Browser_File(string path, string name);
	~EB_Browser_File() {
		if (hasTex)
			delete(thumbnail);
	}
	string path;
	string name;
	bool hasTex;
	Texture* thumbnail;
};

class EB_Browser_SubPath {
public:	
	string name;
	EB_Browser_SubPath* child;
};

class EB_Browser : public EditorBlock {
public:
	EB_Browser(Editor* e, int x1, int y1, int x2, int y2, string dir);
	//~EB_Browser();

	string currentDir;
	vector<string> dirs;
	vector<EB_Browser_File> files;

	void Draw();
	void Refresh();
	void OnMouseM(Vec2 d) {}
	void OnMouseScr(bool up) {}
};

class EB_Viewer : public EditorBlock {
public:
	EB_Viewer(Editor* e, int x1, int y1, int x2, int y2);
	~EB_Viewer() {

	}

	float rz, rw, scale;
	Vec3 arrowVerts[15];
	static const int arrowTIndexs[18];
	static const int arrowSIndexs[18];
	glm::mat4 viewingMatrix;
	bool persp;
	Color arrowX, arrowY, arrowZ; 
	Vec3 axesPos;
	byte selectedTooltip, selectedShading;

	void MakeMatrix();

	void Draw();
	void DrawTArrows(Vec3 pos, float size);
	void DrawSArrows(Vec3 pos, float size);
	void Refresh() {}
	void OnMouseM(Vec2 d);
	void OnMouseScr(bool up);

	static void _OpenMenuW(EditorBlock*);
	static void _OpenMenuX(EditorBlock*);
	static void _OpenMenuChgMani(EditorBlock*);

	static void _SelectAll(EditorBlock*);
	static void _ViewInvis(EditorBlock*);
	static void _ViewPersp(EditorBlock*);
	static void _TooltipT(EditorBlock*);
	static void _TooltipR(EditorBlock*);
	static void _TooltipS(EditorBlock*);
};

class I_EBI_Value {
public:
	I_EBI_Value(string name, byte type, byte sizeH) : name(name), type(type), sizeH(sizeH) {}
	byte type;
	byte sizeH;
	string name;
};

class I_EBI_ValueCollection {
public:
	vector<I_EBI_Value> vals;
};

template <class T>
class EBI_Value : public I_EBI_Value {
public:
	EBI_Value(string name, byte type, byte sizeH, T a, T b, T c, T d) : I_EBI_Value(name, type, sizeH), a(a), b(b), c(c), d(d) {}
	T a, b, c, d;
};

class EBI_Asset {
public:
	//EBI_Asset(string str, string nm);

	bool correct;
	vector<I_EBI_Value> vals;

	virtual void Draw(Editor* e, EditorBlock* b, Color* v) = 0;
};

class EBIA_Shader : public EBI_Asset {
public:
	EBIA_Shader(string path);

	void Draw(Editor* e, EditorBlock* b, Color* v);
};

class EB_Inspector : public EditorBlock {
public:
	EB_Inspector(Editor* e, int x1, int y1, int x2, int y2);

	bool isAsset;
	EBI_Asset* obj;
	string label;
	bool loaded = false, lock = false;
	
	void SelectAsset(EBI_Asset* e, string s);
	void Deselect();
	void Draw();
	void Refresh() {}
	void OnMouseM(Vec2 d) {}
	void OnMouseScr(bool up) {}

	static void DrawScalar(Editor* e, Color v, string label, float& value);
	static void DrawVector2(Editor* e, Color v, string label, float& valueX, float& valueY);
	static void DrawVector3(Editor* e, Color v, string label, float& valueX, float& valueY, float& valueZ);
	static void DrawVector4(Editor* e, Color v, string label, float& valueX, float& valueY, float& valueZ, float& valueW);
	static void DrawColor(Editor* e, Color v, string label, Color& col);
	static void DrawTexture(Editor* e, Color v, string label, Texture* tex, Color& uv);
};

class xPossLerper;
class yPossLerper;
class xPossMerger;
class yPossMerger;

class Editor {
public:
	//prefs
	bool _mouseJump = true;

	string projectFolder;

	static string dataPath;
	int activeX=-1, activeY=-1;
	float amin, amax;
	float dw, dh;
	vector<float> xPoss, yPoss;
	vector<xPossLerper> xLerper;
	vector<yPossLerper> yLerper;
	vector<Int2> xLimits, yLimits; //not include 0 1
	vector<EditorBlock*> blocks;
	Vec2 popupPos;
	EditorBlock* menuBlock;
	int menuPadding;
	vector<string> menuNames; //menu = layer1
	vector<shortcutFunc> menuFuncs;
	//edit = layer2
	//progress = layer3
	string progressName;
	float progressValue;

	bool WAITINGBUILDSTARTFLAG = false;
	//building - layer4: custom progress to look cool
	vector<string> buildLog;
	string buildLabel;
	float buildProgressValue;

	Font* font;
	static HWND hwnd;
	char mousePressType = -1;
	int mouseOn = 0;
	int mouseOnP = 0;
	int scrW, scrH;
	byte editorLayer;

	int gridId[68];
	Vec3 grid[64];

	bool sceneLoaded;
	Scene activeScene;
	SceneObject* selected;

	string selectedFile = "";

	glm::mat4 viewMatrix;
	bool persp;

	ShortcutMapGlobal globalShorts;

	Texture* buttonX;
	Texture* buttonExt; 
	Texture* buttonExtArrow;
	Texture* background;
	Texture* placeholder;
	Texture* checkers;
	Texture* expand;
	Texture* collapse;
	Texture* object;
	vector<Texture*> tooltipTexs;
	vector<Texture*> shadingTexs;
	//Texture buttonDash;

	void LoadDefaultAssets();
	void NewScene();
	void UpdateLerpers();
	void DrawHandles();

	void RegisterMenu(EditorBlock* block, vector<string> names, vector<shortcutFunc> funcs, int padding);
	
	static Texture* GetRes(string name);
	static Texture* GetRes(string name, bool mipmap);
	static Texture* GetRes(string name, bool mipmap, bool nearest);

	static bool ParseAsset(string path);
	static bool GetCache(string& path, I_EBI_ValueCollection& vals);
	static bool SetCache(string& path, I_EBI_ValueCollection* vals);

	static void Compile(Editor* e);
	void DoCompile();

};

class xPossLerper {
public:
	xPossLerper(Editor* e, int i, float fr, float to) : e(e), id(i), lerp(0), fr(fr), to(to) {}
	Editor* e;
	float lerp;
	int id;
	float fr, to;

	bool Update() {
		lerp = 20 * Time::delta*1.01f + (1 - 20 * Time::delta)*lerp;
		if (lerp > 1) {
			e->xPoss[id] = to;
			return true;
		}
		else {
			e->xPoss[id] = lerp*to + (1 - lerp)*fr;
			return false;
		}
	}
};
class yPossLerper {
public:
	yPossLerper(Editor* e, int i, float fr, float to) : e(e), id(i), lerp(0), fr(fr), to(to) {}
	Editor* e;
	float lerp;
	int id;
	float fr, to;

	bool Update() {
		lerp = 20 * Time::delta*1.01f + (1 - 20 * Time::delta)*lerp;
		if (lerp > 1) {
			e->yPoss[id] = to;
			return true;
		}
		else {
			e->yPoss[id] = lerp*to + (1 - lerp)*fr;
			return false;
		}
	}
};

class xPossMerger {
public:
	xPossMerger(Editor*e, int fr, int to): e(e), lerp(0), fr(fr), to(to) {}
	Editor* e;
	float lerp;
	int fr, to;

	bool Update() {
		lerp = 20 * Time::delta*1.01f + (1 - 20 * Time::delta)*lerp;
		if (lerp > 1) {
			return true;
		}
		else {
			return false;
		}
	}
};

#endif