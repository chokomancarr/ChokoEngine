#include "Editor.h"
#include "Engine.h"
#include <GL/glew.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>
#include <algorithm>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shellapi.h>
#include <Windows.h>
#include <Thumbcache.h>
#include <math.h>
#include <mutex>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/freeglut.h>
#include <chrono>
#include <thread>
#include <filesystem>

HWND Editor::hwnd = 0;
//#ifdef IS_EDITOR
Editor* Editor::instance = nullptr;
//#endif
string Editor::dataPath = "";

Vec4 grey1() {
	return Vec4(33.0f / 255, 37.0f / 255, 40.0f / 255, 1);
}
Vec4 grey2() {
	return Vec4(61.0f / 255, 68.0f / 255, 73.0f / 255, 1);
}
Vec4 accent() {
	return Vec4(223.0f / 255, 119.0f / 255, 4.0f / 255, 1);
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

int GetFormatEnum(string ext) {
	if (ext == ".mesh")
		return ASSETTYPE_MESH;
	else if (ext == ".blend")
		return ASSETTYPE_BLEND;
	else if (ext == ".material")
		return ASSETTYPE_MATERIAL;
	else if (ext == ".animclip")
		return ASSETTYPE_ANIMCLIP;
	else if (ext == ".animator")
		return ASSETTYPE_ANIMATOR;
	else if (ext == ".hdr")
		return ASSETTYPE_HDRI;
	else if (ext == ".bmp" || ext == ".jpg" || ext == ".png")
		return ASSETTYPE_TEXTURE;
	else if (ext == ".rendtex")
		return ASSETTYPE_RENDERTEXTURE;
	else if (ext == ".shade")
		return ASSETTYPE_SHADER;
	
	else return ASSETTYPE_UNDEF;
}

void DrawHeaders(Editor* e, EditorBlock* b, Vec4* v, string title) {
	//Engine::Button(v->r, v->g + EB_HEADER_SIZE + 1, v->b, v->a - EB_HEADER_SIZE - 2, black(), white(0.05f), white(0.05f));
	Vec2 v2(v->b*0.1f, EB_HEADER_SIZE*0.1f);
	Engine::DrawQuad(v->r + EB_HEADER_PADDING + 1, v->g, v->b - 3 - 2 * EB_HEADER_PADDING, EB_HEADER_SIZE, e->background->pointer, Vec2(), Vec2(v2.x, 0), Vec2(0, v2.y), v2, false, accent());
	//Engine::Label(round(v->r + 5 + EB_HEADER_PADDING), round(v->g + 2), 10, titleS, e->font, black(), Display::width);
	Engine::Label(v->r + 5 + EB_HEADER_PADDING, v->g, 12, title, e->font, black());
	//return Rect(v->r, v->g + EB_HEADER_SIZE + 1, v->b, v->a - EB_HEADER_SIZE - 2).Inside(Input::mousePos);
}

void CalcV(Vec4& v) {
	v.a = round(v.a - v.g) - 1;
	v.b = round(v.b - v.r) - 1;
	v.g = round(v.g) + 1;
	v.r = round(v.r) + 1;
}

void EB_Empty::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	DrawHeaders(editor, this, &v, "Hatena Title");
}

void EB_Debug::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	DrawHeaders(editor, this, &v, "System Log");
	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	//glDisable(GL_DEPTH_TEST);
	for (int x = drawIds.size() - 1, y = 0; x >= 0; x--, y++) {
		byte t = editor->messageTypes[drawIds[x]];
		Vec4 col = (t == 0) ? white() : ((t==1)? yellow() : red());
		Engine::DrawQuad(v.r + 1, v.g + v.a - 36 - (y*15), v.b - 2, 14, Vec4(1, 1, 1, ((x&1)==1)? 0.2f : 0.1f));
		Engine::Label(v.r + 3, v.g + v.a - 34 - y * 15, 12, editor->messages[drawIds[x]].first + " says: " + editor->messages[drawIds[x]].second, editor->font, col);
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
}

void EBH_DrawItem(SceneObject* sc, Editor* e, Vec4* v, int& i, int indent) {
	int xo = indent * 20;
	//if (indent > 0) {
	//	Engine::DrawLine(Vec2(xo - 10 + v->r, v->g + EB_HEADER_SIZE + 9 + 17 * i), Vec2(xo + v->r, v->g + EB_HEADER_SIZE + 10 + 17 * i), white(0.5f), 1);
	//}
	if (Engine::EButton((e->editorLayer == 0), v->r + xo + ((sc->childCount > 0) ? 16 : 0), v->g + EB_HEADER_SIZE + 1 + 17 * i, v->b - xo - ((sc->childCount > 0) ? 16 : 0), 16, grey2()) == MOUSE_RELEASE) {
		e->selected = sc;
		e->selectGlobal = false;
		e->DeselectFile();
	}
	Engine::Label(v->r + 19 + xo, v->g + EB_HEADER_SIZE + 1 + 17 * i, 12, sc->name, e->font, white());
	i++;
	if (sc->childCount > 0) {
		if (Engine::EButton((e->editorLayer == 0), v->r + xo, v->g + EB_HEADER_SIZE + 1 + 17 * (i - 1), 16, 16, sc->_expanded ? e->collapse : e->expand, white(), white(), white(1, 0.6f)) == MOUSE_RELEASE) {
			sc->_expanded = !sc->_expanded;
		}
	}
	if (e->selected == sc) {
		Engine::DrawQuad(v->r + xo, v->g + EB_HEADER_SIZE + 1 + 17 * (i - 1), v->b - xo, 16, white(0.3f));
	}
	if (sc->childCount > 0 && sc->_expanded) {
		//int oi = i - 1;
		//int oii = i - 1;
		for (SceneObject* scc : sc->children)
		{
			//oii = i - 1;
			EBH_DrawItem(scc, e, v, i, indent + 1);
		}
		//Engine::DrawLine(Vec2(xo + 10 + v->r, v->g + EB_HEADER_SIZE + 18 + 17 * (oi)), Vec2(xo + 10 + v->r, v->g + EB_HEADER_SIZE + 10 + 17 * (oii) + sc->childCount * 17), white(0.5f), 1);
	}
}

void EB_Hierarchy::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	DrawHeaders(editor, this, &v, "Scene Hierarchy");

	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	glDisable(GL_DEPTH_TEST);
	if (editor->sceneLoaded()) {
		if (Engine::EButton((editor->editorLayer == 0), v.r, v.g + EB_HEADER_SIZE + 1, v.b, 16, Vec4(0.2f, 0.2f, 0.4f, 1)) == MOUSE_RELEASE) {
			editor->selected = nullptr;
			editor->selectGlobal = true;
			editor->selectedFileType = ASSETTYPE_UNDEF;
		}
		Engine::Label(v.r + 19, v.g + EB_HEADER_SIZE + 1, 12, "Global", editor->font, white());
		int i = 1;
		for (SceneObject* sc : editor->activeScene->objects)
		{
			EBH_DrawItem(sc, editor, &v, i, 0);
		}
	}
	Engine::EndStencil();
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
}

EB_Browser::EB_Browser(Editor* e, int x1, int y1, int x2, int y2, string dir) : currentDir(dir) {
	editorType = 1;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	Refresh();

	shortcuts.emplace(GetShortcutInt(Key_A), &_AddAsset);
}

bool DrawFileRect(float w, float h, float size, EB_Browser_File* file, EditorBlock* e) {
	bool b = false;
	if (e->editor->editorLayer == 0) {
		byte bb = ((file->thumbnail >= 0) ? Engine::Button(w + 1, h + 1, size - 2, size - 2, (file->tex != nullptr)? file->tex : e->editor->assetIcons[file->thumbnail], white(1, 0.8f), white(), white(1, 0.5f)) : Engine::Button(w + 1, h + 1, size - 2, size - 2, grey2()));
		b = bb == MOUSE_RELEASE;

		if (file->canExpand) {
			if (Engine::Button(w + size*0.5f, h + 1, size*0.5f-1, size - 2, file->expanded? e->editor->assetCollapse : e->editor->assetExpand, white((bb & MOUSE_HOVER_FLAG)>0? 1 : 0.5f, 0.8f), white(), white(1, 0.5f)) == MOUSE_RELEASE) {
				file->expanded = !file->expanded;
			}
		}
	}
	else {
		if (file->thumbnail >= 0)
			Engine::DrawTexture(w + 1, h + 1, size - 2, size - 2, e->editor->assetIcons[file->thumbnail]);
		else
			Engine::DrawQuad(w + 1, h + 1, size - 2, size - 2, grey2());
	}
	Engine::DrawQuad(w + 1, h + 1 + size - 25, size - 2, 23, grey2()*0.7f);
	Engine::Label(w + 2, h + 1 + size - 20, 12, file->name, e->editor->font, white(), size);
	return b;
}

void EB_Browser::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
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
		Engine::Label(v.r + 2, v.g + EB_HEADER_SIZE + (16 * y), 12, dirs.at(y), editor->font, white(), 150);
	}
	float ww = 0;
	int hh = 0;
	byte fileSize = 70;
	for (int ff = files.size() - 1; ff >= 0; ff--) {
		if (DrawFileRect(v.r + 151 + ww, v.g + EB_HEADER_SIZE + (fileSize+1)* hh, fileSize, &files[ff], this)) {
			//editor->_Message("1", files[ff].fullName);
			editor->GetAssetInfo(files[ff].fullName, editor->selectedFileType, editor->selectedFile);
			if (editor->selectedFileType != ASSETTYPE_UNDEF) {
				editor->selectedFileName = files[ff].name;
				editor->selectedFilePath = files[ff].fullName;
				editor->selectedFileCache = _GetCache<void>(editor->selectedFileType, editor->selectedFile);
			}
			//editor->_Message("1", files[ff].fullName + " " + to_string(editor->selectedFileType) + ":" + to_string(editor->selectedFile));
		}
		
		ww += fileSize+1;
		if (ww > (v.b - 152 - fileSize)) {
			ww = 0;
			hh++;
			if (v.g + EB_HEADER_SIZE + (fileSize+1)* hh > Display::height)
				break;
		}
		if (files[ff].expanded) {
			for (int fff = files[ff].children.size() - 1; fff >= 0; fff--) {
				Engine::DrawQuad(v.r + 149 + ww, v.g + EB_HEADER_SIZE + (fileSize + 1)* hh + 1, fileSize + 1.0f, fileSize-2.0f, Vec4(1, 0.494f, 0.176f, 0.3f));
				DrawFileRect(v.r + 153 + ww, v.g + EB_HEADER_SIZE + (fileSize+1)* hh + 2.0f, fileSize - 4.0f, &files[ff].children[fff], this);
				ww += fileSize+1;
				if (ww > (v.b - 152 - fileSize)) {
					ww = 0;
					hh++;
					if (v.g + EB_HEADER_SIZE + (fileSize+1)* hh > Display::height)
						break;
				}
			}
		}
	}

	Engine::EndStencil();
}

void EB_Browser::Refresh() {
	dirs.clear();
	files.clear();
	IO::GetFolders(currentDir, &dirs);
	files = IO::GetFilesE(editor, currentDir);
	//editor->_Message("Browser", to_string(dirs.size()) + " folders and " + to_string(files.size()) + " files from " + currentDir);
	if (dirs.size() == 0) {
		dirs.push_back(".");
		dirs.push_back("..");
	}
}

EB_Viewer::EB_Viewer(Editor* e, int x1, int y1, int x2, int y2) : rz(45), rw(45), scale(1) {
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

	shortcuts.emplace(GetShortcutInt(Key_Escape), &_Escape);
}

void EB_Viewer::MakeMatrix() {
	float csz = cos(rz*3.14159265f / 180.0f);
	float snz = sin(rz*3.14159265f / 180.0f);
	float csw = cos(rw*3.14159265f / 180.0f);
	float snw = sin(rw*3.14159265f / 180.0f);
	viewingMatrix = glm::mat4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
	viewingMatrix = glm::mat4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1)*viewingMatrix;
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

void DrawSceneObjectsOpaque(EB_Viewer* ebv, vector<SceneObject*> oo) {
	for (SceneObject* sc : oo)
	{
		glPushMatrix();
		//Vec3 v2 = sc->transform.worldPosition();
		Vec3 v = sc->transform.position;
		Vec3 vv = sc->transform.scale;
		Quat vvv = sc->transform.rotation;
		//glTranslatef(v.x - v2.x, v.y - v2.y, v.z - v2.z);
		glTranslatef(v.x, v.y, v.z);
		glScalef(vv.x, vv.y, vv.z);
		glMultMatrixf(glm::value_ptr(Quat2Mat(vvv)));
		//glRotatef(rad2deg*vvv.w, vvv.x, vvv.y, vvv.z);
		for (Component* com : sc->_components)
		{
			if (com->componentType == COMP_MRD || com->componentType == COMP_CAM)
				com->DrawEditor(ebv);
		}
		DrawSceneObjectsOpaque(ebv, sc->children);

		if (sc == ebv->editor->selected) {
			GLfloat matrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
			ebv->editor->selectedMvMatrix = (glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));
		}

		glPopMatrix();
	}
}

void DrawSceneObjectsGizmos(EB_Viewer* ebv, vector<SceneObject*> oo) {
	for (SceneObject* sc : oo)
	{
		glPushMatrix();
		Vec3 v = sc->transform.position;
		Vec3 vv = sc->transform.scale;
		Quat vvv = sc->transform.rotation;
		//glTranslatef(v.x - v2.x, v.y - v2.y, v.z - v2.z);
		glTranslatef(v.x, v.y, v.z);
		glScalef(vv.x, vv.y, vv.z);
		glMultMatrixf(glm::value_ptr(Quat2Mat(vvv)));
		for (Component* com : sc->_components)
		{
			if (com->componentType != COMP_MRD && com->componentType != COMP_CAM)
				com->DrawEditor(ebv);
		}
		DrawSceneObjectsGizmos(ebv, sc->children);
		glPopMatrix();
	}
}

void DrawSceneObjectsTrans(EB_Viewer* ebv, vector<SceneObject*> oo) {

}

void EB_Viewer::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
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
	//if (!persp) {
	float h40 = 40 * (hh*Display::height) / (ww*Display::width);
	float mww = max(ww, 0.3f) * scale;
	if (seeingCamera == nullptr) {
		if (persp) glMultMatrixf(glm::value_ptr(glm::perspectiveFov(60 * deg2rad, (float)Display::width, (float)Display::height, 0.01f, 1000.0f)));
		else glMultMatrixf(glm::value_ptr(glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.01f, 1000.0f)));
		glScalef(-mww*Display::width / v.b, mww*Display::width / v.a, mww);
		glTranslatef(0, 0, -30);
	}
	else {
		Quat q = glm::inverse(seeingCamera->object->transform.rotation);
		glScalef(scale, -scale, 1);
		glMultMatrixf(glm::value_ptr(glm::perspectiveFov(seeingCamera->fov * deg2rad, (float)Display::width, (float)Display::height, seeingCamera->nearClip, seeingCamera->farClip)));
		glScalef(1, -1, -1);
		glMultMatrixf(glm::value_ptr(Quat2Mat(q)));
		Vec3 pos = -seeingCamera->object->transform.worldPosition();
		glTranslatef(pos.x, pos.y, pos.z);
	}
	//}
	//else {
		//glMultMatrixf(glm::value_ptr(glm::perspective(30.0f, 1.0f, 0.01f, 1000.0f)));
		//glScalef(1, -1, 1);
	//}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glPushMatrix();
	if (seeingCamera == nullptr) {
		float csz = cos(-rz*3.14159265f / 180.0f);
		float snz = sin(-rz*3.14159265f / 180.0f);
		float csw = cos(rw*3.14159265f / 180.0f);
		float snw = sin(rw*3.14159265f / 180.0f);
		glm::mat4 mMatrix = glm::mat4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1) * glm::mat4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
		//glMultMatrixf(glm::value_ptr(glm::mat4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1)));
		//glMultMatrixf(glm::value_ptr(glm::mat4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1)));
		glMultMatrixf(glm::value_ptr(mMatrix));
	}
	glEnable(GL_DEPTH_TEST);
	glClearDepth(1);

	if (editor->sceneLoaded()) {
		//draw scene
		glDepthFunc(GL_LEQUAL);
		glDepthMask(true);
		DrawSceneObjectsOpaque(this, editor->activeScene->objects);
		glDepthMask(false);
		DrawSceneObjectsGizmos(this, editor->activeScene->objects);
		DrawSceneObjectsTrans(this, editor->activeScene->objects);

		//draw background
		glDepthFunc(GL_EQUAL);
		if (editor->activeScene->settings.sky != nullptr) {
			
		}
		glDepthFunc(GL_LEQUAL);
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
	glDepthFunc(GL_ALWAYS);
	if (editor->selected != nullptr) {
		if (modifying == 0) {
			Vec3 wpos = editor->selected->transform.worldPosition();
			if (selectedTooltip == 0) DrawTArrows(wpos, 2);
			else if (selectedTooltip == 1) DrawRArrows(wpos, 2);
			else DrawSArrows(wpos, 2);
		}
		else {
			byte bb = modifying & 0xf0;
			if (bb == 0x10)
				Engine::DrawLineW(editor->selected->transform.worldPosition() + modAxisDir*-100000.0f, editor->selected->transform.worldPosition() + modAxisDir*100000.0f, white(), 2);
			else if (bb == 0x20) {
				bb = modifying & 0x0f;
				if (bb == 1)
					Engine::DrawCircleW(editor->selected->transform.worldPosition(), Vec3(0, 1, 0), Vec3(0, 0, 1), 2 / scale, 32, white(), 2);
				else if (bb == 2)
					Engine::DrawCircleW(editor->selected->transform.worldPosition(), Vec3(1, 0, 0), Vec3(0, 0, 1), 2 / scale, 32, white(), 2);
				else
					Engine::DrawCircleW(editor->selected->transform.worldPosition(), Vec3(0, 1, 0), Vec3(1, 0, 0), 2 / scale, 32, white(), 2);
			}
			else if ((modifying & 0x0f) != 0)
				Engine::DrawLineW(editor->selected->transform.worldPosition() + modAxisDir*-100000.0f, editor->selected->transform.worldPosition() + modAxisDir*100000.0f, white(), 2);
		}
	}

	//Color::DrawPicker(150, 50, editor->cc);

	GLfloat matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, matrix);
	glm::mat4 projMatrix = (glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));
	
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glMultMatrixf(glm::value_ptr(glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -100.0f, 100.0f)));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (editor->selected != nullptr) {
		Vec4 spos = projMatrix*editor->selectedMvMatrix*Vec4(0, 0, 0, 1);
		spos /= spos.w;
		Engine::DrawCircleW(Vec3(spos.x, spos.y, 0), Vec3(1.0f / Display::width, 0, 0), Vec3(0, 1.0f / Display::height, 0), 20, 24, white(), 2, true);
		if (modifying == 0) {
			if (selectedTooltip == 1) {
				Engine::DrawCircleW(Vec3(spos.x, spos.y, 0), Vec3(1.0f / Display::width, 0, 0), Vec3(0, 1.0f / Display::height, 0), 140, 24, yellow(), 2);
			}
		}
		else {
			Engine::DrawLineWDotted(Vec3(spos.x, spos.y, 0), Vec3(Input::mousePos.x / Display::width * 2 - 1, -(Input::mousePos.y / Display::height * 2 - 1), 0), white(1, 0.1f), 1, 12.0f / Display::height);
		}
	}

	//Engine::EndStencil();
	glViewport(0, 0, Display::width, Display::height);

	Vec3 origin(30 + v.r, v.a + v.g - 30, 10);
	Vec2 axesLabelPos;

	Engine::DrawLine(origin, origin + xyz(arrowX*20.0f), red(), 2); //v->r, v->g + EB_HEADER_SIZE + 1, v->b, v->a - EB_HEADER_SIZE - 2, black(), white(0.05f), white(0.05f));
	axesLabelPos = xy(origin + xyz(arrowX*20.0f)) + Vec2((arrowX.x > 0) ? 5 : -8, -5);
	Engine::Label(axesLabelPos.x, axesLabelPos.y, 10, "X", editor->font, red());
	Engine::DrawLine(origin, origin + xyz(arrowY*20.0f), green(), 2);
	axesLabelPos = xy(origin + xyz(arrowY*20.0f)) + Vec2((arrowY.x > 0) ? 5 : -8, -5);
	Engine::Label(axesLabelPos.x, axesLabelPos.y, 10, "Y", editor->font, green());
	Engine::DrawLine(origin, origin + xyz(arrowZ*20.0f), blue(), 2);
	axesLabelPos = xy(origin + xyz(arrowZ*20.0f)) + Vec2((arrowZ.x > 0) ? 5 : -8, -5);
	Engine::Label(axesLabelPos.x, axesLabelPos.y, 10, "Z", editor->font, blue());

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
		Engine::Label(v.x + 30, v.y + EB_HEADER_SIZE + 15 + 22 * (drawDescLT - 1), 12, descLabelLT, editor->font, black());
	}

	if (editor->_showDebugInfo) {
		Engine::Label(v.x + 50, v.y + 30, 12, "z=" + to_string(rz) + " w = " + to_string(rw), editor->font, white());
		Engine::Label(v.x + 50, v.y + 55, 12, "scale=" + to_string(scale), editor->font, white());
	}
}

const int EB_Viewer::arrowTIndexs[18] = { 0, 1, 2, 0, 2, 3, 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4 };
const int EB_Viewer::arrowSIndexs[18] = { 0, 1, 2, 0, 2, 3, 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4 };

void EB_Viewer::DrawTArrows(Vec3 pos, float size) {
	glDepthFunc(GL_ALWAYS);
	Engine::DrawLineW(pos, pos + Vec3(size / scale, 0, 0), red(), 3);
	Engine::DrawLineW(pos, pos + Vec3(0, size / scale, 0), green(), 3);
	Engine::DrawLineW(pos, pos + Vec3(0, 0, size / scale), blue(), 3);

	
	float s = size / scale;
	float ds = s * 0.07f;
	arrowVerts[0] = pos + Vec3(s, ds, ds);
	arrowVerts[1] = pos + Vec3(s, -ds, ds);
	arrowVerts[2] = pos + Vec3(s, -ds, -ds);
	arrowVerts[3] = pos + Vec3(s, ds, -ds);
	arrowVerts[4] = pos + Vec3(s*1.3f, 0, 0);

	arrowVerts[5] = pos + Vec3(ds, s, ds);
	arrowVerts[6] = pos + Vec3(-ds, s, ds);
	arrowVerts[7] = pos + Vec3(-ds, s, -ds);
	arrowVerts[8] = pos + Vec3(ds, s, -ds);
	arrowVerts[9] = pos + Vec3(0, s*1.3f, 0);

	arrowVerts[10] = pos + Vec3(ds, ds, s);
	arrowVerts[11] = pos + Vec3(-ds, ds, s);
	arrowVerts[12] = pos + Vec3(-ds, -ds, s);
	arrowVerts[13] = pos + Vec3(ds, -ds, s);
	arrowVerts[14] = pos + Vec3(0, 0, s*1.3f);

	glEnableClientState(GL_VERTEX_ARRAY);
	Engine::DrawIndicesI(&arrowVerts[0], &arrowTIndexs[0], 15, 1, 0, 0);
	Engine::DrawIndicesI(&arrowVerts[5], &arrowTIndexs[0], 15, 0, 1, 0);
	Engine::DrawIndicesI(&arrowVerts[10], &arrowTIndexs[0], 15, 0, 0, 1);
	glDepthFunc(GL_LEQUAL);
}

void EB_Viewer::DrawRArrows(Vec3 pos, float size) {
	glDepthFunc(GL_ALWAYS);
	Engine::DrawCircleW(pos, Vec3(0, 1, 0), Vec3(0, 0, 1), size / scale, 32, red(), 3);
	Engine::DrawCircleW(pos, Vec3(1, 0, 0), Vec3(0, 0, 1), size / scale, 32, green(), 3);
	Engine::DrawCircleW(pos, Vec3(0, 1, 0), Vec3(1, 0, 0), size / scale, 32, blue(), 3);
	glDepthFunc(GL_LEQUAL);
}

void EB_Viewer::DrawSArrows(Vec3 pos, float size) {
	glDepthFunc(GL_ALWAYS);
	Engine::DrawLineW(pos, pos + Vec3(size / scale, 0, 0), red(), 3);
	Engine::DrawLineW(pos, pos + Vec3(0, size / scale, 0), green(), 3);
	Engine::DrawLineW(pos, pos + Vec3(0, 0, size / scale), blue(), 3);


	float s = size / scale;
	float ds = s * 0.07f;
	Engine::DrawCube(pos + Vec3((size / scale) + ds, 0, 0), ds, ds, ds, red());
	Engine::DrawCube(pos + Vec3(0, (size / scale) + ds, 0), ds, ds, ds, green());
	Engine::DrawCube(pos + Vec3(0, 0, (size / scale) + ds), ds, ds, ds, blue());
	glDepthFunc(GL_LEQUAL);
}


void EB_Viewer::OnMouseM(Vec2 d) {
	if (editor->mousePressType == 1) {
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
	else if (modifying > 0) {
		//cout << (int)(modifying & 0x0f) << endl;
		modVal += Vec2(d.x / Display::width, -d.y / Display::height);
		if (modifying >> 4 == 1) {
			switch (modifying & 0x0f) {
			case 1:
				editor->selected->transform.position = preModVals + Vec3((modVal.x + modVal.y) * 40 / scale, 0, 0);
				break;
			case 2:
				editor->selected->transform.position = preModVals + Vec3(0, (modVal.x + modVal.y) * 40 / scale, 0);
				break;
			case 3:
				editor->selected->transform.position = preModVals + Vec3(0, 0, (modVal.x + modVal.y) * 40 / scale);
				break;
			}
		}
		else if (modifying >> 4 == 2) {
			switch (modifying & 0x0f) {
			case 1:
				editor->selected->transform.eulerRotation(preModVals + Vec3((modVal.x + modVal.y) * 360 / scale, 0, 0));
				break;
			case 2:
				editor->selected->transform.eulerRotation(preModVals + Vec3(0, (modVal.x + modVal.y) * 360 / scale, 0));
				break;
			case 3:
				editor->selected->transform.eulerRotation(preModVals + Vec3(0, 0, (modVal.x + modVal.y) * 360 / scale));
				break;
			}
		}
		else {
			switch (modifying & 0x0f) {
			case 0:
				editor->selected->transform.scale = Vec3(preModVals + Vec3(1, 1, 1)*((modVal.x + modVal.y) * 40 / scale));
				break;
			case 1:
				editor->selected->transform.scale = Vec3(preModVals + Vec3((modVal.x + modVal.y) * 40 / scale, 0, 0));
				break;
			case 2:
				editor->selected->transform.scale = Vec3(preModVals + Vec3(0, (modVal.x + modVal.y) * 40 / scale, 0));
				break;
			case 3:
				editor->selected->transform.scale = Vec3(preModVals + Vec3(0, 0, (modVal.x + modVal.y) * 40 / scale));
				break;
			}
		}
	}
}

void EB_Viewer::OnMousePress(int i) {
	if (modifying > 0) {
		if (i != 0) {
			switch (modifying >> 4) {
			case 1:
				editor->selected->transform.position = preModVals;
				break;
			case 2:
				editor->selected->transform.eulerRotation(preModVals);
				break;
			case 3:
				editor->selected->transform.scale = preModVals;
				break;
			}
		}
modifying = 0;
	}
}

void EB_Viewer::OnMouseScr(bool up) {
	scale += (up ? 0.1f : -0.1f);
	scale = min(max(scale, 0.01f), 1000);
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

void EBI_DrawObj(Vec4 v, Editor* editor, EB_Inspector* b, SceneObject* o) {
	Engine::DrawTexture(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 18, 18, editor->object);
	Engine::EButton((editor->editorLayer == 0), v.r + 20, v.g + 2 + EB_HEADER_SIZE, v.b - 21, 18, grey2());
	Engine::Label(v.r + 22, v.g + 3 + EB_HEADER_SIZE, 12, o->name, editor->font, white());

	//TRS
	b->DrawVector3(editor, v, 21, "Position", o->transform.position);
	b->DrawVector3(editor, v, 38, "Rotation", (Vec3&)o->transform.eulerRotation());
	b->DrawVector3(editor, v, 55, "Scale", o->transform.scale);

	//draw components
	uint off = 74 + EB_HEADER_SIZE;
	for (Component* c : o->_components)
	{
		c->DrawInspector(editor, c, v, off);
		if (editor->WAITINGREFRESHFLAG) //deleted
			return;
	}
}

void EBI_DrawAss_Tex(Vec4 v, Editor* editor, EB_Inspector* b, float &off) {
	Texture* tex = (Texture*)editor->selectedFileCache;
	float sz = min(v.b - 2.0f, clamp((float)max(tex->width, tex->height), 16.0f, editor->_maxPreviewSize));
	Engine::DrawTexture(v.r + 1 + 0.5f*(v.b - sz), off + 15, sz, sz, tex, DrawTex_Fit);
	tex->_mipmap = Engine::DrawToggle(v.r + 2, off + sz + 17, 14, editor->checkbox, tex->_mipmap, white(), ORIENT_HORIZONTAL);
	Engine::Label(v.r + 18, off + sz + 18, 12, "Use Mipmaps", editor->font, white());
	Engine::Label(v.r + 18, off + sz + 33, 12, "Filtering", editor->font, white());
	vector<string> filterNames = { "Point", "Bilinear", "Trilinear" };
	if (Engine::EButton(editor->editorLayer == 0, v.r + v.b * 0.3f, off + sz + 33, v.b * 0.6f - 1, 14, grey2(), filterNames[tex->_filter], 12, editor->font, white()) == MOUSE_PRESS) {
		editor->RegisterMenu(b, "", filterNames, { &b->_ApplyTexFilter0, &b->_ApplyTexFilter1, &b->_ApplyTexFilter2 }, 0, Vec2(v.r + v.b * 0.3f, off + sz + 33));
	}
	Engine::Label(v.r + 18, off + sz + 49, 12, "Aniso Level", editor->font, white());
	Engine::DrawQuad(v.r + v.b * 0.3f, off + sz + 48, v.b * 0.1f - 1, 14, grey2());
	Engine::Label(v.r + v.b * 0.3f + 2, off + sz + 49, 12, to_string(tex->_aniso), editor->font, white());
	tex->_aniso = (byte)round(Engine::DrawSliderFill(v.r + v.b * 0.4f, off + sz + 48, v.b*0.7f - 1, 14, 0, 10, (float)tex->_aniso, grey2(), white()));
	off += sz + 63;
}

void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off) {
	Material* mat = (Material*)editor->selectedFileCache;
	Engine::Label(v.r + 18, off + 17, 12, "Shader", editor->font, white());
	editor->DrawAssetSelector(v.r + v.b*0.25f, off + 15, v.b*0.75f - 1, 16, grey2(), ASSETTYPE_SHADER, 12, editor->font, &mat->_shader, &EB_Inspector::_ApplyMatShader, mat);
	off += 34;
	/*
	for (auto aa : mat->vals) {
		int r = 0;
		for (auto bb : aa.second) {
			Engine::DrawTexture(v.r + 2, off, 16, 16, editor->matVarTexs[aa.first]);
			Engine::Label(v.r + 19, off + 2, 12, mat->valNames[aa.first][r], editor->font, white());
			switch (aa.first) {
			case SHADER_INT:
				Engine::Button(v.r + v.b * 0.3f + 17, off, v.b*0.7f - 17, 16, grey1(), to_string(*(int*)bb.second), 12, editor->font, white());
				break;
			case SHADER_FLOAT:
				Engine::Button(v.r + v.b * 0.3f + 17, off, v.b*0.7f - 17, 16, grey1(), to_string(*(float*)bb.second), 12, editor->font, white());
				break;
			case SHADER_SAMPLER:
				ASSETID id = ASSETID(((MatVal_Tex*)bb.second)->id);
				int ti = 32;// max(min(ceil(v.b*0.3f - 12), 64), 32);
				editor->DrawAssetSelector(v.r + 19, off + 16, v.b - ti - 21, 16, grey1(), ASSETTYPE_TEXTURE, 12, editor->font, &id);
				if (id > -1)
					Engine::DrawTexture(v.r + v.b - ti - 1, off, ti, ti, _GetCache<Texture>(ASSETTYPE_TEXTURE, id));
				else
					Engine::DrawQuad(v.r + v.b - ti - 1, off, ti, ti, grey1());
				off += ti - 16;
				break;
			}
			r++;
			off += 17;
		}
	}
	//*/
	for (uint q = 0; q < mat->valOrders.size(); q++) {
		int r = 0;
		Engine::DrawTexture(v.r + 2, off, 16, 16, editor->matVarTexs[mat->valOrders[q]]);
		Engine::Label(v.r + 19, off + 2, 12, mat->valNames[mat->valOrders[q]][mat->valOrderIds[q]], editor->font, white());
		void* bbs = mat->vals[mat->valOrders[q]][mat->valOrderGLIds[q]];
		assert(bbs != nullptr);
		switch (mat->valOrders[q]) {
		case SHADER_INT:
			Engine::Button(v.r + v.b * 0.3f + 17, off, v.b*0.7f - 17, 16, grey1(), to_string(*(int*)bbs), 12, editor->font, white());
			break;
		case SHADER_FLOAT:
			Engine::Button(v.r + v.b * 0.3f + 17, off, v.b*0.7f - 17, 16, grey1(), to_string(*(float*)bbs), 12, editor->font, white());
			break;
		case SHADER_SAMPLER:
			ASSETID* id = &ASSETID(((MatVal_Tex*)bbs)->id);
			float ti = 32;// max(min(ceil(v.b*0.3f - 12), 64), 32);
			editor->DrawAssetSelector(v.r + 19, off + 16, v.b - ti - 21, 16, grey1(), ASSETTYPE_TEXTURE, 12, editor->font, id);
			if (*id > -1)
				Engine::DrawTexture(v.r + v.b - ti - 1, off, ti, ti, _GetCache<Texture>(ASSETTYPE_TEXTURE, *id));
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
	float sz = min(v.b - 2.0f, clamp((float)max(tex->width, tex->height), 16.0f, editor->_maxPreviewSize));
	float x = v.r + 1 + 0.5f*(v.b - sz);
	float y = off + 15;
	float w2h = ((float)tex->width) / tex->height;
	Engine::DrawQuad(x, y, sz, sz / w2h, (tex->loaded) ? tex->pointer : Engine::fallbackTex->pointer);
	off += sz + 63;
}

void EB_Inspector::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	DrawHeaders(editor, this, &v, "Inspector");// isAsset ? (loaded ? label : "No object selected") : nm);
	string nm;
	if (lock)
		nm = (lockGlobal == 1) ? lockedObj->name : (lockGlobal == 2) ? "Scene settings" : "No object selected";
	else
		nm = (editor->selected != nullptr) ? editor->selected->name : editor->selectGlobal ? "Scene settings" : "No object selected";
	lock = Engine::DrawToggle(v.r + v.b - EB_HEADER_PADDING - EB_HEADER_SIZE - 2, v.g, EB_HEADER_SIZE, editor->keylock, lock, lock ? Vec4(1, 1, 0, 1) : white(0.5f), ORIENT_HORIZONTAL);
	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	if (!lock) {
		lockGlobal = 0;
		lockedObj = nullptr;
	}
	if (lock) {
		if (lockGlobal == 0) {
			if (editor->selectGlobal)
				lockGlobal = 2;
			else if (editor->selected != nullptr) {
				lockGlobal = 1;
				lockedObj = editor->selected;
			}
			else Engine::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, "Select object to inspect.", editor->font, white());
		}
		if (lockGlobal == 2) { //no else to prevent 1 frame blank
			Engine::DrawQuad(v.r + 20, v.g + 2 + EB_HEADER_SIZE, v.b - 21, 18, grey1());
			Engine::Label(v.r + 22, v.g + 6 + EB_HEADER_SIZE, 12, "Scene Settings", editor->font, white());

			Engine::Label(v.r, v.g + 23 + EB_HEADER_SIZE, 12, "Sky", editor->font, white());
			editor->DrawAssetSelector(v.r + v.b*0.3f, v.g + 21 + EB_HEADER_SIZE, v.b*0.7f - 1, 14, grey2(), ASSETTYPE_HDRI, 12, editor->font, &editor->activeScene->settings.skyId, &_ApplySky, &*editor->activeScene);
		}
		else if (lockGlobal == 1) {
			EBI_DrawObj(v, editor, this, lockedObj);
		}
	}
	else if (editor->selectedFileType != ASSETTYPE_UNDEF) {
		Engine::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, editor->selectedFileName, editor->font, white());
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
		//Engine::DrawTexture(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 18, 18, editor->object);
		Engine::DrawQuad(v.r + 20, v.g + 2 + EB_HEADER_SIZE, v.b - 21, 18, grey1());
		Engine::Label(v.r + 22, v.g + 6 + EB_HEADER_SIZE, 12, "Scene Settings", editor->font, white());

		Engine::Label(v.r, v.g + 23 + EB_HEADER_SIZE, 12, "Sky", editor->font, white());
		editor->DrawAssetSelector(v.r + v.b*0.3f, v.g + 21 + EB_HEADER_SIZE, v.b*0.7f - 1, 14, grey2(), ASSETTYPE_HDRI, 12, editor->font, &editor->activeScene->settings.skyId, &_ApplySky, &*editor->activeScene);

	}
	else if (editor->selected != nullptr){
		EBI_DrawObj(v, editor, this, editor->selected);
	}
	else
		Engine::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, "Select object to inspect.", editor->font, white());
	Engine::EndStencil();
}

void EB_Inspector::DrawVector3(Editor* e, Vec4 v, float dh, string label, Vec3& value) {
	Engine::Label(v.r, v.g + dh + EB_HEADER_SIZE, 12, label, e->font, white());
	Engine::EButton((e->editorLayer == 0), v.r + v.b*0.19f, v.g + dh + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Vec4(0.4f, 0.2f, 0.2f, 1));
	Engine::Label(v.r + v.b*0.19f + 2, v.g + dh + EB_HEADER_SIZE, 12, to_string(value.x), e->font, white());
	Engine::EButton((e->editorLayer == 0), v.r + v.b*0.46f, v.g + dh + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Vec4(0.2f, 0.4f, 0.2f, 1));
	Engine::Label(v.r + v.b*0.46f + 2, v.g + dh + EB_HEADER_SIZE, 12, to_string(value.y), e->font, white());
	Engine::EButton((e->editorLayer == 0), v.r + v.b*0.73f, v.g + dh + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Vec4(0.2f, 0.2f, 0.4f, 1));
	Engine::Label(v.r + v.b*0.73f + 2, v.g + dh + EB_HEADER_SIZE, 12, to_string(value.z), e->font, white());
}

void EB_Inspector::DrawAsset(Editor* e, Vec4 v, float dh, string label, ASSETTYPE type) {
	Engine::Label(v.r, v.g + dh + 2 + EB_HEADER_SIZE, 12, label, e->font, white());
	if (Engine::EButton((e->editorLayer == 0), v.r + v.b*0.19f, v.g + dh + EB_HEADER_SIZE, v.b*0.71f - 1, 16, Vec4(0.4f, 0.2f, 0.2f, 1)) == MOUSE_RELEASE) {
		e->editorLayer = 3;
		e->browseType = type;
		e->browseOffset = 0;
		e->browseSize = e->allAssets[type].size();
	}
	Engine::Label(v.r + v.b*0.19f + 2, v.g + dh + 2 + EB_HEADER_SIZE, 12, label, e->font, white());
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
	mat->shader = _GetCache<ShaderBase>(ASSETTYPE_SHADER, mat->_shader);
	mat->_ReloadParams();
	mat->Save(Editor::instance->selectedFilePath);
}

void EB_Inspector::_ApplySky(void* v) {
	Scene* sc = (Scene*)v;
	sc->settings.sky = _GetCache<Background>(ASSETTYPE_HDRI, sc->settings.skyId);
}

EB_AnimEditor::EB_AnimEditor(Editor* e, int x1, int y1, int x2, int y2): _editingAnim(-1), scale(2), offset(Vec2()) {
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
	DrawHeaders(editor, this, &v, "Animation Editor");
	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);

	Engine::DrawQuad(v.r, v.g + EB_HEADER_SIZE, v.b, v.a, white(0.1f, 0.4f));
	float scl = 1/scale;
	scl *= scl;
	float scl2 = scl;
	while (scl2 < 0.2f)
		scl2 *= 5;
	while (scl2 > 25)
		scl2 *= 0.2f;

	Vec2 off0 = offset*scl + Vec2(v.r, v.g) + 0.5f*Vec2(v.b, v.a);
	for (float x = v.r; x < (v.r + v.b); x+=scl2*32) {
		Engine::DrawLine(Vec2(x, v.g - EB_HEADER_SIZE - 1), Vec2(x, v.g + v.a), black(0.6f*(scl2-0.2f)/0.8f), 1);
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

	editor->DrawAssetSelector(v.r, v.g + EB_HEADER_SIZE + 1, v.b * 0.5f, 14, grey2(), ASSETTYPE_ANIMATOR, 12, editor->font, &_editingAnim, &_SetAnim, this);
	
	if (editingAnim != nullptr) {
		for (Anim_State* state : editingAnim->states) {
			Vec2 poss = off0 + state->editorPos;
			Engine::DrawQuad(poss.x, poss.y, scl * 320, scl * 32, grey2());
			Engine::DrawTriangle(Vec2(poss.x + scl * 16, poss.y + scl * 16), state->editorExpand ? Vec2(0, scl * 13) : Vec2(scl * 13, 0), white());
			if (Engine::Button(poss.x, poss.y, scl * 32, scl * 32) == MOUSE_RELEASE)
				state->editorExpand = !state->editorExpand;
			Engine::EButton(editor->editorLayer == 0, poss.x + scl * 34, poss.y + scl * 2, scl * 284, scl * 28, grey1(), state->name, scl * 24, editor->font, white());
			if (state->editorExpand) {
				Engine::DrawQuad(poss.x, poss.y + scl * 32, scl * 320, (state->isBlend && state->editorShowGraph)? scl*320 : scl * 256, grey1());
				state->isBlend = Engine::DrawToggle(poss.x + scl * 6, poss.y + scl * 34, scl * 28, editor->checkbox, state->isBlend, white(), ORIENT_HORIZONTAL);
				Engine::Label(poss.x + scl * 36, poss.y + scl * 36, scl * 24, "Blending", editor->font, white());
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

	Engine::Label(v.r + 5, v.g + v.a - 16, 12, to_string(scl), editor->font);
	Engine::EndStencil();
}

void EB_AnimEditor::OnMouseScr(bool up) {
	scale += (up ? 0.1f : -0.1f);
	scale = min(max(scale, 1), 5);
}


GLuint EB_Previewer::d_fbo = 0;
GLuint EB_Previewer::d_texs[] = {0, 0, 0};
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
	DrawHeaders(editor, this, &v, "Previewer");

	
	if (viewer == nullptr) FindEditor();
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
		float mww = max(ww, 0.3f) * scale;
		if (seeingCamera == nullptr) {
			if (viewer->persp) glMultMatrixf(glm::value_ptr(glm::perspectiveFov(60 * deg2rad, (float)Display::width, (float)Display::height, 0.01f, 1000.0f)));
			else glMultMatrixf(glm::value_ptr(glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.01f, 1000.0f)));
			glScalef(-mww*Display::width / v.b, mww*Display::width / v.a, mww);
			glTranslatef(0, 0, -30);

			float csz = cos(-viewer->rz*3.14159265f / 180.0f);
			float snz = sin(-viewer->rz*3.14159265f / 180.0f);
			float csw = cos(viewer->rw*3.14159265f / 180.0f);
			float snw = sin(viewer->rw*3.14159265f / 180.0f);
			glm::mat4 mMatrix = glm::mat4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1) * glm::mat4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
			glMultMatrixf(glm::value_ptr(mMatrix));
		}
		else {
			Quat q = glm::inverse(seeingCamera->object->transform.rotation);
			glScalef(scale, -scale, 1);
			glMultMatrixf(glm::value_ptr(glm::perspectiveFov(seeingCamera->fov * deg2rad, (float)Display::width, (float)Display::height, seeingCamera->nearClip, seeingCamera->farClip)));
			glScalef(1, -1, -1);
			glMultMatrixf(glm::value_ptr(Quat2Mat(q)));
			Vec3 pos = -seeingCamera->object->transform.worldPosition();
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


void EB_ColorPicker::Draw() {
	Vec4 v = Vec4(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	CalcV(v);
	DrawHeaders(editor, this, &v, "Color Picker");
}

void Editor::DrawAssetSelector(float x, float y, float w, float h, Vec4 col, ASSETTYPE type, float labelSize, Font* labelFont, ASSETID* tar, callbackFunc func, void* param) {
	if (editorLayer == 0) {
		if (Engine::Button(x, y, w, h, col, LerpVec4(col, white(), 0.5f), LerpVec4(col, black(), 0.5f)) == MOUSE_RELEASE) {
			editorLayer = 3;
			browseType = type;
			browseTarget = tar;
			browseCallback = func;
			browseCallbackParam = param;
		}
	}
	else
		Engine::DrawQuad(x, y, w, h, col);
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDLEFT;
	Engine::Label(round(x + 2), round(y + 0.5f*h), labelSize, (*tar == -1) ? "undefined" : normalAssets[type][*tar], labelFont, (*tar == -1) ? Vec4(0.7f, 0.4f, 0.4f, 1) : Vec4(0.4f, 0.4f, 0.7f, 1));
	labelFont->alignment = al;
}

void Editor::DrawCompSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, CompRef* tar, callbackFunc func, void* param) {
	if (editorLayer == 0) {
		if (Engine::Button(x, y, w, h, col, LerpVec4(col, white(), 0.5f), LerpVec4(col, black(), 0.5f)) == MOUSE_RELEASE) {
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
	Engine::Label(round(x + 2), round(y + 0.5f*h), labelSize, (tar->comp == nullptr) ? "undefined" : tar->path + " (" + tar->comp->name + ")", labelFont, (tar->comp == nullptr) ? Vec4(0.7f, 0.4f, 0.4f, 1) : Vec4(0.4f, 0.4f, 0.7f, 1));
	labelFont->alignment = al;
}

void Editor::DrawColorSelector(float x, float y, float w, float h, Vec4 col, float labelSize, Font* labelFont, Vec4* tar) {
	Engine::DrawQuad(x, y, w * 0.7f - 1, h, col);
	Engine::Label(x + 2, y + 2, labelSize, Color::Col2Hex(*tar), labelFont, white());
	Engine::DrawQuad(x + w*0.7f, y, w*0.3f, h*0.8f, Vec4(tar->r, tar->g, tar->b, 1));
	Engine::DrawQuad(x + w*0.7f, y + h * 0.8f, w*0.3f, h*0.2f, black());
	Engine::DrawQuad(x + w*0.7f, y + h * 0.8f, w*0.3f*tar->a, h*0.2f, white());
}

Editor::Editor() {
	instance = this;
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
				GetCache(type, i);
				return x;
			}
			string uu;
			if (t.first == ASSETTYPE_MATERIAL)
				uu = projectFolder + "Assets\\" + u;
			//if (t.first == ASSETTYPE_MESH)
			//	u = projectFolder + "Assets\\" + u + ".mesh.meta";
			else
				uu = projectFolder + "Assets\\" + u + ".meta";
			if (uu == p) {
				type = t.first;
				i = x;
				GetCache(type, i);
				return x;
			}
			x++;
		}
	}
	type = -1;
	i = -1;
	return -1;
}

ASSETID Editor::GetAssetId(void* i) {
	ASSETTYPE t;
	return GetAssetId(i, t);
}
ASSETID Editor::GetAssetId(void* i, ASSETTYPE&) {
	if (i == nullptr)
		return -1;
	else {
		for (auto& a : normalAssetCaches) {
			for (int b = a.second.size()-1; b >= 0; b--) {
				if (a.second[b] == i){
					return b;
				}
			}
		}
	}
	return -1;
}

void Editor::ReadPrefs() {
	string ss = projectFolder;
	while (ss[ss.size() - 1] == '\\')
		ss = ss.substr(0, ss.size()-1);
	string s = projectFolder + "\\" + ss.substr(ss.find_last_of('\\'), string::npos) + ".Bordom";
	ifstream strm(s, ios::in | ios::binary);
	if (!strm.is_open()) {
		_Error("Editor", "Fail to load project prefs!");
		return;
	}
	ushort n;
	_Strm2Val(strm, n);
	savedIncludedScenes = n;
	char* cc = new char[100];
	for (short a = 0; a < n; a++) {
		char c;
		strm >> c;
		if ((byte)c > 1) {
			_Error("Editor", "Strange byte in prefs file! " + to_string(strm.tellg()));
			return;
		}
		includedScenesUse.push_back(c == 1);
		strm.getline(cc, 100, (char)0);
		includedScenes.push_back(cc);
	}
}

void Editor::SavePrefs() {
	string ss = projectFolder;
	while (ss[ss.size() - 1] == '\\')
		ss = ss.substr(0, ss.size() - 1);
	string s = projectFolder + "\\" + ss.substr(ss.find_last_of('\\'), string::npos) + ".Bordom";
	ofstream strm(s, ios::out | ios::binary | ios::trunc);
	ushort u = includedScenes.size();
	_StreamWrite(&u, &strm, 2);
	for (int a = 0; a < u; a++) {
		if (includedScenesUse[a])
			strm << (char)1;
		else
			strm << (char)0;
		strm << includedScenes[a] << (char)0;
	}

	savedIncludedScenes = includedScenes.size();
}

void Editor::LoadDefaultAssets() {
	font = new Font(dataPath + "res\\arial.ttf", 32);

	buttonX = GetRes("xbutton");
	buttonExt = GetRes("extbutton");
	buttonPlus = GetRes("plusbutton");
	buttonExtArrow = GetRes("extbutton arrow");
	background = GetRes("lines");

	placeholder = GetRes("placeholder");
	checkers = GetRes("checkers", false, true);
	expand = GetRes("expand");
	collapse = GetRes("collapse");
	object = GetRes("object");

	shadingTexs.push_back(GetRes("shading_solid"));
	shadingTexs.push_back(GetRes("shading_trans"));

	tooltipTexs.push_back(GetRes("tooltip_tr"));
	tooltipTexs.push_back(GetRes("tooltip_rt"));
	tooltipTexs.push_back(GetRes("tooltip_sc"));

	orientTexs.push_back(GetRes("orien_global"));
	orientTexs.push_back(GetRes("orien_local"));
	orientTexs.push_back(GetRes("orien_view"));

	ebIconTexs.emplace(0, checkers);
	ebIconTexs.emplace(1, GetResExt("eb icon browser", "png"));
	ebIconTexs.emplace(2, GetResExt("eb icon viewer", "png"));
	ebIconTexs.emplace(3, GetResExt("eb icon inspector", "png"));
	ebIconTexs.emplace(4, GetResExt("eb icon hierarchy", "png"));
	ebIconTexs.emplace(5, GetResExt("eb icon anim", "png"));
	ebIconTexs.emplace(6, GetResExt("eb icon previewer", "png"));
	ebIconTexs.emplace(10, GetResExt("eb icon hierarchy", "png"));

	checkbox = GetRes("checkbox");
	keylock = GetRes("keylock");

	assetExpand = GetRes("asset_expand");
	assetCollapse = GetRes("asset_collapse");
	
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
		gridId[a*2] = a;
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

	assetTypes.emplace("scene", ASSETTYPE_SCENE);
	assetTypes.emplace("mesh", ASSETTYPE_MESH);
	assetTypes.emplace("shade", ASSETTYPE_SHADER);
	assetTypes.emplace("material", ASSETTYPE_MATERIAL);
	assetTypes.emplace("bmp", ASSETTYPE_TEXTURE);
	assetTypes.emplace("png", ASSETTYPE_TEXTURE);
	assetTypes.emplace("jpg", ASSETTYPE_TEXTURE);
	assetTypes.emplace("hdr", ASSETTYPE_HDRI);
	assetTypes.emplace("animclip", ASSETTYPE_ANIMCLIP);
	assetTypes.emplace("animator", ASSETTYPE_ANIMATOR);
	
	allAssets.emplace(ASSETTYPE_SCENE, vector<string>());
	allAssets.emplace(ASSETTYPE_MESH, vector<string>());
	allAssets.emplace(ASSETTYPE_ANIMCLIP, vector<string>());
	allAssets.emplace(ASSETTYPE_ANIMATOR, vector<string>());
	allAssets.emplace(ASSETTYPE_MATERIAL, vector<string>());
	allAssets.emplace(ASSETTYPE_TEXTURE, vector<string>());
	allAssets.emplace(ASSETTYPE_HDRI, vector<string>());
}

void AddH(Editor* e, string dir, vector<string>* h, vector<string>* cpp) {
	for (string s : IO::GetFiles(dir)) {
		if (s.size() > 3 && s.substr(s.size() - 2, string::npos) == ".h")
			h->push_back(s.substr(e->projectFolder.size() + 7, string::npos));
		if (s.size() > 5 && s.substr(s.size() - 4, string::npos) == ".cpp")
			cpp->push_back(s.substr(e->projectFolder.size() + 7, string::npos));
		else if (s.size() > 7 && s.substr(s.size() - 6, string::npos) == ".scene")
			e->allAssets[ASSETTYPE_SCENE].push_back(s);
		else if (s.size() > 10 && s.substr(s.size() - 9, string::npos) == ".material")
			e->allAssets[ASSETTYPE_MATERIAL].push_back(s);
		else {
			if (s.size() < 7) continue;
			string s2 = s.substr(0, s.size() - 5);
			//cout << ">" << s2 << endl;
			unordered_map<string, ASSETTYPE>::const_iterator got = e->assetTypes.find(s2.substr(s2.find_last_of('.') + 1));
			if (got != e->assetTypes.end()) {
				//cout << s << endl;
				e->allAssets[(got->second)].push_back(s);
			}
		}
	}
	vector<string> dirs;
	IO::GetFolders(dir, &dirs, true);
	for (string ss : dirs) {
		if (ss != "." && ss != "..")
			AddH(e, dir + "\\" + ss, h, cpp);
	}
}

void Editor::GenerateScriptResolver() {
	ifstream vcxIn(dataPath + "res\\vcxproj.txt", ios::in);
	if (!vcxIn.is_open()){
		_Error("Script Resolver", "Vcxproj template not found!");
		return;
	}
	stringstream sstr;
	sstr << vcxIn.rdbuf();
	string vcx = sstr.str();
	vcxIn.close();
	//<ClCompile Include = "Assets/main.cpp" / >< / ItemGroup><ItemGroup>

	ofstream vcxOut(projectFolder + "TestProject2.vcxproj", ios::out | ios::trunc);
	if (!vcxOut.is_open()) {
		_Error("Script Resolver", "Cannot write to vcxproj!");
		return;
	}
	vcxOut << vcx.substr(0, vcx.find('#'));
	for (string cpp2 : IO::GetFiles(projectFolder + "System\\", ".cpp")) {
		vcxOut << "<ClCompile Include=\"" + cpp2.substr(projectFolder.size(), string::npos) + "\" />" << endl;
	}
	for (string cpp : cppAssets) {
		vcxOut << "<ClCompile Include=\"Assets\\" + cpp + "\" />" << endl;
	}
	vcxOut << "</ItemGroup>\r\n<ItemGroup>" << endl;
	for (string hd2 : IO::GetFiles(projectFolder + "System\\", ".h")) {
		vcxOut << "<ClInclude Include=\"" + hd2.substr(projectFolder.size(), string::npos) + "\" />" << endl;
	}
	for (string hd : headerAssets) {
		vcxOut << "<ClInclude Include=\"Assets\\" + hd + "\" />" << endl;
	}
	vcxOut << vcx.substr(vcx.find('#') + 1, string::npos);
	vcxOut.close();

	//SceneScriptResolver.h
	string h = "#include <vector>\n#include <fstream>\n#include \"Engine.h\"\ntypedef SceneScript*(*sceneScriptInstantiator)();\ntypedef void (*sceneScriptAssigner)(SceneScript*, ifstream&);\nclass SceneScriptResolver {\npublic:\n\tSceneScriptResolver();\n\tstatic SceneScriptResolver* instance;\n\tstd::vector<string> names;\n\tstd::vector<sceneScriptInstantiator> map;\n\tstd::vector<sceneScriptAssigner> ass;\n\t\tSceneScript* Resolve(ifstream& strm);\n";
	//*
	h += "\n\tstatic SceneScript ";
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		h += "*_Inst" + to_string(a) + "()";
		if (a == b - 1)
			h += ";\n";
		else
			h += ", ";
	}
	
	h += "\n\tstatic void ";
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		h += "_Ass" + to_string(a) + "(SceneScript* sscr, ifstream& strm)";
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
		s += "SceneScript* SceneScriptResolver::_Inst" + to_string(a) + "() { return new " + ss + "(); }\n\n";
	}
	//*/
	s += "\n\nusing namespace std;\r\nSceneScriptResolver* SceneScriptResolver::instance = nullptr;\nSceneScriptResolver::SceneScriptResolver() {\n\tinstance = this;\n";
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		s += "\tnames.push_back(R\"(" + headerAssets[a] + ")\");\n";
		s += "\tmap.push_back(&_Inst" + to_string(a) + ");\n";
		s += "\tass.push_back(&_Ass" + to_string(a) + ");\n";
	}
	s += "}\n\n";
	s += "SceneScript* SceneScriptResolver::Resolve(ifstream& strm) {\n\tchar* c = new char[100];\n\tstrm.getline(c, 100, 0);\n\tstring s(c);\n\tdelete[](c);";
	s += "\n\tint a = 0;\n\tfor (string ss : names) {\n\t\tif (ss == s) {\n\t\t\tSceneScript* scr = map[a]();\n\t\t\tscr->name = s + \" (Script)\";\n\t\t\t(*ass[a])(scr, strm);\n\t\t\treturn scr;\n\t\t}\n\t\ta++;\n\t}\n\treturn nullptr;\n}";
	
	GenerateScriptValuesReader(s);
	
	string cppO = projectFolder + "\\System\\SceneScriptResolver.cpp";
	string hO = projectFolder + "\\System\\SceneScriptResolver.h";

	std::remove(hO.c_str());
	std::remove(cppO.c_str());

	ofstream ofs (cppO.c_str(), ios::out | ios::trunc);
	ofs << s;
	ofs.close();
	//SetFileAttributes(cppO.c_str(), FILE_ATTRIBUTE_HIDDEN);
	ofs.open(hO.c_str(), ios::out | ios::trunc);
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
		
		s += "void SceneScriptResolver::_Ass" + to_string(a) + " (SceneScript* sscr, ifstream& strm) {\n\t" + ha + "* scr = (" + ha  + "*)sscr;\n" + tmpl;
		ifstream mstrm(projectFolder + "Assets\\" + headerAssets[a] + ".meta", ios::in | ios::binary);
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
	activeScene = make_shared<Scene>();
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

void Editor::DrawHandles() {
	bool moused = false;
	int r = 0;
	for (EditorBlock* b : blocks)
	{
		Vec4 v = Vec4(Display::width*xPoss[b->x1], Display::height*yPoss[b->y1], Display::width*xPoss[b->x2], Display::height*yPoss[b->y2]);

		//Engine::DrawQuad(v.r + 1, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_SIZE - 1 - EB_HEADER_PADDING, grey1());
		//Engine::DrawQuad(v.b - EB_HEADER_PADDING, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_SIZE - 1 - EB_HEADER_PADDING, grey1());
		if (Engine::EButton((b->editor->editorLayer == 0), v.r + 1, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonExt, white(1, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) { //splitter top left
			if (b->headerStatus == 1)
				b->headerStatus = 0;
			//else b->headerStatus = (b->headerStatus == 0 ? 1 : 0);
		}
		if (b->headerStatus == 1) {
			Engine::RotateUI(90, Vec2(v.r + 2 + 1.5f*EB_HEADER_PADDING, v.g + 1 + 0.5f*EB_HEADER_PADDING));
			if (Engine::EButton((b->editor->editorLayer == 0), v.r + 2 + EB_HEADER_PADDING, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonExtArrow, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {
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
			if (Engine::EButton((b->editor->editorLayer == 0), v.r + 1, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonExtArrow, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {
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
		Engine::DrawQuad(v.b - EB_HEADER_PADDING, v.a - EB_HEADER_PADDING - 1, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonExt->pointer); //splitter bot right
		if (Engine::EButton((editorLayer == 0), v.b - EB_HEADER_PADDING, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonX, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {//window mod
			//blocks.erase(blocks.begin() + r);
			//delete(b);
			//break;
		}
		r++;
	}

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
			xPoss[activeX] = clamp(Input::mousePosRelative.x, dw * 2, 1 - dw * 5);
		}
		moused = true;
	}
	else {
		if (!Input::mouse0) {
			activeY = -1;
		}
		else {
			Engine::DrawQuad(0.0f, Input::mousePosRelative.y*Display::height - 2, (float)Display::width, 4.0f, white(0.7f, 1));
			yPoss[activeY] = clamp(Input::mousePosRelative.y, dh * 2, 1 - dh * 5);
		}
		moused = true;
	}

	if (editorLayer > 0) {
		if (editorLayer == 1) {
			Engine::DrawQuad(0.0f, 0.0f, (float)Display::width, (float)Display::height, black(0.3f));
			Engine::Label(popupPos.x + 2, popupPos.y, 12, menuTitle, font, white());
			int off = 14;
			for (int r = 0, q = menuNames.size(); r < q; r++) {
				if (Engine::Button(popupPos.x, popupPos.y + off, 200, 15, white(1, Input::KeyHold(InputKey(Key_1 + r))? 0.3f : 0.7f), "(" + to_string(r + 1) + ") " + menuNames[r], 12, font, black()) == MOUSE_RELEASE || Input::KeyUp(InputKey(Key_1 + r))) {
					editorLayer = 0;
					if (menuFuncIsSingle) {
						if (menuFuncSingle != nullptr) {
							menuFuncSingle(menuBlock, menuFuncVals[r]);
							for (void* v : menuFuncVals)
								delete(v);
						}
					}
					else if (menuFuncs[r] != nullptr)
						menuFuncs[r](menuBlock);
					return;
				}
				off += 16;
				if ((menuPadding & (r + 1)) > 0) {
					off++;
				}

			}
			if (off == 14) {
				Engine::Label(popupPos.x + 2, popupPos.y + off, 12, "Nothing here!", font, white(0.7f));
			}
			if (Input::mouse0State == MOUSE_UP)
				editorLayer = 0;
		}
		else if (editorLayer == 2) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.4f));
			Engine::DrawQuad(editingArea.x, editingArea.y, editingArea.w, editingArea.h, grey2());
			Engine::Label(editingArea.x + 2, editingArea.y, 12, editingVal, font, editingCol);
		}
		else if (editorLayer == 3) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.8f));
			Engine::Label(Display::width*0.2f + 6, Display::height*0.2f, 22, browseIsComp? "Select Component" : "Select Asset", font, white());
			if (Engine::Button(Display::width*0.2f + 6, Display::height*0.2f + 26, Display::width*0.3f - 7, 14, grey2(), "undefined", 12, font, white()) == MOUSE_RELEASE) {
				if (browseIsComp) {
					browseTargetComp->comp = nullptr;
					browseTargetComp->path = "";
				}
				else
					*browseTarget = -1;
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
				for (int r = 0, rr = normalAssets[browseType].size(); r < rr; r++) {
					if (Engine::Button(Display::width*0.2f + 6 + (Display::width*0.3f - 5)*((r + 1) & 1), Display::height*0.2f + 41 + 15 * (((r + 1) >> 1) - 1), Display::width*0.3f - 7, 14, grey2(), normalAssets[browseType][r], 12, font, white()) == MOUSE_RELEASE) {
						*browseTarget = r;
						editorLayer = 0;
						if (browseCallback != nullptr)
							(*browseCallback)(browseCallbackParam);
						return;
					}
				}
			}
		}
		else if (editorLayer == 4) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.8f));
			Engine::DrawProgressBar(Display::width*0.5f - 300, Display::height*0.5f - 10.0f, 600.0f, 20.0f, progressValue, white(1, 0.2f), background, Vec4(0.43f, 0.57f, 0.14f, 1), 1, 2);
			Engine::Label(Display::width*0.5f - 298, Display::height*0.5f - 25.0f, 12, progressName, font, white());
			Engine::Label(Display::width*0.5f - 296, Display::height*0.5f - 6.0f, 12, progressDesc, font, white(0.7f));
		}
		else if (editorLayer == 5) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.6f));
			Engine::DrawQuad(Display::width*0.1f, Display::height*0.1f, Display::width*0.8f, Display::height*0.8f, black(0.8f));
			int offy = 18;

			Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy, 21, "Viewer settings", font, white());
			_showGrid = Engine::DrawToggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 26, 14, checkbox, _showGrid, white(), ORIENT_HORIZONTAL);
			Engine::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 28, 12, "Show grid", font, white());
			_mouseJump = Engine::DrawToggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 46, 14, checkbox, _mouseJump, white(), ORIENT_HORIZONTAL);
			Engine::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 48, 12, "Mouse stay inside window", font, white());

			offy = 100;
			Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy, 21, "Editor settings", font, white());
			Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy + 28, 12, "Background Image", font, white());
			Engine::Button(Display::width*0.1f + 200, Display::height*0.1f + offy + 25, 500, 16, grey2(), backgroundPath, 12, font, white());
			if (backgroundTex != nullptr) {
				Engine::Button(Display::width*0.1f + 702, Display::height*0.1f + offy + 25, 16, 16, buttonX, white(0.8f), white(), white(0.4f));

				Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy + 48, 12, "Background Brightness", font, white());
				Engine::Button(Display::width*0.1f + 200, Display::height*0.1f + offy + 46, 70, 16, grey2(), to_string(backgroundAlpha), 12, font, white());
				backgroundAlpha = (byte)Engine::DrawSliderFill(Display::width*0.1f + 271, Display::height*0.1f + offy + 46, 200, 16, 0, 100, backgroundAlpha, grey2(), white());
			}

			offy = 190;
			Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy, 21, "Build settings", font, white());
			Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy + 28, 12, "Data bundle size (x100Mb)", font, white());
			Engine::Button(Display::width*0.1f + 200, Display::height*0.1f + offy + 25, 70, 16, grey2(), to_string(_assetDataSize), 12, font, white());
			_cleanOnBuild = Engine::DrawToggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 46, 14, checkbox, _cleanOnBuild, white(), ORIENT_HORIZONTAL);
			Engine::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 48, 12, "Remove visual studio files on build", font, white());
		}
		else if (editorLayer == 6) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.8f));
			Engine::DrawProgressBar(50.0f, Display::height - 30.0f, Display::width - 100.0f, 20.0f, buildProgressValue, white(1, 0.2f), background, buildProgressVec4, 1, 2);
			Engine::Label(55.0f, Display::height - 26.0f, 12, buildLabel, font, white());
			for (int i = buildLog.size() - 1, dy = 0; i >= 0; i--) {
				Engine::Label(30.0f, Display::height - 50.0f - 15.0f*dy, 12.0f, buildLog[i], font, buildLogErrors[i] ? red() : (buildLogWarnings[i] ? yellow() : white(0.7f)), Display::width - 50.0f);
				dy++;
			}
		}
		else if (editorLayer == 7) {
			Engine::DrawQuad(0, 0, (float)Display::width, (float)Display::height, black(0.6f));
			Engine::DrawQuad(Display::width*0.2f, Display::height*0.2f, Display::width*0.6f, Display::height*0.6f, black(0.8f));

			Engine::Label(Display::width*0.2f + 10, Display::height*0.2f + 10, 21, "Build settings", font, white());

			uint sz = includedScenes.size();
			Engine::Label(Display::width*0.2f + 15, Display::height*0.2f + 35, 12, "Included scenes: " + to_string(sz), font, white());
			Engine::DrawQuad(Display::width*0.2f + 12, Display::height*0.2f + 50, Display::width*0.3f, 306, white(0.05f));
			for (uint a = 0; a < sz; a++) {
				includedScenesUse[a] = Engine::DrawToggle(Display::width*0.2f + 14, Display::height*0.2f + 51 + a*15, 14, checkbox, includedScenesUse[a], white(), ORIENT_HORIZONTAL);
				Engine::Label(Display::width*0.2f + 30, Display::height*0.2f + 52 + a*15, 12, includedScenes[a], font, white());
			}

			if (Engine::Button(Display::width*0.8f - 80, Display::height*0.2f + 2, 78, 18, grey2(), "Save", 18, font, white()) == MOUSE_RELEASE) {
				SavePrefs();
			}
		}
	}
}

void Editor::RegisterMenu(EditorBlock* block, string title, vector<string> names, vector<shortcutFunc> funcs, int padding, Vec2 pos) {
	editorLayer = 1;
	menuFuncIsSingle = false;
	menuTitle = title;
	menuBlock = block;
	menuNames = names;
	menuFuncs = funcs;
	popupPos = pos;
	menuPadding = padding;
}

void Editor::RegisterMenu(EditorBlock* block, string title, vector<string> names, dataFunc func, vector<void*> vals, int padding, Vec2 pos) {
	editorLayer = 1;
	menuFuncIsSingle = true;
	menuTitle = title;
	menuBlock = block;
	menuNames = names;
	menuFuncSingle = func;
	menuFuncVals = vals;
	popupPos = pos;
	menuPadding = padding;
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
	return new Texture(dataPath + "res\\" + name + "." + ext, mipmap, nearest? TEX_FILTER_POINT : TEX_FILTER_TRILINEAR, 0);
}

void Editor::_Message(string a, string b) {
	messages.push_back(pair<string, string>(a, b));
	messageTypes.push_back(0);
	for (EditorBlock* eb : blocks) {
		if (eb->editorType == 10)
			eb->Refresh();
	}
}
void Editor::_Warning(string a, string b) {
	messages.push_back(pair<string, string>(a, b));
	messageTypes.push_back(1);
	for (EditorBlock* eb : blocks) {
		if (eb->editorType == 10)
			eb->Refresh();
	}
}
void Editor::_Error(string a, string b) {
	messages.push_back(pair<string, string>(a, b));
	messageTypes.push_back(2);
	for (EditorBlock* eb : blocks) {
		if (eb->editorType == 10)
			eb->Refresh();
	}
}

void DoScanAssetsGet(Editor* e, vector<string>& list, string p, bool rec) {
	vector<string> files = IO::GetFiles(p);
	for (string w : files) {
		string ww = w.substr(w.find_last_of("."), string::npos);
		ASSETTYPE type = GetFormatEnum(ww);
		if (type != ASSETTYPE_UNDEF) {
			//cout << "file " << w << endl;
			string ss = w + ".meta";//ss.substr(0, ss.size() - 5);
			ww = (w.substr(e->projectFolder.size() + 7, string::npos));
			//string ww2 = ww.substr(0, ww.size()-5);
			if (type == ASSETTYPE_BLEND)
				e->blendAssets.push_back(ww.substr(0, ww.find_last_of('\\')) + ww.substr(ww.find_last_of('\\') + 1, string::npos));
			else {
				e->normalAssets[type].push_back(ww.substr(0, ww.find_last_of('\\')) + ww.substr(ww.find_last_of('\\') + 1, string::npos));
				e->normalAssetCaches[type].push_back(nullptr);
			}
			if (type == ASSETTYPE_MATERIAL || type == ASSETTYPE_SCENE || type == ASSETTYPE_ANIMATOR) //no meta file
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
		vector<string> dirs;
		IO::GetFolders(p, &dirs, true);
		for (string d : dirs) {
			if (d != "." && d != "..")
				DoScanAssetsGet(e, list, p + d + "\\", true);
		}
	}
}

void DoScanMeshesGet(Editor* e, vector<string>& list, string p, bool rec) {
	vector<string> files = IO::GetFiles(p);
	for (string w : files) {
		if ((w.size() > 10) && ((w.substr(w.size() - 10, string::npos)) == ".mesh.meta")) {
			string ww(w.substr(e->projectFolder.size() + 7, string::npos));
			ww = ww.substr(0, ww.find_last_of('\\')) + ww.substr(ww.find_last_of('\\') + 1, string::npos);
			e->normalAssets[ASSETTYPE_MESH].push_back(ww.substr(0, ww.size()-10));
			e->normalAssetCaches[ASSETTYPE_MESH].push_back(nullptr);
		}
	}
	if (rec) {
		vector<string> dirs;
		IO::GetFolders(p, &dirs, true);
		for (string d : dirs) {
			if (d != "." && d != "..")
				DoScanMeshesGet(e, list, p + d + "\\", true);
		}
	}
}

void DoReloadAssets(Editor* e, string path, bool recursive, mutex* l) {
	vector<string> files;
	DoScanAssetsGet(e, files, path, recursive);
	int r = files.size(), i = 0;
	for (string f : files) {
		{
			lock_guard<mutex> lock(*l);
			e->progressDesc = f;
		}
		e->ParseAsset(f);
		i++;
		e->progressValue = i*100.0f / r;
	}
	DoScanMeshesGet(e, files, path, true);
	{
		lock_guard<mutex> lock(*l);
		e->headerAssets.clear();
		AddH(e, e->projectFolder + "Assets", &e->headerAssets, &e->cppAssets);
	}
	for (string s : e->headerAssets) {
		lock_guard<mutex> lock(*l);
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
	normalAssets[ASSETTYPE_TEXTURE] = vector<string>();
	normalAssets[ASSETTYPE_HDRI] = vector<string>();
	normalAssets[ASSETTYPE_RENDERTEXTURE] = vector<string>();
	normalAssets[ASSETTYPE_MATERIAL] = vector<string>();
	normalAssets[ASSETTYPE_MESH] = vector<string>();
	normalAssets[ASSETTYPE_BLEND] = vector<string>();
	normalAssets[ASSETTYPE_ANIMCLIP] = vector<string>();
	normalAssets[ASSETTYPE_ANIMATOR] = vector<string>();

	normalAssetCaches[ASSETTYPE_TEXTURE] = vector<void*>(); //Texture*
	normalAssetCaches[ASSETTYPE_HDRI] = vector<void*>(); //Background*
	normalAssetCaches[ASSETTYPE_RENDERTEXTURE] = vector<void*>(); //RenderTexture*
	normalAssetCaches[ASSETTYPE_MATERIAL] = vector<void*>(); //Material*
	normalAssetCaches[ASSETTYPE_MESH] = vector<void*>(); //Mesh*
	normalAssetCaches[ASSETTYPE_ANIMCLIP] = vector<void*>(); //AnimClip*
	normalAssetCaches[ASSETTYPE_ANIMATOR] = vector<void*>(); //Animator*
}

void Editor::ReloadAssets(string path, bool recursive) {

	//normalAssetsOld = unordered_map<ASSETTYPE, vector<string>>(normalAssets);

	ResetAssetMap();

	blendAssets.clear();
	editorLayer = 4;
	progressName = "Loading assets...";
	progressValue = 0;
	thread t(DoReloadAssets, this, path, recursive, lockMutex);
	t.detach();
}

bool Editor::ParseAsset(string path) {
	cout << "parsing " << path << endl;
	ifstream stream(path.c_str(), ios::in | ios::binary);
	bool ok;
	if (!stream.good()) {
		_Message("Asset Parser", "asset not found! " + path);
		return false;
	}
	string ext = path.substr(path.find_last_of('.') + 1, string::npos);
	string p = (path + ".meta");
	SetFileAttributes(p.c_str(), FILE_ATTRIBUTE_NORMAL);
	if (ext == "shade") {
		ok = ShaderBase::Parse(&stream, path + ".meta");
	}
	else if (ext == "blend"){
		ok = Mesh::ParseBlend(this, path);
	}
	else if (ext == "bmp" || ext == "jpg" || ext == "png") {
		ok = Texture::Parse(this, path);
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
		delete(backgroundTex); //ごめんなさいpandaman先生
	backgroundTex = new Texture(s);
	if (a >= 0)
		backgroundAlpha = (int)(a*100);
}

void DoScanBrowseComp(SceneObject* o, COMPONENT_TYPE t, string p, vector<Component*>& vc, vector<string>& vs) {
	Component* c = o->GetComponent(t);
	string nm = p + ((p == "") ? "" : "/") + o->name;
	if (c != nullptr) {
		vc.push_back(c);
		vs.push_back(nm);
	}
	for (auto oo : o->children)
		DoScanBrowseComp(oo, t, nm, vc, vs);
}

void Editor::ScanBrowseComp() {
	browseCompList.clear();
	browseCompListNames.clear();
	for (auto o : activeScene->objects)
		DoScanBrowseComp(o, browseTargetComp->type, "", browseCompList, browseCompListNames);
}

void Editor::AddBuildLog(Editor* e, string s, bool forceE) {
	lock_guard<mutex> lock(*Editor::lockMutex);
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
	//glutPostRedisplay();
}

void* Editor::GetCache(ASSETTYPE type, int i) {
	if (i < 0)
		return nullptr;
	void *data = normalAssetCaches[type][i];
	if (data != nullptr)
		return data;
	else {
		return GenCache(type, i);
	}
}

void* Editor::GenCache(ASSETTYPE type, int i) {
	switch (type) {
	case ASSETTYPE_MESH:
		normalAssetCaches[type][i] = new Mesh(normalAssets[type][i]);
		break;
	case ASSETTYPE_ANIMCLIP:
		normalAssetCaches[type][i] = new AnimClip(normalAssets[type][i]);
		break;
	case ASSETTYPE_ANIMATOR:
		normalAssetCaches[type][i] = new Animator(normalAssets[type][i]);
		break;
	case ASSETTYPE_SHADER:
		normalAssetCaches[type][i] = new ShaderBase(normalAssets[type][i]);
		break;
	case ASSETTYPE_MATERIAL:
		normalAssetCaches[type][i] = new Material(normalAssets[type][i]);
		break;
	case ASSETTYPE_TEXTURE:
		normalAssetCaches[type][i] = new Texture(i, this);
		break;
	case ASSETTYPE_HDRI:
		normalAssetCaches[type][i] = new Background(i, this);
		break;
	default:
		return nullptr;
	}
	return normalAssetCaches[type][i];
}

/*
app.exe
content0.data -> asset list (type, name, index) (ascii), [\n], level data (binary)
content(1+).data ->assets (index, data) (binary)
*/
bool MergeAssets(Editor* e) {
	string ss = e->projectFolder + "Release\\data0";
	ofstream file;
	char null = 0, etx = 3;
	e->AddBuildLog(e, "Writing to " + ss);
	cout << ss << endl;
	file.open(ss.c_str(), ios::out | ios::binary | ios::trunc);
	if (!file.is_open())
		return false;
	//headers
	file << "D0";

	//asset data
	//D0[NUM(2)]SUM([TYPE][IPOSS][NAME0])
	vector<uint> dataPoss;
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
			ifstream f2(sss, ios::in | ios::binary);
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
	string nm = e->projectFolder + "Release\\data" + to_string(incre);
	e->AddBuildLog(e, "Writing to " + nm);
	ofstream file2(nm.c_str(), ios::out | ios::binary | ios::trunc); 
	if (!file2.is_open()) {
		e->AddBuildLog(e, "Cannot open " + nm + "!", true);
		return false;
	}
	uint xxx = 0;
	for (auto& it : e->normalAssets) {
		if (it.second.size() > 0) {
			ushort i = 0;
			for (string s : it.second) {
				e->buildLabel = "Build: merging assets... " + s + "(" + to_string(xxx + 1) + "/" + to_string(xx) + ") ";
				string nmm;
				if (it.first == ASSETTYPE_MESH)
					nmm = e->projectFolder + "Assets\\" + s + ".mesh.meta";
				else if (it.first == ASSETTYPE_MATERIAL || it.first == ASSETTYPE_ANIMATOR)
					nmm = e->projectFolder + "Assets\\" + s;
				else
					nmm = e->projectFolder + "Assets\\" + s + ".meta";
				ifstream f2(nmm.c_str(), ios::in | ios::binary);
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
					nm = e->projectFolder + "Release\\data" + to_string(++incre);
					e->AddBuildLog(e, "Writing to " + nm);
					file2.open(nm.c_str(), ios::out | ios::binary | ios::trunc);
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
	return true;
}

bool DoMsBuild(Editor* e) {
	char s[255];
	DWORD i = 255;
	if (RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\4.0", "MSBuildToolsPath", RRF_RT_ANY, nullptr, &s, &i) == ERROR_SUCCESS) {
		cout << "Executing " << s << "msbuild.exe" << endl;

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = NULL;
		HANDLE stdOutR, stdOutW;
		if (!CreatePipe(&stdOutR, &stdOutW, &sa, 0)) {
			e->AddBuildLog(e, "failed to create pipe for stdout!");
			return false;
		}
		if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0)){
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
			cout << "Putenv failed for msbuilder!" << endl;
		if (CreateProcess(ss.c_str(), "D:\\TestProject2\\TestProject2.vcxproj /nr:false /t:Build /p:Configuration=Release /v:n /nologo /fl /flp:LogFile=D:\\TestProject2\\BuildLog.txt", NULL, NULL, true, 0, NULL, "D:\\TestProject2\\", &startInfo, &processInfo) != 0) {
			e->AddBuildLog(e, "Compiling from " + ss);
			cout << "compiling" << endl;
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
			//e->AddBuildLog("Compile finished with code " + to_string(ii) + ".");
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
	e->WAITINGBUILDSTARTFLAG = true;
}

void AddOtherScenes(Editor* e, string dir, vector<string> &v1, vector<bool> &v2) {
	for (string s : IO::GetFiles(dir)) {
		if (s.size() > 7 && s.substr(s.size() - 6, string::npos) == ".scene") {
			string pp = s.substr(s.find_last_of("\\")+1, string::npos);
			for (string p2 : v1)
				if (p2==pp)
					goto cancel;
			v1.push_back(pp);
			v2.push_back(false);
		}
	cancel:;
	}
	vector<string> f;
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
	e->activeScene->Save(e);
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
	this_thread::sleep_for(chrono::seconds(2));
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
				string se = s2.substr(s2.size()-4, string::npos);
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
	float a = Rect(v.r, v.g, v.b, v.a).Inside(Input::mousePos)? 1 : 0.3f;
	for (uint x = 0, y = blocks.size(); x < y; x++) {
		if (Engine::EButton(editor->editorLayer == 0, v.r + v.b - EB_HEADER_PADDING - 2 - EB_HEADER_SIZE*(y - x)*1.2f, v.g + 1, EB_HEADER_SIZE - 2, EB_HEADER_SIZE - 2, editor->ebIconTexs[blocks[x]->editorType], white(((x == active) ? 1 : 0.7f)*a)) == MOUSE_RELEASE) {
			active = x;
			Set();
		}
	}
	if (Engine::EButton(editor->editorLayer == 0, v.r + v.b - EB_HEADER_PADDING - 2 - EB_HEADER_SIZE*(blocks.size() + 1)*1.2f, v.g + 1, EB_HEADER_SIZE - 2, EB_HEADER_SIZE - 2, editor->buttonPlus, white(a)) == MOUSE_RELEASE) {

	}
}