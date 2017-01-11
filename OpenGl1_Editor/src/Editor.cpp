#include "Editor.h"
#include "Engine.h"
#include <GL/glew.h>
#include <iostream>
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
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/freeglut.h>
#include <chrono>
#include <thread>
#include <filesystem>

HWND Editor::hwnd = 0;
string Editor::dataPath = "";

Color grey1() {
	return Color(33.0f / 255, 37.0f / 255, 40.0f / 255, 1);
}
Color grey2() {
	return Color(61.0f / 255, 68.0f / 255, 73.0f / 255, 1);
}
Color accent() {
	return Color(223.0f / 255, 119.0f / 255, 4.0f / 255, 1);
}

int GetShortcutInt(byte c, int m) { return c << 4 | m; }

bool DrawHeaders(Editor* e, EditorBlock* b, Color* v, string titleS, string titleB) {
	//Engine::Button(v->r, v->g + EB_HEADER_SIZE + 1, v->b, v->a - EB_HEADER_SIZE - 2, black(), white(0.05f), white(0.05f));
	Vec2 v2(v->b*0.1f, EB_HEADER_SIZE*0.1f);
	Engine::DrawQuad(v->r + EB_HEADER_PADDING + 1, v->g, v->b - 3 - 2 * EB_HEADER_PADDING, EB_HEADER_SIZE, e->background->pointer, Vec2(), Vec2(v2.x, 0), Vec2(0, v2.y), v2, false, accent());
	Engine::Label(round(v->r + 5 + EB_HEADER_PADDING), round(v->g + 2), 10, titleS, e->font, black(), Display::width);
	Engine::Label(round(v->r + 8 + EB_HEADER_PADDING), round(v->g + 12), 22, titleB, e->font, black());
	return Rect(v->r, v->g + EB_HEADER_SIZE + 1, v->b, v->a - EB_HEADER_SIZE - 2).Inside(Input::mousePos);
}

//Texture SystemButtons::x = Texture("F:\\xbutton.bmp");
//Texture SystemButtons::dash = Texture("F:\\dashbutton.bmp");

void EB_Empty::Draw() {
	Color v = Color(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	v.a = round(v.a - v.g) - 1;
	v.b = round(v.b - v.r) - 1;
	v.g = round(v.g) + 1;
	v.r = round(v.r) + 1;
	DrawHeaders(editor, this, &v, "hatena header", "Hatena Title");
}

void EBH_DrawItem(SceneObject* sc, Editor* e, Color* v, int& i, int indent) {
	int xo = indent * 20;
	//if (indent > 0) {
	//	Engine::DrawLine(Vec2(xo - 10 + v->r, v->g + EB_HEADER_SIZE + 9 + 17 * i), Vec2(xo + v->r, v->g + EB_HEADER_SIZE + 10 + 17 * i), white(0.5f), 1);
	//}
	if (Engine::EButton((e->editorLayer == 0), v->r + xo + ((sc->childCount > 0) ? 16 : 0), v->g + EB_HEADER_SIZE + 1 + 17 * i, v->b - xo - ((sc->childCount > 0) ? 16 : 0), 16, grey2()) == MOUSE_RELEASE) {
		e->selected = sc;
	}
	Engine::Label(v->r + 19 + xo, v->g + EB_HEADER_SIZE + 4 + 17 * i, 12, sc->name, e->font, white());
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
		for each (SceneObject* scc in sc->children)
		{
			//oii = i - 1;
			EBH_DrawItem(scc, e, v, i, indent + 1);
		}
		//Engine::DrawLine(Vec2(xo + 10 + v->r, v->g + EB_HEADER_SIZE + 18 + 17 * (oi)), Vec2(xo + 10 + v->r, v->g + EB_HEADER_SIZE + 10 + 17 * (oii) + sc->childCount * 17), white(0.5f), 1);
	}
}

void EB_Hierarchy::Draw() {
	Color v = Color(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	v.a = round(v.a - v.g) - 1;
	v.b = round(v.b - v.r) - 1;
	v.g = round(v.g) + 1;
	v.r = round(v.r) + 1;
	DrawHeaders(editor, this, &v, "Scene objects", "Hierarchy");

	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	glDisable(GL_DEPTH_TEST);
	int i = 0;
	for each (SceneObject* sc in editor->activeScene.objects)
	{
		EBH_DrawItem(sc, editor, &v, i, 0);
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

EB_Browser_File::EB_Browser_File(Editor* e, string path, string nm) : path(path), thumbnail(-1) {
	name = nm;
	string ext = name.substr(name.find_last_of('.') + 1, string::npos);
	for (int a = e->assetIcons.size() - 1; a >= 0; a--) {
		if (ext == e->assetIconsExt[a]) {
			thumbnail = a;
			name = name.substr(0, name.find_last_of('.'));
			break;
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
}

bool DrawFileRect(float w, float h, float size, EB_Browser_File* file, EditorBlock* e) {
	bool b = false;
	if (e->editor->editorLayer == 0)
		b = ((file->thumbnail >= 0) ? Engine::Button(w + 1, h + 1, size - 2, size - 2, e->editor->assetIcons[file->thumbnail], white(1, 0.8f), white(), white(1, 0.5f)) : Engine::Button(w + 1, h + 1, size - 2, size - 2, grey2())) == MOUSE_RELEASE;
	else {
		b = false;
		if (file->thumbnail >= 0)
			Engine::DrawTexture(w + 1, h + 1, size - 2, size - 2, e->editor->assetIcons[file->thumbnail]);
		else
			Engine::DrawQuad(w + 1, h + 1, size - 2, size - 2, grey2());
	}
	Engine::DrawQuad(w + 1, h + 1 + size*0.6f, size - 2, size*0.4f - 2, grey2()*0.7f);
	Engine::Label(w + 2, h + 1 + size*0.7f, 12, file->name, e->editor->font, white(), size);
	return b;
}

void EB_Browser::Draw() {
	Color v = Color(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	v.a = round(v.a - v.g) - 1;
	v.b = round(v.b - v.r) - 1;
	v.g = round(v.g) + 1;
	v.r = round(v.r) + 1;
	bool in = DrawHeaders(editor, this, &v, currentDir, "Browser");

	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);
	glDisable(GL_DEPTH_TEST);

	for (int y = dirs.size() - 1; y >= 0; y--) {
		if (Engine::EButton((editor->editorLayer == 0), v.r, v.g + EB_HEADER_SIZE + 1 + (16 * y), 150, 15, grey1()) == MOUSE_RELEASE) {
			if (dirs.at(y) != ".") {
				if (dirs.at(y) == "..")
					currentDir = currentDir.substr(0, currentDir.find_last_of('\\'));
				else currentDir += "\\" + dirs.at(y);
				Refresh();
				return;
			}
		}
		Engine::Label(v.r + 2, v.g + EB_HEADER_SIZE + 4 + (16 * y), 12, dirs.at(y), editor->font, white(), 150);
	}
	float ww = 0;
	int hh = 0;
	for (int ff = files.size() - 1; ff >= 0; ff--) {
		if (DrawFileRect(v.r + 151 + ww, v.g + EB_HEADER_SIZE + 101 * hh, 100, &files[ff], this)) {
			editor->selectedFile = files[ff].path + "\\" + files[ff].name;
		}
		ww += 101;
		if (ww > (v.b - 252)) {
			ww = 0;
			hh++;
			if (v.g + EB_HEADER_SIZE + 101 * hh > Display::height)
				break;
		}
	}

	Engine::EndStencil();
}

void EB_Browser::Refresh() {
	dirs.clear();
	files.clear();
	IO::GetFolders(currentDir, &dirs);
	cout << dirs.size() << "folders from " << currentDir << endl;
	files = IO::GetFilesE(editor, currentDir);
	cout << files.size() << "files from " << currentDir << endl;
}

EB_Viewer::EB_Viewer(Editor* e, int x1, int y1, int x2, int y2) : rz(45), rw(45), scale(1) {
	editorType = 2;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	MakeMatrix();

	shortcuts.emplace(GetShortcutInt('a', GLUT_ACTIVE_SHIFT), &_OpenMenuAdd);
	shortcuts.emplace(GetShortcutInt('c', GLUT_ACTIVE_SHIFT), &_OpenMenuCom);
	shortcuts.emplace(GetShortcutInt('w', 0), &_OpenMenuW);
	shortcuts.emplace(GetShortcutInt(' ', GLUT_ACTIVE_CTRL), &_OpenMenuChgMani);
	shortcuts.emplace(GetShortcutInt(' ', GLUT_ACTIVE_ALT), &_OpenMenuChgOrient);

	shortcuts.emplace(GetShortcutInt('x', 0), &_X);
	shortcuts.emplace(GetShortcutInt('y', 0), &_Y);
	shortcuts.emplace(GetShortcutInt('z', 0), &_Z);

	shortcuts.emplace(GetShortcutInt('a', 0), &_SelectAll);
	shortcuts.emplace(GetShortcutInt('z', 0), &_ViewInvis);
	shortcuts.emplace(GetShortcutInt('5', 0), &_ViewPersp);

	shortcuts.emplace(GetShortcutInt('g', 0), &_Grab);
	shortcuts.emplace(GetShortcutInt('r', 0), &_Rotate);
	shortcuts.emplace(GetShortcutInt('s', 0), &_Scale);

	shortcuts.emplace(GetShortcutInt('1', 0), &_ViewFront);
	shortcuts.emplace(GetShortcutInt('1', GLUT_ACTIVE_ALT), &_ViewBack);
	shortcuts.emplace(GetShortcutInt('3', 0), &_ViewRight);
	shortcuts.emplace(GetShortcutInt('3', GLUT_ACTIVE_ALT), &_ViewLeft);
	shortcuts.emplace(GetShortcutInt('7', 0), &_ViewTop);
	shortcuts.emplace(GetShortcutInt('7', GLUT_ACTIVE_ALT), &_ViewBottom);
}

void EB_Viewer::MakeMatrix() {
	float csz = cos(rz*3.14159265f / 180.0f);
	float snz = sin(rz*3.14159265f / 180.0f);
	float csw = cos(rw*3.14159265f / 180.0f);
	float snw = sin(rw*3.14159265f / 180.0f);
	viewingMatrix = glm::mat4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
	viewingMatrix = glm::mat4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1)*viewingMatrix;
	arrowX = viewingMatrix*Color(-1, 0, 0, 0);
	arrowY = viewingMatrix*Color(0, 1, 0, 0);
	arrowZ = viewingMatrix*Color(0, 0, 1, 0);
}

Vec3 xyz(Color v) {
	return Vec3(v.x, -v.y, v.z);
}
Vec2 xy(Vec3 v) {
	return Vec2(v.x, v.y);
}

void EB_Viewer::Draw() {
	Color v = Color(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	v.a = round(v.a - v.g) - 1;
	v.b = round(v.b - v.r) - 1;
	v.g = round(v.g) + 1;
	v.r = round(v.r) + 1;
	DrawHeaders(editor, this, &v, "scene ID here", "Viewer");

	Engine::BeginStencil(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2);

	Vec2 v2 = Vec2(Display::width, Display::height)*0.03f;
	Engine::DrawQuad(0, 0, Display::width, Display::height, white(1, 0.2f));//editor->checkers->pointer, Vec2(), Vec2(v2.x, 0), Vec2(0, v2.y), v2, true, white(0.05f));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float ww1 = editor->xPoss[x1];
	float hh1 = editor->yPoss[y1];
	float ww = editor->xPoss[x2] - ww1;
	float hh = editor->yPoss[y2] - hh1;
	//if (!persp) {
	float h40 = 40 * (hh*Display::height) / (ww*Display::width);
	glMultMatrixf(glm::value_ptr(glm::ortho(-20 * ww, 40.0f - 20 * ww, -40.0f + 20 * hh, 20 * hh, 0.01f, 1000.0f)));
	float mww = max(ww, 0.3f) * scale;
	glScalef(-mww, mww*Display::width/Display::height, mww);
	//}
	//else {
		//glMultMatrixf(glm::value_ptr(glm::perspective(30.0f, 1.0f, 0.01f, 1000.0f)));
		//glScalef(1, -1, 1);
	//}
	glTranslatef(0, 0, -30);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	float csz = cos(-rz*3.14159265f / 180.0f);
	float snz = sin(-rz*3.14159265f / 180.0f);
	float csw = cos(rw*3.14159265f / 180.0f);
	float snw = sin(rw*3.14159265f / 180.0f);
	glMultMatrixf(glm::value_ptr(glm::mat4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1)));
	glMultMatrixf(glm::value_ptr(glm::mat4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1)));
	glEnable(GL_DEPTH_TEST);

	//draw scene//
	for each (SceneObject* sc in editor->activeScene.objects)
	{
		glPushMatrix();
		Vec3 v = sc->transform.position;
		//rotation matrix here
		glTranslatef(v.x, v.y, v.z);
		for each (Component* com in sc->_components)
		{
			if (com->drawable) {
				com->DrawEditor();
			}
		}
		glPopMatrix();
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
			if (selectedTooltip == 0) DrawTArrows(editor->selected->transform.position, 2);
			else if (selectedTooltip == 1) DrawTArrows(editor->selected->transform.position, 2);
			else DrawSArrows(editor->selected->transform.position, 2);
		}
		else {
			Engine::DrawLineW(editor->selected->transform.position + modAxisDir*-100000.0f, editor->selected->transform.position + modAxisDir*100000.0f, white(), 3);
		}
	}

	glPopMatrix();

	Engine::EndStencil();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glMultMatrixf(glm::value_ptr(glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -100.0f, 100.0f)));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
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
		Engine::DrawQuad(v.x + 28, v.y + EB_HEADER_SIZE + 10 + 22 * (drawDescLT - 1), 200, 20, white(1, 0.6f));
		Engine::Label(v.x + 30, v.y + EB_HEADER_SIZE + 15 + 22 * (drawDescLT - 1), 12, descLabelLT, editor->font, black());
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
		MakeMatrix();
	}
	else if (modifying > 0) {
		cout << (int)(modifying & 0x0f) << endl;
		modVal += Vec2(d.x / Display::width, d.y / Display::height);
		if (modifying >> 4 == 1) {
			switch (modifying & 0x0f) {
				/*
			case 0: {
			Color c = glm::inverse(viewingMatrix)*Color(modVal.y, 0, -modVal.x, 0);
			editor->selected->transform.position = preModVals + Vec3(c.x, c.y, c.z)*40.0f/scale;
			break;
			}
			*/
			case 1:
				editor->selected->transform.position = preModVals + Vec3((-modVal.x + modVal.y) * 40 / scale, 0, 0);
				break;
			case 2:
				editor->selected->transform.position = preModVals + Vec3(0, (modVal.x - modVal.y) * 40 / scale, 0);
				break;
			case 3:
				editor->selected->transform.position = preModVals + Vec3(0, 0, (modVal.x - modVal.y) * 40 / scale);
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
				editor->selected->transform.eulerRotation = preModVals;
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
	scale += (up?0.1f:-0.1f);
	scale = min(max(scale, 0.01f), 1000);
}

EB_Inspector::EB_Inspector(Editor* e, int x1, int y1, int x2, int y2): label("") {
	editorType = 3;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	loaded = false;
	lock = false;
}

void EB_Inspector::SelectAsset(EBI_Asset* e, string s) {
	if (loaded && isAsset)
		delete (obj);
	obj = e;
	isAsset = true;
	label = s;
	if (obj->correct)
		loaded = true;
}

void EB_Inspector::Draw() {
	Color v = Color(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	v.a = round(v.a - v.g) - 1;
	v.b = round(v.b - v.r) - 1;
	v.g = round(v.g) + 1;
	v.r = round(v.r) + 1;
	DrawHeaders(editor, this, &v, isAsset ? (loaded ? label : "No object selected") : ((editor->selected != nullptr) ? editor->selected->name : "No object selected"), "Inspector");

	if (isAsset) {
		if (loaded)
			obj->Draw(editor, this, &v);
		else
			Engine::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, "Select object to inspect.", editor->font, white());
	}
	else if (editor->selected != nullptr){
		Engine::DrawTexture(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 18, 18, editor->object);
		Engine::EButton((editor->editorLayer == 0), v.r + 20, v.g + 2 + EB_HEADER_SIZE, v.b - 21, 18, grey2());
		Engine::Label(v.r + 22, v.g + 6 + EB_HEADER_SIZE, 12, editor->selected->name, editor->font, white());

		//TRS
		Engine::Label(v.r, v.g + 23 + EB_HEADER_SIZE, 12, "Position", editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.19f, v.g + 21 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.4f, 0.2f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.19f + 2, v.g + 23 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.position.x), editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.46f, v.g + 21 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.4f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.46f + 2, v.g + 23 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.position.y), editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.73f, v.g + 21 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.2f, 0.4f, 1));
		Engine::Label(v.r + v.b*0.73f + 2, v.g + 23 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.position.z), editor->font, white());

		Engine::Label(v.r, v.g + 40 + EB_HEADER_SIZE, 12, "Rotation", editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.19f, v.g + 38 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.4f, 0.2f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.19f + 2, v.g + 40 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.eulerRotation.x), editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.46f, v.g + 38 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.4f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.46f + 2, v.g + 40 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.eulerRotation.y), editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.73f, v.g + 38 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.2f, 0.4f, 1));
		Engine::Label(v.r + v.b*0.73f + 2, v.g + 40 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.eulerRotation.z), editor->font, white());

		Engine::Label(v.r, v.g + 57 + EB_HEADER_SIZE, 12, "Scale", editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.19f, v.g + 55 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.4f, 0.2f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.19f + 2, v.g + 57 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.scale.x), editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.46f, v.g + 55 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.4f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.46f + 2, v.g + 57 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.scale.y), editor->font, white());
		Engine::EButton((editor->editorLayer == 0), v.r + v.b*0.73f, v.g + 55 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.2f, 0.4f, 1));
		Engine::Label(v.r + v.b*0.73f + 2, v.g + 57 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.scale.z), editor->font, white());
		
		//draw components
		uint off = 74 + EB_HEADER_SIZE;
		for each (Component* c in editor->selected->_components)
		{
			c->DrawInspector(editor, c, v, off);
		}
	}
	else
		Engine::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, "Select object to inspect.", editor->font, white());
}

void EB_Inspector::DrawScalar(Editor* e, Color v, string label, float& value) {
	Engine::Label(v.r + 2, v.g, 12, label, e->font, white(), v.b*0.19f - 2);
	Engine::EButton((e->editorLayer == 0), v.r + v.b*0.19f, v.g, v.b*0.71 - 1, 16, Color(0.3f, 0.3f, 0.3f, 1));
	Engine::Label(v.r + v.b*0.19f + 2, v.g, 12, label, e->font, white(), v.b * 0.71f - 4);
}

void Editor::LoadDefaultAssets() {
	font = new Font(dataPath + "res\\default_s.font", dataPath + "res\\default_l.font", 17);

	buttonX = GetRes("xbutton");
	buttonExt = GetRes("extbutton");
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

	checkbox0 = GetRes("checkbox_false");
	checkbox1 = GetRes("checkbox_true");
	
	assetIconsExt.push_back("h");
	assetIconsExt.push_back("blend");
	assetIconsExt.push_back("cpp");
	assetIconsExt.push_back("shade");
	assetIconsExt.push_back("txt");
	assetIcons.push_back(new Texture(dataPath + "res\\asset_header.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_blend.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_cpp.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_shade.bmp", false));
	assetIcons.push_back(new Texture(dataPath + "res\\asset_txt.bmp", false));
	//assetIcons.push_back(GetRes("asset_blend"));
	//assetIcons.push_back(GetRes("asset_cpp"));
	//assetIcons.push_back(GetRes("asset_shade"));

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

	globalShorts.emplace(GetShortcutInt('b', GLUT_ACTIVE_CTRL), &Compile);
	globalShorts.emplace(GetShortcutInt('u', GLUT_ACTIVE_CTRL), &ShowPrefs);
	globalShorts.emplace(GetShortcutInt('s', GLUT_ACTIVE_CTRL), &SaveScene);
}

void AddH(string dir, vector<string>* h) {
	for each (string s in IO::GetFiles(dir)) {
		if (s.substr(s.size() - 2, string::npos) == ".h")
			h->push_back(s.substr(s.find_last_of('\\') + 1, string::npos));
	}
	vector<string> dirs;
	IO::GetFolders(dir, &dirs);
	for each (string ss in dirs) {
		if (ss != "." && ss != "..")
			AddH(dir + "\\" + ss, h);
	}
}

void Editor::RefreshScriptAssets() {
	headerAssets.clear();
	AddH(projectFolder + "Assets", &headerAssets);
	GenerateScriptResolver();
}

void Editor::GenerateScriptResolver() {
	string h = "#include <unordered_map>\n#include \"Engine.h\"\ntypedef SceneScript(*sceneScriptInstantiator)();\nclass SceneScriptResolver {\npublic:\n\tSceneScriptResolver();\n\tstd::vector<sceneScriptInstantiator> map;\n};";
	string s = "#include \"SceneScriptResolver.h\"\n#include \"Engine.h\"\n\n";
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		s += "#include \"" + headerAssets[a] + "\"\n";
		string ss = headerAssets[a].substr(0, headerAssets[a].size()-2);
		s += ss + " _Inst" + to_string(a) + "() { return new " + ss + "(); }\n\n";
	}
	s += "\n\nusing namespace std;\r\nSceneScriptResolver::SceneScriptResolver() {\n";
	for (int a = 0, b = headerAssets.size(); a < b; a++) {
		s += "\tmap.push_back(&Inst" + to_string(a) + ");\n";
	}
	s += "}";
	ofstream ofs (projectFolder + "\\System\\SceneScriptResolver.cpp");
	ofs << s;
	ofs.close();
	ofs.open(projectFolder + "\\System\\SceneScriptResolver.h");
	ofs << h;
	ofs.close();
}

void Editor::NewScene() {
	if (sceneLoaded)
		delete(&activeScene);
	activeScene = Scene();
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
	for each (EditorBlock* b in blocks)
	{
		Color v = Color(Display::width*xPoss[b->x1], Display::height*yPoss[b->y1], Display::width*xPoss[b->x2], Display::height*yPoss[b->y2]);

		Engine::DrawQuad(v.r + 1, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_SIZE - 1 - EB_HEADER_PADDING, grey1());
		if (Engine::EButton((b->editor->editorLayer == 0), v.r + 1, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonExt, white(1, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) { //splitter top left
			if (b->headerStatus == 1)
				b->headerStatus = 0;
			else b->headerStatus = (b->headerStatus == 0 ? 1 : 0);
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
			blocks.erase(blocks.begin() + r);
			delete(b);
			break;
		}
		Engine::DrawQuad(v.b - EB_HEADER_PADDING, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_SIZE - 1 - EB_HEADER_PADDING, grey1());
		r++;
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
			Engine::DrawQuad(Input::mousePosRelative.x*Display::width - 2, 0, 4, Display::height, white(0.7f, 1));
			xPoss[activeX] = clamp(Input::mousePosRelative.x, dw * 2, 1 - dw * 5);
		}
		moused = true;
	}
	else {
		if (!Input::mouse0) {
			activeY = -1;
		}
		else {
			Engine::DrawQuad(0, Input::mousePosRelative.y*Display::height - 2, Display::width, 4, white(0.7f, 1));
			yPoss[activeY] = clamp(Input::mousePosRelative.y, dh * 2, 1 - dh * 5);
		}
		moused = true;
	}

	if (editorLayer > 0) {
		if (editorLayer == 1) {
			Engine::DrawQuad(0, 0, Display::width, Display::height, black(0.3f));
			Engine::Label(popupPos.x + 2, popupPos.y, 12, menuTitle, font, white());
			int off = 14;
			for (int r = 0, q = menuNames.size(); r < q; r++) {
				if (Engine::Button(popupPos.x, popupPos.y + off, 200, 15, white(1, 0.7f), menuNames[r], 12, font, black()) == MOUSE_RELEASE) {
					editorLayer = 0;
					if (menuFuncIsSingle) {
						if (menuFuncSingle != nullptr)
							menuFuncSingle(menuBlock, menuFuncVals[r]);
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
		else if (editorLayer == 4) {
			Engine::DrawQuad(0, 0, Display::width, Display::height, black(0.8f));
			Engine::DrawProgressBar(50.0f, Display::height - 30.0f, Display::width - 100.0f, 20.0f, buildProgressValue, white(1, 0.2f), background, buildProgressColor, 1, 2);
			Engine::Label(55.0f, Display::height - 26.0f, 12, buildLabel, font, white());
			for (int i = buildLog.size() - 1, dy = 0; i >= 0; i--) {
				Engine::Label(30.0f, Display::height - 50.0f - 15.0f*dy, 12, buildLog[i], font, buildLogErrors[i]? red() : white(0.7f), Display::width - 50);
				dy++;
			}
		}
		else if (editorLayer == 5) {
			Engine::DrawQuad(0, 0, Display::width, Display::height, black(0.6f));
			Engine::DrawQuad(Display::width*0.1f, Display::height*0.1f, Display::width*0.8f, Display::height*0.8f, black(0.8f));
			int offy = 18;

			Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy, 21, "Viewer settings", font, white());
			_showGrid = Engine::DrawToggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 26, 14, _showGrid ? checkbox1 : checkbox0, _showGrid);
			Engine::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 28, 12, "Show grid", font, white());
			_mouseJump = Engine::DrawToggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 46, 14, _mouseJump ? checkbox1 : checkbox0, _mouseJump);
			Engine::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 48, 12, "Mouse stay inside window", font, white());

			offy = 100;
			Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy, 21, "Build settings", font, white());
			Engine::Label(Display::width*0.1f + 10, Display::height*0.1f + offy + 28, 12, "Data bundle size (x100Mb)", font, white());
			Engine::Button(Display::width*0.1f + 200, Display::height*0.1f + offy + 25, 70, 16, grey2(), to_string(_assetDataSize), 12, font, white());
			_cleanOnBuild = Engine::DrawToggle(Display::width*0.1f + 12, Display::height*0.1f + offy + 46, 14, _cleanOnBuild ? checkbox1 : checkbox0, _cleanOnBuild);
			Engine::Label(Display::width*0.1f + 30, Display::height*0.1f + offy + 48, 12, "Remove visual studio files on build", font, white());
		}
	}
}

void Editor::RegisterMenu(EditorBlock* block, string title, vector<string> names, vector<shortcutFunc> funcs, int padding) {
	editorLayer = 1;
	menuFuncIsSingle = false;
	menuTitle = title;
	menuBlock = block;
	menuNames = names;
	menuFuncs = funcs;
	popupPos = Input::mousePos;
	menuPadding = padding;
}

void Editor::RegisterMenu(EditorBlock* block, string title, vector<string> names, dataFunc func, vector<void*> vals, int padding) {
	editorLayer = 1;
	menuFuncIsSingle = true;
	menuTitle = title;
	menuBlock = block;
	menuNames = names;
	menuFuncSingle = func;
	menuFuncVals = vals;
	popupPos = Input::mousePos;
	menuPadding = padding;
}

Texture* Editor::GetRes(string name) {
	return GetRes(name, false, false);
}
Texture* Editor::GetRes(string name, bool mipmap) {
	return GetRes(name, mipmap, false);
}
Texture* Editor::GetRes(string name, bool mipmap, bool nearest) {
	return new Texture(dataPath + "res\\" + name + ".bmp", mipmap, nearest);
}

bool Editor::ParseAsset(string path) {
	ifstream stream(path.c_str());
	string parsed = "";
	if (!stream.good()) {
		cout << "asset not found!" << endl;
		return false;
	}
	string ext = path.substr(path.find_last_of('.') + 1, string::npos);
	if (ext == "shade") {
		parsed = ShaderBase::Parse(&stream);
	}
	else if (ext == "blend"){

		return false;
	}
	else if (ext == "bmp" || ext == "png" || ext == "jpg" || ext == "jpeg") {

		return false;
	}
	else return false;
	ofstream strm;
	strm.open((path + ".meta"), ofstream::trunc | ofstream::out);
	strm.write(parsed.c_str(), parsed.size());
	strm.close();
	SetFileAttributes((path + ".meta").c_str(), FILE_ATTRIBUTE_HIDDEN);
	return true;
}

void Editor::AddBuildLog(string s) {
	buildLog.push_back(s);
	buildLogErrors.push_back(s.find("error C") != string::npos);
}

bool Editor::GetCache(string& path, I_EBI_ValueCollection& vals) {
	
	vals.vals.clear();
	return false;
}

bool Editor::SetCache(string& path, I_EBI_ValueCollection* vals) {
	return false;
}

bool DoMsBuild(Editor* e) {
	LPDWORD word;
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
			e->AddBuildLog("failed to create pipe for stdout!");
			return false;
		}
		if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0)){
			e->AddBuildLog("failed to set handle for stdout!");
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
		if (IO::HasFile("F:\\TestProject\\Debug\\TestProject.exe")) {
			e->buildLog.push_back("removing previous executable");
			if (remove("F:\\TestProject\\Debug\\TestProject.exe") != 0) {
				e->buildLog.push_back("unable to remove previous executable!");
				return false;
			}
		}
		*/
		bool failed = true;
		byte FINISH = 0;
		_putenv("MSBUILDDISABLENODEREUSE=1");
		if (CreateProcess(ss.c_str(), "F:\\TestProject\\TestProject.vcxproj /nr:false /t:Build /p:Configuration=Release /v:n /nologo /fl /flp:LogFile=F:\\TestProject\\BuildLog.txt", NULL, NULL, true, 0, NULL, "F:\\TestProject\\", &startInfo, &processInfo) != 0) {
			e->AddBuildLog("Compiling from " + ss);
			cout << "compiling" << endl;
			DWORD w;
			do {
				w = WaitForSingleObject(processInfo.hProcess, 0.5f);
				DWORD dwRead;
				CHAR chBuf[4096];
				bool bSuccess = FALSE;
				string out = "";
				bSuccess = ReadFile(stdOutR, chBuf, 4096, &dwRead, NULL);
				if (bSuccess && dwRead > 0) {
					string s(chBuf, dwRead);
					out += s;
				}
				for (int r = 0; r < out.size();) {
					int rr = out.find_first_of('\n', r);
					if (rr == string::npos)
						rr = out.size() - 1;
					string sss = out.substr(r, rr - r);
					e->AddBuildLog(sss);
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

			//DWORD ii;
			//GetExitCodeProcess(processInfo.hProcess, &ii);
			//e->AddBuildLog("Compile finished with code " + to_string(ii) + ".");
			return (!failed);
		}
		else {
			e->AddBuildLog("Cannot start msbuild!");
			CloseHandle(stdOutR);
			CloseHandle(stdOutW);
			return false;
		}
		CloseHandle(stdOutR);
		CloseHandle(stdOutW);
	}
	else {
		e->AddBuildLog("msbuild version 4.0 not found!");
		return false;
	}
}

void Editor::Compile(Editor* e) {
	e->WAITINGBUILDSTARTFLAG = true;
}

void Editor::ShowPrefs(Editor* e) {
	e->editorLayer = 5;
}

void Editor::SaveScene(Editor* e) {
	e->activeScene.Save(e);
}

void SetBuildFail(Editor* e) {
	e->buildLabel = "Build: failed.";
	e->buildProgressColor = red(1, 0.7f);
}

void Editor::DoCompile() {
	buildProgressColor = white(1, 0.35f);
	editorLayer = 4;
	buildLog.clear();
	buildLogErrors.clear();
	buildLabel = "Build: copying files...";
	buildProgressValue = 0;
	//copy files
	AddBuildLog("Copying: dummy source directory -> dummy target directory");
	AddBuildLog("Copying: dummy source directory2 -> dummy target directory2");
	AddBuildLog("Copying: dummy source directory3 -> dummy target directory3");
	this_thread::sleep_for(chrono::seconds(2));
	//compile
	buildProgressValue = 50;
	buildLabel = "Build: executing msbuild...";
	if (!DoMsBuild(this)) {
		SetBuildFail(this);
	}
	else {//if (IO::HasFile("F:\\TestProject\\Debug\\TestProject.exe")) {
		if (_cleanOnBuild) {
			buildProgressValue = 90;
			AddBuildLog("Cleaning up...");
			buildLabel = "Build: cleaning up...";
			//tr2::sys::remove_all("F:\\TestProject\\Release\\TestProject.tlog");
			for each (string s1 in IO::GetFiles("F:\\TestProject\\Release\\TestProject.tlog"))
			{
				AddBuildLog("deleting " + s1);
				remove(s1.c_str());
			}
			RemoveDirectory("F:\\TestProject\\Release\\TestProject.tlog\\");
			for each (string s2 in IO::GetFiles("F:\\TestProject\\Release"))
			{
				string ss = s2.substr(s2.size() - 4, string::npos);
				if (ss != ".dll" && s2 != "F:\\TestProject\\Release\\TestProject.exe") {
					AddBuildLog("deleting " + s2);
					remove(s2.c_str());
				}
			}
		}
		buildProgressValue = 100;
		buildLabel = "Build: complete.";
		buildProgressColor = green(1, 0.7f);
		AddBuildLog("Build finished: press Escape to exit.");
		ShellExecute(NULL, "open", "explorer", " /select,F:\\TestProject\\Release\\TestProject.exe", NULL, SW_SHOW);
	}
	//else SetBuildFail(this);
}

/*
EBI_Asset::EBI_Asset(string str, string nm) {
string noEd = nm.substr(0, nm.find_last_of('.'));
string ed = nm.substr(nm.find_last_of('.'), nm.size()-1);
if (ed == ".h") {
/*
#include "Engine.h"
class TestScript : public SceneScript {
public:
TestScript() {}
int foo = 0;
int boo= 1;
int koo =2;
int roo= 3;
Vec2 v2 = Vec2(1, 2);
void Start();
};
/
ifstream f;
f.open(str);
string s;
string cs = "class" + noEd + ":publicSceneScript{";
bool corrClass = false;
int brackets = 0;
while (!f.eof()){
getline(f, s);
if (!corrClass) {
s.erase(remove(s.begin(), s.end(), ' '), s.end());
s.erase(remove(s.begin(), s.end(), '\t'), s.end());
if (s.size() >= cs.size()) {
if (s.substr(0, cs.size()) == cs) {
corrClass = true;
correct = true;
brackets = 1;
continue;
}
}
}
else {
while (s.find_first_of(' ') == 0 || s.find_first_of('\t') == 0)
s = s.substr(1, s.size());
int fs = s.find_first_of(' ');
string ss = s.substr(0, fs);
size_t eq = s.find_first_of('=');
string nm = s.substr(fs + 1, min(eq, s.size()) - 1 - fs);
nm.erase(remove(nm.begin(), nm.end(), ' '), nm.end());
nm.erase(remove(nm.begin(), nm.end(), '\t'), nm.end());
if (ss == "int") {
if (eq != string::npos) {
vals.push_back(new EBI_Value<int>(nm, 1, 1, stoi(s.substr(eq + 1, s.size() - 1)), 0, 0, 0));
}
else {
vals.push_back(new EBI_Value<int>(nm, 1, 1, 0, 0, 0, 0));
}
}
if (ss == "Vec2") {
if (eq != string::npos) {
string v = s.substr(eq + 1, s.size() - 1);
v.erase(remove(v.begin(), v.end(), ' '), v.end());
int bb = v.find_first_of('(');
int bc = v.find_first_of(',');
int be = v.find_first_of(')');
vals.push_back(new EBI_Value<float>(nm, 2, 1, stof(v.substr(bb + 1, bc - bb - 1)), stof(v.substr(bc + 1, be - bc - 1)), 0, 0));
}
else {
vals.push_back(new EBI_Value<float>(nm, 2, 1, 0, 0, 0, 0));
}
}
s.erase(remove(s.begin(), s.end(), ' '), s.end());
s.erase(remove(s.begin(), s.end(), '\t'), s.end());
brackets += count(s.begin(), s.end(), '{');
brackets -= count(s.begin(), s.end(), '}');
if (brackets == 0) {
corrClass = false;
}
}
}
f.close();
}
}

void EBI_Asset::Draw(Editor* e, EditorBlock* b, Color* v) {
int allH = 0;
for each (I_EBI_Value* val in vals)
{
e->font->alignment = ALIGN_MIDLEFT;
Engine::Label(v->r + 3, v->g + EB_HEADER_SIZE + 1 + 26 * (allH + 0.5f), 22, val->name, e->font, white());
e->font->alignment = ALIGN_TOPLEFT;
switch (val->type) {
case 1:
Engine::EButton((e->editorLayer == 0), v->r + v->b*0.3f + 1, v->g + EB_HEADER_SIZE + 1 + 26 * allH, v->b*0.7f - 2, 25, white(1, 0.7f), to_string(((EBI_Value<int>*)val)->a), 22, e->font, black());
break;
case 2:
Engine::EButton((e->editorLayer == 0), v->r + v->b*0.3f + 1, v->g + EB_HEADER_SIZE + 1 + 26 * allH, v->b*0.35f - 1, 25, white(1, 0.7f), to_string(((EBI_Value<float>*)val)->a), 22, e->font, black());
Engine::EButton((e->editorLayer == 0), v->r + v->b*0.65f + 1, v->g + EB_HEADER_SIZE + 1 + 26 * allH, v->b*0.35f - 2, 25, white(1, 0.7f), to_string(((EBI_Value<float>*)val)->b), 22, e->font, black());
break;
}

allH += val->sizeH;
}
}
*/