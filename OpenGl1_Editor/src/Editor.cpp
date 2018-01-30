#ifdef IS_EDITOR

#include "Editor.h"
#include "Engine.h"
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shellapi.h>
#include <Thumbcache.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <filesystem>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <glfw3native.h>

//#include "MD.h"
//#include "Water.h"

#pragma region UndoStack
#ifdef IS_EDITOR
UndoStack::UndoObj::UndoObj(void* loc, uint sz, uint nsz, UNDO_TYPE type, void* val, bool* dirty, string desc) :
	loc(loc), type(type), sz(sz), nsz(nsz), desc(desc), dirty(dirty) {
	switch (type) {
	case UNDO_TYPE_NORMAL: //same size, no special care needed
		this->val = new byte[sz];
		memcpy(this->val, (val) ? val : loc, sz);
		break;
	case UNDO_TYPE_STRING:
		this->val = new string(*((string*)((val) ? val : loc)));
		if (desc != "") this->desc = *((string*)(this->val)) + "->" + desc;
		break;
	}
}
UndoStack::UndoObj::UndoObj(void* loc, void* val, uint sz, callbackFunc func, void* funcVal) :
	loc(loc), type(UNDO_TYPE_ASSET), sz(sz), desc(""), func(func), funcVal(funcVal) {
	this->val = new byte[sz];
	memcpy(this->val, val, sz);
}

UndoStack::UndoObj::~UndoObj() {
	switch (type) {
	case UNDO_TYPE_NORMAL:
	case UNDO_TYPE_ASSET:
		delete[](val);
		break;
	case UNDO_TYPE_STRING:
		delete((string*)val);
		break;
	}
}

void UndoStack::UndoObj::Apply() {
	switch (type) {
	case UNDO_TYPE_NORMAL:
		memcpy(loc, val, sz);
		break;
	case UNDO_TYPE_STRING:
		*((string*)loc) = *((string*)val);
		break;
	case UNDO_TYPE_ASSET:
		memcpy(loc, val, sz);
		if (func) (*func)(funcVal);
		break;
	}
	if (dirty) *dirty = true;
}

byte UndoStack::maxStack = 30;
std::vector<UndoStack::UndoObj*> UndoStack::stack = {};
std::vector<UndoStack::UndoObj*> UndoStack::rstack = {};
byte UndoStack::esp = 0;

void UndoStack::Init() {
	stack.reserve(maxStack);
	rstack.reserve(maxStack);
}

void UndoStack::Add(UndoObj* obj) {
	if (esp < stack.size()) {
		for (int a = stack.size() - 1; a >= esp; a--) delete(stack[a]);
		stack.resize(esp);
	}
	if (stack.size() == maxStack) {
		delete(stack[0]);
		stack.erase(stack.begin());
	}

	else esp++;
	stack.push_back(obj);
	rstack.clear();
}

void UndoStack::Undo(byte cnt) {
	if (!esp) return;
	for (byte a = 0; a < cnt; a++) {
		esp--;
		stack[esp]->Apply();
		if (!esp) return;
	}
}
void UndoStack::Redo(byte cnt) {

}
#endif
#pragma endregion


HWND Editor::hwnd = 0, Editor::hwnd2 = 0;
Editor* Editor::instance = nullptr;
string Editor::dataPath = "";
bool Editor::onFocus;

Vec4 grey1() {
	return Vec4(33.0f / 255, 37.0f / 255, 40.0f / 255, 1);
}
Vec4 grey2() {
	return Vec4(61.0f / 255, 68.0f / 255, 73.0f / 255, 1);
}
Vec4 headerCol() {
	return Vec4(223.0f / 255, 119.0f / 255, 4.0f / 255, 1);
}
Vec4 hlCol() {
	return Vec4(0, 120.0f / 255, 215.0f / 255, 1);
}

float Dw2(float f) {
	return (f / Display::width);
}
float Dh2(float f) {
	return (f / Display::height);
}
Vec3 Ds2(Vec3 v) {
	return Vec3(Dw2(v.x) * 2 - 1, 1 - Dh2(v.y) * 2, 1);
}

int GetShortcutInt(InputKey key, InputKey mod1, InputKey mod2, InputKey mod3) {
	int i = (byte)key << 4; //kkkkmcas (m=usemod?)
	if (mod1 == Key_None)
		return i;
	else {
		i |= 8;
		if ((mod1 == Key_Control) || (mod2 == Key_Control) || (mod3 == Key_Control)) {
			i |= 4;
		}
		if ((mod1 == Key_Alt) || (mod2 == Key_Alt) || (mod3 == Key_Alt)) {
			i |= 2;
		}
		if ((mod1 == Key_Shift) || (mod2 == Key_Shift) || (mod3 == Key_Shift)) {
			i |= 1;
		}
		return i;
	}
}

bool ShortcutTriggered(int i, bool c, bool a, bool s) {
	if (!Input::KeyDown((InputKey)((i&(0xff << 4)) >> 4))) {
		return false;
	}
	else if ((i & 8) == 0)
		return true;
	else {
		return ((c == ((i & 4) == 4)) && (a == ((i & 2) == 2)) && (s == ((i & 1) == 1)));
	}
}

ASSETTYPE GetFormatEnum(string ext) {
	if (ext == ".mesh")
		return ASSETTYPE_MESH;
	else if (ext == ".blend")
		return ASSETTYPE_BLEND;
	else if (ext == ".material")
		return ASSETTYPE_MATERIAL;
	else if (ext == ".animclip")
		return ASSETTYPE_ANIMCLIP;
	else if (ext == ".anim")
		return ASSETTYPE_ANIMATION;
	else if (ext == ".hdr")
		return ASSETTYPE_HDRI;
	else if (ext == ".bmp" || ext == ".jpg" || ext == ".png" || ext == ".rendtex")
		return ASSETTYPE_TEXTURE;
	else if (ext == ".shade")
		return ASSETTYPE_SHADER;
	else if (ext == ".effect")
		return ASSETTYPE_CAMEFFECT;

	else return ASSETTYPE_UNDEF;
}

void DrawHeaders(Editor* e, EditorBlock* b, Vec4* v, string title) {
	e->font->Align(ALIGN_TOPLEFT);
	//Engine::Button(v->r, v->g + EB_HEADER_SIZE + 1, v->b, v->a - EB_HEADER_SIZE - 2, black(), white(0.05f), white(0.05f));
	Vec2 v2(v->b*0.1f, EB_HEADER_SIZE*0.1f);
	Engine::DrawQuad(v->r + EB_HEADER_PADDING + 1, v->g, v->b - 3 - 2 * EB_HEADER_PADDING, EB_HEADER_SIZE, e->tex_background->pointer, Vec2(), Vec2(v2.x, 0), Vec2(0, v2.y), v2, false, headerCol());
	//UI::Label(round(v->r + 5 + EB_HEADER_PADDING), round(v->g + 2), 10, titleS, e->font, black(), Display::width);
	UI::Label(v->r + 5 + EB_HEADER_PADDING, v->g, 12, title, e->font, black());
	//return Rect(v->r, v->g + EB_HEADER_SIZE + 1, v->b, v->a - EB_HEADER_SIZE - 2).Inside(Input::mousePos);
}

void CalcV(Vec4& v) {
	v.a = round(v.a - v.g) - 1;
	v.b = round(v.b - v.r) - 1;
	v.g = round(v.g) + 1;
	v.r = round(v.r) + 1;
}
std::vector<char> StreamToBuffer(std::istream* strm) {
	auto pos = strm->tellg();
	strm->seekg(strm->end);
	auto sz = strm->tellg();
	std::vector<char> buf((uint)sz);
	std::ostringstream strm2;
	strm2.rdbuf()->pubsetbuf(&buf[0], sz);
	strm->seekg(pos);
	return buf;
}

void EB_DrawScrollBar(const Vec4& v, const float& maxScroll, const float& scrollOffset) {
	if (Rect(v.r, v.g, v.b, v.a).Inside(Input::mousePos) && (maxScroll - (v.a - EB_HEADER_SIZE - 1))>0) {
		Engine::DrawQuad(v.r + v.b - 8, v.g + EB_HEADER_SIZE + 1 + scrollOffset * (v.a - EB_HEADER_SIZE - 1) / (maxScroll - EB_HEADER_SIZE - 1), 6, (v.a - EB_HEADER_SIZE - 1)*(v.a - EB_HEADER_SIZE - 1) / (maxScroll - EB_HEADER_SIZE - 1), white(0.4f));
	}
}

float PopupBlock::x() { return editor->popupPos.x; }
float PopupBlock::y() { return editor->popupPos.y; }

void EB_Empty::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "Hatena Title");
}

void EB_Debug::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "System Log");
	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	//glDisable(GL_DEPTH_TEST);
	for (int x = drawIds.size() - 1, y = 0; x >= 0; x--, y++) {
		byte t = editor->messageTypes[drawIds[x]];
		Vec4 col = (t == 0) ? white() : ((t == 1) ? yellow() : red());
		Engine::DrawQuad(v.r + 1, v.g + v.a - 36 - y * 15 + scrollOffset, v.b - 2, 14, Vec4(1, 1, 1, ((x & 1) == 1) ? 0.2f : 0.1f));
		UI::Label(v.r + 3, v.g + v.a - 36 - y * 15 + scrollOffset, 12, editor->messages[drawIds[x]].first + " says: " + editor->messages[drawIds[x]].second, editor->font, col);
	}
	if (Engine::EButton((editor->editorLayer == 0), v.r + 1, v.g + v.a - 21, 80, 20, grey1(), "Messages", 12, editor->font, drawM ? white() : grey2()) == MOUSE_RELEASE) {
		drawM = !drawM;
		Refresh();
	}
	if (Engine::EButton((editor->editorLayer == 0), v.r + 82, v.g + v.a - 21, 80, 20, grey1(), "Warnings", 12, editor->font, drawW ? white() : grey2()) == MOUSE_RELEASE) {
		drawW = !drawW;
		Refresh();
	}
	if (Engine::EButton((editor->editorLayer == 0), v.r + 163, v.g + v.a - 21, 80, 20, grey1(), "Errors", 12, editor->font, drawE ? white() : grey2()) == MOUSE_RELEASE) {
		drawE = !drawE;
		Refresh();
	}
	if (Engine::EButton((editor->editorLayer == 0), v.r + v.b - 100, v.g + v.a - 21, 80, 20, grey1(), "Clear", 12, editor->font, white()) == MOUSE_RELEASE) {
		editor->ClearLogs();
	}
	Engine::EndStencil();
}

void EB_Debug::Refresh() {
	drawIds.clear();
	int q = 0;
	for (int i : editor->messageTypes) {
		if ((i == 0 && drawM) || (i == 1 && drawW) || (i == 2 && drawE))
			drawIds.push_back(q);
		q++;
	}
	maxScroll = q*17.0f;
	scrollOffset = 0;
}

void EB_Debug::OnMouseScr(bool up) {
	scrollOffset += up ? 17 : -17;
	scrollOffset = Clamp(scrollOffset, 0.0f, max(maxScroll - (Display::height*(editor->yPoss[y2] - editor->yPoss[y1]) - EB_HEADER_SIZE - 1), 0.0f));
}


std::unordered_map<string, consoleFunc> EB_Console::funcs;

EB_Console::EB_Console(Editor* e, int x1, int y1, int x2, int y2) {
	editorType = 20;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	if (funcs.size() == 0) InitFuncs();
}

void EB_Console::Put(string s) {
	outputs.push_back(s);
	outputCount++;
}

void EB_Console::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "System Console");
	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	Engine::DrawQuad(v.r, v.g, v.b, v.a - 17, black());

	for (uint c = outputCount; c > 0; c--) {
		UI::Label(v.r + 2, v.g + v.a - 17 * (2 + outputCount - c), 12, outputs[c - 1], editor->font, white());
	}


	if (Rect(v).Inside(Input::mousePos)) {
		if (Input::KeyDown(Key_Enter)) {
			auto ss = string_split(input, ' ');
			if (ss.size() > 0) {
				bool hasFunc = false;
				for (auto& f : funcs) {
					if (f.first == ss[0]) {
						(*f.second)(ss.size() > 1 ? ss[1] : "");
						hasFunc = true;
					}
				}
				Put("> " + input + (hasFunc ? "" : ": invalid command!"));
			}
			input = "";
		}
		else if (Input::KeyDown(Key_Escape)) {
			input = "";
		}
		else if (Input::KeyDown(Key_Backspace)) {
			input = input.substr(0, input.length() - 1);
		}
		else input += Input::inputString;
	}
	Engine::DrawQuad(v.r, v.g + v.a - 16, v.b, 16, grey1());
	UI::Label(v.r + 2, v.g + v.a - 15, 12, input, editor->font, white());
	Engine::EndStencil();
}

void EBH_DrawItem(pSceneObject sc, Editor* e, Vec4* v, int& i, const float& offset, int indent) {
	int xo = indent * 20;
	if (Engine::EButton((e->editorLayer == 0), v->r + xo, v->g + EB_HEADER_SIZE + 1 + 17 * i + offset, v->b - xo, 16, grey2()) == MOUSE_RELEASE) {
		e->selected(sc);
		e->selectGlobal = false;
		e->DeselectFile();
	}
	UI::Label(v->r + 19 + xo, v->g + EB_HEADER_SIZE + 1 + 17 * i + offset, 12, sc->name, e->font, white());
	i++;
	if (sc->childCount > 0) {
		if (Engine::EButton((e->editorLayer == 0), v->r + xo, v->g + EB_HEADER_SIZE + 1 + 17 * (i - 1) + offset, 16, 16, sc->_expanded ? e->tex_collapse : e->tex_expand, white(), white(), white(1, 0.6f)) == MOUSE_RELEASE) {
			sc->_expanded = !sc->_expanded;
		}
	}
	if (e->selected() == sc) {
		Engine::DrawQuad(v->r + xo, v->g + EB_HEADER_SIZE + 1 + 17 * (i - 1) + offset, v->b - xo, 16, white(0.3f));
	}
	if (sc->childCount > 0 && sc->_expanded) {
		for (auto& scc : sc->children)
		{
			EBH_DrawItem(scc, e, v, i, offset, indent + 1);
		}
	}
}

void EB_Hierarchy::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "Scene Hierarchy");

	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	glDisable(GL_DEPTH_TEST);
	if (editor->playSyncer.status == Editor_PlaySyncer::EPS_Offline) {
		if (editor->sceneLoaded()) {
			if (Engine::EButton((editor->editorLayer == 0), v.r, v.g + EB_HEADER_SIZE + 1 - scrollOffset, v.b, 16, Vec4(0.2f, 0.2f, 0.4f, 1)) == MOUSE_RELEASE) {
				editor->selected.clear();
				editor->selectGlobal = true;
				editor->selectedFileType = ASSETTYPE_UNDEF;
			}
			UI::Label(v.r + 19, v.g + EB_HEADER_SIZE + 1 - scrollOffset, 12, "Global", editor->font, white());
			int i = 1;
			for (auto& sc : editor->activeScene->objects)
			{
				EBH_DrawItem(sc, editor, &v, i, -scrollOffset, 0);
			}
			maxScroll = 17.0f * i;

			if (Rect(v.r, v.g, v.b, v.a).Inside(Input::mousePos) && (maxScroll - (Display::height*(editor->yPoss[y2] - editor->yPoss[y1]) - EB_HEADER_SIZE - 1)) > 0) {
				Engine::DrawQuad(v.r + v.b - 8, v.g + EB_HEADER_SIZE + 1 + scrollOffset * (v.a - EB_HEADER_SIZE - 1) / (maxScroll - EB_HEADER_SIZE - 1), 6, (v.a - EB_HEADER_SIZE - 1)*(v.a - EB_HEADER_SIZE - 1) / (maxScroll - EB_HEADER_SIZE - 1), white(0.4f));
			}
		}
	}
	else if (!!editor->playSyncer.syncedSceneSz) {
		if (Engine::EButton((editor->editorLayer == 0), v.r, v.g + EB_HEADER_SIZE + 1 - scrollOffset, v.b, 16, Vec4(0.2f, 0.2f, 0.4f, 1)) == MOUSE_RELEASE) {
			editor->selected.clear();
			//editor->selectGlobal = true;
			editor->selectedFileType = ASSETTYPE_UNDEF;
		}
		UI::Label(v.r + 19, v.g + EB_HEADER_SIZE + 1 - scrollOffset, 12, "Global", editor->font, white());
		int i = 1;
		for (auto& sc : editor->playSyncer.syncedScene)
		{
			EBH_DrawItem(sc, editor, &v, i, -scrollOffset, 0);
		}
		maxScroll = 17.0f * i;

		if (Rect(v.r, v.g, v.b, v.a).Inside(Input::mousePos) && (maxScroll - (Display::height*(editor->yPoss[y2] - editor->yPoss[y1]) - EB_HEADER_SIZE - 1)) > 0) {
			Engine::DrawQuad(v.r + v.b - 8, v.g + EB_HEADER_SIZE + 1 + scrollOffset * (v.a - EB_HEADER_SIZE - 1) / (maxScroll - EB_HEADER_SIZE - 1), 6, (v.a - EB_HEADER_SIZE - 1)*(v.a - EB_HEADER_SIZE - 1) / (maxScroll - EB_HEADER_SIZE - 1), white(0.4f));
		}
	}
	else {
		UI::Texture(v.r + v.b / 2 - 16, v.g + v.a / 2 - 16, 32, 32, editor->tex_waiting);
	}
	Engine::EndStencil();
}

void EB_Hierarchy::OnMouseScr(bool up) {
	scrollOffset += up ? -17 : 17;
	scrollOffset = Clamp(scrollOffset, 0.0f, max(maxScroll - (Display::height*(editor->yPoss[y2] - editor->yPoss[y1]) - EB_HEADER_SIZE - 1), 0.0f));
}

HICON GetHighResolutionIcon(LPTSTR pszPath)
{
	SHFILEINFOA sfi = { 0 };
	SHGetFileInfo(pszPath, -1, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);

	IImageList* imageList;
	HRESULT hResult = SHGetImageList(SHIL_EXTRALARGE, IID_IImageList, (void**)&imageList);

	if (hResult == S_OK) {
		HICON hIcon;
		hResult = imageList->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon);

		if (hResult == S_OK) {
			return hIcon;
		}
		else return nullptr;
	}
	else return nullptr;
}

EB_Browser_File::EB_Browser_File(Editor* e, string path, string nm, string fn) : path(path), thumbnail(-1), tex(nullptr), expanded(false), name(nm), fullName(path + fn) {
#ifdef IS_EDITOR
	string ext = name.substr(name.find_last_of('.') + 1, string::npos);
	canExpand = false;
	/*
	if (ext == "jpg" || ext == "bmp" || ext == "png") {
	//name = name.substr(0, name.find_last_of('.'));
	ASSETTYPE t;
	ASSETID i;
	if (e->GetAssetInfo(fullName, t, i) != ASSETTYPE_UNDEF) {
	tex = _GetCache<Texture>(t, i);
	thumbnail = 1;
	}
	}
	*/
	for (int a = e->assetIcons.size() - 1; a >= 0; a--) {
		std::stringstream ss;
		ss.str(e->assetIconsExt[a]);
		string arr;
		while (!ss.eof()) {
			ss >> arr;
			if (ext == arr) {
				thumbnail = a;
				name = name.substr(0, name.find_last_of('.'));
				if (ext == "blend") {
					canExpand = true;
					children = IO::GetFilesE(e, path + name + "_blend\\");
				}
				return;
			}
		}
	}
#endif
}

EB_Browser::EB_Browser(Editor* e, int x1, int y1, int x2, int y2, string dir) : currentDir(dir) {
	editorType = 1;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	//Refresh();

	shortcuts.emplace(GetShortcutInt(Key_A), &_AddAsset);
}

bool DrawFileRect(float w, float h, float size, EB_Browser_File* file, EditorBlock* e) {
	bool b = false;
	if (e->editor->editorLayer == 0) {
		byte bb = ((file->thumbnail >= 0) ? Engine::Button(w + 1, h + 1, size - 2, size - 2, (file->tex != nullptr) ? file->tex : e->editor->assetIcons[file->thumbnail], white(1, 0.8f), white(), white(1, 0.5f)) : Engine::Button(w + 1, h + 1, size - 2, size - 2, grey2()));
		b = bb == MOUSE_RELEASE;

		if (file->canExpand) {
			if (Engine::Button(w + size*0.5f, h + 1, size*0.5f - 1, size - 2, file->expanded ? e->editor->tex_assetCollapse : e->editor->tex_assetExpand, white((bb & MOUSE_HOVER_FLAG)>0 ? 1 : 0.5f, 0.8f), white(), white(1, 0.5f)) == MOUSE_RELEASE) {
				file->expanded = !file->expanded;
			}
		}
	}
	else {
		if (file->thumbnail >= 0)
			UI::Texture(w + 1, h + 1, size - 2, size - 2, e->editor->assetIcons[file->thumbnail]);
		else
			Engine::DrawQuad(w + 1, h + 1, size - 2, size - 2, grey2());
	}
	Engine::DrawQuad(w + 1, h + 1 + size - 25, size - 2, 23, grey2()*0.7f);
	UI::Label(w + 2, h + 1 + size - 20, 12, file->name, e->editor->font, white(), size);
	return b;
}

bool IsTextFile(const string& path) {
	std::vector<string> exts = std::vector<string>({ "cpp", "h", "txt" });
	string s = path.substr(path.find_last_of('.') + 1);
	return std::find(exts.begin(), exts.end(), s) != exts.end();
}

string ReplaceTab(const string& str) {
	string s = "";
	for (auto a = str.begin(); a != str.end(); a++) {
		if (*a == '\t') s += "    ";
		else s += *a;
	}
	return s;
}

std::vector<string> GetTexts(const string& path) {
	std::ifstream strm(path, std::ios::in);
	std::vector<string> o;
	char cc[500];
	while (!strm.eof()) {
		strm.getline(cc, 500);
		string s(cc);
		o.push_back(ReplaceTab(s));
	}
	return o;
}

void EB_Browser::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "Browser: " + currentDir);

	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	glDisable(GL_DEPTH_TEST);

	for (int y = dirs.size() - 1; y >= 0; y--) {
		if (Engine::EButton((editor->editorLayer == 0), v.r, v.g + EB_HEADER_SIZE + 1 + (16 * y), 150, 15, grey1()) == MOUSE_RELEASE) {
			if (dirs.at(y) != ".") {
				if (dirs.at(y) == "..") {
					if (currentDir.size() < (editor->projectFolder.size() + 7) || currentDir.substr(0, editor->projectFolder.size() + 7) != (editor->projectFolder + "Assets\\"))
						currentDir = currentDir.substr(0, currentDir.find_last_of('\\'));
					else
						currentDir = editor->projectFolder + "Assets\\";
				}
				else currentDir += dirs.at(y) + "\\";
				Refresh();
				return;
			}
		}
		UI::Label(v.r + 2, v.g + EB_HEADER_SIZE + (16 * y), 12, dirs.at(y), editor->font, white(), 150);
	}
	float ww = 0;
	int hh = 0;
	byte fileSize = 70;
	for (int ff = files.size() - 1; ff >= 0; ff--) {
		if (DrawFileRect(v.r + 151 + ww, v.g + EB_HEADER_SIZE + (fileSize + 1)* hh, fileSize, &files[ff], this)) {
			editor->GetAssetInfo(files[ff].fullName, editor->selectedFileType, editor->selectedFile);
			editor->selectedFileTexts.clear();
			editor->selectedFileName = files[ff].name;
			editor->selectedFilePath = files[ff].fullName;
			if (editor->selectedFileType != ASSETTYPE_UNDEF) {
				editor->selectedFileCache = _GetCache<void>(editor->selectedFileType, editor->selectedFile);
			}
			else if (IsTextFile(files[ff].fullName)) {
				editor->selectedFileTexts = GetTexts(files[ff].fullName);
			}
		}

		ww += fileSize + 1;
		if (ww > (v.b - 152 - fileSize)) {
			ww = 0;
			hh++;
			if (v.g + EB_HEADER_SIZE + (fileSize + 1)* hh > Display::height)
				break;
		}
		if (files[ff].expanded) {
			for (int fff = files[ff].children.size() - 1; fff >= 0; fff--) {
				Engine::DrawQuad(v.r + 149 + ww, v.g + EB_HEADER_SIZE + (fileSize + 1)* hh + 1, fileSize + 1.0f, fileSize - 2.0f, Vec4(1, 0.494f, 0.176f, 0.3f));
				if (DrawFileRect(v.r + 153 + ww, v.g + EB_HEADER_SIZE + (fileSize + 1)* hh + 2.0f, fileSize - 4.0f, &files[ff].children[fff], this)) {
					editor->GetAssetInfo(files[ff].children[fff].fullName, editor->selectedFileType, editor->selectedFile);
					editor->selectedFileTexts.clear();
					editor->selectedFileName = files[ff].children[fff].name;
					editor->selectedFilePath = files[ff].children[fff].fullName;
					if (editor->selectedFileType != ASSETTYPE_UNDEF) {
						editor->selectedFileCache = _GetCache<void>(editor->selectedFileType, editor->selectedFile);
					}
					else if (IsTextFile(files[ff].children[fff].fullName)) {
						editor->selectedFileTexts = GetTexts(files[ff].children[fff].fullName);
					}
				}
				ww += fileSize + 1;
				if (ww > (v.b - 152 - fileSize)) {
					ww = 0;
					hh++;
					if (v.g + EB_HEADER_SIZE + (fileSize + 1)* hh > Display::height)
						break;
				}
			}
		}
	}

	Engine::EndStencil();
}

void EB_Browser::Refresh() {
#ifdef IS_EDITOR
	dirs.clear();
	files.clear();
	IO::GetFolders(currentDir, &dirs);
	files = IO::GetFilesE(editor, currentDir);
	//editor->_Message("Browser", std::to_string(dirs.size()) + " folders and " + std::to_string(files.size()) + " files from " + currentDir);
	if (dirs.size() == 0) {
		dirs.push_back(".");
		dirs.push_back("..");
	}
#endif
}

EB_Viewer::EB_Viewer(Editor* e, int x1, int y1, int x2, int y2) : rz(45), rw(45), scale(1), fov(60), rotCenter(0, 0, 0) {
	editorType = 2;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	MakeMatrix();

	shortcuts.emplace(GetShortcutInt(Key_A, Key_Shift), &_OpenMenuAdd);
	shortcuts.emplace(GetShortcutInt(Key_C, Key_Shift), &_OpenMenuCom);
	shortcuts.emplace(GetShortcutInt(Key_W), &_OpenMenuW);
	shortcuts.emplace(GetShortcutInt(Key_Space, Key_Control), &_OpenMenuChgMani);
	shortcuts.emplace(GetShortcutInt(Key_Space, Key_Alt), &_OpenMenuChgOrient);

	shortcuts.emplace(GetShortcutInt(Key_X), &_X);
	shortcuts.emplace(GetShortcutInt(Key_Y), &_Y);
	shortcuts.emplace(GetShortcutInt(Key_Z), &_Z);

	shortcuts.emplace(GetShortcutInt(Key_A), &_SelectAll);
	shortcuts.emplace(GetShortcutInt(Key_Z), &_ViewInvis);
	//shortcuts.emplace(GetShortcutInt(Key_5), &_ViewPersp);

	shortcuts.emplace(GetShortcutInt(Key_G), &_Grab);
	shortcuts.emplace(GetShortcutInt(Key_R), &_Rotate);
	shortcuts.emplace(GetShortcutInt(Key_S), &_Scale);

	shortcuts.emplace(GetShortcutInt(Key_NumPad1), &_ViewFront);
	shortcuts.emplace(GetShortcutInt(Key_NumPad1, Key_Control), &_ViewBack);
	shortcuts.emplace(GetShortcutInt(Key_NumPad3), &_ViewRight);
	shortcuts.emplace(GetShortcutInt(Key_NumPad3, Key_Control), &_ViewLeft);
	shortcuts.emplace(GetShortcutInt(Key_NumPad5), &_TogglePersp);
	shortcuts.emplace(GetShortcutInt(Key_NumPad7), &_ViewTop);
	shortcuts.emplace(GetShortcutInt(Key_NumPad7, Key_Control), &_ViewBottom);
	shortcuts.emplace(GetShortcutInt(Key_NumPad0), &_ViewCam);
	shortcuts.emplace(GetShortcutInt(Key_NumPad0, Key_Alt), &_SnapCenter);

	shortcuts.emplace(GetShortcutInt(Key_Escape), &_Escape);
}

void EB_Viewer::MakeMatrix() {
	float csz = cos(rz*deg2rad);
	float snz = sin(rz*deg2rad);
	float csw = cos(rw*deg2rad);
	float snw = sin(rw*deg2rad);
	viewingMatrix = Mat4x4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
	viewingMatrix = Mat4x4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1)*viewingMatrix;
	arrowX = viewingMatrix*Vec4(-1, 0, 0, 0);
	arrowY = viewingMatrix*Vec4(0, 1, 0, 0);
	arrowZ = viewingMatrix*Vec4(0, 0, 1, 0);
}

Vec3 xyz(Vec4 v) {
	return Vec3(v.x, -v.y, v.z);
}
Vec2 xy(Vec3 v) {
	return Vec2(v.x, v.y);
}

void EB_Viewer::DrawSceneObjectsOpaque(EB_Viewer* ebv, const std::vector<pSceneObject>& oo) {
	for (auto& sc : oo)
	{
		glPushMatrix();
		glMultMatrixf(glm::value_ptr(sc->transform._localMatrix));
		for (auto& com : sc->_components)
		{
			if (com->componentType == COMP_MRD || com->componentType == COMP_SRD || com->componentType == COMP_CAM)
				com->DrawEditor(ebv);
		}
		DrawSceneObjectsOpaque(ebv, sc->children);

		if (sc == ebv->editor->selected()) {
			GLfloat matrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
			ebv->editor->selectedMvMatrix = (Mat4x4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));
		}

		glPopMatrix();
	}
}

void EB_Viewer::DrawSceneObjectsGizmos(EB_Viewer* ebv, const std::vector<pSceneObject>& oo) {
	for (auto& psc : oo)
	{
		SceneObject* sc = psc.get();
		glPushMatrix();
		//Vec3 v = sc->transform.position;
		//Vec3 vv = sc->transform.scale;
		//Quat vvv = sc->transform.rotation();
		//glTranslatef(v.x - v2.x, v.y - v2.y, v.z - v2.z);
		//glTranslatef(v.x, v.y, v.z);
		//glScalef(vv.x, vv.y, vv.z);
		//glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(vvv)));
		glMultMatrixf(glm::value_ptr(sc->transform._localMatrix));
		for (auto& com : sc->_components)
		{
			if (com->componentType != COMP_MRD && com->componentType != COMP_CAM)
				com->DrawEditor(ebv);
		}
		DrawSceneObjectsGizmos(ebv, sc->children);
		glPopMatrix();
	}
}

void EB_Viewer::DrawSceneObjectsTrans(EB_Viewer* ebv, const std::vector<pSceneObject>& oo) {

}

void EB_Viewer::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "Viewer: SceneNameHere");

	//Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	glViewport((int)v.r, (int)(Display::height - v.g - v.a), (int)v.b, (int)(v.a - EB_HEADER_SIZE - 2));

	Vec2 v2 = Vec2(Display::width, Display::height)*0.03f;
	Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, white(1, 0.2f));//editor->checkers->pointer, Vec2(), Vec2(v2.x, 0), Vec2(0, v2.y), v2, true, white(0.05f));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float ww1 = editor->xPoss[x1];
	float hh1 = editor->yPoss[y1];
	float ww = editor->xPoss[x2] - ww1;
	float hh = editor->yPoss[y2] - hh1;
	float h40 = 40 * (hh*Display::height) / (ww*Display::width);
	float mww = max(ww, 0.3f) * (float)pow(2, scale);
	if (seeingCamera == nullptr) {
		//glTranslatef(0, 0, 1);
		glScalef(-mww*Display::width / v.b, mww*Display::width / v.a, 1);
		if (persp) glMultMatrixf(glm::value_ptr(glm::perspectiveFov(fov * deg2rad, (float)Display::width, (float)Display::width, 0.01f, 1000.0f)));
		else glMultMatrixf(glm::value_ptr(glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.01f, 1000.0f)));
		//glScalef(-mww*Display::width / v.b, mww*Display::width / v.a, 1);
		glTranslatef(0, 0, -20);
	}
	else {
		Quat q = glm::inverse(seeingCamera->object->transform.rotation());
		glScalef(scale, -scale, 1);
		glMultMatrixf(glm::value_ptr(glm::perspectiveFov(seeingCamera->fov * deg2rad, (float)Display::width, (float)Display::height, seeingCamera->nearClip, seeingCamera->farClip)));
		glScalef(1, -1, -1);
		glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(q)));
		Vec3 pos = -seeingCamera->object->transform.position();
		glTranslatef(pos.x, pos.y, pos.z);
	}
	glPushMatrix();
	if (seeingCamera == nullptr) {
		float csz = cos(-rz*deg2rad);
		float snz = sin(-rz*deg2rad);
		float csw = cos(rw*deg2rad);
		float snw = sin(rw*deg2rad);
		Mat4x4 mMatrix = Mat4x4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1) * Mat4x4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
		glMultMatrixf(glm::value_ptr(mMatrix));
		glTranslatef(-rotCenter.x, -rotCenter.y, -rotCenter.z);
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	glClearDepth(1);

	//get matrix
	GLfloat matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, matrix);
	projMatrix = (Mat4x4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));

	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	Mat4x4 tmpMat = (Mat4x4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));
	invMatrix = glm::inverse(projMatrix * tmpMat);

	if (editor->playSyncer.status == Editor_PlaySyncer::EPS_Offline) {
		if (editor->sceneLoaded()) {
			//draw scene
			glDepthFunc(GL_LEQUAL);
			glDepthMask(true);
			DrawSceneObjectsOpaque(this, editor->activeScene->objects);
			DrawSceneObjectsGizmos(this, editor->activeScene->objects);
			glDepthMask(false);
			DrawSceneObjectsTrans(this, editor->activeScene->objects);

			/*draw background
			glDepthFunc(GL_EQUAL);
			if (editor->activeScene->settings.sky != nullptr) {

			}
			glDepthFunc(GL_LEQUAL);
			*/
		}
	}
	else if (!!editor->playSyncer.syncedSceneSz) {

	}

	//draw grid
	if (editor->_showGrid) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, &editor->grid[0]);
		glLineWidth(0.5f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor4f(0.3f, 0.3f, 0.3f, 1.0f);
		glDrawElements(GL_LINES, 64, GL_UNSIGNED_INT, &editor->gridId[0]);
		glColor4f(1, 0, 0, 1.0f);
		glLineWidth(1);
		glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, &editor->gridId[64]);
		glColor4f(0, 0, 1, 1.0f);
		glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, &editor->gridId[66]);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	//draw tooltip
	auto sel = editor->selected();
	if (sel) {
		if (modifying == 0) {
			Vec3 wpos = sel->transform.position();
			if (selectedTooltip == 0) DrawTArrows(wpos, 2);
			else if (selectedTooltip == 1) DrawRArrows(wpos, 2);
			else DrawSArrows(wpos, 2);
		}
		else {
			byte bb = modifying & 0xf0;
			if (bb == 0x10)
				Engine::DrawLineW(sel->transform.position() + modAxisDir*-100000.0f, sel->transform.position() + modAxisDir*100000.0f, white(), 2);
			else if (bb == 0x20) {
				bb = modifying & 0x0f;
				if (bb == 0) {
					Vec4 rx = invMatrix*Vec4(1, 0, 0, 0);
					Vec4 ry = invMatrix*Vec4(0, 1, 0, 0);
					Engine::DrawCircleW(sel->transform.position(), Normalize(Vec3(rx.x, rx.y, rx.z)), Normalize(Vec3(ry.x, ry.y, ry.z)), 2 / scale, 32, white(), 2);
				}
				else if (bb == 1)
					Engine::DrawCircleW(sel->transform.position(), Vec3(0, 1, 0), Vec3(0, 0, 1), 2 / scale, 32, white(), 2);
				else if (bb == 2)
					Engine::DrawCircleW(sel->transform.position(), Vec3(1, 0, 0), Vec3(0, 0, 1), 2 / scale, 32, white(), 2);
				else
					Engine::DrawCircleW(sel->transform.position(), Vec3(0, 1, 0), Vec3(1, 0, 0), 2 / scale, 32, white(), 2);
			}
			else if ((modifying & 0x0f) != 0)
				Engine::DrawLineW(sel->transform.position() + modAxisDir*-100000.0f, sel->transform.position() + modAxisDir*100000.0f, white(), 2);
		}
	}
	glDepthFunc(GL_ALWAYS);

	//Color::DrawPicker(150, 50, editor->cc);
	glPopMatrix();

	/*
	MD::me->Update();

	glBindBuffer(GL_ARRAY_BUFFER, MD::me->psb->pointer);
	glUseProgram(0);

	glTranslatef(-MD::me->wall / 2, -MD::me->wall / 2, -MD::me->wall / 2);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPointSize(4);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, 16, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glColorPointer(4, GL_FLOAT, 0, MD::me->colors);
	glDrawArrays(GL_POINTS, 0, MD::me->particlecount);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	Engine::DrawCubeLinesW(0, MD::me->wall, 0, MD::me->wall, 0, MD::me->wall, 1, white());
	*/

#ifdef _WATERY
	Water::me->Update();
	Water::me->Draw();
#endif

	//Engine::DrawMeshInstanced(_GetCache<Mesh>(ASSETTYPE_MESH, 2), 0, _GetCache<Material>(ASSETTYPE_MATERIAL, 0), 10000);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glMultMatrixf(glm::value_ptr(glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -100.0f, 100.0f)));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (editor->selected()) {
		editor->selectedSpos = projMatrix*editor->selectedMvMatrix*Vec4(0, 0, 0, 1);
		editor->selectedSpos /= editor->selectedSpos.w;
		Engine::DrawCircleW(Vec3(editor->selectedSpos.x, editor->selectedSpos.y, 0), Vec3(1.0f / Display::width, 0, 0), Vec3(0, 1.0f / Display::height, 0), 20, 24, white(), 2, true);
		if (modifying == 0) {
			if (selectedTooltip == 1) {
				Engine::DrawCircleW(Vec3(editor->selectedSpos.x, editor->selectedSpos.y, 0), Vec3(1.0f / Display::width, 0, 0), Vec3(0, 1.0f / Display::height, 0), 140, 24, yellow(), 2);
			}
		}
		else {
			Engine::DrawLineWDotted(Vec3(editor->selectedSpos.x, editor->selectedSpos.y, 0), Vec3((Input::mousePos.x - v.r) / v.b * 2 - 1, -((Input::mousePos.y - v.g - EB_HEADER_SIZE - 1) / (v.a - EB_HEADER_SIZE - 1) * 2 - 1), 0), white(1, 0.1f), 1, 12.0f / Display::height);
		}
	}


	//Engine::EndStencil();
	glViewport(0, 0, Display::width, Display::height);

	if (!editor->playSyncer.syncedSceneSz && (editor->playSyncer.status != Editor_PlaySyncer::EPS_Offline)) {
		UI::Texture(v.r + v.b / 2 - 16, v.g + v.a / 2 - 16, 32, 32, editor->tex_waiting);
	}

	Vec3 origin(30 + v.r, v.a + v.g - 30, 10);
	Vec2 axesLabelPos;

	Engine::DrawLine(origin, origin + xyz(arrowX*20.0f), red(), 2); //v->r, v->g + EB_HEADER_SIZE + 1, v->b, v->a - EB_HEADER_SIZE - 2, black(), white(0.05f), white(0.05f));
	axesLabelPos = xy(origin + xyz(arrowX*20.0f)) + Vec2((arrowX.x > 0) ? 5 : -8, -5);
	UI::Label(axesLabelPos.x, axesLabelPos.y, 10, "X", editor->font, red());
	Engine::DrawLine(origin, origin + xyz(arrowY*20.0f), green(), 2);
	axesLabelPos = xy(origin + xyz(arrowY*20.0f)) + Vec2((arrowY.x > 0) ? 5 : -8, -5);
	UI::Label(axesLabelPos.x, axesLabelPos.y, 10, "Y", editor->font, green());
	Engine::DrawLine(origin, origin + xyz(arrowZ*20.0f), blue(), 2);
	axesLabelPos = xy(origin + xyz(arrowZ*20.0f)) + Vec2((arrowZ.x > 0) ? 5 : -8, -5);
	UI::Label(axesLabelPos.x, axesLabelPos.y, 10, "Z", editor->font, blue());

	byte drawDescLT;
	string descLabelLT;

	drawDescLT = 0;
	if (Engine::EButton((editor->editorLayer == 0), v.x + 5, v.y + EB_HEADER_SIZE + 10, 20, 20, editor->tooltipTexs[selectedTooltip], white(0.7f), white(), white(1, 0.5f)) && MOUSE_HOVER_FLAG) {
		drawDescLT = 1;
		switch (selectedTooltip) {
		case 0:
			descLabelLT = "Selected Axes: Transform (Ctrl-Space)";
			break;
		case 1:
			descLabelLT = "Selected Axes: Rotate (Ctrl-Space)";
			break;
		case 2:
			descLabelLT = "Selected Axes: Scale (Ctrl-Space)";
			break;
		}
	}
	if (Engine::EButton((editor->editorLayer == 0), v.x + 5, v.y + EB_HEADER_SIZE + 32, 20, 20, editor->shadingTexs[selectedShading], white(0.7f), white(), white(1, 0.5f)) && MOUSE_HOVER_FLAG) {
		drawDescLT = 2;
		switch (selectedShading) {
		case 0:
			descLabelLT = "Shading: Solid(Z)";
			break;
		case 1:
			descLabelLT = "Shading: Wire(Z)";
			break;
		}
	}
	if (Engine::EButton((editor->editorLayer == 0), v.x + 5, v.y + EB_HEADER_SIZE + 54, 20, 20, editor->orientTexs[selectedOrient], white(0.7f), white(), white(1, 0.5f)) && MOUSE_HOVER_FLAG) {
		drawDescLT = 3;
		switch (selectedOrient) {
		case 0:
			descLabelLT = "Selected Orientation: Global (Alt-Space)";
			break;
		case 1:
			descLabelLT = "Selected Orientation: Local (Alt-Space)";
			break;
		case 2:
			descLabelLT = "Selected Orientation: View (Alt-Space)";
			break;
		}
	}

	if (drawDescLT > 0) {
		Engine::DrawQuad(v.x + 28, v.y + EB_HEADER_SIZE + 10 + 22 * (drawDescLT - 1), 282, 20, white(1, 0.6f));
		UI::Label(v.x + 30, v.y + EB_HEADER_SIZE + 15 + 22 * (drawDescLT - 1), 12, descLabelLT, editor->font, black());
	}

	if (editor->_showDebugInfo) {
		UI::Label(v.x + 50, v.y + 30, 12, "z=" + std::to_string(rz) + " w = " + std::to_string(rw), editor->font, white());
		UI::Label(v.x + 50, v.y + 55, 12, "fov=" + std::to_string(fov) + " scale=" + std::to_string(scale), editor->font, white());
		//Vec4 r = invMatrix * Vec4(1, 0, 0, 0);
		UI::Label(v.x + 50, v.y + 80, 12, "center=" + std::to_string(rotCenter), editor->font, white());
	}
}

const int EB_Viewer::arrowTIndexs[18] = { 0, 1, 2, 0, 2, 3, 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4 };
const int EB_Viewer::arrowSIndexs[18] = { 0, 1, 2, 0, 2, 3, 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4 };

#define _rot(vec) _wr*vec
void EB_Viewer::DrawTArrows(Vec3 pos, float size) {
	float s = size / (float)pow(2, scale);
	auto _wr = editor->selected->transform.rotation();
	glDepthFunc(GL_ALWAYS);
	Engine::DrawLineW(pos, pos + _rot(Vec3(s, 0, 0)), red(), 3);
	Engine::DrawLineW(pos, pos + _rot(Vec3(0, s, 0)), green(), 3);
	Engine::DrawLineW(pos, pos + _rot(Vec3(0, 0, s)), blue(), 3);


	float ds = s * 0.07f;
	arrowVerts[0] = pos + _rot(Vec3(s, ds, ds));
	arrowVerts[1] = pos + _rot(Vec3(s, -ds, ds));
	arrowVerts[2] = pos + _rot(Vec3(s, -ds, -ds));
	arrowVerts[3] = pos + _rot(Vec3(s, ds, -ds));
	arrowVerts[4] = pos + _rot(Vec3(s*1.3f, 0, 0));

	arrowVerts[5] = pos + _rot(Vec3(ds, s, ds));
	arrowVerts[6] = pos + _rot(Vec3(-ds, s, ds));
	arrowVerts[7] = pos + _rot(Vec3(-ds, s, -ds));
	arrowVerts[8] = pos + _rot(Vec3(ds, s, -ds));
	arrowVerts[9] = pos + _rot(Vec3(0, s*1.3f, 0));

	arrowVerts[10] = pos + _rot(Vec3(ds, ds, s));
	arrowVerts[11] = pos + _rot(Vec3(-ds, ds, s));
	arrowVerts[12] = pos + _rot(Vec3(-ds, -ds, s));
	arrowVerts[13] = pos + _rot(Vec3(ds, -ds, s));
	arrowVerts[14] = pos + _rot(Vec3(0, 0, s*1.3f));

	glEnableClientState(GL_VERTEX_ARRAY);
	Engine::DrawIndicesI(&arrowVerts[0], &arrowTIndexs[0], 15, 1, 0, 0);
	Engine::DrawIndicesI(&arrowVerts[5], &arrowTIndexs[0], 15, 0, 1, 0);
	Engine::DrawIndicesI(&arrowVerts[10], &arrowTIndexs[0], 15, 0, 0, 1);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDepthFunc(GL_LEQUAL);
}

void EB_Viewer::DrawRArrows(Vec3 pos, float size) {
	float sz = size / (float)pow(2, scale);
	auto _wr = editor->selected->transform.rotation();
	glDepthFunc(GL_ALWAYS);
	Engine::DrawCircleW(pos, _rot(Vec3(0, 1, 0)), _rot(Vec3(0, 0, 1)), sz, 32, red(), 3);
	Engine::DrawCircleW(pos, _rot(Vec3(1, 0, 0)), _rot(Vec3(0, 0, 1)), sz, 32, green(), 3);
	Engine::DrawCircleW(pos, _rot(Vec3(0, 1, 0)), _rot(Vec3(1, 0, 0)), sz, 32, blue(), 3);
	glDepthFunc(GL_LEQUAL);
}

void EB_Viewer::DrawSArrows(Vec3 pos, float size) {
	float sz = size / (float)pow(2, scale);
	auto _wr = editor->selected->transform.rotation();
	glDepthFunc(GL_ALWAYS);
	Engine::DrawLineW(pos, pos + Vec3(sz, 0, 0), red(), 3);
	Engine::DrawLineW(pos, pos + Vec3(0, sz, 0), green(), 3);
	Engine::DrawLineW(pos, pos + Vec3(0, 0, sz), blue(), 3);


	//float s = size / pow(2, scale);
	float ds = sz * 0.07f;
	Engine::DrawCube(pos + Vec3((sz)+ds, 0, 0), ds, ds, ds, red());
	Engine::DrawCube(pos + Vec3(0, (sz)+ds, 0), ds, ds, ds, green());
	Engine::DrawCube(pos + Vec3(0, 0, (sz)+ds), ds, ds, ds, blue());
	glDepthFunc(GL_LEQUAL);
}
#undef _rot

void EB_Viewer::OnMouseM(Vec2 d) {
	if (editor->mousePressType == 1 || (editor->mousePressType == 0 && Input::KeyHold(Key_Alt))) {
		if (Input::KeyHold(Key_Shift)) {
			//float w = Display::width*(editor->xPoss[x2] - editor->xPoss[x1]);
			//float h = Display::height*(editor->yPoss[y2] - editor->yPoss[y1]);
			Vec4 r = invMatrix * Vec4(2.0f / Display::width, 0, 0, 0);
			rotCenter -= d.x * Vec3(r.x, r.y, r.z);
			r = invMatrix * Vec4(0, -2.0f / Display::height, 0, 0);
			rotCenter -= d.y * Vec3(r.x, r.y, r.z);
		}
		else {
			rz += d.x;
			rw += d.y;
			if (rz > 360)
				rz -= 360;
			else if (rz < 0)
				rz += 360;
			if (rw > 360)
				rw -= 360;
			else if (rw < 0)
				rw += 360;
			MakeMatrix();
		}
	}
	else if (modifying > 0) {
		auto& sel = editor->selected();
		float scl = (float)pow(2, scale);
		modVal += Vec2(Input::mouseDelta.x / Display::width, Input::mouseDelta.y / Display::height);
		if (modifying >> 4 == 1) {
			switch (modifying & 0x0f) {
			case 1:
				sel->transform.Translate(Vec3((Input::mouseDelta.x / Display::width) * 40 / scl, 0, 0));
				break;
			case 2:
				sel->transform.Translate(Vec3(0, (Input::mouseDelta.x / Display::width) * 40 / scl, 0));
				break;
			case 3:
				sel->transform.Translate(Vec3(0, 0, (Input::mouseDelta.x / Display::width) * 40 / scl));
				break;
			}
		}
		else if (modifying >> 4 == 2) {
			Vec4 r;
			switch (modifying & 0x0f) {
			case 0:
				r = Vec4(0, 0, -1, 0);
				r = invMatrix*r;
				sel->transform.Rotate(Normalize(Vec3(r.x, r.y, r.z))*(Input::mouseDelta.x / Display::width)*360.0f, Space_World);
				break;
			case 1:
				sel->transform.Rotate(Vec3((Input::mouseDelta.x / Display::width) * 360, 0, 0), selectedOrient == 0 ? Space_World : Space_Self);
				break;
			case 2:
				sel->transform.Rotate(Vec3(0, (Input::mouseDelta.x / Display::width) * 360, 0), selectedOrient == 0 ? Space_World : Space_Self);
				break;
			case 3:
				sel->transform.Rotate(Vec3(0, 0, (Input::mouseDelta.x / Display::width) * 360), selectedOrient == 0 ? Space_World : Space_Self);
				break;
			}
		}
		else {
			switch (modifying & 0x0f) {
			case 0:
				sel->transform.localScale(Vec3(preModVals + Vec3(1, 1, 1)*((modVal.x) * 40 / scl)));
				break;
			case 1:
				sel->transform.localScale(Vec3(preModVals + Vec3((modVal.x) * 40 / scl, 0, 0)));
				break;
			case 2:
				sel->transform.localScale(Vec3(preModVals + Vec3(0, (modVal.x) * 40 / scl, 0)));
				break;
			case 3:
				sel->transform.localScale(Vec3(preModVals + Vec3(0, 0, (modVal.x) * 40 / scl)));
				break;
			}
		}
	}
}

void EB_Viewer::OnMousePress(int i) {
	if (modifying > 0) {
		auto& sel = editor->selected();
		if (i != 0) {
			switch (modifying >> 4) {
			case 1:
				sel->transform.localPosition(preModVals);
				break;
			case 2:
				sel->transform.localEulerRotation(preModVals);
				break;
			case 3:
				sel->transform.localScale(preModVals);
				break;
			}
		}
		sel->transform._UpdateLMatrix();
		modifying = 0;
	}
}

void EB_Viewer::OnMouseScr(bool up) {
	if (Input::KeyHold(Key_Alt)) {
		fov += (up ? 5 : -5);
		fov = min(max(fov, 1.0f), 179.0f);
	}
	else {
		scale += (up ? 0.1f : -0.1f);
		scale = min(max(scale, -10.0f), 10.0f);
	}
}

EB_Inspector::EB_Inspector(Editor* e, int x1, int y1, int x2, int y2) : label("") {
	editorType = 3;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	lock = false;
}

/*
void EB_Inspector::SelectAsset(EBI_Asset* e, string s) {
if (loaded && isAsset)
delete (obj);
obj = e;
isAsset = true;
label = s;
if (obj->correct)
loaded = true;
}
*/

void EB_Inspector::DrawObj(Vec4 v, Editor* editor, EB_Inspector* b, SceneObject* o) {
	UI::Texture(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 18, 18, editor->tex_object);
	o->name = UI::EditText(v.r + 20, v.g + 2 + EB_HEADER_SIZE, v.b - 21, 18, 12, grey2(), o->name, editor->font, true, nullptr, white());

	//TRS (don't chain them together or the latter may be skipped)
	bool chg = b->DrawVector3(editor, v, 21, "Position", o->transform._localPosition, &o->transform.dirty);
	chg |= b->DrawVector3(editor, v, 38, "Rotation", o->transform._localEulerRotation, &o->transform.dirty);
	chg |= b->DrawVector3(editor, v, 55, "Scale", o->transform._localScale, &o->transform.dirty);
	if (chg || o->transform.dirty) {
		o->transform._L2WPos();
		o->transform._UpdateLQuat();
		o->transform._UpdateLMatrix();
	}
	//draw components
	uint off = 74 + EB_HEADER_SIZE;
	for (auto& c : o->_components)
	{
		c->DrawInspector(editor, c.get(), v, off);
		if (!!(editor->flags & WAITINGREFRESHFLAG)) //deleted
			return;
	}
}

void EBI_DrawAss_Tex(Vec4 v, Editor* editor, EB_Inspector* b, float &off) {
	Texture* tex = (Texture*)editor->selectedFileCache;
	float sz = min(v.b - 2.0f, Clamp((float)max(tex->width, tex->height), 16.0f, (float)editor->_maxPreviewSize));
	UI::Texture(v.r + 1 + 0.5f*(v.b - sz), off + 15, sz, sz, tex, DRAWTEX_FIT, b->previewMip);
	if (tex->_mipmap) {
		UI::Texture(v.r + 2, off + sz + 17, 16, 16, editor->tex_mipLow);
		UI::Texture(v.r + v.b - 18, off + sz + 17, 16, 16, editor->tex_mipHigh);
		b->previewMip = Engine::DrawSliderFill(v.r + 19, off + sz + 17, v.b - 37, 16, 0, 6, b->previewMip, grey2(), white());
		off += 17;
	}
	tex->_mipmap = Engine::Toggle(v.r + 2, off + sz + 17, 14, editor->tex_checkbox, tex->_mipmap, white(), ORIENT_HORIZONTAL);
	UI::Label(v.r + 18, off + sz + 18, 12, "Use Mipmaps", editor->font, white());
	if (tex->_mipmap) {
		tex->_blurmips = Engine::Toggle(v.r + v.b*0.5f, off + sz + 17, 14, editor->tex_checkbox, tex->_blurmips, white(), ORIENT_HORIZONTAL);
		UI::Label(v.r + v.b*0.5f + 16, off + sz + 18, 12, "Blur", editor->font, white());
	}
	UI::Label(v.r + 18, off + sz + 33, 12, "Filtering", editor->font, white());
	std::vector<string> filterNames = { "Point", "Bilinear", "Trilinear" };
	if (Engine::EButton(editor->editorLayer == 0, v.r + v.b * 0.3f, off + sz + 33, v.b * 0.6f - 1, 14, grey2(), filterNames[tex->_filter], 12, editor->font, white()) == MOUSE_PRESS) {
		editor->RegisterMenu(b, "", filterNames, { &b->_ApplyTexFilter0, &b->_ApplyTexFilter1, &b->_ApplyTexFilter2 }, 0, Vec2(v.r + v.b * 0.3f, off + sz + 33));
	}
	UI::Label(v.r + 18, off + sz + 49, 12, "Aniso Level", editor->font, white());
	Engine::DrawQuad(v.r + v.b * 0.3f, off + sz + 48, v.b * 0.1f - 1, 14, grey2());
	UI::Label(v.r + v.b * 0.3f + 2, off + sz + 49, 12, std::to_string(tex->_aniso), editor->font, white());
	tex->_aniso = (byte)round(Engine::DrawSliderFill(v.r + v.b * 0.4f, off + sz + 48, v.b*0.7f - 1, 14, 0, 10, (float)tex->_aniso, grey2(), white()));
	off += sz + 63;
}

void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off) {
	Material* mat = (Material*)editor->selectedFileCache;
	UI::Label(v.r + 18, off + 17, 12, "Shader", editor->font, white());
	editor->DrawAssetSelector(v.r + v.b*0.25f, off + 15, v.b*0.75f - 1, 16, grey1(), ASSETTYPE_SHADER, 12, editor->font, &mat->_shaderId, &EB_Inspector::_ApplyMatShader, mat);
	off += 34;
	for (uint q = 0, qq = mat->valOrders.size(); q < qq; q++) {
		int r = 0;
		UI::Texture(v.r + 2, off, 16, 16, editor->matVarTexs[mat->valOrders[q]]);
		UI::Label(v.r + 19, off + 2, 12, mat->valNames[mat->valOrders[q]][mat->valOrderIds[q]], editor->font, white());
		void* bbs = mat->vals[mat->valOrders[q]][mat->valOrderGLIds[q]];
		assert(bbs != nullptr);
		switch (mat->valOrders[q]) {
		case SHADER_INT:
			Engine::Button(v.r + v.b * 0.3f + 17, off, v.b*0.7f - 17, 16, grey1(), std::to_string(*(int*)bbs), 12, editor->font, white());
			break;
		case SHADER_FLOAT:
			Engine::Button(v.r + v.b * 0.3f + 17, off, v.b*0.7f - 17, 16, grey1(), std::to_string(*(float*)bbs), 12, editor->font, white());
			break;
		case SHADER_SAMPLER:
			ASSETID* id = &ASSETID(((MatVal_Tex*)bbs)->id);
			float ti = 32;// max(min(ceil(v.b*0.3f - 12), 64), 32);
			editor->DrawAssetSelector(v.r + 19, off + 16, v.b - ti - 21, 16, grey1(), ASSETTYPE_TEXTURE, 12, editor->font, id, &Material::_UpdateTexCache, mat);
			if (*id > -1)
				UI::Texture(v.r + v.b - ti - 1, off, ti, ti, _GetCache<Texture>(ASSETTYPE_TEXTURE, *id));
			else
				Engine::DrawQuad(v.r + v.b - ti - 1, off, ti, ti, grey1());
			off += ti - 16;
			break;
		}
		r++;
		off += 17;
	}
	//*/
}

void EBI_DrawAss_Hdr(Vec4 v, Editor* editor, EB_Inspector* b, float &off) {
	Background* tex = (Background*)editor->selectedFileCache;
	float sz = min(v.b - 2.0f, Clamp((float)max(tex->width, tex->height), 16.0f, (float)editor->_maxPreviewSize));
	float x = v.r + 1 + 0.5f*(v.b - sz);
	float y = off + 15;
	float w2h = ((float)tex->width) / tex->height;
	Engine::DrawQuad(x, y, sz, sz / w2h, (tex->loaded) ? tex->pointer : Engine::fallbackTex->pointer, b->previewMip);
	UI::Texture(v.r + 2, off + sz + 17, 16, 16, editor->tex_mipLow);
	UI::Texture(v.r + v.b - 18, off + sz + 17, 16, 16, editor->tex_mipHigh);
	b->previewMip = Engine::DrawSliderFill(v.r + 19, off + sz + 17, v.b - 37, 16, 0, 6, b->previewMip, grey2(), white());
	off += sz + 63;
}

void EBI_DrawAss_Eff(Vec4 v, Editor* editor, EB_Inspector* b, float &off) {
	CameraEffect* eff = (CameraEffect*)editor->selectedFileCache;
	UI::Label(v.r + 18, off + 17, 12, "Material", editor->font, white());
	editor->DrawAssetSelector(v.r + v.b*0.25f, off + 15, v.b*0.75f - 1, 16, grey2(), ASSETTYPE_MATERIAL, 12, editor->font, &eff->_material, &EB_Inspector::_ApplyEffMat, eff);
	off += 34;
}

void EBI_DrawAss_Txt(Vec4 v, Editor* editor, EB_Inspector* b, float off) {
	uint sz = editor->selectedFileTexts.size();
	Engine::DrawQuad(v.r + 2.0f, off, v.b - 4.0f, 14.0f*sz + 4.0f, black(0.8f));
	for (uint a = 0; a < sz; a++) {
		UI::Label(v.r + 4, off + a * 14 + 2, 12, editor->selectedFileTexts[a], editor->font, white());
	}
}

void EB_Inspector::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "Inspector");// isAsset ? (loaded ? label : "No object selected") : nm);
	string nm;
	if (lock)
		nm = (lockGlobal == 1) ? lockedObj()->name : (lockGlobal == 2) ? "Scene settings" : "No object selected";
	else
		nm = (editor->selected()) ? editor->selected->name : editor->selectGlobal ? "Scene settings" : "No object selected";
	lock = Engine::Toggle(v.r + v.b - EB_HEADER_PADDING - EB_HEADER_SIZE - 2, v.g, EB_HEADER_SIZE, editor->tex_keylock, lock, lock ? Vec4(1, 1, 0, 1) : white(0.5f), ORIENT_HORIZONTAL);
	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	if (!lock) {
		lockGlobal = 0;
		lockedObj = nullptr;
	}
	if (lock) {
		if (lockGlobal == 0) {
			if (editor->selectGlobal)
				lockGlobal = 2;
			else if (editor->selected()) {
				lockGlobal = 1;
				lockedObj(editor->selected);
			}
			else UI::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, "Select object to inspect.", editor->font, white());
		}
		if (lockGlobal == 2) { //no else to prevent 1 frame blank
			DrawGlobal(v);
		}
		else if (lockGlobal == 1) {
			DrawObj(v, editor, this, lockedObj.raw());
		}
	}
	else if (editor->selectedFileType != ASSETTYPE_UNDEF) {
		UI::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, editor->selectedFileName, editor->font, white());
		float off = v.g + EB_HEADER_SIZE;
		bool canApply = false;
		switch (editor->selectedFileType) {
		case ASSETTYPE_TEXTURE:
			EBI_DrawAss_Tex(v, editor, this, off);
			canApply = true;
			break;
		case ASSETTYPE_MATERIAL:
			EBI_DrawAss_Mat(v, editor, this, off);
			canApply = true;
			break;
		case ASSETTYPE_HDRI:
			EBI_DrawAss_Hdr(v, editor, this, off);
			canApply = true;
			break;
		case ASSETTYPE_CAMEFFECT:
			EBI_DrawAss_Eff(v, editor, this, off);
			canApply = true;
			break;
		}

		if (canApply && (Engine::EButton(editor->editorLayer == 0, v.r + v.b - 81, off, 80, 14, grey2(), "Apply", 12, editor->font, white())) == MOUSE_RELEASE) {
			switch (editor->selectedFileType) {
			case ASSETTYPE_TEXTURE:
				((Texture*)editor->selectedFileCache)->_ApplyPrefs(editor->selectedFilePath);
				break;
			case ASSETTYPE_MATERIAL:
				((Material*)editor->selectedFileCache)->Save(Editor::instance->selectedFilePath);
				break;
			}
		}
	}
	else if (editor->selectGlobal) {
		DrawGlobal(v);
	}
	else if (editor->selected()) {
		DrawObj(v, editor, this, editor->selected.raw());
	}
	else if (editor->selectedFileTexts.size() > 0) {
		UI::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, editor->selectedFileName, editor->font, white());
		EBI_DrawAss_Txt(v, editor, this, v.g + EB_HEADER_SIZE + 18);
	}
	else
		UI::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, "Select object to inspect.", editor->font, white());
	if (!editor->playSyncer.syncedSceneSz && (editor->playSyncer.status != Editor_PlaySyncer::EPS_Offline)) {
		UI::Texture(v.r + v.b / 2 - 16, v.g + v.a / 2 - 16, 32, 32, editor->tex_waiting);
	}
	Engine::EndStencil();
}

void EB_Inspector::OnMouseScr(bool up) {
	scrollOffset += up ? -17 : 17;
	scrollOffset = Clamp(scrollOffset, 0.0f, max(maxScroll - (Display::height*(editor->yPoss[y2] - editor->yPoss[y1]) - EB_HEADER_SIZE - 1), 0.0f));
}

void EB_Inspector::DrawGlobal(Vec4 v) {
	int off = 0;
	Engine::DrawQuad(v.r + 20, v.g + 2 + EB_HEADER_SIZE, v.b - 21, 18, grey1());
	UI::Label(v.r + 22, v.g + 6 + EB_HEADER_SIZE, 12, "Scene Settings", editor->font, white());

	UI::Label(v.r, v.g + 23 + EB_HEADER_SIZE, 12, "Sky", editor->font, white());
	editor->DrawAssetSelector(v.r + v.b*0.3f, v.g + 21 + EB_HEADER_SIZE, v.b*0.7f - 1, 14, grey2(), ASSETTYPE_HDRI, 12, editor->font, &editor->activeScene->settings.skyId, &_ApplySky, &*editor->activeScene);
	off = 40 + EB_HEADER_SIZE;
	if (editor->activeScene->settings.skyId > -1) {
		UI::Label(v.r, v.g + off, 12, "Sky Strength", editor->font, white());
		Engine::DrawQuad(v.r + v.b*0.3f, v.g + off, v.b*0.3f - 1, 16, grey2());
		UI::Label(v.r + v.b*0.3f, v.g + off, 12, std::to_string(editor->activeScene->settings.skyStrength), editor->font, white());
		editor->activeScene->settings.skyStrength = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + off, v.b*0.4f - 1, 16, 0, 3, editor->activeScene->settings.skyStrength, grey2(), white());
	}
	off += 17;
	UI::Label(v.r, v.g + off, 12, "RSM Radius", editor->font, white());
	Engine::DrawQuad(v.r + v.b*0.3f, v.g + off, v.b*0.3f - 1, 16, grey2());
	UI::Label(v.r + v.b*0.3f, v.g + off, 12, std::to_string(editor->activeScene->settings.rsmRadius), editor->font, white());
	editor->activeScene->settings.rsmRadius = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + off, v.b*0.4f - 1, 16, 0, 3, editor->activeScene->settings.rsmRadius, grey2(), white());
}

bool EB_Inspector::DrawVector3(Editor* e, Vec4 v, float dh, string label, Vec3& value, bool* dirty) {
	Vec3 vo = value;
	bool changed = false, chg = false;
	UI::Label(v.r, v.g + dh + EB_HEADER_SIZE, 12, label, e->font, white());
	//Engine::EButton((e->editorLayer == 0), v.r + v.b*0.19f, v.g + dh + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Vec4(0.4f, 0.2f, 0.2f, 1));
	//UI::Label(v.r + v.b*0.19f + 2, v.g + dh + EB_HEADER_SIZE, 12, std::to_string(value.x), e->font, white());
	auto res = UI::EditText(v.r + v.b*0.19f, v.g + dh + EB_HEADER_SIZE, v.b*0.27f - 1, 16, 12, Vec4(0.4f, 0.2f, 0.2f, 1), std::to_string(value.x), e->font, true, &chg, white(), hlCol(), white(), false);
	value.x = TryParse(res, value.x);
	changed |= chg;
	res = UI::EditText(v.r + v.b*0.46f, v.g + dh + EB_HEADER_SIZE, v.b*0.27f - 1, 16, 12, Vec4(0.2f, 0.4f, 0.2f, 1), std::to_string(value.y), e->font, true, &chg, white(), hlCol(), white(), false);
	value.y = TryParse(res, value.y);
	changed |= chg;
	res = UI::EditText(v.r + v.b*0.73f, v.g + dh + EB_HEADER_SIZE, v.b*0.27f - 1, 16, 12, Vec4(0.2f, 0.2f, 0.4f, 1), std::to_string(value.z), e->font, true, &chg, white(), hlCol(), white(), false);
	value.z = TryParse(res, value.z);
	changed |= chg;
#ifdef IS_EDITOR
	if (changed) {
		UndoStack::Add(new UndoStack::UndoObj(&value, sizeof(Vec3), sizeof(Vec3), UndoStack::UNDO_TYPE_NORMAL, &vo, dirty));
	}
#endif
	return changed;
}

void EB_Inspector::DrawAsset(Editor* e, Vec4 v, float dh, string label, ASSETTYPE type) {
	UI::Label(v.r, v.g + dh + 2 + EB_HEADER_SIZE, 12, label, e->font, white());
	if (Engine::EButton((e->editorLayer == 0), v.r + v.b*0.19f, v.g + dh + EB_HEADER_SIZE, v.b*0.71f - 1, 16, Vec4(0.4f, 0.2f, 0.2f, 1)) == MOUSE_RELEASE) {
		e->editorLayer = 3;
		e->browseType = type;
		e->browseOffset = 0;
		e->browseSize = e->allAssets[type].size();
	}
	UI::Label(v.r + v.b*0.19f + 2, v.g + dh + 2 + EB_HEADER_SIZE, 12, label, e->font, white());
}

void EB_Inspector::_ApplyTexFilter0(EditorBlock* b) {
	((Texture*)(b->editor->selectedFileCache))->_filter = TEX_FILTER_POINT;
}
void EB_Inspector::_ApplyTexFilter1(EditorBlock* b) {
	((Texture*)(b->editor->selectedFileCache))->_filter = TEX_FILTER_BILINEAR;
}
void EB_Inspector::_ApplyTexFilter2(EditorBlock* b) {
	((Texture*)(b->editor->selectedFileCache))->_filter = TEX_FILTER_TRILINEAR;
}

void EB_Inspector::_ApplyMatShader(void* v) {
	Material* mat = (Material*)v;
	mat->_shader = _GetCache<Shader>(ASSETTYPE_SHADER, mat->_shaderId);
	mat->_ReloadParams();
	mat->Save(Editor::instance->selectedFilePath);
}

void EB_Inspector::_ApplySky(void* v) {
	Scene* sc = (Scene*)v;
	sc->settings.sky = _GetCache<Background>(ASSETTYPE_HDRI, sc->settings.skyId);
}

void EB_Inspector::_ApplyEffMat(void* v) {
	CameraEffect* eff = (CameraEffect*)v;
	eff->material = _GetCache<Material>(ASSETTYPE_MATERIAL, eff->_material);
	eff->Save(Editor::instance->selectedFilePath);
}

EB_AnimEditor::EB_AnimEditor(Editor* e, int x1, int y1, int x2, int y2) : _editingAnim(-1), scale(2), offset(Vec2()) {
	editorType = 5;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;

	shortcuts.emplace(GetShortcutInt(Key_A, Key_Shift), &_AddState);
}

void EB_AnimEditor::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "Animation Editor");
	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);

	Engine::DrawQuad(v.r, v.g + EB_HEADER_SIZE, v.b, v.a, white(0.1f, 0.4f));
	float scl = 1 / scale;
	scl *= scl;
	float scl2 = scl;
	while (scl2 < 0.2f)
		scl2 *= 5;
	while (scl2 > 25)
		scl2 *= 0.2f;

	Vec2 off0 = offset*scl + Vec2(v.r, v.g) + 0.5f*Vec2(v.b, v.a);
	for (float x = v.r; x < (v.r + v.b); x += scl2 * 32) {
		Engine::DrawLine(Vec2(x, v.g - EB_HEADER_SIZE - 1), Vec2(x, v.g + v.a), black(0.6f*(scl2 - 0.2f) / 0.8f), 1);
	}
	for (float y = v.g; y < (v.g + v.a); y += scl2 * 32) {
		Engine::DrawLine(Vec2(v.r, y), Vec2(v.r + v.b, y), black(0.6f*(scl2 - 0.2f) / 0.8f), 1);
	}
	for (float x = v.r; x < (v.r + v.b); x += scl2 * 160) {
		Engine::DrawLine(Vec2(x, v.g - EB_HEADER_SIZE - 1), Vec2(x, v.g + v.a), black(0.6f), 1);
	}
	for (float y = v.g; y < (v.g + v.a); y += scl2 * 160) {
		Engine::DrawLine(Vec2(v.r, y), Vec2(v.r + v.b, y), black(0.6f), 1);
	}

	editor->DrawAssetSelector(v.r, v.g + EB_HEADER_SIZE + 1, v.b * 0.5f, 14, grey2(), ASSETTYPE_ANIMATION, 12, editor->font, &_editingAnim, &_SetAnim, this);

	if (editingAnim != nullptr) {
		for (Anim_State* state : editingAnim->states) {
			Vec2 poss = off0 + state->editorPos;
			Engine::DrawQuad(poss.x, poss.y, scl * 320, scl * 32, grey2());
			Engine::DrawTriangle(Vec2(poss.x + scl * 16, poss.y + scl * 16), state->editorExpand ? Vec2(0, scl * 13) : Vec2(scl * 13, 0), white());
			if (Engine::Button(poss.x, poss.y, scl * 32, scl * 32) == MOUSE_RELEASE)
				state->editorExpand = !state->editorExpand;
			Engine::EButton(editor->editorLayer == 0, poss.x + scl * 34, poss.y + scl * 2, scl * 284, scl * 28, grey1(), state->name, scl * 24, editor->font, white());
			if (state->editorExpand) {
				Engine::DrawQuad(poss.x, poss.y + scl * 32, scl * 320, (state->isBlend && state->editorShowGraph) ? scl * 320 : scl * 256, grey1());
				state->isBlend = Engine::Toggle(poss.x + scl * 6, poss.y + scl * 34, scl * 28, editor->tex_checkbox, state->isBlend, white(), ORIENT_HORIZONTAL);
				UI::Label(poss.x + scl * 36, poss.y + scl * 36, scl * 24, "Blending", editor->font, white());
				float off = poss.y + scl * 64;
				if (state->isBlend) {
					if (Engine::EButton(editor->editorLayer == 0, poss.x + scl * 290, poss.y + scl * 34, scl * 28, scl * 28, white(1, state->editorShowGraph ? 0.5f : 0.2f)) == MOUSE_RELEASE) {
						state->editorShowGraph = !state->editorShowGraph;
					}
					if (state->editorShowGraph) {
						Engine::DrawQuad(poss.x + scl * 2, off, scl * 316, scl * 64, black());
						off += scl * 66;
					}

				}
				else {
					editor->DrawAssetSelector(poss.x + scl * 2, off, scl * 316, scl * 28, grey2(), ASSETTYPE_ANIMCLIP, scl * 24, editor->font, &state->_clip);
				}
			}
		}
	}

	UI::Label(v.r + 5, v.g + v.a - 16, 12, std::to_string(scl), editor->font);
	Engine::EndStencil();
}

void EB_AnimEditor::OnMouseScr(bool up) {
	scale += (up ? 0.1f : -0.1f);
	scale = min(max(scale, 1.0f), 5.0f);
}

GLuint EB_Previewer::d_fbo = 0;
GLuint EB_Previewer::d_texs[] = { 0, 0, 0 };
GLuint EB_Previewer::d_depthTex = 0;

GLuint EB_Previewer::b_fbo = 0;
GLuint EB_Previewer::b_texs[] = { 0, 0 };
GLuint EB_Previewer::bb_fbo = 0;
GLuint EB_Previewer::bb_tex = 0;
GLuint EB_Previewer::lumiProgram = 0;

EB_Previewer::EB_Previewer(Editor* e, int x1, int y1, int x2, int y2) {
	editorType = 6;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;

	if (d_fbo == 0) {
		InitGBuffer();
	}
	e->playSyncer.previewer = this;
	shortcuts.emplace(GetShortcutInt(Key_Z, Key_Shift), &_ToggleBuffers);
	shortcuts.emplace(GetShortcutInt(Key_Z), &_ToggleLumi);
}

void EB_Previewer::FindEditor() {
	for (auto b : editor->blocks) {
		if (b->editorType == 2) {
			viewer = (EB_Viewer*)b;
			break;
		}
	}
}

void EB_Previewer::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	if (maximize) v = Vec4(0, 0, Display::width, Display::height);
	DrawHeaders(editor, this, &v, "Previewer (" + std::to_string(editor->playSyncer.playW) + "x" + std::to_string(editor->playSyncer.playH) + ")");

	if (viewer == nullptr) FindEditor();

	if (editor->playSyncer.status == Editor_PlaySyncer::EPS_Offline) {
		Camera* seeingCamera = viewer->seeingCamera;
		float scale = viewer->scale;

		previewWidth = v.b;
		previewHeight = v.a;

		//glViewport(v.r, Display::height - v.g - v.a, v.b, v.a - EB_HEADER_SIZE - 2);
		Vec2 v2 = Vec2(Display::width, Display::height)*0.03f;
		//Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black());
		if (editor->sceneLoaded()) {
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			float ww1 = editor->xPoss[x1];
			float hh1 = editor->yPoss[y1];
			float ww = editor->xPoss[x2] - ww1;
			float hh = editor->yPoss[y2] - hh1;
			//if (!persp) {
			float h40 = 40 * (hh*Display::height) / (ww*Display::width);
			float mww = max(ww, 0.3f) * (float)pow(2, scale);
			if (seeingCamera == nullptr) {
				glScalef(-mww*Display::width / v.b, mww*Display::width / v.a, 1);
				if (viewer->persp) glMultMatrixf(glm::value_ptr(glm::perspectiveFov(viewer->fov * deg2rad, (float)Display::width, (float)Display::width, 0.01f, 1000.0f)));
				else glMultMatrixf(glm::value_ptr(glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.01f, 1000.0f)));
				glTranslatef(0, 0, -20);

				float csz = cos(-viewer->rz*deg2rad);
				float snz = sin(-viewer->rz*deg2rad);
				float csw = cos(viewer->rw*deg2rad);
				float snw = sin(viewer->rw*deg2rad);
				Mat4x4 mMatrix = Mat4x4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1) * Mat4x4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
				glMultMatrixf(glm::value_ptr(mMatrix));
				glTranslatef(-viewer->rotCenter.x, -viewer->rotCenter.y, -viewer->rotCenter.z);
			}
			else {
				Quat q = glm::inverse(seeingCamera->object->transform.rotation());
				glScalef(scale, -scale, 1);
				glMultMatrixf(glm::value_ptr(glm::perspectiveFov(seeingCamera->fov * deg2rad, (float)Display::width, (float)Display::height, seeingCamera->nearClip, seeingCamera->farClip)));
				glScalef(1, -1, -1);
				glMultMatrixf(glm::value_ptr(QuatFunc::ToMatrix(q)));
				Vec3 pos = -seeingCamera->object->transform.position();
				glTranslatef(pos.x, pos.y, pos.z);
			}

			if (previewWidth != previewWidth_o || previewHeight != previewHeight_o) {
				previewWidth_o = previewWidth;
				previewHeight_o = previewHeight;
				InitGBuffer();
			}
			DrawPreview(v);
		}
		//glViewport(0, 0, Display::width, Display::height);
	}
	else {
		Engine::DrawQuad(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a, black());
		editor->font->Align(ALIGN_MIDCENTER);
		switch (editor->playSyncer.status) {
		case Editor_PlaySyncer::EPS_Running:
			if (editor->playSyncer.pixelCount == 0)
				UI::Label(v.r + v.b*0.5f, v.g + v.a*0.5f, 12, "Waiting for pixels...", editor->font, white());
			else {
				Engine::DrawQuad(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 1, editor->playSyncer.texPointer);
			}
			break;
		case Editor_PlaySyncer::EPS_Starting:
			UI::Label(v.r + v.b*0.5f, v.g + v.a*0.5f, 12, "Initializing...", editor->font, white());
			break;
		case Editor_PlaySyncer::EPS_RWFailure:
			UI::Label(v.r + v.b*0.5f, v.g + v.a*0.5f, 12, "Read/Write failure!", editor->font, red());
			if (Engine::Button(v.r + v.b*0.4f, v.g + v.a*0.5f + 12, v.b*0.2f, 18, grey2(), "Close", 12, editor->font, white(), true) == MOUSE_RELEASE) {
				editor->playSyncer.Terminate();
			}
			break;
		case Editor_PlaySyncer::EPS_Crashed:
			UI::Label(v.r + v.b*0.5f, v.g + v.a*0.5f, 12, "Crashed! Exit Code: " + std::to_string(editor->playSyncer.exitCode), editor->font, red());
			if (Engine::Button(v.r + v.b*0.4f, v.g + v.a*0.5f + 12, v.b*0.2f, 18, grey2(), "Close", 12, editor->font, white(), true) == MOUSE_RELEASE) {
				editor->playSyncer.Terminate();
			}
			break;
		}
	}
}


void PB_ColorPicker::Draw() {
	//Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	//CalcV(v);
	//DrawHeaders(editor, this, &v, "Color Picker");
	Color::DrawPicker(x(), y(), col);
	*target = col.vec4();
}

void PB_ProceduralGenerator::Draw() {
	Engine::DrawQuad(x(), y(), (float)w, (float)h, white(0.8f, 0.1f));
}


Editor* PopupSelector::editor = nullptr;
bool PopupSelector::focused = false;
bool PopupSelector::show = false;
uint PopupSelector::width, PopupSelector::height;
Vec2 PopupSelector::mousePos, PopupSelector::mouseDownPos;
bool PopupSelector::mouse0, PopupSelector::mouse1, PopupSelector::mouse2;
byte PopupSelector::mouse0State, PopupSelector::mouse1State, PopupSelector::mouse2State;

POPUP_SELECT_TYPE PopupSelector::_type;
rSceneObject* PopupSelector::_browseTargetObj;
ASSETTYPE PopupSelector::assettype;
int* PopupSelector::_browseTarget;
callbackFunc PopupSelector::_browseCallback;
void* PopupSelector::_browseCallbackParam;

bool PopupSelector::drawIcons = false;
float PopupSelector::minIconSize = 100;

GLFWwindow* PopupSelector::window;

void PopupSelector::Init() {
	glfwWindowHint(GLFW_VISIBLE, 0);
	window = glfwCreateWindow(1024, 600, "Selector", NULL, Display::window);
	glfwMakeContextCurrent(window);
	glfwSetWindowPos(window, 200, 200);
	glfwSetWindowSize(window, 200, 500);
	editor = Editor::instance;

	auto hwnd = glfwGetWin32Window(window);
	auto style = GetWindowLong(hwnd, GWL_STYLE);
	style &= ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
	SetWindowLong(hwnd, GWL_STYLE, style);

	glViewport(0, 0, 200, 500);
	glfwSetFramebufferSizeCallback(window, Reshape);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFrontFace(GL_CW);
}

void PopupSelector::Enable_Object(rSceneObject* target) {
	_type = POPUP_TYPE_OBJECT;
	_browseTargetObj = target;

	show = true;
	glfwShowWindow(window);
}

void PopupSelector::Enable_Asset(ASSETTYPE type, int* target, callbackFunc callback, void* param) {
	_type = POPUP_TYPE_ASSET;
	assettype = type;
	_browseTarget = target;
	_browseCallback = callback;
	_browseCallbackParam = param;

	show = true;
	glfwShowWindow(window);
}

void PopupSelector::Draw() {
	if (!glfwWindowShouldClose(window)) {
		if (mouse0)
			mouse0State = min<byte>(mouse0State + 1U, MOUSE_HOLD);
		else
			mouse0State = ((mouse0State == MOUSE_UP) | (mouse0State == 0)) ? 0 : MOUSE_UP;
		if (mouse1)
			mouse1State = min<byte>(mouse1State + 1U, MOUSE_HOLD);
		else
			mouse1State = ((mouse1State == MOUSE_UP) | (mouse1State == 0)) ? 0 : MOUSE_UP;
		if (mouse2)
			mouse2State = min<byte>(mouse2State + 1U, MOUSE_HOLD);
		else
			mouse2State = ((mouse2State == MOUSE_UP) | (mouse2State == 0)) ? 0 : MOUSE_UP;

		if (mouse0State == MOUSE_DOWN)
			mouseDownPos = mousePos;
		else if (!mouse0State)
			mouseDownPos = Vec2(-1, -1);

		bool foc = UI::focused;
		auto w = Display::width;
		auto h = Display::height;
		auto p1 = Input::mousePos;
		auto p2 = Input::mousePosRelative;
		auto m0 = Input::mouse0;
		auto m1 = Input::mouse1;
		auto m2 = Input::mouse2;
		auto m0s = Input::mouse0State;
		auto m1s = Input::mouse1State;
		auto m2s = Input::mouse2State;
		auto mdp = Input::mouseDownPos;
		UI::focused = focused;
		Display::width = width;
		Display::height = height;
		Input::mousePos = mousePos;
		Input::mousePosRelative = Vec2(mousePos.x*1.0f / width, mousePos.y*1.0f / height);
		Input::mouse0 = mouse0;
		Input::mouse1 = mouse1;
		Input::mouse2 = mouse2;
		Input::mouse0State = mouse0State;
		Input::mouse1State = mouse1State;
		Input::mouse2State = mouse2State;
		Input::mouseDownPos = mouseDownPos;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0, 0, 0, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_BLEND);

		if (editor->backgroundTex)
			UI::Texture(0, 0, (float)width, (float)height, editor->backgroundTex, editor->backgroundAlpha*0.01f, DRAWTEX_CROP);

		switch (_type) {
		case POPUP_TYPE_OBJECT:
			Draw_Object();
			break;
		case POPUP_TYPE_ASSET:
			Draw_Asset();
			break;
		}

		UI::focused = foc;
		Display::width = w;
		Display::height = h;
		Input::mousePos = p1;
		Input::mousePosRelative = p2;
		Input::mouse0 = m0;
		Input::mouse1 = m1;
		Input::mouse2 = m2;
		Input::mouse0State = m0s;
		Input::mouse1State = m1s;
		Input::mouse2State = m2s;
		Input::mouseDownPos = mdp;

		if (!show) Close();
	}
	else Close();
}

void PopupSelector::Draw_Object() {
	uint off = 0;
	if (Do_Draw_Object(Scene::active->objects, off, 0)) {
		show = false;
	}
}

bool PopupSelector::Do_Draw_Object(const std::vector<pSceneObject>& objs, uint& off, uint inc) {
	for (auto& o : objs) {
		auto r = Engine::Button(1.0f + inc * 5.0f, 20.0f + 17.0f * off, width - 2.0f - 5.0f * inc, 16.0f, grey1(), o->name, 12, editor->font, white());
		if (r == MOUSE_RELEASE) {
			(*_browseTargetObj)(o);
			return true;
		}
		off++;
		if (!!o->childCount)
			if (Do_Draw_Object(o->children, off, inc + 1))
				return true;
	}
	return false;
}

void PopupSelector::Draw_Asset() {
	if (drawIcons) {
		uint nx = (uint)ceil(width / minIconSize);
		uint sx = width / nx;
		for (uint a = 0; a < editor->normalAssets[assettype].size(); a++) {
			//Engine::Button(1.0f, 20.0f + 17 * a, width - 2.0f, 16.0f, grey1(), editor->normalAssets[assettype][a], 12, editor->font, white());
			uint px = a % nx;
			uint py = a / nx;
			Engine::DrawQuad(px * sx + 1.0f, py * sx + 1.0f, sx - 1.0f, sx - 1.0f, white());
		}
	}
	else {
		for (uint a = 0; a < editor->normalAssets[assettype].size(); a++) {
			if (Engine::Button(1.0f, 20.0f + 17 * a, width - 2.0f, 16.0f, grey1(), editor->normalAssets[assettype][a], 12, editor->font, white()) == MOUSE_RELEASE) {
				(*_browseTarget) = a;
				if (_browseCallback) _browseCallback(_browseCallbackParam);
				show = false;
			}
		}
	}
}

void PopupSelector::Close() {
	show = false;
	glfwHideWindow(window);
	glfwSetWindowShouldClose(window, false);
}

void PopupSelector::Reshape(GLFWwindow* window, int w, int h) {
	glfwMakeContextCurrent(window);
	glViewport(0, 0, w, h);
	width = w;
	height = h;
}


#pragma region Editor PlaySyncer

template<typename T>
bool EPS_RWMem(bool write, Editor_PlaySyncer* syncer, T* val, uint loc, ulong sz = 0) {
	assert(syncer && val && !!loc);
	sz = max<ulong>(sz, sizeof(T));
	SIZE_T c;
	bool ok;
	if (write) ok = !!WriteProcessMemory(syncer->pInfo.hProcess, (LPVOID)loc, val, sz, &c);
	else ok = !!ReadProcessMemory(syncer->pInfo.hProcess, (LPVOID)loc, val, sz, &c);
	if (!ok || c != sz) {
		syncer->status = Editor_PlaySyncer::EPS_RWFailure;
		Debug::Error("RWMem", "Unable to " + (string)(write ? "write " : "read ") + std::to_string(sz) + " bytes to " + std::to_string(loc) + "! (" + std::to_string(c) + " bytes succeeded)");
		return false;
	}
	else return true;
}
/*
uint vecsize;
vector<T> vec; vec->[iterator??][pointer to first element]
*/
template <typename T>
bool EPS_ReadVec(std::vector<T>& vec, uint& sz, Editor_PlaySyncer* syncer, uint loc) {
	assert(!!loc);
	byte* a = new byte[sizeof(std::vector<T>)];
	if (!EPS_RWMem(false, syncer, a, loc, sizeof(std::vector<T>))) return false;
	sz = ((std::vector<T>*)a)->size();
	if (!sz) return true;
	vec.resize(sz);
	bool ok = EPS_RWMem(false, syncer, &vec[0], (uint)((std::vector<T>*)a)->data(), sizeof(T) * sz);
	delete[](a);
	return ok;
}

bool EPS_ReadStr(std::string* str, Editor_PlaySyncer* syncer, uint loc, ushort maxSz = 100) {
	char *cc = new char[maxSz];
	if (!EPS_RWMem(false, syncer, cc, loc, maxSz)) return false;
	str = new string(cc);
	delete[](cc);
	return true;
}

void Editor_PlaySyncer::Update() {
#ifdef IS_EDITOR
	switch (status) {
	case EPS_Starting:
		timer -= Time::delta;
		if (timer <= 0) {
			if (syncStatus == 0) {
				auto wl = Editor::instance->projectFolder + "Release\\winloc.txt";
				if (!IO::HasFile(wl)) {
					timer = 0.1f;
					return;
				}
				std::ifstream strm(wl, std::ios::binary);
				_Strm2Val(strm, pointerLoc);
				strm.close();
				if (!pointerLoc) {
					Editor::instance->_Error("PlayMode(Internal)", "Error reading syncer struct!");
					Disconnect();
					return;
				}
				DeleteFile(&wl[0]);

				Debug::Message("Player", "reading info struct at " + std::to_string(pointerLoc));
				if (!EPS_RWMem(false, this, &pointers, pointerLoc)) return;
				Debug::Message("Player", "writing screen size to " + std::to_string((uint)pointers.screenSizeLoc));
				uint sz = (playH << 16) + playW;
				if (!EPS_RWMem(true, this, &sz, pointers.screenSizeLoc)) return;

				eCacheSzLocs = std::vector<uint>(AssetManager::dataECacheIds.size());
				if (!EPS_RWMem(false, this, &eCacheSzLocs[0], pointers.assetCacheSzLoc, sizeof(uint)*AssetManager::dataECacheIds.size())) return;
				ushort an = 0;
				for (auto& ids : AssetManager::dataECacheIds) {
					auto as = Editor::instance->normalAssetCaches[ids.first][ids.second];
					if (as && as->_eCache) {
						if (!EPS_RWMem(true, this, &as->_eCacheSz, eCacheSzLocs[an])) return;
						std::cout << "(sz)wrote " << (int)ids.first << "." << ids.second << ": " << as->_eCacheSz << "B to " << std::to_string(eCacheSzLocs[an]) << std::endl;
					}
					an++;
				}

				syncStatus = 1;
				if (!EPS_RWMem(true, this, &syncStatus, pointers.statusLoc)) return;

				//update all offsets
				if (!EPS_RWMem(false, this, &offsets, pointers.offsetsLoc)) return;
				if (!EPS_RWMem(false, this, &Scene::_offsets, offsets.scene)) return;
				if (!EPS_RWMem(false, this, &Transform::_offsets, offsets.transform)) return;
				if (!EPS_RWMem(false, this, &SceneObject::_offsets, offsets.sceneobject)) return;

				timer = 0.1f;
			}
			else if (syncStatus == 1) {
				timer -= Time::delta;
				if (timer <= 0) {
					if (!EPS_RWMem(false, this, &syncStatus, pointers.statusLoc)) return;
					if (syncStatus != 2)
						timer = 0.1f;
				}
			}
			else if (syncStatus == 2) {
				eCacheLocs = std::vector<uint>(AssetManager::dataECacheIds.size());
				if (!EPS_RWMem(false, this, &eCacheLocs[0], pointers.assetCacheLoc, sizeof(uint)*AssetManager::dataECacheIds.size())) return;
				ushort an = 0;
				for (auto& ids : AssetManager::dataECacheIds) {
					//auto as = Editor::instance->GetCache(ids.first, ids.second);
					auto as = Editor::instance->normalAssetCaches[ids.first][ids.second];
					if (as && as->_eCache) {
						if (!EPS_RWMem(true, this, as->_eCache, eCacheLocs[an], as->_eCacheSz + 1)) return;
						std::cout << "wrote " << (int)ids.first << "." << ids.second << ": " << as->_eCacheSz << "B" << std::endl;
					}
					an++;
				}
				syncStatus = 255;
				if (!EPS_RWMem(true, this, &syncStatus, pointers.statusLoc)) return;
				status = EPS_Running;
				timer = 0.5f;
			}
		}
		break;
	case EPS_Running:
		DWORD code;
		GetExitCodeProcess(pInfo.hProcess, &code);
		if (code == 259) {
			if (timer > 0) {
				timer -= Time::delta;
				if (timer <= 0) {
					if (!EPS_RWMem(false, this, &pointers, pointerLoc)) return;
					if (pointers.pixelsLoc == 0) timer = 0.5f;
				}
			}
			else {
				bool hasData;
				if (!EPS_RWMem(false, this, &hasData, pointers.hasDataLoc)) return;
				if (hasData) {
					auto e = Editor::instance;
					//scene data
					SyncScene();

					//screen output
					input._mousePos.x = playW*(Input::mousePos.x - Display::width*e->xPoss[previewer->x1]) / (Display::width*(e->xPoss[previewer->x2] - e->xPoss[previewer->x1]));
					input._mousePos.y = playH*(Input::mousePos.y - Display::height*e->yPoss[previewer->y1] - EB_HEADER_SIZE - 1) / (Display::height*(e->yPoss[previewer->y2] - e->yPoss[previewer->y1]) - EB_HEADER_SIZE - 1);
					input._mouse0 = Input::mouse0;
					input._mouse1 = Input::mouse1;
					input._mouse2 = Input::mouse2;
					if (!EPS_RWMem(true, this, &input._mousePos, pointers.mousePosLoc, 4 * sizeof(Vec2) + 3)) return;
					if (!EPS_RWMem(true, this, Input::keyStatusNew, pointers.keyboardLoc, 256)) return;

					if (!EPS_RWMem(false, this, &pixelCount, pointers.pixelCountLoc)) return;
					if (pixelCount == 0) return;
					if (pixelCountO != pixelCount) {
						pixelCountO = pixelCount;
						ReloadTex();
						delete[](pixels);
						pixels = new byte[pixelCount];
					}
					if (!EPS_RWMem(false, this, pixels, pointers.pixelsLoc, pixelCount)) return;
					glBindTexture(GL_TEXTURE_2D, texPointer);
					GLenum err = glGetError();
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, playW, playH, GL_RGB, GL_UNSIGNED_BYTE, pixels);
					err = glGetError();
					if (err != 0) {
						Debug::Warning("PlayMode", "Writing pixels failed with code " + std::to_string(err) + "!");
						//status = EPS_RWFailure;
					}
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			}
		}
		else {
			if (code == 0) status = EPS_Offline;
			else status = EPS_Crashed;
			exitCode = code;
		}
		break;
	}
#endif
}

bool Editor_PlaySyncer::DoSyncObj(const std::vector<uint>& locs, const uint sz, std::vector<pSceneObject>& objs) {
	objs.clear();
	objs.reserve(sz);
	for (uint i = 0; i < sz; i++) {
		byte dat[sizeof(SceneObject)];
		if (!EPS_RWMem(false, this, dat, locs[i], sizeof(SceneObject))) return false;
		auto& obj = SceneObject::New(dat);
		objs.push_back(obj);
		obj->_componentCount = *((int*)(dat + SceneObject::_offsets.components) - 1);
		if (!!obj->_componentCount) {
			std::vector<uint> clocs(obj->_componentCount);
			if (!EPS_RWMem(false, this, &clocs[0], *((uint*)(dat + SceneObject::_offsets.components)), 4 * obj->_componentCount)) return false;
			for (int c = 0; c < obj->_componentCount; c++) {
				COMPONENT_TYPE ct;
				if (!EPS_RWMem(false, this, &ct, clocs[c] + offsetof(Component, componentType))) return false;
			}
		}
		obj->childCount = *((uint*)(dat + SceneObject::_offsets.children) - 1);
		if (!obj->childCount) continue;
		std::vector<uint> locs2(obj->childCount);
		if (!EPS_RWMem(false, this, &locs2[0], *((uint*)(dat + SceneObject::_offsets.children)), 4 * obj->childCount)) return false;
		DoSyncObj(locs2, obj->childCount, obj->children);
	}
	return true;
}

bool Editor_PlaySyncer::SyncScene() {
	auto sid = Editor::instance->selected() ? Editor::instance->selected->id : 0UL;
	if (!EPS_RWMem(false, this, &pointers.sceneLoc, pointerLoc + offsetof(_PipeModeObj, sceneLoc))) return false;
	syncedScene.clear();
	std::vector<uint> locs; //SceneObject*
	if (!EPS_ReadVec(locs, syncedSceneSz, this, pointers.sceneLoc + Scene::_offsets.objs)) return false;
	if (!DoSyncObj(locs, syncedSceneSz, syncedScene)) return false;
	if (Editor::instance->selected())
		Editor::instance->selected(SceneObject::_FromId(syncedScene, sid));
	return true;
}

bool Editor_PlaySyncer::Connect() {
	if (status != EPS_Offline) return 0;
	STARTUPINFO si = {};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNOACTIVATE;

	SetWindowPos(Editor::hwnd2, HWND_TOPMOST, 0, 0, 10, 10, SWP_NOMOVE | SWP_NOSIZE);
	std::string str("D:\\TestProject2\\Release\\TestProject2.exe -piped " + std::to_string((unsigned long)Editor::hwnd2));
	CreateProcess("D:\\TestProject2\\Release\\TestProject2.exe", &str[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pInfo);
	//CreateProcess("D:\\TestProject2\\Release\\TestProject2.exe", &str[0], NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pInfo);
	//SetForegroundWindow(Editor::hwnd2);
	SetWindowPos(Editor::hwnd2, HWND_TOP, 0, 0, 10, 10, SWP_NOMOVE | SWP_NOSIZE);
	timer = 0.2f;
	status = EPS_Starting;
	syncStatus = 0;
	syncedScene.clear();
	syncedSceneSz = 0;
	Editor::instance->selected.clear();
	Debug::Message("Player", "Starting...");
	Resize(600, 400);
	return 1;
}

bool Editor_PlaySyncer::Disconnect() {
	Terminate();
	Debug::Message("Player", "Stopped.");
	return 1;
}

bool Editor_PlaySyncer::Terminate() {
	TerminateProcess(pInfo.hProcess, 0);
	status = EPS_Offline;
	syncedScene.clear();
	syncedSceneSz = 0;
	Editor::instance->selected.clear();
	return 1;
}

bool Editor_PlaySyncer::Resize(int w, int h) {
	playW = w;
	playH = h;
	ReloadTex();
	return 1;
}

bool Editor_PlaySyncer::ReloadTex() {
	if (!!texPointer) glDeleteTextures(1, &texPointer);
	GLenum e = glGetError();
	glGenTextures(1, &texPointer);
	e = glGetError();
	glBindTexture(GL_TEXTURE_2D, texPointer);
	e = glGetError();
	byte* dat = new byte[playW*playH * 4]();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, playW, playH, 0, GL_RGB, GL_UNSIGNED_BYTE, dat);
	e = glGetError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	e = glGetError();
	delete[](dat);
	return 1;
}

#pragma endregion

#pragma region Editor

void Editor::DrawObjectSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, rSceneObject* tar) {
	if (editorLayer == 0) {
		if (Engine::Button(x, y, w, h, col, Lerp(col, white(), 0.5f), Lerp(col, black(), 0.5f)) == MOUSE_RELEASE) {

			PopupSelector::Enable_Object(tar);
		}
	}
	else
		Engine::DrawQuad(x, y, w, h, col);
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDLEFT;
	UI::Label(round(x + 2), round(y + 0.4f*h), labelSize, (!(*tar)) ? "undefined" : (*tar)->name, labelFont, (!(*tar)) ? Vec4(0.7f, 0.4f, 0.4f, 1) : Vec4(0.4f, 0.4f, 0.7f, 1));
	labelFont->alignment = al;
}

void Editor::DrawAssetSelector(float x, float y, float w, float h, Vec4 col, ASSETTYPE type, float labelSize, Font* labelFont, ASSETID* tar, callbackFunc func, void* param) {
	if (*tar > -1) {
		if (Engine::EButton(editorLayer == 0, x, y, h, h, tex_browse, white()) == MOUSE_RELEASE) {
			selected.clear();
			selectedFilePath = normalAssets[type][*tar];
			size_t p0 = selectedFilePath.find_last_of('\\');
			if (p0 != string::npos) selectedFileName = selectedFilePath.substr(p0 + 1);
			else selectedFileName = selectedFilePath;
			selectedFileName = selectedFileName.substr(0, selectedFileName.find_last_of('.'));
			selectedFileCache = _GetCache<void>(type, *tar);
			selectedFileType = type;
			selectedFile = *tar;
		}
	}
	byte b = Engine::EButton(editorLayer == 0, (*tar == -1) ? x : x + h + 1, y, w, h, col, Lerp(col, white(), 0.1f), Lerp(col, black(), 0.5f));
	if (b & MOUSE_HOVER_FLAG) {
		if (*tar > -1) {
			pendingPreview = true;
			previewType = type;
			previewId = *tar;
		}
		if (b == MOUSE_RELEASE) {
			//editorLayer = 3;
			//browseType = type;
			browseTarget = tar;
			browseCallback = func;
			browseCallbackParam = param;

			PopupSelector::Enable_Asset(type, tar, func, param);
		}
	}
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDLEFT;
	UI::Label(round((*tar == -1) ? x + 2 : x + h + 3), round(y + 0.4f*h), labelSize, GetAssetName(type, *tar), labelFont, (*tar == -1) ? Vec4(0.7f, 0.4f, 0.4f, 1) : Vec4(0.4f, 0.4f, 0.7f, 1));
	labelFont->alignment = al;
}

void Editor::DrawCompSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, CompRef* tar, callbackFunc func, void* param) {
	if (editorLayer == 0) {
		if (Engine::Button(x, y, w, h, col, Lerp(col, white(), 0.5f), Lerp(col, black(), 0.5f)) == MOUSE_RELEASE) {
			editorLayer = 3;
			browseIsComp = true;
			browseTargetComp = tar;
			browseCallback = func;
			browseCallbackParam = param;
			ScanBrowseComp();
		}
	}
	else
		Engine::DrawQuad(x, y, w, h, col);
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDLEFT;
	UI::Label(round(x + 2), round(y + 0.4f*h), labelSize, (tar->comp == nullptr) ? "undefined" : tar->path + " (" + tar->comp->name + ")", labelFont, (tar->comp == nullptr) ? Vec4(0.7f, 0.4f, 0.4f, 1) : Vec4(0.4f, 0.4f, 0.7f, 1));
	labelFont->alignment = al;
}

void Editor::DrawColorSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, Vec4* tar) {
	Engine::DrawQuad(x, y, w * 0.7f - 1, h, col);
	UI::Label(x + 2, y + 2, labelSize, Color::Col2Hex(*tar), labelFont, white());
	//Engine::DrawQuad(x + w*0.7f, y, w*0.3f, h*0.8f, Vec4(tar->r, tar->g, tar->b, 1));
	if (Engine::EButton(editorLayer == 0, x + w*0.7f, y, w*0.3f, h, Vec4(tar->r, tar->g, tar->b, 1)) == MOUSE_RELEASE) {
		RegisterPopup(new PB_ColorPicker(this, tar), Vec2(Display::width*0.5f, Display::height*0.5f));
	}
	Engine::DrawQuad(x + w*0.7f, y + h * 0.8f, w*0.3f, h*0.2f, black());
	Engine::DrawQuad(x + w*0.7f, y + h * 0.8f, w*0.3f*tar->a, h*0.2f, white());
}

Editor::Editor() {
	instance = this;
#ifdef IS_EDITOR
	UndoStack::Init();
#endif
}

void Editor::InitShaders() {
	Camera::GenShaderFromPath("lightPassVert.txt", "e_popupShadowFrag.txt", &instance->popupShadeProgram);
}

ASSETID Editor::GetAssetInfoH(string p) {
	ushort i = 0;
	for (string s : headerAssets) {
		if (s == p)
			return i;
		i++;
	}
	return -1;
}

ASSETID Editor::GetAssetInfo(string p, ASSETTYPE &type, ASSETID& i) {
	for (auto& t : normalAssets) {
		ushort x = 0;
		for (auto u : t.second) {
			if (u == p) {
				type = t.first;
				i = x;
				//GetCache(type, i);
				return x;
			}
			string uu;
			switch (t.first) {
			case ASSETTYPE_MATERIAL:
			case ASSETTYPE_CAMEFFECT:
			case ASSETTYPE_ANIMATION:
			case ASSETTYPE_ANIMCLIP:
				uu = projectFolder + "Assets\\" + u;
				break;
			default:
				uu = projectFolder + "Assets\\" + u + ".meta";
				break;
			}
			if (uu == p) {
				type = t.first;
				i = x;
				//GetCache(type, i);
				return x;
			}
			x++;
		}
	}
	type = ASSETTYPE_UNDEF;
	i = -1;
	return -1;
}

ASSETID Editor::GetAssetId(AssetObject* i) {
	ASSETTYPE t;
	return GetAssetId(i, t);
}
ASSETID Editor::GetAssetId(AssetObject* i, ASSETTYPE&) {
	if (i == nullptr)
		return -1;
	else {
		for (auto& a : normalAssetCaches) {
			for (int b = a.second.size() - 1; b >= 0; b--) {
				if (a.second[b].get() == i) {
					return b;
				}
			}
		}
	}
	return -1;
}

string Editor::GetAssetName(ASSETTYPE type, ASSETID i) {
	if (i == -1)
		return "undefined";
	else if (i < 0) {
		if (i <= -(1 << 8))
			return proceduralAssets[type][(-i) - (1 << 8)];
		else
			return internalAssets[type][-i - 2];
	}
	else return normalAssets[type][i];
}

void Editor::ReadPrefs() {
	string ss = projectFolder;
	while (ss[ss.size() - 1] == '\\')
		ss = ss.substr(0, ss.size() - 1);
	string s = projectFolder + "\\" + ss.substr(ss.find_last_of('\\'), string::npos) + ".Bordom";
	std::ifstream strm(s, std::ios::in | std::ios::binary);
	if (!strm.is_open()) {
		_Error("Editor", "Fail to load project prefs!");
		return;
	}
	byte n;
	_Strm2Val(strm, n);
	savedIncludedScenes = n;
	char* cc = new char[100];
	for (short a = 0; a < n; a++) {
		char c;
		strm >> c;
		if ((byte)c > 1) {
			_Error("Editor", "Strange byte in prefs file! " + std::to_string(strm.tellg()));
			return;
		}
		includedScenesUse.push_back(c == 1);
		strm.getline(cc, 100, (char)0);
		includedScenes.push_back(cc);
	}
	_Strm2Val(strm, n);
	if (n != 255) {
		_Error("Editor", "Separator byte missing in prefs file! " + std::to_string(strm.tellg()));
		return;
	}
	_Strm2Val(strm, n);
	for (short a = 0; a < n; a++) {
		ASSETTYPE t;
		_Strm2Val(strm, t);
		strm.getline(cc, 100, (char)0);
		string s(cc);
		if (s.substr(0, 11) == "procedural\\") {
			size_t is = s.find_first_of(' ');
			string fn = s.substr(0, is);
			string nm = fn.substr(11);
			switch (t) {
			case ASSETTYPE_MESH:
				if (nm == "plane") {
					string sx = s.substr(is + 1, 2);
					string sy = s.substr(is + 3, 2);
					uint xc = (uint)std::stoi(sx);
					uint yc = (uint)std::stoi(sy);
					proceduralAssets[t].push_back(fn + "(x=" + sx + ",y=" + sy + ")");
					proceduralAssetCaches[t].push_back(std::shared_ptr<AssetObject>(Procedurals::Plane(xc, yc)));
				}
				else if (nm == "sphere") {
					string sx = s.substr(is + 1, 2);
					string sy = s.substr(is + 3, 2);
					uint xc = (uint)std::stoi(sx);
					uint yc = (uint)std::stoi(sy);
					proceduralAssets[t].push_back(fn + "(u=" + sx + ",v=" + sy + ")");
					proceduralAssetCaches[t].push_back(std::shared_ptr<AssetObject>(Procedurals::UVSphere(xc, yc)));
				}
				break;
			}
		}
	}
}

void Editor::SavePrefs() {
	string ss = projectFolder;
	while (ss[ss.size() - 1] == '\\')
		ss = ss.substr(0, ss.size() - 1);
	string s = projectFolder + "\\" + ss.substr(ss.find_last_of('\\'), string::npos) + ".Bordom";
	std::ofstream strm(s, std::ios::out | std::ios::binary | std::ios::trunc);
	ushort u = (ushort)includedScenes.size();
	_StreamWrite(&u, &strm, 2);
	for (int a = 0; a < u; a++) {
		if (includedScenesUse[a])
			strm << (char)1;
		else
			strm << (char)0;
		strm << includedScenes[a] << (char)0;
	}

	savedIncludedScenes = (ushort)includedScenes.size();
}

void Editor::LoadDefaultAssets() {
	font = new Font(dataPath + "res\\arial.ttf", 32);

	tex_buttonX = GetRes("xbutton");
	tex_buttonExt = GetRes("extbutton");
	tex_buttonPlus = GetRes("plusbutton");
	tex_buttonExtArrow = GetRes("extbutton arrow");
	tex_background = GetRes("lines");

	tex_placeholder = GetRes("placeholder");
	tex_checkers = GetRes("checkers", false, true);
	tex_expand = GetResExt("expand", "png");
	tex_collapse = GetResExt("collapse", "png");
	tex_object = GetRes("object");
	tex_browse = GetRes("browseicon");
	tex_mipLow = GetRes("miplow", false, true);
	tex_mipHigh = GetRes("miphigh", false, true);
	tex_waiting = GetResExt("waiting", "png");

	shadingTexs.push_back(GetRes("shading_solid"));
	shadingTexs.push_back(GetRes("shading_trans"));

	tooltipTexs.push_back(GetRes("tooltip_tr"));
	tooltipTexs.push_back(GetRes("tooltip_rt"));
	tooltipTexs.push_back(GetRes("tooltip_sc"));

	orientTexs.push_back(GetRes("orien_global"));
	orientTexs.push_back(GetRes("orien_local"));
	orientTexs.push_back(GetRes("orien_view"));

	ebIconTexs.emplace(0, tex_checkers);
	ebIconTexs.emplace(1, GetResExt("eb icon browser", "png"));
	ebIconTexs.emplace(2, GetResExt("eb icon viewer", "png"));
	ebIconTexs.emplace(3, GetResExt("eb icon inspector", "png"));
	ebIconTexs.emplace(4, GetResExt("eb icon hierarchy", "png"));
	ebIconTexs.emplace(5, GetResExt("eb icon anim", "png"));
	ebIconTexs.emplace(6, GetResExt("eb icon previewer", "png"));
	ebIconTexs.emplace(10, GetResExt("eb icon debug", "png"));
	ebIconTexs.emplace(20, GetResExt("eb icon console", "png"));

	tex_checkbox = GetResExt("checkbox", "png");
	tex_keylock = GetRes("keylock");

	tex_assetExpand = GetRes("asset_expand");
	tex_assetCollapse = GetRes("asset_collapse");

	assetIconsExt.push_back("h");
	assetIconsExt.push_back("blend");
	assetIconsExt.push_back("cpp");
	assetIconsExt.push_back("material");
	assetIconsExt.push_back("shade");
	assetIconsExt.push_back("txt");
	assetIconsExt.push_back("mesh");
	assetIconsExt.push_back("animclip");
	assetIconsExt.push_back("animator");
	assetIconsExt.push_back("bmp jpg png");
	assetIconsExt.push_back("effect");
	assetIcons.push_back(new Texture(dataPath + "res\\asset_header.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_blend.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_cpp.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_mat.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_shade.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_txt.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_mesh.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_animclip.jpg", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_animator.jpg", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_tex.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_effect.bmp", false));

	matVarTexs.emplace(SHADER_INT, GetRes("mat_i"));
	matVarTexs.emplace(SHADER_FLOAT, GetRes("mat_f"));
	matVarTexs.emplace(SHADER_SAMPLER, GetRes("mat_tex"));

	for (int x = 0; x < 16; x++) {
		grid[x] = Vec3((x > 7) ? x - 7 : x - 8, 0, -8);
		grid[x + 44] = Vec3((x > 7) ? x - 7 : x - 8, 0, 8);
	}
	for (int y = 0; y < 14; y++) {
		grid[y * 2 + 16] = Vec3(-8, 0, (y > 6) ? y - 6 : y - 7);
		grid[y * 2 + 17] = Vec3(8, 0, (y > 6) ? y - 6 : y - 7);
	}
	grid[60] = Vec3(-8, 0, 0);
	grid[61] = Vec3(8, 0, 0);
	grid[62] = Vec3(0, 0, -8);
	grid[63] = Vec3(0, 0, 8);

	for (int a = 0; a < 16; a++) {
		gridId[a * 2] = a;
		gridId[a * 2 + 1] = a + 44;
	}
	for (int b = 0; b < 14; b++) {
		gridId[b * 2 + 32] = b * 2 + 16;
		gridId[b * 2 + 33] = b * 2 + 17;
	}
	gridId[60] = 0;
	gridId[61] = 15;
	gridId[62] = 44;
	gridId[63] = 59;
	gridId[64] = 60;
	gridId[65] = 61;
	gridId[66] = 62;
	gridId[67] = 63;

	globalShorts.emplace(GetShortcutInt(Key_B, Key_Control), &Compile);
	globalShorts.emplace(GetShortcutInt(Key_B, Key_Control, Key_Shift), &ShowCompileSett);
	globalShorts.emplace(GetShortcutInt(Key_U, Key_Control, Key_Alt), &ShowPrefs);
	globalShorts.emplace(GetShortcutInt(Key_S, Key_Control), &SaveScene);
	globalShorts.emplace(GetShortcutInt(Key_O, Key_Control), &OpenScene);
	globalShorts.emplace(GetShortcutInt(Key_X, Key_Control), &DeleteActive);
	//globalShorts.emplace(GetShortcutInt(Key_Space, Key_Shift), &Maximize);
	globalShorts.emplace(GetShortcutInt(Key_P, Key_Control), &TogglePlay);
	globalShorts.emplace(GetShortcutInt(Key_Z, Key_Control), &Undo);

	assetTypes.emplace("scene", ASSETTYPE_SCENE);
	assetTypes.emplace("mesh", ASSETTYPE_MESH);
	assetTypes.emplace("shade", ASSETTYPE_SHADER);
	assetTypes.emplace("material", ASSETTYPE_MATERIAL);
	assetTypes.emplace("material", ASSETTYPE_CAMEFFECT);
	assetTypes.emplace("bmp", ASSETTYPE_TEXTURE);
	assetTypes.emplace("png", ASSETTYPE_TEXTURE);
	assetTypes.emplace("jpg", ASSETTYPE_TEXTURE);
	assetTypes.emplace("rendtex", ASSETTYPE_TEXTURE);
	assetTypes.emplace("hdr", ASSETTYPE_HDRI);
	assetTypes.emplace("animclip", ASSETTYPE_ANIMCLIP);
	assetTypes.emplace("animator", ASSETTYPE_ANIMATION);

	allAssets.emplace(ASSETTYPE_SCENE, std::vector<string>());
	allAssets.emplace(ASSETTYPE_MESH, std::vector<string>());
	allAssets.emplace(ASSETTYPE_ANIMCLIP, std::vector<string>());
	allAssets.emplace(ASSETTYPE_ANIMATION, std::vector<string>());
	allAssets.emplace(ASSETTYPE_MATERIAL, std::vector<string>());
	allAssets.emplace(ASSETTYPE_CAMEFFECT, std::vector<string>());
	allAssets.emplace(ASSETTYPE_TEXTURE, std::vector<string>());
	allAssets.emplace(ASSETTYPE_HDRI, std::vector<string>());
}

void AddH(Editor* e, string dir, std::vector<string>* h, std::vector<string>* cpp) {
	for (string s : IO::GetFiles(dir)) {
		if (s.size() > 3 && s.substr(s.size() - 2, string::npos) == ".h")
			h->push_back(s.substr(e->projectFolder.size() + 7, string::npos));
		if (s.size() > 5 && s.substr(s.size() - 4, string::npos) == ".cpp")
			cpp->push_back(s.substr(e->projectFolder.size() + 7, string::npos));
		else if (s.size() > 7 && s.substr(s.size() - 6, string::npos) == ".scene")
			e->allAssets[ASSETTYPE_SCENE].push_back(s);
		else if (s.size() > 10 && s.substr(s.size() - 9, string::npos) == ".material")
			e->allAssets[ASSETTYPE_MATERIAL].push_back(s);
		else if (s.size() > 10 && s.substr(s.size() - 7, string::npos) == ".effect")
			e->allAssets[ASSETTYPE_CAMEFFECT].push_back(s);
		else {
			if (s.size() < 7) continue;
			string s2 = s.substr(0, s.size() - 5);
			//std::cout << ">" << s2 << std::endl;
			std::unordered_map<string, ASSETTYPE>::const_iterator got = e->assetTypes.find(s2.substr(s2.find_last_of('.') + 1));
			if (got != e->assetTypes.end()) {
				//std::cout << s << std::endl;
				e->allAssets[(got->second)].push_back(s);
			}
		}
	}
	std::vector<string> dirs;
	IO::GetFolders(dir, &dirs, true);
	for (string ss : dirs) {
		if (ss != "." && ss != "..")
			AddH(e, dir + "\\" + ss, h, cpp);
	}
}

void Editor::GenerateScriptResolver() {
	std::ifstream vcxIn(dataPath + "res\\vcxproj.txt", std::ios::in);
	if (!vcxIn.is_open()) {
		_Error("Script Resolver", "Vcxproj template not found!");
		return;
	}
	std::stringstream sstr;
	sstr << vcxIn.rdbuf();
	string vcx = sstr.str();
	vcxIn.close();
	//<ClCompile Include = "Assets/main.cpp" / >< / ItemGroup><ItemGroup>

	std::ofstream vcxOut(projectFolder + "TestProject2.vcxproj", std::ios::out | std::ios::trunc);
	if (!vcxOut.is_open()) {
		_Error("Script Resolver", "Cannot write to vcxproj!");
		return;
	}
	vcxOut << vcx.substr(0, vcx.find('#'));
	for (string cpp2 : IO::GetFiles(projectFolder + "System\\", ".cpp")) {
		vcxOut << "<ClCompile Include=\"" + cpp2.substr(projectFolder.size(), string::npos) + "\" />" << std::endl;
	}
	for (string cpp : cppAssets) {
		vcxOut << "<ClCompile Include=\"Assets\\" + cpp + "\" />" << std::endl;
	}
	vcxOut << "</ItemGroup>\r\n<ItemGroup>" << std::endl;
	for (string hd2 : IO::GetFiles(projectFolder + "System\\", ".h")) {
		vcxOut << "<ClInclude Include=\"" + hd2.substr(projectFolder.size(), string::npos) + "\" />" << std::endl;
	}
	for (string hd : headerAssets) {
		vcxOut << "<ClInclude Include=\"Assets\\" + hd + "\" />" << std::endl;
	}
	vcxOut << vcx.substr(vcx.find('#') + 1, string::npos);
	vcxOut.close();

	//SceneScriptResolver.h
	string h = "#include <vector>\n#include <fstream>\n#include \"Engine.h\"\ntypedef SceneScript*(*sceneScriptInstantiator)();\ntypedef void (*sceneScriptAssigner)(SceneScript*, std::ifstream&);\nclass SceneScriptResolver {\npublic:\n\tSceneScriptResolver();\n\tstatic SceneScriptResolver* instance;\n\tstd::vector<string> names;\n\tstd::vector<sceneScriptInstantiator> map;\n\tstd::vector<sceneScriptAssigner> ass;\n\t\tSceneScript* Resolve(std::ifstream& strm);\n";
	//*
	h += "\n\tstatic SceneScript ";
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		h += "*_Inst" + std::to_string(a) + "()";
		if (a == b - 1)
			h += ";\n";
		else
			h += ", ";
	}

	h += "\n\tstatic void ";
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		h += "_Ass" + std::to_string(a) + "(SceneScript* sscr, std::ifstream& strm)";
		if (a == b - 1)
			h += ";\n";
		else
			h += ", ";
	}
	//*/
	h += "};";

	//SceneScriptResolver.cpp
	string s = "#include \"SceneScriptResolver.h\"\n#include \"Engine.h\"\n\n";
	//*
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		int flo = headerAssets[a].find_last_of('\\') + 1;
		if (flo == string::npos) flo = 0;
		string ha = headerAssets[a].substr(flo);
		s += "#include \"..\\Assets\\" + headerAssets[a] + "\"\n";
		string ss = ha.substr(0, ha.size() - 2);
		s += "SceneScript* SceneScriptResolver::_Inst" + std::to_string(a) + "() { return new " + ss + "(); }\n\n";
	}
	//*/
	s += "\n\nusing namespace std;\r\nSceneScriptResolver* SceneScriptResolver::instance = nullptr;\nSceneScriptResolver::SceneScriptResolver() {\n\tinstance = this;\n";
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		s += "\tnames.push_back(R\"(" + headerAssets[a] + ")\");\n";
		s += "\tmap.push_back(&_Inst" + std::to_string(a) + ");\n";
		s += "\tass.push_back(&_Ass" + std::to_string(a) + ");\n";
	}
	s += "}\n\n";
	s += "SceneScript* SceneScriptResolver::Resolve(std::ifstream& strm) {\n\tchar* c = new char[100];\n\tstrm.getline(c, 100, 0);\n\tstring s(c);\n\tdelete[](c);";
	s += "\n\tint a = 0;\n\tfor (string ss : names) {\n\t\tif (ss == s) {\n\t\t\tSceneScript* scr = map[a]();\n\t\t\tscr->name = s + \" (Script)\";\n\t\t\t(*ass[a])(scr, strm);\n\t\t\treturn scr;\n\t\t}\n\t\ta++;\n\t}\n\treturn nullptr;\n}";

	GenerateScriptValuesReader(s);

	string cppO = projectFolder + "\\System\\SceneScriptResolver.cpp";
	string hO = projectFolder + "\\System\\SceneScriptResolver.h";

	std::remove(hO.c_str());
	std::remove(cppO.c_str());

	std::ofstream ofs(cppO.c_str(), std::ios::out | std::ios::trunc);
	ofs << s;
	ofs.close();
	//SetFileAttributes(cppO.c_str(), FILE_ATTRIBUTE_HIDDEN);
	ofs.open(hO.c_str(), std::ios::out | std::ios::trunc);
	ofs << h;
	ofs.close();
	//SetFileAttributes(hO.c_str(), FILE_ATTRIBUTE_HIDDEN);
}

void Editor::GenerateScriptValuesReader(string& s) {
	string tmpl = "\tushort sz = 0;\n\t_Strm2Val<ushort>(strm, sz);\n\tSCR_VARTYPE t;\n";
	tmpl += "\tfor (ushort x = 0; x < sz; x++) {\n\t\t_Strm2Val<SCR_VARTYPE>(strm, t);\n";
	tmpl += "\t\tchar c[100];\n\t\tstrm.getline(&c[0], 100, 0);\n\t\tstring s(c);\n\n";
	s += "\n\n";

	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		int flo = headerAssets[a].find_last_of('\\') + 1;
		if (flo == string::npos) flo = 0;
		string ha = headerAssets[a].substr(flo);
		ha = ha.substr(0, ha.size() - 2);

		s += "void SceneScriptResolver::_Ass" + std::to_string(a) + " (SceneScript* sscr, std::ifstream& strm) {\n\t" + ha + "* scr = (" + ha + "*)sscr;\n" + tmpl;
		std::ifstream mstrm(projectFolder + "Assets\\" + headerAssets[a] + ".meta", std::ios::in | std::ios::binary);
		if (!mstrm.is_open()) {
			_Error("Script Component", "Cannot read meta file!");
			return;
		}
		ushort sz;
		_Strm2Val<ushort>(mstrm, sz);
		SCR_VARTYPE t;
		for (ushort x = 0; x < sz; x++) {
			char c[100];
			_Strm2Val<SCR_VARTYPE>(mstrm, t);
			mstrm.getline(&c[0], 100, 0);
			if (t == SCR_VAR_COMMENT)
				continue;
			string vn = string(c);
			s += "\t\tif (s == \"" + vn + "\") {\n";
			switch (t) {
			case SCR_VAR_INT:
				s += "\t\t\tint ii;\n\t\t\t_Strm2Val<int>(strm, ii);\n";
				s += "\t\t\tscr->" + vn + "+=ii;";
				break;
			case SCR_VAR_FLOAT:
				s += "\t\t\tfloat ff;\n\t\t\t_Strm2Val<float>(strm, ff);\n";
				s += "\t\t\tscr->" + vn + "+=ff;";
				break;
			case SCR_VAR_TEXTURE:
				s += "\t\t\tASSETTYPE tdd;\n\t\t\tASSETID idd;\n\t\t\t_Strm2Asset(strm, nullptr, tdd, idd);\n";
				s += "\t\t\tif (idd > -1) {\n\t\t\t\tscr->" + vn + "=_GetCache<Texture>(tdd, idd);\n\t\t\t}\n";
				break;
			}
			s += "\n\t\t}\n";
		}
		s += "\t}\n}\n\n";
	}
}

void Editor::NewScene() {
	if (activeScene) delete(activeScene);
	activeScene = new Scene();
}

void Editor::UpdateLerpers() {
	for (int a = xLerper.size() - 1; a >= 0; a--) {
		if (xLerper[a].Update()) {
			xLerper.erase(xLerper.begin() + a);
		}
	}
	for (int a = yLerper.size() - 1; a >= 0; a--) {
		if (yLerper[a].Update()) {
			yLerper.erase(yLerper.begin() + a);
		}
	}
}

Color _col(Vec4(1.0f, 0.0f, 0.0f, 1.0f));
void Editor::DrawPopup() {
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, Camera::screenRectVerts);
	glUseProgram(popupShadeProgram);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, Camera::screenRectVerts);

	glUniform2f(glGetUniformLocation(popupShadeProgram, "screenSize"), (float)Display::width, (float)Display::height);
	glUniform1f(glGetUniformLocation(popupShadeProgram, "distance"), 10.0f);
	glUniform1f(glGetUniformLocation(popupShadeProgram, "intensity"), 0.2f);
	glUniform4f(glGetUniformLocation(popupShadeProgram, "pos"), popupPos.x, Display::height - popupPos.y - popup->h, (float)popup->w, (float)popup->h);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);
	popup->Draw();
}

void Editor::DrawHandles() {
	bool moused = false;
	int r = 0;
	for (EditorBlock* b : blocks)
	{
		Vec4 v = Vec4(Display::width*xPoss[b->x1], Display::height*yPoss[b->y1], Display::width*xPoss[b->x2], Display::height*yPoss[b->y2]);
		if (b->maximize) v = Vec4(0, 0, Display::width, Display::height);
		else if (hasMaximize) continue;

		//Engine::DrawQuad(v.r + 1, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_SIZE - 1 - EB_HEADER_PADDING, grey1());
		//Engine::DrawQuad(v.b - EB_HEADER_PADDING, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_SIZE - 1 - EB_HEADER_PADDING, grey1());
		if (Engine::EButton((b->editor->editorLayer == 0), v.r + 1, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, tex_buttonExt, white(1, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) { //splitter top left
			if (b->headerStatus == 1)
				b->headerStatus = 0;
			//else b->headerStatus = (b->headerStatus == 0 ? 1 : 0);
		}
		if (b->headerStatus == 1) {
			Engine::RotateUI(90, Vec2(v.r + 2 + 1.5f*EB_HEADER_PADDING, v.g + 1 + 0.5f*EB_HEADER_PADDING));
			if (Engine::EButton((b->editor->editorLayer == 0), v.r + 2 + EB_HEADER_PADDING, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, tex_buttonExtArrow, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {
				b->headerStatus = 0;
				xPoss.push_back(xPoss.at(b->x1));
				xLerper.push_back(xPossLerper(this, xPoss.size() - 1, xPoss.at(b->x1), 0.5f*(xPoss.at(b->x1) + xPoss.at(b->x2))));
				//xPoss.at(xPoss.size()-1) = 0.5f*(xPoss.at(b->x1) + xPoss.at(b->x2));
				xLimits.push_back(xLimits.at(xLimits.size() - 1));
				blocks.push_back(new EB_Empty(this, b->x1, b->y1, xPoss.size() - 1, b->y2));

				b->x1 = xPoss.size() - 1;
				Engine::ResetUIMatrix();
				break;
			}
			Engine::ResetUIMatrix();
			Engine::RotateUI(180, Vec2(v.r + 1 + 0.5f*EB_HEADER_PADDING, v.g + 2 + 1.5f*EB_HEADER_PADDING));
			if (Engine::EButton((b->editor->editorLayer == 0), v.r + 1, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_PADDING, tex_buttonExtArrow, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {
				b->headerStatus = 0;
				yPoss.push_back(yPoss.at(b->y1));
				yLerper.push_back(yPossLerper(this, yPoss.size() - 1, yPoss.at(b->y1), 0.5f*(yPoss.at(b->y1) + yPoss.at(b->y2))));
				//b->editor->yPoss.at(b->editor->yPoss.size() - 1) = 0.5f*(b->editor->yPoss.at(b->y1) + b->editor->yPoss.at(b->y2));
				yLimits.push_back(yLimits.at(yLimits.size() - 1));
				blocks.push_back(new EB_Empty(this, b->x1, b->y1, b->x2, yPoss.size() - 1));

				b->y1 = yPoss.size() - 1;
				Engine::ResetUIMatrix();
				break;
			}
			Engine::ResetUIMatrix();
		}
		//Engine::DrawQuad(v.b - EB_HEADER_PADDING, v.a - EB_HEADER_PADDING - 1, EB_HEADER_PADDING, EB_HEADER_PADDING, tex_buttonExt->pointer); //splitter bot right
		if (Engine::EButton((editorLayer == 0), v.b - EB_HEADER_PADDING, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, tex_buttonX, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {//window mod
																																																	//blocks.erase(blocks.begin() + r);
																																																	//delete(b);
																																																	//break;
		}
		r++;
	}

	if (!hasMaximize) {
		for (BlockCombo* c : blockCombos) {
			c->Draw();
		}

		// adjust lines
		if (activeX == -1 && activeY == -1) {
			byte b;
			for (int q = xPoss.size() - 1; q >= 2; q--) { //ignore 0 1
				Int2 lim = xLimits.at(q);
				b = Engine::EButton((editorLayer == 0), xPoss.at(q)*Display::width - 2, Display::height*yPoss.at(lim.x), 4, Display::height*yPoss.at(lim.y), black(0), white(0.3f, 1), white(0.7f, 1));
				if (b && MOUSE_HOVER_FLAG > 0) {
					if (b == MOUSE_CLICK) {
						activeX = q;
						dw = 1.0f / Display::width;
						dh = 1.0f / Display::height;
						//mousePosOld = Input::mousePosRelative.x;
					}
					moused = true;
				}
			}
			for (int q = yPoss.size() - 1; q >= 2 && !moused; q--) { //ignore 0 1
				Int2 lim = yLimits.at(q);
				b = Engine::EButton((editorLayer == 0), xPoss.at(lim.x)*Display::width, yPoss.at(q)*Display::height - 2, xPoss.at(lim.y)*Display::width, 4, black(0), white(0.3f, 1), white(0.7f, 1));
				if (b && MOUSE_HOVER_FLAG > 0) {
					if (b == MOUSE_CLICK) {
						activeY = q;
						dw = 1.0f / Display::width;
						dh = 1.0f / Display::height;
						//mousePosOld = Input::mousePosRelative.x;
					}
					moused = true;
				}
			}
		}
		else if (activeX > -1) {
			if (!Input::mouse0) {
				activeX = -1;
			}
			else {
				Engine::DrawQuad(Input::mousePosRelative.x*Display::width - 2, 0.0f, 4.0f, (float)Display::height, white(0.7f, 1));
				xPoss[activeX] = Clamp(Input::mousePosRelative.x, dw * 2, 1 - dw * 5);
			}
			moused = true;
		}
		else {
			if (!Input::mouse0) {
				activeY = -1;
			}
			else {
				Engine::DrawQuad(0.0f, Input::mousePosRelative.y*Display::height - 2, (float)Display::width, 4.0f, white(0.7f, 1));
				yPoss[activeY] = Clamp(Input::mousePosRelative.y, dh * 2, 1 - dh * 5);
			}
			moused = true;
		}
	}

	if (editorLayer > 0) {
		if (editorLayer == 1) {
			Engine::DrawQuad(0.0f, 0.0f, (float)Display::width, (float)Display::height, black(0.3f));
			UI::Label(popupPos.x + 2, popupPos.y, 12, menuTitle, font, white());
			int off = 14;
			for (int r = 0, q = menuNames.size(); r < q; r++) {
				bool canPress = false;
				if (menuFuncIsSingle)
					canPress = menuFuncSingle != nullptr;
				else
					canPress = menuFuncs[r] != nullptr;
				if (Engine::Button(popupPos.x, popupPos.y + off, 200, 15, white(1, Input::KeyHold(InputKey(Key_1 + r)) ? 0.3f : 0.7f), "(" + std::to_string(r + 1) + ") " + menuNames[r], 12, font, canPress ? black() : red(1, 0.6f)) == MOUSE_RELEASE || Input::KeyUp(InputKey(Key_1 + r))) {
					editorLayer = 0;
					if (menuFuncIsSingle) {
						if (canPress) {
							menuFuncSingle(menuBlock, menuFuncVals[r]);
						}
						for (void* v : menuFuncVals)
							delete(v);
					}
					else if (canPress)
						menuFuncs[r](menuBlock);
					return;
				}
				off += 16;
				if ((menuPadding & (1 << r)) > 0) {
					off++;
				}

			}
			if (off == 14) {
				UI::Label(popupPos.x + 2, popupPos.y + off, 12, "Nothing here!", font, white(0.7f));
			}
			if (Input::mouse0State == MOUSE_UP)
				editorLayer = 0;
		}
		else if (editorLayer == 2) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.4f));
			Engine::DrawQuad(editingArea.x, editingArea.y, editingArea.w, editingArea.h, grey2());
			UI::Label(editingArea.x + 2, editingArea.y, 12, editingVal, font, editingCol);
		}
		else if (editorLayer == 3) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.8f));
			UI::Label(Display::width*0.2f + 6, Display::height*0.2f, 22, browseIsComp ? "Select Component" : "Select Asset", font, white());
			if (Engine::Button(Display::width*0.2f + 6, Display::height*0.2f + 26, Display::width*0.3f - 7, 14, Vec4(0.4f, 0.2f, 0.2f, 1), "undefined", 12, font, white()) == MOUSE_RELEASE) {
				if (browseIsComp) {
					browseTargetComp->comp = nullptr;
					browseTargetComp->path = "";
				}
				else {
#ifdef IS_EDITOR
					UndoStack::Add(new UndoStack::UndoObj(browseTarget, browseTarget, 4, browseCallback, browseCallbackParam));
#endif
					*browseTarget = -1;
				}
				editorLayer = 0;
				if (browseCallback != nullptr)
					(*browseCallback)(browseCallbackParam);
				return;
			}
			if (browseIsComp) {
				for (int r = 0, rr = browseCompList.size(); r < rr; r++) {
					if (Engine::Button(Display::width*0.2f + 6 + (Display::width*0.3f - 5)*((r + 1) & 1), Display::height*0.2f + 41 + 15 * (((r + 1) >> 1) - 1), Display::width*0.3f - 7, 14, grey2(), browseCompListNames[r], 12, font, white()) == MOUSE_RELEASE) {
						browseTargetComp->comp = browseCompList[r];
						browseTargetComp->path = browseCompListNames[r];
						editorLayer = 0;
						if (browseCallback != nullptr)
							(*browseCallback)(browseCallbackParam);
						return;
					}
				}
			}
			else {
				int ar = 1;
				for (int e = 0, ee = internalAssets[browseType].size(); e < ee; e++, ar++) {
					byte b = Engine::Button(Display::width*0.2f + 6 + (Display::width*0.3f - 5)*(ar & 1), Display::height*0.2f + 41 + 15 * ((ar >> 1) - 1), Display::width*0.3f - 7, 14, Vec4(0.2f, 0.2f, 0.4f, 1), internalAssets[browseType][e], 12, font, white());
					if (b & MOUSE_HOVER_FLAG) {

					}
				}
				for (int r = 0, rr = proceduralAssets[browseType].size(); r < rr; r++, ar++) {
					byte b = Engine::Button(Display::width*0.2f + 6 + (Display::width*0.3f - 5)*(ar & 1), Display::height*0.2f + 41 + 15 * ((ar >> 1) - 1), Display::width*0.3f - 7, 14, Vec4(0.2f, 0.4f, 0.2f, 1), proceduralAssets[browseType][r], 12, font, white());
					if (b & MOUSE_HOVER_FLAG) {
						if (b == MOUSE_RELEASE) {
							*browseTarget = -(r + (1 << 8));
							editorLayer = 0;
							if (browseCallback != nullptr)
								(*browseCallback)(browseCallbackParam);
							return;
						}
					}
				}
				for (int r = 0, rr = normalAssets[browseType].size(); r < rr; r++, ar++) {
					byte b = Engine::Button(Display::width*0.2f + 6 + (Display::width*0.3f - 5)*(ar & 1), Display::height*0.2f + 41 + 15 * ((ar >> 1) - 1), Display::width*0.3f - 7, 14, grey2(), normalAssets[browseType][r], 12, font, white());
					if (b & MOUSE_HOVER_FLAG) {
						pendingPreview = true;
						previewType = browseType;
						previewId = r;
						if (b == MOUSE_RELEASE) {
#ifdef IS_EDITOR
							UndoStack::Add(new UndoStack::UndoObj(browseTarget, browseTarget, 4, browseCallback, browseCallbackParam));
#endif
							*browseTarget = r;
							editorLayer = 0;
							if (browseCallback != nullptr)
								(*browseCallback)(browseCallbackParam);
							return;
						}
					}
				}
			}
		}
		else if (editorLayer == 4) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.8f));
			Engine::DrawProgressBar(Display::width*0.5f - 300, Display::height*0.5f - 10.0f, 600.0f, 20.0f, progressValue, white(1, 0.2f), tex_background, Vec4(0.43f, 0.57f, 0.14f, 1), 1, 2);
			UI::Label(Display::width*0.5f - 298, Display::height*0.5f - 25.0f, 12, progressName, font, white());
			UI::Label(Display::width*0.5f - 296, Display::height*0.5f - 6.0f, 12, progressDesc, font, white(0.7f));
		}
		else if (editorLayer == 5) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.6f));
			Engine::DrawQuad(Display::width*0.1f, Display::height*0.1f, Display::width*0.8f, Display::height*0.8f, black(0.8f));
			int offy = 18;

			UI::Label(Display::width*0.1f + 10, Display::height*0.1f + offy, 21, "Viewer settings", font, white());
			_showGrid = Engine::Toggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 26, 14, tex_checkbox, _showGrid, white(), ORIENT_HORIZONTAL);
			UI::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 28, 12, "Show grid", font, white());
			_mouseJump = Engine::Toggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 46, 14, tex_checkbox, _mouseJump, white(), ORIENT_HORIZONTAL);
			UI::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 48, 12, "Mouse stay inside window", font, white());

			offy = 100;
			UI::Label(Display::width*0.1f + 10, Display::height*0.1f + offy, 21, "Editor settings", font, white());
			UI::Label(Display::width*0.1f + 10, Display::height*0.1f + offy + 28, 12, "Background Image", font, white());
			Engine::Button(Display::width*0.1f + 200, Display::height*0.1f + offy + 25, 500, 16, grey2(), backgroundPath, 12, font, white());
			if (backgroundTex != nullptr) {
				Engine::Button(Display::width*0.1f + 702, Display::height*0.1f + offy + 25, 16, 16, tex_buttonX, white(0.8f), white(), white(0.4f));

				UI::Label(Display::width*0.1f + 10, Display::height*0.1f + offy + 48, 12, "Background Brightness", font, white());
				Engine::Button(Display::width*0.1f + 200, Display::height*0.1f + offy + 46, 70, 16, grey2(), std::to_string(backgroundAlpha), 12, font, white());
				backgroundAlpha = (byte)Engine::DrawSliderFill(Display::width*0.1f + 271, Display::height*0.1f + offy + 46, 200, 16, 0, 100, backgroundAlpha, grey2(), white());
			}

			offy = 190;
			UI::Label(Display::width*0.1f + 10, Display::height*0.1f + offy, 21, "Build settings", font, white());
			UI::Label(Display::width*0.1f + 10, Display::height*0.1f + offy + 28, 12, "Data bundle size (x100Mb)", font, white());
			Engine::Button(Display::width*0.1f + 200, Display::height*0.1f + offy + 25, 70, 16, grey2(), std::to_string(_assetDataSize), 12, font, white());
			_cleanOnBuild = Engine::Toggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 46, 14, tex_checkbox, _cleanOnBuild, white(), ORIENT_HORIZONTAL);
			UI::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 48, 12, "Remove visual studio files on build", font, white());
		}
		else if (editorLayer == 6) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.8f));
			Engine::DrawProgressBar(50.0f, Display::height - 30.0f, Display::width - 100.0f, 20.0f, buildProgressValue, white(1, 0.2f), tex_background, buildProgressVec4, 1, 2);
			UI::Label(55.0f, Display::height - 26.0f, 12, buildLabel, font, white());
			for (int i = buildLog.size() - 1, dy = 0; i >= 0; i--) {
				UI::Label(30.0f, Display::height - 50.0f - 15.0f*dy, 12.0f, buildLog[i], font, buildLogErrors[i] ? red() : (buildLogWarnings[i] ? yellow() : white(0.7f)), Display::width - 50.0f);
				dy++;
			}
		}
		else if (editorLayer == 7) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.6f));
			Engine::DrawQuad(Display::width*0.2f, Display::height*0.2f, Display::width*0.6f, Display::height*0.6f, black(0.8f));

			UI::Label(Display::width*0.2f + 10, Display::height*0.2f + 10, 21, "Build settings", font, white());

			uint sz = includedScenes.size();
			UI::Label(Display::width*0.2f + 15, Display::height*0.2f + 35, 12, "Included scenes: " + std::to_string(sz), font, white());
			Engine::DrawQuad(Display::width*0.2f + 12, Display::height*0.2f + 50, Display::width*0.3f, 306, white(0.05f));
			for (uint a = 0; a < sz; a++) {
				includedScenesUse[a] = Engine::Toggle(Display::width*0.2f + 14, Display::height*0.2f + 51 + a * 15, 14, tex_checkbox, includedScenesUse[a], white(), ORIENT_HORIZONTAL);
				UI::Label(Display::width*0.2f + 30, Display::height*0.2f + 52 + a * 15, 12, includedScenes[a], font, white());
			}

			if (Engine::Button(Display::width*0.8f - 80, Display::height*0.2f + 2, 78, 18, grey2(), "Save", 18, font, white()) == MOUSE_RELEASE) {
				SavePrefs();
			}
		}
	}

	if (popup) {
		DrawPopup();
		if (Input::mouse0State == MOUSE_DOWN && !Rect(popupPos.x, popupPos.y, (float)popup->w, (float)popup->h).Inside(Input::mousePos)) {
			delete(popup);
			popup = nullptr;
		}
	}

	if (pendingPreview) {
		pendingPreview = false;
		previewTime += Time::delta;
		if (previewTime > minPreviewTime) {
			float px = Input::mousePos.x;
			if (px > Display::width - 300) px -= 300;
			float py = Input::mousePos.y;
			if (py > 300) py -= 300;
			Engine::DrawQuad(px + 20, py + 20, 256, 256, black(0.85f));
			if (!((AssetObject*)GetCache(previewType, previewId))->DrawPreview((uint)(px + 22), (uint)(py + 22), 252, 252)) {
				Engine::DrawLine(Vec3(px + 20, py + 20, 0), Vec3(px + 276, py + 276, 0), grey1(), 1.5f);
				Engine::DrawLine(Vec3(px + 20, py + 276, 0), Vec3(px + 276, py + 20, 0), grey1(), 1.5f);
				font->Align(ALIGN_MIDCENTER);
				UI::Label(px + 148, py + 148, 12, "No preview", font, white());
				font->Align(ALIGN_BOTLEFT);
			}
		}
	}
	else previewTime = 0;
}

void Editor::RegisterMenu(EditorBlock* block, string title, std::vector<string> names, std::vector<shortcutFunc> funcs, int padding, Vec2 pos) {
	editorLayer = 1;
	menuFuncIsSingle = false;
	menuTitle = title;
	menuBlock = block;
	menuNames = names;
	menuFuncs = funcs;
	popupPos = pos;
	menuPadding = padding;
	if (popup) {
		delete(popup);
		popup = nullptr;
	}
}

void Editor::RegisterMenu(EditorBlock* block, string title, std::vector<string> names, dataFunc func, std::vector<void*> vals, int padding, Vec2 pos) {
	editorLayer = 1;
	menuFuncIsSingle = true;
	menuTitle = title;
	menuBlock = block;
	menuNames = names;
	menuFuncSingle = func;
	menuFuncVals = vals;
	popupPos = pos;
	menuPadding = padding;
	if (popup) {
		delete(popup);
		popup = nullptr;
	}
}

Texture* Editor::GetRes(string name) {
	return GetRes(name, false, false);
}
Texture* Editor::GetResExt(string name, string ext) {
	return GetRes(name, false, false, ext);
}
Texture* Editor::GetRes(string name, bool mipmap, string ext) {
	return GetRes(name, mipmap, false, ext);
}
Texture* Editor::GetRes(string name, bool mipmap, bool nearest, string ext) {
	return new Texture(dataPath + "res\\" + name + "." + ext, mipmap, nearest ? TEX_FILTER_POINT : TEX_FILTER_TRILINEAR, 0);
}

void Editor::_Message(string a, string b) {
	messages.push_back(std::pair<string, string>(a, b));
	messageTypes.push_back(0);
	for (EditorBlock* eb : blocks) {
		if (eb->editorType == 10)
			eb->Refresh();
	}
}
void Editor::_Warning(string a, string b) {
	messages.push_back(std::pair<string, string>(a, b));
	messageTypes.push_back(1);
	for (EditorBlock* eb : blocks) {
		if (eb->editorType == 10)
			eb->Refresh();
	}
}
void Editor::_Error(string a, string b) {
	messages.push_back(std::pair<string, string>(a, b));
	messageTypes.push_back(2);
	for (EditorBlock* eb : blocks) {
		if (eb->editorType == 10)
			eb->Refresh();
	}
}

void DoScanAssetsGet(Editor* e, std::vector<string>& list, string p, bool rec) {
	std::vector<string> files = IO::GetFiles(p);
	for (string w : files) {
		string ww = w.substr(w.find_last_of("."), string::npos);
		ASSETTYPE type = GetFormatEnum(ww);
		if (type != ASSETTYPE_UNDEF) {
			//std::cout << "file " << w << std::endl;
			string ss = w + ".meta";//ss.substr(0, ss.size() - 5);
			ww = (w.substr(e->projectFolder.size() + 7, string::npos));
			//string ww2 = ww.substr(0, ww.size()-5);
			if (type == ASSETTYPE_BLEND)
				e->blendAssets.push_back(ww.substr(0, ww.find_last_of('\\')) + ww.substr(ww.find_last_of('\\') + 1, string::npos));
			else {
				e->normalAssets[type].push_back(ww.substr(0, ww.find_last_of('\\')) + ww.substr(ww.find_last_of('\\') + 1, string::npos));
				e->normalAssetCaches[type].push_back(nullptr);
				e->normalAssetMakings[type].push_back(false);
			}
			if (type == ASSETTYPE_MATERIAL || type == ASSETTYPE_CAMEFFECT || type == ASSETTYPE_SCENE || type == ASSETTYPE_ANIMATION) //no meta file
				continue;
			if (IO::HasFile(ss.c_str())) {
				FILETIME metaTime, realTime;
				HANDLE metaF = CreateFile(ss.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
				HANDLE realF = CreateFile(w.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
				GetFileTime(metaF, nullptr, nullptr, &metaTime);
				GetFileTime(realF, nullptr, nullptr, &realTime);
				CloseHandle(metaF);
				CloseHandle(realF);
				ULARGE_INTEGER metaT, realT;
				metaT.HighPart = metaTime.dwHighDateTime;
				metaT.LowPart = metaTime.dwHighDateTime;
				realT.HighPart = realTime.dwHighDateTime;
				realT.LowPart = realTime.dwLowDateTime;
				if (realT.QuadPart < metaT.QuadPart) {
					continue;
				}
			}
			e->_Message("Asset Loader", "Reloading " + w);
			list.push_back(w);
		}
	}
	if (rec) {
		std::vector<string> dirs;
		IO::GetFolders(p, &dirs, true);
		for (string d : dirs) {
			if (d != "." && d != "..")
				DoScanAssetsGet(e, list, p + d + "\\", true);
		}
	}
}

void DoScanMeshesGet(Editor* e, std::vector<string>& list, string p, bool rec) {
	std::vector<string> files = IO::GetFiles(p);
	for (string w : files) {
		if ((w.size() > 10) && ((w.substr(w.size() - 10, string::npos)) == ".mesh.meta")) {
			string ww(w.substr(e->projectFolder.size() + 7, string::npos));
			ww = ww.substr(0, ww.find_last_of('\\')) + ww.substr(ww.find_last_of('\\') + 1, string::npos);
			e->normalAssets[ASSETTYPE_MESH].push_back(ww.substr(0, ww.size() - 10));
			e->normalAssetCaches[ASSETTYPE_MESH].push_back(nullptr);
			e->normalAssetMakings[ASSETTYPE_MESH].push_back(nullptr);
		}
	}
	if (rec) {
		std::vector<string> dirs;
		IO::GetFolders(p, &dirs, true);
		for (string d : dirs) {
			if (d != "." && d != "..")
				DoScanMeshesGet(e, list, p + d + "\\", true);
		}
	}
}

void DoReloadAssets(Editor* e, string path, bool recursive, std::mutex* l) {
	std::vector<string> files;
	DoScanAssetsGet(e, files, path, recursive);
	int r = files.size(), i = 0;
	for (string f : files) {
		{
			std::lock_guard<std::mutex> lock(*l);
			e->progressDesc = f;
		}
		e->ParseAsset(f);
		i++;
		e->progressValue = i*100.0f / r;
	}
	DoScanMeshesGet(e, files, path, true);
	{
		std::lock_guard<std::mutex> lock(*l);
		e->headerAssets.clear();
		AddH(e, e->projectFolder + "Assets", &e->headerAssets, &e->cppAssets);
	}
	for (string s : e->headerAssets) {
		std::lock_guard<std::mutex> lock(*l);
		SceneScript::Parse(s, e);
	}
	{
		//e->GenerateScriptResolver();
		for (EditorBlock* eb : e->blocks) {
			if (eb->editorType == 1)
				eb->Refresh();
		}
	}
	e->editorLayer = 0;
	std::lock_guard<std::mutex> lock(*l);
	for (auto b : e->blocks) {
		b->Refresh();
	}
	e->progressValue = 100;
	e->progressDesc = "Generating index...";
	e->MergeAssets_(e);
}

void Editor::ClearLogs() {
	messages.clear();
	messageTypes.clear();
	for (EditorBlock* eb : blocks) {
		if (eb->editorType == 10)
			eb->Refresh();
	}
}

void Editor::DeselectFile() { //do not free pointer as cache is reused
	if (selectedFileType != ASSETTYPE_UNDEF) {
		selectedFileName = "";
		selectedFileType = ASSETTYPE_UNDEF;
		selectedFileCache = nullptr;
	}
}

void Editor::ResetAssetMap() {
	normalAssets[ASSETTYPE_TEXTURE] = std::vector<string>();
	normalAssets[ASSETTYPE_HDRI] = std::vector<string>();
	normalAssets[ASSETTYPE_MATERIAL] = std::vector<string>();
	normalAssets[ASSETTYPE_MESH] = std::vector<string>();
	normalAssets[ASSETTYPE_BLEND] = std::vector<string>();
	normalAssets[ASSETTYPE_ANIMCLIP] = std::vector<string>();
	normalAssets[ASSETTYPE_ANIMATION] = std::vector<string>();
	normalAssets[ASSETTYPE_CAMEFFECT] = std::vector<string>();

	derivedAssets[ASSETTYPE_TEXTURE_REND] = std::pair<ASSETTYPE, std::vector<uint>>(ASSETTYPE_TEXTURE, std::vector<uint>());

	normalAssetCaches[ASSETTYPE_TEXTURE] = std::vector<pAssetObject>(); //Texture*
	normalAssetCaches[ASSETTYPE_HDRI] = std::vector<pAssetObject>(); //Background*
	normalAssetCaches[ASSETTYPE_MATERIAL] = std::vector<pAssetObject>(); //Material*
	normalAssetCaches[ASSETTYPE_MESH] = std::vector<pAssetObject>(); //Mesh*
	normalAssetCaches[ASSETTYPE_ANIMCLIP] = std::vector<pAssetObject>(); //AnimClip*
	normalAssetCaches[ASSETTYPE_ANIMATION] = std::vector<pAssetObject>(); //Animator*
	normalAssetCaches[ASSETTYPE_CAMEFFECT] = std::vector<pAssetObject>(); //CameraEffect*

	normalAssetMakings[ASSETTYPE_TEXTURE] = std::vector<bool>();
	normalAssetMakings[ASSETTYPE_HDRI] = std::vector<bool>();
	normalAssetMakings[ASSETTYPE_MATERIAL] = std::vector<bool>();
	normalAssetMakings[ASSETTYPE_MESH] = std::vector<bool>();
	normalAssetMakings[ASSETTYPE_BLEND] = std::vector<bool>();
	normalAssetMakings[ASSETTYPE_ANIMCLIP] = std::vector<bool>();
	normalAssetMakings[ASSETTYPE_ANIMATION] = std::vector<bool>();
	normalAssetMakings[ASSETTYPE_CAMEFFECT] = std::vector<bool>();

	if (!internalAssetsLoaded) {
		internalAssets = std::unordered_map<ASSETTYPE, std::vector<string>>(normalAssets);
		internalAssetCaches = std::unordered_map<ASSETTYPE, std::vector<pAssetObject>>(normalAssetCaches);
		proceduralAssets = std::unordered_map<ASSETTYPE, std::vector<string>>(normalAssets);
		proceduralAssetCaches = std::unordered_map<ASSETTYPE, std::vector<pAssetObject>>(normalAssetCaches);
		LoadInternalAssets();
	}
}

void Editor::LoadInternalAssets() {
	std::ifstream strm(dataPath + "res\\InternalAssets\\index");
	char cs[50];
	ASSETTYPE actt = ASSETTYPE_UNDEF;
	while (!strm.eof()) {
		strm.getline(cs, 50);
		string s(cs);
		if (s.empty() || cs[0] == '#') continue;
		if (s.substr(0, 2) == "0x") actt = (ASSETTYPE)std::stoul(s, nullptr, 16);
		else {
			/*
			if (s.substr(0, 11) == "procedural\\") {
			proceduralAssets[actt].push_back(s);
			switch (actt) {
			case ASSETTYPE_MESH:
			proceduralAssetCaches[actt].push_back(Procedurals::UVSphere(16, 12));
			break;
			}
			}
			*/
		}
	}

	internalAssetsLoaded = true;
}

void Editor::ReloadAssets(string path, bool recursive) {

	//normalAssetsOld = std::unordered_map<ASSETTYPE, std::vector<string>>(normalAssets);

	ResetAssetMap();

	blendAssets.clear();
	editorLayer = 4;
	progressName = "Loading assets...";
	progressValue = 0;
	std::thread t(DoReloadAssets, this, path, recursive, lockMutex);
	t.detach();
}

bool Editor::ParseAsset(string path) {
	std::cout << "parsing " << path << std::endl;
	std::ifstream stream(path.c_str(), std::ios::in | std::ios::binary);
	bool ok;
	if (!stream.good()) {
		_Message("Asset Parser", "asset not found! " + path);
		return false;
	}
	string ext = path.substr(path.find_last_of('.') + 1, string::npos);
	string p = (path + ".meta");
	SetFileAttributes(p.c_str(), FILE_ATTRIBUTE_NORMAL);
	if (ext == "shade") {
		ok = Shader::Parse(&stream, path + ".meta");
	}
	else if (ext == "blend") {
		ok = Mesh::ParseBlend(this, path);
	}
	else if (ext == "bmp" || ext == "jpg" || ext == "png") {
		ok = Texture::Parse(this, path);
	}
	else if (ext == "rendtex") {
		ok = RenderTexture::Parse(path);
	}
	else if (ext == "hdr") {
		ok = Background::Parse(path);
	}
	else {
		_Message("Asset Parser", "format not supported! (" + ext + ")");
		return false;
	}
	stream.close();
	if (ok) {
		SetFileAttributes(p.c_str(), FILE_ATTRIBUTE_HIDDEN);
		return true;
	}
	else return false;
}

void Editor::SetEditing(byte t, string val, Rect a, Vec4 c) {
	editorLayer = 2;
	editingType = t;
	editingVal = val;
	editingArea = a;
	editingCol = c;
}

void Editor::SetBackground(string s, float a) {
	backgroundPath = s;
	if (backgroundTex)
		delete(backgroundTex);
	backgroundTex = new Texture(s);
	if (a >= 0)
		backgroundAlpha = (int)(a * 100);
}

void DoScanBrowseComp(SceneObject* o, COMPONENT_TYPE t, string p, std::vector<Component*>& vc, std::vector<string>& vs) {
	auto c = o->GetComponent(t).get();
	string nm = p + ((p == "") ? "" : "/") + o->name;
	if (c != nullptr) {
		vc.push_back(c);
		vs.push_back(nm);
	}
	for (auto oo : o->children)
		DoScanBrowseComp(oo.get(), t, nm, vc, vs);
}

void Editor::ScanBrowseComp() {
	browseCompList.clear();
	browseCompListNames.clear();
	for (auto o : activeScene->objects)
		DoScanBrowseComp(o.get(), browseTargetComp->type, "", browseCompList, browseCompListNames);
}

void Editor::RegisterPopup(PopupBlock* blk, Vec2 pos) {
	if (popup) delete(popup);
	popup = blk;
	popupPos = pos;
}

void Editor::AddBuildLog(Editor* e, string s, bool forceE) {
	std::lock_guard<std::mutex> lock(*Editor::lockMutex);
	buildLog.push_back(s);
	bool a = s.find("error C") != string::npos;
	bool b = s.find("error LNK") != string::npos;
	bool c = (s.find("warning C") != string::npos) || (s.find("warning LNK") != string::npos);
	buildLogErrors.push_back(a || b || forceE);
	buildLogWarnings.push_back(c);
	if (a && (buildErrorPath == "")) {
		while (s[0] == ' ' || s[0] == '\t')
			s = s.substr(1, string::npos);
		int i = s.find_first_of('(');
		if (i == string::npos)
			return;
		buildErrorPath = projectFolder + s.substr(0, i);
		string ii = s.substr(i + 1, s.find_first_of(')') - i - 1);
		buildErrorLine = stoi(ii);
	}
}

AssetObject* Editor::GetCache(ASSETTYPE type, int i, bool async) {
	if (i == -1)
		return nullptr;
	else if (i < 0) {
		if (i <= -(1 << 8))
			return proceduralAssetCaches[type][(-i) - (1 << 8)].get();
		else
			return internalAssetCaches[type][-i - 2].get();
	}
	else {
		bool making = normalAssetMakings[type][i];
		AssetObject *data = normalAssetCaches[type][i].get();
		if (!making && data)
			return data;
		else {
			if (async) {
				if (!making) GenCache(type, i, true);
				return nullptr;
			}
			else {
				if (!making)
					return GenCache(type, i);
				else {
					return nullptr;
				}
			}
		}
	}
}

AssetObject* Editor::GenCache(ASSETTYPE type, int i, bool async) {
	string pth = projectFolder + "Assets\\" + normalAssets[type][i];
	switch (type) {
	case ASSETTYPE_MESH:
		normalAssetCaches[type][i] = std::make_shared<Mesh>(pth + ".mesh.meta");
		break;
	case ASSETTYPE_ANIMCLIP:
		normalAssetCaches[type][i] = std::make_shared<AnimClip>(pth);
		break;
	case ASSETTYPE_ANIMATION:
		normalAssetCaches[type][i] = std::make_shared<Animation>(pth);
		break;
	case ASSETTYPE_SHADER:
		normalAssetCaches[type][i] = std::make_shared<Shader>(pth + ".meta");
		break;
	case ASSETTYPE_MATERIAL:
		normalAssetCaches[type][i] = std::make_shared<Material>(pth);
		break;
	case ASSETTYPE_TEXTURE:
		normalAssetCaches[type][i] = std::make_shared<Texture>(i, this);
		break;
	case ASSETTYPE_HDRI:
		normalAssetCaches[type][i] = std::make_shared<Background>(i, this);
		break;
	case ASSETTYPE_CAMEFFECT:
		normalAssetCaches[type][i] = std::make_shared<CameraEffect>(pth);
		break;
	default:
		return nullptr;
	}
	return normalAssetCaches[type][i].get();
}

/*
app.exe
content0_.data -> basepath, asset locs (type, name, index) (ascii), level locs
*/
bool Editor::MergeAssets_(Editor* e) {
#ifdef IS_EDITOR
	string ss = e->projectFolder + "Release\\data0_";
	AssetManager::dataECacheIds.clear();
	//std::cout << ss << std::endl;
	std::ofstream file(ss.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file.is_open())
		return false;
	//headers
	file << "D0";
	file << e->projectFolder + "Assets\\" << char0;

	//asset data
	//D0[NUM(2)]SUM([TYPE][LOC][NAME0])
	ushort xx = 0;
	long long poss1 = file.tellp();
	file << "00";
	for (auto& as : e->normalAssets) {
		ushort ii = 0;
		for (auto as2 : as.second) {
			AssetManager::dataECacheIds.push_back(std::pair<ASSETTYPE, ASSETID>(as.first, ii));
			//file << (char)as.first;
			_StreamWrite(&as.first, &file, 1);
			_StreamWrite(&ii, &file, 2);
			switch (as.first) {
			case ASSETTYPE_MESH:
				file << as2 + ".mesh.meta" << char0;
				break;
			case ASSETTYPE_ANIMCLIP:
				file << as2 + ".animclip.meta" << char0;
				break;
			case ASSETTYPE_MATERIAL:
			case ASSETTYPE_ANIMATION:
			case ASSETTYPE_CAMEFFECT:
				file << as2 << char0;
				break;
			case ASSETTYPE_SHADER:
			case ASSETTYPE_TEXTURE:
			case ASSETTYPE_HDRI:
				file << as2 + ".meta" << char0;
				break;
			default:
				Debug::Error("AssetMerger", "No type defined for " + std::to_string(as.first) + "!");
				return false;
			}
			file << as2 << (char)0;
			ii++;
		}
		xx += ii;
	}
	long long poss2 = file.tellp();
	file.seekp(poss1);
	_StreamWrite(&xx, &file, 2);
	file.seekp(poss2);

	//scene data
	ushort q = 0;
	poss1 = file.tellp();
	file << "00";
	for (string ss : e->includedScenes) {
		if (e->includedScenesUse[q]) {
			file << e->projectFolder + "Assets\\" + ss << char0;
			q++;
		}
	}
	poss2 = file.tellp();
	file.seekp(poss1);
	_StreamWrite(&q, &file, 2);
#endif
	return true;
}

/*
app.exe
content0.data -> asset list (type, name, index) (ascii), level data (binary)
content(1+).data ->assets (index, data) (binary)
*/
bool Editor::MergeAssets(Editor* e) {
#ifdef IS_EDITOR
	string ss = e->projectFolder + "Release\\data0";
	std::ofstream file;
	char null = 0, etx = 3;
	e->AddBuildLog(e, "Writing to " + ss);
	std::cout << ss << std::endl;
	file.open(ss.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file.is_open())
		return false;
	//headers
	file << "D0";

	//asset data
	//D0[NUM(2)]SUM([TYPE][IPOSS][NAME0])
	std::vector<uint> dataPoss;
	ushort xx = 0;
	long long poss1 = file.tellp();
	file << "00";
	for (auto& as : e->normalAssets) {
		//ushort ii = 0;
		for (auto as2 : as.second) {
			file << (char)as.first;
			//_StreamWrite(&ii, &file, 2);
			dataPoss.push_back((uint)file.tellp());
			file << "IPOSS";
			file << as2 << (char)0;
			//ii++;
			xx++;
		}
	}
	long long poss2 = file.tellp();
	file.seekp(poss1);
	_StreamWrite(&xx, &file, 2);
	file.seekp(poss2);

	//scene data
	ushort q = 0;
	poss1 = file.tellp();
	file << "00";
	for (string ss : e->includedScenes) {
		if (e->includedScenesUse[q]) {
			string sss = e->projectFolder + "Assets\\" + ss;
			std::ifstream f2(sss, std::ios::in | std::ios::binary);
			if (!f2.is_open()) {
				e->AddBuildLog(e, "Cannot open " + ss + "!", true);
				e->_Error("Asset Compiler", "Scene not found! " + ss);
				return false;
			}
			e->AddBuildLog(e, "Including " + ss);
			long long pos = file.tellp();
			file << "0000" << null << f2.rdbuf() << null;
			long long pos2 = file.tellp();
			uint size = (uint)(pos2 - pos - 6);
			file.seekp(pos);
			_StreamWrite(&size, &file, 4);
			file.seekp(pos2);
			f2.close();
			q++;
		}
	}
	poss2 = file.tellp();
	file.seekp(poss1);
	_StreamWrite(&q, &file, 2);

	byte incre = 1;
	string nm = e->projectFolder + "Release\\data" + std::to_string(incre);
	e->AddBuildLog(e, "Writing to " + nm);
	std::ofstream file2(nm.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file2.is_open()) {
		e->AddBuildLog(e, "Cannot open " + nm + "!", true);
		return false;
	}
	uint xxx = 0;
	for (auto& it : e->normalAssets) {
		if (it.second.size() > 0) {
			ushort i = 0;
			for (string s : it.second) {
				e->buildLabel = "Build: merging assets... " + s + "(" + std::to_string(xxx + 1) + "/" + std::to_string(xx) + ") ";
				string nmm;
				if (it.first == ASSETTYPE_MESH)
					nmm = e->projectFolder + "Assets\\" + s + ".mesh.meta";
				else if (it.first == ASSETTYPE_MATERIAL || it.first == ASSETTYPE_ANIMATION || it.first == ASSETTYPE_CAMEFFECT)
					nmm = e->projectFolder + "Assets\\" + s;
				else
					nmm = e->projectFolder + "Assets\\" + s + ".meta";
				std::ifstream f2(nmm.c_str(), std::ios::in | std::ios::binary);
				if (!f2.is_open()) {
					e->_Error("Asset Compiler", "Asset not found! " + nmm);
					return false;
				}
				//file2 << (char)it.first;
				//_StreamWrite(&i, &file2, 2);
				uint pos = (uint)file2.tellp();
				file.seekp(dataPoss[xxx]);
				file << incre;
				_StreamWrite(&pos, &file, 4);
				file2 << "0000" << f2.rdbuf();
				uint pos2 = (uint)file2.tellp();
				uint sz = pos2 - pos - 4;
				file2.seekp(pos);
				_StreamWrite(&sz, &file2, 4);
				file2.seekp(pos2);
				//file2 << "0000XX" << f2.rdbuf() << null;
				//file2.seekp(pos);
				//uint size = (uint)(pos2 - pos - 6);
				//_StreamWrite(&size, &file2, 4);
				//file2.seekp(pos2);
				if (pos2 > e->_assetDataSize * uint(10000000)) {
					//file2 << etx;
					file2.close();
					nm = e->projectFolder + "Release\\data" + std::to_string(++incre);
					e->AddBuildLog(e, "Writing to " + nm);
					file2.open(nm.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
					if (!file2.is_open()) {
						e->AddBuildLog(e, "Cannot open " + nm + "!", true);
						return false;
					}
				}
				i++;
				xxx++;
			}
		}
	}
	file2.close();
	file.close();
#endif
	return true;
}

bool DoMsBuild(Editor* e) {
	char s[255];
	DWORD i = 255;
	if (RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\4.0", "MSBuildToolsPath", RRF_RT_ANY, nullptr, &s, &i) == ERROR_SUCCESS) {
		std::cout << "Executing " << s << "msbuild.exe" << std::endl;

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = NULL;
		HANDLE stdOutR, stdOutW;
		if (!CreatePipe(&stdOutR, &stdOutW, &sa, 0)) {
			e->AddBuildLog(e, "failed to create pipe for stdout!");
			return false;
		}
		if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0)) {
			e->AddBuildLog(e, "failed to set handle for stdout!");
			return false;
		}
		STARTUPINFO startInfo;
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&startInfo, sizeof(STARTUPINFO));
		ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
		startInfo.cb = sizeof(STARTUPINFO);
		startInfo.hStdOutput = stdOutW;
		startInfo.dwFlags |= STARTF_USESTDHANDLES;

		string ss = (string(s) + "\\msbuild.exe");
		/*
		if (IO::HasFile("D:\\TestProject\\Debug\\TestProject.exe")) {
		e->buildLog.push_back("removing previous executable");
		if (remove("D:\\TestProject\\Debug\\TestProject.exe") != 0) {
		e->buildLog.push_back("unable to remove previous executable!");
		return false;
		}
		}
		*/
		bool failed = true;
		byte FINISH = 0;
		if (_putenv("MSBUILDDISABLENODEREUSE=1") == -1)
			std::cout << "Putenv failed for msbuilder!" << std::endl;
		if (CreateProcess(ss.c_str(), "D:\\TestProject2\\TestProject2.vcxproj /nr:false /t:Build /p:Configuration=Release /v:n /nologo /fl /flp:LogFile=D:\\TestProject2\\BuildLog.txt", NULL, NULL, true, 0, NULL, "D:\\TestProject2\\", &startInfo, &processInfo) != 0) {
			e->AddBuildLog(e, "Compiling from " + ss);
			std::cout << "compiling" << std::endl;
			DWORD w;
			do {
				w = WaitForSingleObject(processInfo.hProcess, DWORD(200));
				DWORD dwRead;
				CHAR chBuf[4096];
				bool bSuccess = FALSE;
				string out = "";
				bSuccess = ReadFile(stdOutR, chBuf, 4096, &dwRead, NULL) != 0;
				if (bSuccess && dwRead > 0) {
					string s(chBuf, dwRead);
					out += s;
				}
				for (uint r = 0; r < out.size();) {
					int rr = out.find_first_of('\n', r);
					if (rr == string::npos)
						rr = out.size() - 1;
					string sss = out.substr(r, rr - r);
					e->AddBuildLog(e, sss);
					if (sss.substr(0, 16) == "Build succeeded.") {
						failed = false;
						FINISH = 1;
					}
					else if (sss.substr(0, 12) == "Build FAILED") {
						FINISH = 1;
					}
					r = rr + 1;
				}
				if (FINISH == 1 && e->buildLog[e->buildLog.size() - 1].substr(0, 13) == "Time Elapsed ")
					FINISH = 2;
			} while (w == WAIT_TIMEOUT && FINISH != 2);

			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);

			//DWORD ii;
			//GetExitCodeProcess(processInfo.hProcess, &ii);
			//e->AddBuildLog("Compile finished with code " + std::to_string(ii) + ".");
			return (!failed);
		}
		else {
			e->AddBuildLog(e, "Cannot start msbuild!");
			CloseHandle(stdOutR);
			CloseHandle(stdOutW);
			return false;
		}
		CloseHandle(stdOutR);
		CloseHandle(stdOutW);
	}
	else {
		e->AddBuildLog(e, "msbuild version 4.0 not found!");
		return false;
	}
}

void Editor::Compile(Editor* e) {
	e->flags |= WAITINGBUILDSTARTFLAG;
}

void AddOtherScenes(Editor* e, string dir, std::vector<string> &v1, std::vector<bool> &v2) {
	for (string s : IO::GetFiles(dir)) {
		if (s.size() > 7 && s.substr(s.size() - 6, string::npos) == ".scene") {
			string pp = s.substr(s.find_last_of("\\") + 1, string::npos);
			for (string p2 : v1)
				if (p2 == pp)
					goto cancel;
			v1.push_back(pp);
			v2.push_back(false);
		}
	cancel:;
	}
	std::vector<string> f;
	IO::GetFolders(dir, &f);
	for (string ff : f)
		if (ff != "." && ff != "..")
			AddOtherScenes(e, ff, v1, v2);
}

void Editor::ShowPrefs(Editor* e) {
	e->editorLayer = 5;
}

void Editor::ShowCompileSett(Editor* e) {
	e->includedScenes.resize(e->savedIncludedScenes);
	e->includedScenesUse.resize(e->savedIncludedScenes);
	AddOtherScenes(e, e->projectFolder + "Assets\\", e->includedScenes, e->includedScenesUse);
	e->editorLayer = 7;
}

void Editor::SaveScene(Editor* e) {
#ifdef IS_EDITOR
	e->activeScene->Save(e);
#endif
}


void Editor::DoCompile() {
	buildEnd = false;
	buildErrorPath = "";
	buildProgressVec4 = white(1, 0.35f);
	editorLayer = 6;
	buildLog.clear();
	buildLogErrors.clear();
	buildLogWarnings.clear();
	buildLabel = "Build: rescanning assets...";
	AddBuildLog(this, "Creating msbuild files");
	buildProgressValue = 0;
	GenerateScriptResolver();
	//buildLabel = "Build: copying files...";
	buildProgressValue = 10;
	buildLabel = "Build: merging assets...";
	AddBuildLog(this, "Creating data files");
	string ss = projectFolder + "Release\\";
	CreateDirectory(ss.c_str(), NULL);
	/*copy files
	AddBuildLog(this, "Copying: dummy source directory -> dummy target directory");
	AddBuildLog(this, "Copying: dummy source directory2 -> dummy target directory2");
	AddBuildLog(this, "Copying: dummy source directory3 -> dummy target directory3");
	this_std::thread::sleep_for(chrono::seconds(2));
	*///merge assets
	//buildProgressValue = 10;
	if (!MergeAssets(this)) {
		buildLabel = "Build: failed.";
		buildProgressVec4 = red(1, 0.7f);
		buildEnd = true;
		return;
	}
	//compile
	buildProgressValue = 50;
	buildLabel = "Build: executing msbuild...";
	if (!DoMsBuild(this)) {
		buildLabel = "Build: failed.";
		buildProgressVec4 = red(1, 0.7f);
		AddBuildLog(this, "Press Enter to open first error file.");
	}
	else {//if (IO::HasFile("D:\\TestProject\\Debug\\TestProject.exe")) {
		if (_cleanOnBuild) {
			buildProgressValue = 90;
			AddBuildLog(this, "Cleaning up...");
			buildLabel = "Build: cleaning up...";
			//tr2::sys::remove_all("D:\\TestProject\\Release\\TestProject.tlog");
			for (string s1 : IO::GetFiles(projectFolder + "Release\\TestProject.tlog"))
			{
				AddBuildLog(this, "deleting " + s1);
				std::remove(s1.c_str());
			}
			string ss = projectFolder + "Release\\TestProject.tlog\\";
			RemoveDirectory(ss.c_str());
			for (string s2 : IO::GetFiles(projectFolder + "Release"))
			{
				string ss = s2.substr(s2.find_last_of('\\') + 1, string::npos);
				string se = s2.substr(s2.size() - 4, string::npos);
				if (ss == "glewinfo.exe" || ss == "visualinfo.exe" || se == ".pdb" || se == ".obj") {
					AddBuildLog(this, "deleting " + s2);
					std::remove(s2.c_str());
				}
			}
		}
		buildProgressValue = 100;
		buildLabel = "Build: complete.";
		buildProgressVec4 = green(1, 0.7f);
		AddBuildLog(this, "Build finished: press Escape to exit.");
		string ss = " /select,\"" + projectFolder + "Release\\TestProject2.exe\"";
		ShellExecute(NULL, "open", "explorer", ss.c_str(), NULL, SW_SHOW);
	}
	buildEnd = true;
	//else SetBuildFail(this);
}

#pragma endregion

void BlockCombo::Set() {
	for (EditorBlock* b : blocks) {
		b->hidden = true;
	}
	blocks[active]->hidden = false;
}

void BlockCombo::Draw() {
	Editor* editor = blocks[0]->editor;
	Vec4 v = Vec4(Display::width*editor->xPoss[blocks[0]->x1], Display::height*editor->yPoss[blocks[0]->y1], Display::width*editor->xPoss[blocks[0]->x2], Display::height*editor->yPoss[blocks[0]->y2]);
	CalcV(v);
	float a = Rect(v.r, v.g, v.b, v.a).Inside(Input::mousePos) ? 1 : 0.3f;
	for (uint x = 0, y = blocks.size(); x < y; x++) {
		if (Engine::EButton(editor->editorLayer == 0, v.r + v.b - EB_HEADER_PADDING - 2 - EB_HEADER_SIZE*(y - x)*1.2f, v.g + 1, EB_HEADER_SIZE - 2, EB_HEADER_SIZE - 2, editor->ebIconTexs[blocks[x]->editorType], white(((x == active) ? 1 : 0.7f)*a)) == MOUSE_RELEASE) {
			active = x;
			Set();
		}
	}
	if (Engine::EButton(editor->editorLayer == 0, v.r + v.b - EB_HEADER_PADDING - 2 - EB_HEADER_SIZE*(blocks.size() + 1)*1.2f, v.g + 1, EB_HEADER_SIZE - 2, EB_HEADER_SIZE - 2, editor->tex_buttonPlus, white(a)) == MOUSE_RELEASE) {

	}
}

#else

#include "Editor.h"

Editor* Editor::instance = nullptr;

#endif