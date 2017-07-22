#ifndef EDITOR_H
#define EDITOR_H
#include "Engine.h"

/*
Editor functions
*/

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_map>

using namespace std;

#define EB_HEADER_SIZE 16
#define EB_HEADER_PADDING 16

//class Editor;
//class EditorBlock;

typedef unsigned char byte;
typedef void(*dataFunc)(EditorBlock*, void*);
typedef void(*shortcutFunc)(EditorBlock*);
typedef void(*shortcutFuncGlobal)(Editor*);
typedef void(*callbackFunc)(void*);
typedef string(*editCallbackFunc)(string);
typedef pair<string, shortcutFunc> funcMap;
typedef pair<string, shortcutFunc> funcMapGlobal;
typedef unordered_map<int, shortcutFunc> ShortcutMap;
typedef unordered_map<int, shortcutFuncGlobal> ShortcutMapGlobal;
typedef unordered_map<int, funcMap[]> CommandsMap;
int GetShortcutInt(InputKey k, InputKey m1 = Key_None, InputKey m2 = Key_None, InputKey m3 = Key_None);
bool ShortcutTriggered(int i, bool c, bool a, bool s);

Vec4 grey1(), grey2(), accent();

class EditorBlock {
public:

	virtual ~EditorBlock() {}

	int x1, y1, x2, y2;
	byte editorType;
	Editor* editor;
	ShortcutMap shortcuts;
	bool hidden = false;

	//bool clipM0, clipM1, clipM2;

	byte headerStatus;

	virtual void Draw() = 0;
	virtual void Refresh() = 0;
	virtual void OnMouseM(Vec2 d) {}
	virtual void OnMousePress(int i) {}
	virtual void OnMouseScr(bool up) {}
//protected:
	//void DrawControlButtons();
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
	EB_Browser_File(Editor* e, string path, string name, string fN);
	string path, name, fullName;
	int thumbnail;
	bool canExpand, expanded;
	vector<EB_Browser_File> children;
	Texture* tex;
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

	static void _AddAsset(EditorBlock* b);
	static void _DoAddAssetH(EditorBlock* b), _DoAddAssetMat(EditorBlock* b), _DoAddAssetShad(EditorBlock* b), _DoAddAssetEff(EditorBlock* b);
	static void _DoAddShadStd(EditorBlock* b), _DoAddShadEff(EditorBlock* b);
};

class EB_Viewer : public EditorBlock {
public:
	EB_Viewer(Editor* e, int x1, int y1, int x2, int y2);
	~EB_Viewer() {}

	float rz, rw, scale;
	Vec3 arrowVerts[15];
	static const int arrowTIndexs[18];
	static const int arrowSIndexs[18];
	glm::mat4 viewingMatrix;
	bool persp;
	float fov;
	Vec3 rotCenter;
	Vec4 arrowX, arrowY, arrowZ;
	//Vec3 axesPos;
	byte selectedTooltip, selectedShading, selectedOrient;

	Vec3 preModVals;
	Vec2 modVal; //delta
	byte modifying;//[type 1g 2r 3s][axis 0f, 1x 2y 3z]
	Vec3 modAxisDir;
	Camera* seeingCamera;

	glm::mat4 invMatrix, projMatrix;
	void MakeMatrix();

	void Draw();
	void DrawTArrows(Vec3 pos, float size);
	void DrawRArrows(Vec3 pos, float size);
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
	static void _D2AddComCam(EditorBlock*), _D2AddComMrd(EditorBlock*), _D2AddComLht(EditorBlock*), _D2AddComRdp(EditorBlock*), _D2AddComMft(EditorBlock*);
	
	static void _AddObjAsI(EditorBlock*); //, _AddObjAsC(EditorBlock*), _AddObjAsP(EditorBlock*);
	
	static void _TooltipT(EditorBlock*), _TooltipR(EditorBlock*), _TooltipS(EditorBlock*);
	static void _SelectAll(EditorBlock*), _ViewInvis(EditorBlock*), _ViewPersp(EditorBlock*);
	static void _OrientG(EditorBlock*), _OrientL(EditorBlock*), _OrientV(EditorBlock*);
	static void _ViewFront(EditorBlock*), _ViewBack(EditorBlock*), _ViewLeft(EditorBlock*), _ViewRight(EditorBlock*), _ViewTop(EditorBlock*), _ViewBottom(EditorBlock*), _ViewCam(EditorBlock*);
	static void _SnapCenter(EditorBlock*), _TogglePersp(EditorBlock*);
	static void _Escape(EditorBlock*);
};

class I_EBI_Value {
public:
	I_EBI_Value(string name, byte type, byte sizeH) : name(name), type(type), sizeH(sizeH) {}
	byte type;
	byte sizeH;
	string name;
};

class EB_Inspector : public EditorBlock {
public:
	EB_Inspector(Editor* e, int x1, int y1, int x2, int y2);

	string label;

	bool lock = false;
	SceneObject* lockedObj;
	byte lockGlobal;

	//void SelectAsset(EBI_Asset* e, string s);
	void Deselect();
	void Draw();
	void Refresh() {}

	static void DrawScalar(Editor* e, Vec4 v, float dh, string label, float& value);
	static void DrawVector2(Editor* e, Vec4 v, float dh, string label, Vec2& value);
	static void DrawVector3(Editor* e, Vec4 v, float dh, string label, Vec3& value);
	static void DrawVector4(Editor* e, Vec4 v, float dh, string label, Vec4& value);
	static void DrawVec4(Editor* e, Vec4 v, float dh, string label, Vec4& col);
	static void DrawAsset(Editor* e, Vec4 v, float dh, string label, ASSETTYPE type);

	static void _ApplyTexFilter0(EditorBlock* e), _ApplyTexFilter1(EditorBlock* e), _ApplyTexFilter2(EditorBlock* e);
	static void _ApplyMatShader(void* v), _ApplySky(void* v), _ApplyEffMat(void* v);
	//static void DrawTexture(Editor* e, Vec4 v, string label, Texture* tex, Vec4& uv);
};

class EB_AnimEditor : public EditorBlock {
public:
	EB_AnimEditor(Editor* e, int x1, int y1, int x2, int y2);

	float scale = 1;
	Vec2 offset;

	Animator* editingAnim;
	ASSETID _editingAnim;
	uint selected;
	bool selectingTransition;

	void Refresh() {}
	void Draw();
	void OnMouseScr(bool up) override;

	static void _SetAnim(void* b);
	static void _AddState(EditorBlock* eb);
	static void _AddEmpty(EditorBlock* b), _AddBlend(EditorBlock* b);
};

class EB_Previewer : public EditorBlock {
public:
	EB_Previewer(Editor* e, int x1, int y1, int x2, int y2);

	void Draw();
	void Refresh() {}

protected:
	bool showBuffers = false, showLumi = false, blitting2 = false;

	void FindEditor();
	EB_Viewer* viewer;
	void InitGBuffer();
	void _InitDummyBBuffer();
	void _InitDebugPrograms();
	void DrawPreview(Vec4 v);
	void _RenderLights(Vec4 v);
	void _RenderSky(glm::mat4 mat), _DrawLights(vector<SceneObject*> oo, glm::mat4 ip);

	static void _ToggleBuffers(EditorBlock* v), _ToggleLumi(EditorBlock* v);

	float previewWidth, previewHeight;
	float previewWidth_o, previewHeight_o;
	static GLuint d_fbo, d_texs[3], d_depthTex;
	static GLuint b_fbo, b_texs[2], bb_fbo, bb_tex;
	static GLuint lumiProgram;
	void Blit(GLuint prog, uint w, uint h);
};

class EB_ColorPicker : public EditorBlock {
public:
	EB_ColorPicker(Editor* e, int x1, int y1, int x2, int y2, Color* tar): target(tar) {
		if (tar == nullptr)
			runtime_error("Color Picker with no color target!");
		editorType = 100;
		editor = e;
		this->x1 = x1;
		this->y1 = y1;
		this->x2 = x2;
		this->y2 = y2;
	}

	Color* target;

	void Draw();
	void Refresh(){}
};

class CompRef {
public:
	CompRef(COMPONENT_TYPE t) : type(t), comp(nullptr), path("") {}

	COMPONENT_TYPE type;
	Component* comp;
	string path;
};

class BlockCombo;
class xPossLerper;
class yPossLerper;
class xPossMerger;
class yPossMerger;

class Editor {
public:
	Editor();
	static Editor* instance;
	Color cc;
	//prefs
	bool _showDebugInfo = true;
	bool _showGrid = true;
	bool _mouseJump = true;
	ushort _maxPreviewSize = 256;
	byte _assetDataSize = 100; //x10Mb
	bool _cleanOnBuild = false;
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
	vector<BlockCombo*> blockCombos;
	Vec2 popupPos;

	EditorBlock* dialogBlock;

	EditorBlock* menuBlock; //menu = layer1
	int menuPadding;
	string menuTitle;
	vector<string> menuNames;
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
	editCallbackFunc editingCallback;

	string backgroundPath;
	Texture* backgroundTex;
	byte backgroundAlpha;
	void SetBackground(string s, float a = -1);

	//select = layer3
	bool browseIsComp;
	ASSETTYPE browseType;
	vector<Component*> browseCompList;
	vector<string> browseCompListNames;
	void ScanBrowseComp();
	float browseOffset;
	int browseSize;
	int* browseTarget;
	CompRef* browseTargetComp;
	callbackFunc browseCallback;
	void* browseCallbackParam;
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
	vector<bool> buildLogErrors, buildLogWarnings;
	string buildErrorPath;
	int buildErrorLine;
	string buildLabel;
	float buildProgressValue;
	Vec4 buildProgressVec4;
	bool buildEnd; //allow esc
	//build settings = layer7

	Font* font;
	static HWND hwnd;
	char mousePressType = -1;
	int mouseOn = 0;
	int mouseOnP = 0;
	int scrW, scrH;
	byte editorLayer;

	int gridId[68];
	Vec3 grid[64];

	shared_ptr<Scene> activeScene = nullptr;
	bool sceneLoaded() { return activeScene != nullptr; }
	SceneObject* selected;
	glm::mat4 selectedMvMatrix;
	bool selectGlobal = false;
	vector<string> includedScenes;
	vector<bool> includedScenesUse;
	ushort savedIncludedScenes;

	string selectedFileName;
	string selectedFilePath;
	ASSETTYPE selectedFileType;
	ASSETID selectedFile;
	void* selectedFileCache;
	void DeselectFile();

	glm::mat4 viewMatrix;
	bool persp;

	ShortcutMapGlobal globalShorts;

	Texture* buttonX, *buttonExt, *buttonPlus, *buttonExtArrow, *background, *placeholder, *checkers, *expand;
	Texture* collapse, *object, *checkbox, *keylock, *assetExpand, *assetCollapse, *browse;
	vector<Texture*> tooltipTexs;
	vector<Texture*> shadingTexs;
	vector<Texture*> orientTexs;
	unordered_map<SHADER_VARTYPE, Texture*> matVarTexs;
	unordered_map<byte, Texture*> ebIconTexs;

	vector<string> assetIconsExt;
	vector<Texture*> assetIcons;
	//Texture buttonDash;
	unordered_map<string, ASSETTYPE> assetTypes;
	unordered_map<ASSETTYPE, vector<string>> allAssets;
	vector<string> headerAssets, cppAssets, blendAssets;
	unordered_map<ASSETTYPE, vector<string>> normalAssets;
	unordered_map<ASSETTYPE, vector<void*>> normalAssetCaches;

	void ResetAssetMap();

	void DrawAssetSelector(float x, float y, float w, float h, Vec4 col, ASSETTYPE type, float labelSize, Font* labelFont, ASSETID* tar, callbackFunc func = nullptr, void* param = nullptr);
	void DrawCompSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, CompRef* tar, callbackFunc func = nullptr, void* param = nullptr);
	void DrawColorSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, Vec4* tar);
	ASSETID GetAssetInfoH(string p), GetAssetInfo(string p, ASSETTYPE &type, ASSETID& i);
	ASSETID GetAssetId(void* p), GetAssetId(void* p, ASSETTYPE& t);

	void ReadPrefs(), SavePrefs();
	void LoadDefaultAssets();
	void GenerateScriptResolver();
	void GenerateScriptValuesReader(string& s);
	void NewScene();
	void UpdateLerpers();
	void DrawHandles();

	void RegisterMenu(EditorBlock* block, string title, vector<string> names, vector<shortcutFunc> funcs, int padding, Vec2 pos = Input::mousePos);
	void RegisterMenu(EditorBlock* block, string title, vector<string> names, dataFunc func, vector<void*> vals, int padding, Vec2 pos = Input::mousePos);

	static Texture* GetRes(string name);
	static Texture* GetResExt(string name, string ext = "bmp");
	static Texture* GetRes(string name, bool mipmap, string ext = "bmp");
	static Texture* GetRes(string name, bool mipmap, bool nearest, string ext = "bmp");

	vector<pair<string, string>> messages;
	vector<byte> messageTypes;

	void _Message(string c, string s);
	void _Warning(string c, string s);
	void _Error(string c, string s);
	void ClearLogs();

	void ReloadAssets(string path, bool recursive);
	bool ParseAsset(string path);
	void* GetCache(ASSETTYPE type, int id);
	void* GenCache(ASSETTYPE type, int id);

	static void Compile(Editor* e);
	static void ShowPrefs(Editor* e);
	static void ShowCompileSett(Editor* e);
	static void SaveScene(Editor* e);
	static void OpenScene(Editor* e);
	static void DoOpenScene(EditorBlock* b, void* v);
	static void DeleteActive(Editor* e);
	static void DoDeleteActive(EditorBlock* b);

	void DoCompile();

private:
	Editor(Editor const &);
	Editor& operator= (Editor const &);
};

class BlockCombo {
public:
	BlockCombo() : active(0) {}
	vector<EditorBlock*> blocks;
	uint active;
	void Set(), Draw();
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