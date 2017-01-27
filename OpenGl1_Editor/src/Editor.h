#ifndef EDITOR_H
#define EDITOR_H
#include "Engine.h"

/*
Editor functions
*/

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

using namespace std;

#define EB_HEADER_SIZE 35
#define EB_HEADER_PADDING 16

//class Editor;
class EditorBlock;

typedef unsigned char byte;
typedef void(*dataFunc)(EditorBlock*, void*);
typedef void(*shortcutFunc)(EditorBlock*);
typedef void(*shortcutFuncGlobal)(Editor*);
typedef pair<string, shortcutFunc> funcMap;
typedef pair<string, shortcutFunc> funcMapGlobal;
typedef unordered_map<int, shortcutFunc> ShortcutMap;
typedef unordered_map<int, shortcutFuncGlobal> ShortcutMapGlobal;
typedef unordered_map<int, funcMap[]> CommandsMap;
int GetShortcutInt(InputKey k, InputKey m1, InputKey m2 = Key_None, InputKey m3 = Key_None);
bool ShortcutTriggered(int i, bool c, bool a, bool s);

Vec4 grey1(), grey2(), accent();

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
	virtual void OnMouseM(Vec2 d) {}
	virtual void OnMousePress(int i) {}
	virtual void OnMouseScr(bool up) {}
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
};

class EB_Debug : public EditorBlock {
public:
	EB_Debug(Editor* e, int x1, int y1, int x2, int y2) {
		editorType = 10;
		editor = e;
		this->x1 = x1;
		this->y1 = y1;
		this->x2 = x2;
		this->y2 = y2;
		drawM = drawW = drawE = true;
		Refresh();
	}

	bool drawM, drawW, drawE;
	vector<int> drawIds;

	void Draw();
	void Refresh();
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
};

class EB_Browser_File {
public:
	EB_Browser_File(Editor* e, string path, string name);
	string path;
	string name;
	int thumbnail;
	bool canExpand, expanded;
	vector<EB_Browser_File> children;
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
	int selectFile; //for options
	vector<string> dirs;
	vector<EB_Browser_File> files;

	void Draw();
	void Refresh();
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
	Vec4 arrowX, arrowY, arrowZ; 
	Vec3 axesPos;
	byte selectedTooltip, selectedShading, selectedOrient;

	Vec3 preModVals;
	Vec2 modVal; //delta
	byte modifying;//[type 1g 2r 3s][axis 0f, 1x 2y 3z]
	Vec3 modAxisDir;

	void MakeMatrix();

	void Draw();
	void DrawTArrows(Vec3 pos, float size);
	void DrawSArrows(Vec3 pos, float size);
	void Refresh() {}
	void OnMouseM(Vec2 d) override;
	void OnMousePress(int i) override;
	void OnMouseScr(bool up) override;

	static void _Grab(EditorBlock*), _Rotate(EditorBlock*), _Scale(EditorBlock*);
	static void _X(EditorBlock*), _Y(EditorBlock*), _Z(EditorBlock*);

	static void _OpenMenuAdd(EditorBlock*), _OpenMenuCom(EditorBlock*), _OpenMenuW(EditorBlock*), _OpenMenuX(EditorBlock*);
	static void _OpenMenuChgMani(EditorBlock*), _OpenMenuChgOrient(EditorBlock*);

	static byte preAddType;
	static void _AddObjectE(EditorBlock*), _AddObjectBl(EditorBlock*), _AddObjectCam(EditorBlock*), _AddObjectAud(EditorBlock*);
	static void _AddComScr(EditorBlock*), _AddComAud(EditorBlock*), _AddComRend(EditorBlock*), _AddComMesh(EditorBlock*);
	
	static void _DoAddObjectBl(EditorBlock* b, void* v);
	static void _DoAddComScr(EditorBlock* b, void* v), _DoAddComRend(EditorBlock* b, void* v);
	static void _D2AddComCam(EditorBlock*), _D2AddComMrd(EditorBlock*), _D2AddComMft(EditorBlock*);
	
	static void _AddObjAsI(EditorBlock*); //, _AddObjAsC(EditorBlock*), _AddObjAsP(EditorBlock*);
	
	static void _TooltipT(EditorBlock*), _TooltipR(EditorBlock*), _TooltipS(EditorBlock*);
	static void _SelectAll(EditorBlock*), _ViewInvis(EditorBlock*), _ViewPersp(EditorBlock*);
	static void _OrientG(EditorBlock*), _OrientL(EditorBlock*), _OrientV(EditorBlock*);
	static void _ViewFront(EditorBlock*), _ViewBack(EditorBlock*), _ViewLeft(EditorBlock*), _ViewRight(EditorBlock*), _ViewTop(EditorBlock*), _ViewBottom(EditorBlock*);
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

	virtual void Draw(Editor* e, EditorBlock* b, Vec4* v) = 0;
};

class EBIA_Shader : public EBI_Asset {
public:
	EBIA_Shader(string path);

	void Draw(Editor* e, EditorBlock* b, Vec4* v);
};

class EB_Inspector : public EditorBlock {
public:
	EB_Inspector(Editor* e, int x1, int y1, int x2, int y2);

	bool isAsset;
	EBI_Asset* obj;
	string label;

	bool loaded = false, lock = false;
	SceneObject* lockedObj;
	byte lockGlobal;

	void SelectAsset(EBI_Asset* e, string s);
	void Deselect();
	void Draw();
	void Refresh() {}

	static void DrawScalar(Editor* e, Vec4 v, float dh, string label, float& value);
	static void DrawVector2(Editor* e, Vec4 v, float dh, string label, Vec2& value);
	static void DrawVector3(Editor* e, Vec4 v, float dh, string label, Vec3& value);
	static void DrawVector4(Editor* e, Vec4 v, float dh, string label, Vec4& value);
	static void DrawVec4(Editor* e, Vec4 v, float dh, string label, Vec4& col);
	static void DrawAsset(Editor* e, Vec4 v, float dh, string label, ASSETTYPE type);
	//static void DrawTexture(Editor* e, Vec4 v, string label, Texture* tex, Vec4& uv);
};

class xPossLerper;
class yPossLerper;
class xPossMerger;
class yPossMerger;

class Editor {
public:
	Editor() {instance = this;}
	static Editor* instance;
	//prefs
	bool _showDebugInfo = true;
	bool _showGrid = true;
	bool _mouseJump = true;
	int _assetDataSize = 10; //x100Mb
	bool _cleanOnBuild = true;
	string _blenderInstallationPath = "C:\\Program Files\\Blender Foundation\\Blender\\blender.exe";

	string projectFolder;

	static string dataPath;
	int activeX = -1, activeY = -1;
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
	string menuTitle;
	vector<string> menuNames; //menu = layer1
	bool menuFuncIsSingle;
	vector<shortcutFunc> menuFuncs;
	dataFunc menuFuncSingle;
	vector<void*> menuFuncVals;
	//edit = layer2
	byte editingType; //0none 1int 2float 3string
	string editingVal;
	Rect editingArea;
	Vec4 editingCol;
	void SetEditing(byte t, string val, Rect a, Vec4 c2 = white());

	string backgroundPath;
	Texture* backgroundTex;
	byte backgroundAlpha;
	void SetBackground(string s, float a = -1);

	//select = layer3
	ASSETTYPE browseType;
	float browseOffset;
	int browseSize;
	int* browseTarget;
	//progress = layer4
	string progressName;
	float progressValue;
	string progressDesc;
	void BeginProgress(string n);
	//prefs = layer5

	bool WAITINGBUILDSTARTFLAG = false, WAITINGREFRESHFLAG = false;
	mutex* lockMutex;
	//building - layer6: custom progress to look cool
	vector<string> buildLog;
	void AddBuildLog(Editor* e, string s, bool forceE = false);
	vector<bool> buildLogErrors;
	string buildErrorPath;
	int buildErrorLine;
	string buildLabel;
	float buildProgressValue;
	Vec4 buildProgressVec4;
	bool buildEnd; //allow esc

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
	bool selectGlobal;
	vector<string> includedScenes;
	unordered_map<string, ASSETTYPE> assetTypes;
	unordered_map<ASSETTYPE, vector<string>> allAssets;

	string selectedFile = "";

	glm::mat4 viewMatrix;
	bool persp;

	ShortcutMapGlobal globalShorts;

	Texture* buttonX, *buttonExt, *buttonExtArrow, *background, *placeholder, *checkers, *expand;
	Texture* collapse, *object, *checkbox, *keylock, *assetExpand, *assetCollapse;
	vector<Texture*> tooltipTexs;
	vector<Texture*> shadingTexs;
	vector<Texture*> orientTexs;

	vector<string> assetIconsExt;
	vector<Texture*> assetIcons;
	//Texture buttonDash;
	vector<string> headerAssets, blendAssets;
	unordered_map<ASSETTYPE, vector<string>> normalAssets;

	void DrawAssetSelector(float x, float y, float w, float h, Vec4 col, ASSETTYPE type, float labelSize, Font* labelFont, int* tar);

	void LoadDefaultAssets();
	void RefreshAssets();
	void GenerateScriptResolver();
	void NewScene();
	void UpdateLerpers();
	void DrawHandles();

	void RegisterMenu(EditorBlock* block, string title, vector<string> names, vector<shortcutFunc> funcs, int padding);
	void RegisterMenu(EditorBlock* block, string title, vector<string> names, dataFunc func, vector<void*> vals, int padding);

	static Texture* GetRes(string name);
	static Texture* GetRes(string name, bool mipmap);
	static Texture* GetRes(string name, bool mipmap, bool nearest);

	vector<pair<string, string>> messages;
	vector<byte> messageTypes;

	void _Message(string c, string s);
	void _Warning(string c, string s);
	void _Error(string c, string s);
	void ClearLogs();

	void ReloadAssets(string path, bool recursive);
	bool ParseAsset(string path);
	static bool GetCache(string& path, I_EBI_ValueCollection& vals);
	static bool SetCache(string& path, I_EBI_ValueCollection* vals);

	static void Compile(Editor* e);
	static void ShowPrefs(Editor* e);
	static void SaveScene(Editor* e);
	static void DeleteActive(Editor* e);
	static void DoDeleteActive(EditorBlock* b);

	void DoCompile();

private:
	Editor(Editor const &);
	Editor& operator= (Editor const &);
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