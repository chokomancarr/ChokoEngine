#include "Defines.h"

//#ifndef IS_EDITOR
//#error Editor inclusion is not allowed in game
//#endif

#ifndef EDITOR_H
#define EDITOR_H
#include "Engine.h"

using namespace ChokoEngine;
/*
Editor functions
*/

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_map>

#define EB_HEADER_SIZE 16
#define EB_HEADER_PADDING 16


//class Editor;
//class EditorBlock;

typedef void(*dataFunc)(EditorBlock*, void*);
typedef void(*shortcutFunc)(EditorBlock*);
typedef void(*shortcutFuncGlobal)(Editor*);
typedef void(*callbackFunc)(void*);
typedef string(*editCallbackFunc)(string);
typedef std::pair<string, shortcutFunc> funcMap;
typedef std::pair<string, shortcutFunc> funcMapGlobal;
typedef std::unordered_map<int, shortcutFunc> ShortcutMap;
typedef std::unordered_map<int, shortcutFuncGlobal> ShortcutMapGlobal;
typedef std::unordered_map<int, funcMap[]> CommandsMap;
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
	bool hidden = false, maximize = false;

	//bool clipM0, clipM1, clipM2;

	byte headerStatus;

	virtual void Draw() = 0;
	virtual void Refresh() = 0;
	virtual void OnMouseM(Vec2 d) {}
	virtual void OnMousePress(int i) {}
	virtual void OnMouseScr(bool up) {}
protected:
	float scrollOffset;
	float maxScroll;
	//void DrawControlButtons();
	//EditorBlock(byte t, float x1, float y1, float x2, float y2) : type(t), x1(x1), x2(x2), y1(y1), y2(y2) {}
};

class PopupBlock {
public:

	virtual ~PopupBlock() {}
	Editor* editor;

	float x(), y();
	int w, h;
	bool exitOnClick = true;

	virtual void Draw() = 0;
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
	std::vector<int> drawIds;

	void Draw();
	void Refresh();
	void OnMouseScr(bool up) override;
};

typedef void(*consoleFunc)(string);
class EB_Console : public EditorBlock {
public:
	EB_Console(Editor* e, int x1, int y1, int x2, int y2);
	std::vector<string> outputs;
	uint outputCount;
	std::streampos coutLoc;
	string input;

	void Put(string);
	void Draw();
	void Refresh() {}
	//void OnMouseScr(bool up) override {}

protected:
	static void InitFuncs();
	static std::unordered_map<string, consoleFunc> funcs;
	static void Cmd_editor_playmode_connect(string);
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
	void OnMouseScr(bool up) override;
};

class EB_Browser_File {
public:
	EB_Browser_File(Editor* e, string path, string name, string fN);
	string path, name, fullName;
	int thumbnail;
	bool canExpand, expanded;
	std::vector<EB_Browser_File> children;
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
	std::vector<string> dirs;
	std::vector<EB_Browser_File> files;

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
	Mat4x4 viewingMatrix;
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

	Mat4x4 invMatrix, projMatrix;
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
	static void _AddObjectE(EditorBlock*), _AddObjectPr(EditorBlock*), _AddObjectBl(EditorBlock*), _AddObjectCam(EditorBlock*), _AddObjectAud(EditorBlock*);
	static void _AddComScr(EditorBlock*), _AddComAud(EditorBlock*), _AddComRend(EditorBlock*), _AddComMesh(EditorBlock*);
	
	static void _DoAddObjectBl(EditorBlock* b, void* v);
	static void _DoAddComScr(EditorBlock* b, void* v), _DoAddComRend(EditorBlock* b, void* v);
	static void _D2AddComCam(EditorBlock*), _D2AddComMrd(EditorBlock*), _D2AddComLht(EditorBlock*), _D2AddComRfq(EditorBlock*), _D2AddComRdp(EditorBlock*), _D2AddComMft(EditorBlock*);
	
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
	float previewMip;

	//void SelectAsset(EBI_Asset* e, string s);
	void Deselect();
	void Draw(), DrawGlobal(Vec4);
	void Refresh() {}

	void OnMouseScr(bool up) override;

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
	void _RenderSky(Mat4x4 mat), _DrawLights(std::vector<SceneObject*> oo, Mat4x4 ip);

	static void _ToggleBuffers(EditorBlock* v), _ToggleLumi(EditorBlock* v);

	float previewWidth, previewHeight;
	float previewWidth_o, previewHeight_o;
	static GLuint d_fbo, d_texs[4], d_depthTex;
	static GLuint b_fbo, b_texs[2], bb_fbo, bb_tex;
	static GLuint lumiProgram;
	void Blit(GLuint prog, uint w, uint h);
};

class PB_ColorPicker : public PopupBlock {
public:
	PB_ColorPicker(Editor* e, Vec4* tar, bool hasA = true): target(tar), col(*tar) {
		editor = e;
		this->w = 270;
		this->h = hasA? 335 : 318;
		col.useA = hasA;
	}

	Vec4* target;
	Color col;

	void Draw() override;
};

class PB_ProceduralGenerator : public PopupBlock {
public:
	PB_ProceduralGenerator(Editor* e) {
		editor = e;
		this->w = 400;
		this->h = 500;
	}

	void Draw() override;
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

#define WAITINGBUILDSTARTFLAG 1U
#define WAITINGREFRESHFLAG 2U

struct Editor_PlaySyncer {
	enum Editor_PlaySyncerStatus { //okbit = 1
		EPS_Offline = 0,
		EPS_Starting = 1,
		EPS_Running = 3,
		EPS_Paused = 5,
		EPS_Crashed = 2,
		EPS_RWFailure = 4,
		EPS_NoSignal = 6
	} status;
	DWORD exitCode;
	PROCESS_INFORMATION pInfo = {};
	HWND hwnd;
	struct _PipeModeObj {
		uint hasDataLoc, //inout: pbo buffer updated? (bool) (do we need this?)
			pixelsLoc, //out: glreadpixels buffer (byte array, count per channel)
			pixelCountLoc, //out: buffer size (ulong)
			screenSizeLoc, //in: screen size (ushort ushort)
			okLoc; //inout: confirmation (bool)
	} pointers;
	uint pointerLoc;
	int playW, playH;
	float timer;

	void Update();
	bool Connect(), Disconnect(), Terminate();
	bool Resize(int w, int h), ReloadTex();
	byte* pixels;
	ulong pixelCount = 0, pixelCountO = 0;
	GLuint texPointer;
};

class Editor {
public:
	Editor();
	static Editor* instance;

	//bool IS_PLAY_MODE() { return !!(playSyncer.status & 1); }
	Editor_PlaySyncer playSyncer;
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
	std::vector<float> xPoss, yPoss;
	std::vector<xPossLerper> xLerper;
	std::vector<yPossLerper> yLerper;
	std::vector<Int2> xLimits, yLimits; //not include 0 1
	std::vector<EditorBlock*> blocks;
	std::vector<BlockCombo*> blockCombos;
	bool hasMaximize;

	EditorBlock* dialogBlock;

	//menu = layer1
	EditorBlock* menuBlock;
	int menuPadding;
	string menuTitle;
	std::vector<string> menuNames;
	bool menuFuncIsSingle;
	std::vector<shortcutFunc> menuFuncs;
	dataFunc menuFuncSingle;
	std::vector<void*> menuFuncVals;

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
	std::vector<Component*> browseCompList;
	std::vector<string> browseCompListNames;
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

	PopupBlock* popup;
	Vec2 popupPos;
	GLuint popupShadeProgram;
	void RegisterPopup(PopupBlock* blk, Vec2 pos);

	byte flags;

	std::mutex* lockMutex;
	//building - layer6: custom progress to look cool
	std::vector<string> buildLog;
	void AddBuildLog(Editor* e, string s, bool forceE = false);
	std::vector<bool> buildLogErrors, buildLogWarnings;
	string buildErrorPath;
	int buildErrorLine;
	string buildLabel;
	float buildProgressValue;
	Vec4 buildProgressVec4;
	bool buildEnd; //allow esc
	//build settings = layer7

	Font* font;
	static HWND hwnd, hwnd2;
	char mousePressType = -1;
	int mouseOn = 0;
	int mouseOnP = 0;
	int scrW, scrH;
	byte editorLayer;

	bool pendingPreview;
	float previewTime;
	float minPreviewTime = 0.5f;
	string previewStr;
	ASSETTYPE previewType; //undef if string
	ASSETID previewId;

	int gridId[68];
	Vec3 grid[64];

	std::shared_ptr<Scene> activeScene = nullptr;
	bool sceneLoaded() { return activeScene != nullptr; }
	SceneObject* selected;
	Mat4x4 selectedMvMatrix;
	Vec4 selectedSpos;
	bool selectGlobal = false;
	std::vector<string> includedScenes;
	std::vector<bool> includedScenesUse;
	ushort savedIncludedScenes;

	string selectedFileName;
	string selectedFilePath;
	ASSETTYPE selectedFileType;
	ASSETID selectedFile;
	void* selectedFileCache;
	std::vector<string> selectedFileTexts;
	void DeselectFile();

	Mat4x4 viewMatrix;
	bool persp;

	ShortcutMapGlobal globalShorts;

	Texture* buttonX, *buttonExt, *buttonPlus, *buttonExtArrow, *background, *placeholder, *checkers, *expand;
	Texture* collapse, *object, *checkbox, *keylock, *assetExpand, *assetCollapse, *browse, *mipLow, *mipHigh;
	std::vector<Texture*> tooltipTexs;
	std::vector<Texture*> shadingTexs;
	std::vector<Texture*> orientTexs;
	std::unordered_map<SHADER_VARTYPE, Texture*> matVarTexs;
	std::unordered_map<byte, Texture*> ebIconTexs;

	std::vector<string> assetIconsExt;
	std::vector<Texture*> assetIcons;
	//Texture buttonDash;
	std::unordered_map<string, ASSETTYPE> assetTypes;
	std::unordered_map<ASSETTYPE, std::vector<string>> allAssets;
	std::vector<string> headerAssets, cppAssets, blendAssets;
	std::unordered_map<ASSETTYPE, std::vector<string>> normalAssets, internalAssets, proceduralAssets;
	std::unordered_map <ASSETTYPE, std::pair<ASSETTYPE, std::vector<uint>>> derivedAssets;
	std::unordered_map<ASSETTYPE, std::vector<AssetObject*>> normalAssetCaches, internalAssetCaches, proceduralAssetCaches;
	bool internalAssetsLoaded;

	static void InitShaders();
	void ResetAssetMap(), LoadInternalAssets();

	void DrawAssetSelector(float x, float y, float w, float h, Vec4 col, ASSETTYPE type, float labelSize, Font* labelFont, ASSETID* tar, callbackFunc func = nullptr, void* param = nullptr);
	void DrawCompSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, CompRef* tar, callbackFunc func = nullptr, void* param = nullptr);
	void DrawColorSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, Vec4* tar);
	
	void InitMaterialPreviewer();
	Mesh* matPreviewerSphere;
	Background* matPreviewerBg;
	GLuint matPrev_fbo, matPrev_texs[4], matPrev_depthTex;
	void DrawMaterialPreviewer(float x, float y, float w, float h, float& rx, float& rz, Material* mat);

	ASSETID GetAssetInfoH(string p), GetAssetInfo(string p, ASSETTYPE &type, ASSETID& i);
	ASSETID GetAssetId(AssetObject* p), GetAssetId(AssetObject* p, ASSETTYPE& t);
	string GetAssetName(ASSETTYPE t, ASSETID id);

	void ReadPrefs(), SavePrefs();
	void LoadDefaultAssets();
	void GenerateScriptResolver();
	void GenerateScriptValuesReader(string& s);
	void NewScene();
	void UpdateLerpers();
	void DrawHandles(), DrawPopup();

	void RegisterMenu(EditorBlock* block, string title, std::vector<string> names, std::vector<shortcutFunc> funcs, int padding, Vec2 pos = Input::mousePos);
	void RegisterMenu(EditorBlock* block, string title, std::vector<string> names, dataFunc func, std::vector<void*> vals, int padding, Vec2 pos = Input::mousePos);

	static Texture* GetRes(string name);
	static Texture* GetResExt(string name, string ext = "bmp");
	static Texture* GetRes(string name, bool mipmap, string ext = "bmp");
	static Texture* GetRes(string name, bool mipmap, bool nearest, string ext = "bmp");

	std::vector<std::pair<string, string>> messages;
	std::vector<byte> messageTypes;

	void _Message(string c, string s);
	void _Warning(string c, string s);
	void _Error(string c, string s);
	void ClearLogs();

	void ReloadAssets(string path, bool recursive);
	bool ParseAsset(string path);
	AssetObject* GetCache(ASSETTYPE type, int id);
	AssetObject* GenCache(ASSETTYPE type, int id);

	static void Compile(Editor* e);
	static void ShowPrefs(Editor* e);
	static void ShowCompileSett(Editor* e);
	static void SaveScene(Editor* e);
	static void OpenScene(Editor* e);
	static void DoOpenScene(EditorBlock* b, void* v);
	static void DeleteActive(Editor* e);
	static void DoDeleteActive(EditorBlock* b);
	static void Maximize(Editor* e);

	void DoCompile();

	struct PipeModeObj {
	public:
		uint pboLoc, //out: pbo array buffer (byte*pboCount)
			pboCount, //out: pbo array size (uint)
			hasDataLoc, //inout: pbo buffer updated? (bool)
			screenSizeLoc, //in: screen size (ushort ushort)
			okLoc; //inout: confirmation (bool)
	};

private:
	Editor(Editor const &);
	Editor& operator= (Editor const &);
};

class BlockCombo {
public:
	BlockCombo() : active(0) {}
	std::vector<EditorBlock*> blocks;
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