#include "Editor.h"
#include "Engine.h"
#include <GL/glew.h>
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
#include <GL/glew.h>

HWND Editor::hwnd = 0;

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
	Engine::Label(round(v->r + 8 + EB_HEADER_PADDING), round(v->g + 12), 21, titleB, e->font, black());
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
	if (indent > 0) {
		Engine::DrawLine(Vec2(xo - 10 + v->r, v->g + EB_HEADER_SIZE + 9 + 17 * i), Vec2(xo + v->r, v->g + EB_HEADER_SIZE + 10 + 17 * i), white(0.5f), 1);
	}
	if (Engine::Button(v->r + xo, v->g + EB_HEADER_SIZE + 1 + 17 * i, v->b - xo, 16, (e->selected == sc)? white(1, 0.5f) : grey2()) == MOUSE_RELEASE) {
		e->selected = sc;
	}
	Engine::Label(v->r + 19 + xo, v->g + EB_HEADER_SIZE + 4 + 17 * i, 12, sc->name, e->font, white());
	i++;
	if (sc->childCount > 0) {
		if (sc->_expanded) {
			Engine::DrawTexture(v->r + xo, v->g + EB_HEADER_SIZE + 1 + 17 * (i - 1), 16, 16, e->collapse);
			Engine::DrawLine(Vec2(xo + 10 + v->r, v->g + EB_HEADER_SIZE + 18 + 17 * (i - 1)), Vec2(xo + 10 + v->r, v->g + EB_HEADER_SIZE + 10 + 17 * (i - 1) + sc->childCount * 17), white(0.5f), 1);
			for each (SceneObject* scc in sc->children)
			{
				EBH_DrawItem(scc, e, v, i, indent + 1);
			}
		}
		else
			Engine::DrawTexture(v->r + xo, v->g + EB_HEADER_SIZE + 1 + 17 * (i - 1), 16, 16, e->expand);
	}
}

void EB_Hierarchy::Draw() {
	Color v = Color(Display::width*editor->xPoss[x1], Display::height*editor->yPoss[y1], Display::width*editor->xPoss[x2], Display::height*editor->yPoss[y2]);
	v.a = round(v.a - v.g) - 1;
	v.b = round(v.b - v.r) - 1;
	v.g = round(v.g) + 1;
	v.r = round(v.r) + 1;
	DrawHeaders(editor, this, &v, "Scene objects", "Hierarchy");
	int i = 0;
	for each (SceneObject* sc in editor->activeScene.objects)
	{
		EBH_DrawItem(sc, editor, &v, i, 0);
	}
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
	}
}

EB_Browser_File::EB_Browser_File(string path, string name) : path(path), name(name), hasTex(false) {
	/*
	string s = path + "\\" + name;
	std::wstring stemp = std::wstring(s.begin(), s.end());
	PCWSTR pp = stemp.c_str();

	HRESULT hr = CoInitialize(nullptr);

	// Get the thumbnail
	IShellItem* item = nullptr;
	hr = SHCreateItemFromParsingName(pp, nullptr, IID_PPV_ARGS(&item));

	if (hr != S_OK) {
		cout << "thumbnail fail 0: " << hr << endl;
		return;
	}

	IThumbnailCache* cache = nullptr;
	hr = CoCreateInstance(
		CLSID_LocalThumbnailCache,
		nullptr,
		CLSCTX_INPROC,
		IID_PPV_ARGS(&cache));

	if (hr != S_OK) {
		cout << "thumbnail fail 1: " << hr << endl;
		return;
	}

	ISharedBitmap* shared_bitmap;
	hr = cache->GetThumbnail(
		item,
		1024,
		WTS_EXTRACT,
		&shared_bitmap,
		nullptr,
		nullptr);

	if (hr != S_OK) {
		cout << "thumbnail fail 2: " << hr << endl;
		return;
	}

	// Retrieve thumbnail HBITMAP
	HBITMAP hbitmap = NULL;
	hr = shared_bitmap->GetSharedBitmap(&hbitmap);

	HDC dc = GetDC(NULL);
	HDC dc_mem = CreateCompatibleDC(dc);

	// Get required buffer size
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	cout << "reading image from hbitmap" << endl;

	BYTE* pixels = new BYTE[bmi.bmiHeader.biSizeImage];
	bmi.bmiHeader.biCompression = BI_RGB;

	unsigned char data = 0;
	if (GetDIBits(dc_mem, hbitmap, 0, bmi.bmiHeader.biHeight, (LPVOID)pixels, &bmi, DIB_RGB_COLORS) == 0) {
		cout << "image load from hbitmap failed" << endl;
		return;
	}
	thumbnail = new Texture();
	glGenTextures(1, &thumbnail->pointer);
	glBindTexture(GL_TEXTURE_2D, thumbnail->pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmi.bmiHeader.biWidth, abs(bmi.bmiHeader.biHeight), 0, GL_BGR, GL_UNSIGNED_BYTE, pixels);
	//glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	thumbnail->loaded = true;
	cout << "image loaded from hbitmap: " << bmi.bmiHeader.biWidth << "x" << abs(bmi.bmiHeader.biHeight) << endl;
	hasTex = true;
	*/
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
	bool b = ((file->hasTex) ? Engine::Button(w + 1, h + 1, size - 2, size - 2, file->thumbnail, white(1, 0.8f), white(), white(1, 0.5f)) : Engine::Button(w + 1, h + 1, size - 2, size - 2, grey2())) == MOUSE_RELEASE;
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

	for (int y = dirs.size() - 1; y >= 0; y--) {
		if (Engine::Button(v.r, v.g + EB_HEADER_SIZE + 1 + (16 * y), 150, 15, grey1()) == MOUSE_RELEASE) {
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
	for each (EB_Browser_File f in files) {
		if (DrawFileRect(v.r + 151 + ww, v.g + EB_HEADER_SIZE + 101 * hh, 100, &f, this)) {
			editor->selectedFile = f.path + "\\" + f.name;
		}
		ww += 101;
		if (ww > (v.b - 252)) {
			ww = 0;
			hh++;
		}
	}
}

void EB_Browser::Refresh() {
	dirs.clear();
	files.clear();
	IO::GetFolders(currentDir, &dirs);
	cout << dirs.size() << "folders from " << currentDir << endl;
	files = IO::GetFiles(currentDir);
	cout << files.size() << "files from " << currentDir << endl;
}

EB_Viewer::EB_Viewer(Editor* e, int x1, int y1, int x2, int y2): rz(45), rw(45), scale(1) {
	editorType = 2;
	editor = e;
	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	MakeMatrix();

	shortcuts.emplace(GetShortcutInt('a', 0), &_SelectAll);
	shortcuts.emplace(GetShortcutInt('z', 0), &_ViewInvis);
	shortcuts.emplace(GetShortcutInt('5', 0), &_ViewPersp);
	shortcuts.emplace(GetShortcutInt('t', 0), &_TooltipT);
	shortcuts.emplace(GetShortcutInt('r', 0), &_TooltipR);
	shortcuts.emplace(GetShortcutInt('s', 0), &_TooltipS);
}

void EB_Viewer::MakeMatrix() {
	float csz = cos(rz*3.14159265f / 180.0f);
	float snz = sin(rz*3.14159265f / 180.0f);
	float csw = cos(rw*3.14159265f / 180.0f);
	float snw = sin(rw*3.14159265f / 180.0f);
	viewingMatrix = glm::mat4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
	viewingMatrix = glm::mat4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1)*viewingMatrix;
	arrowX = viewingMatrix*Color(1, 0, 0, 0);
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

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glStencilFunc(GL_NEVER, 1, 0xFF);
	glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP); // draw 1s on test fail (always)
	glStencilMask(0xFF); // draw stencil pattern
	glClear(GL_STENCIL_BUFFER_BIT); // needs mask=0xFF
	Engine::DrawQuad(v.r, v.g + EB_HEADER_SIZE + 1, v.b, v.a - EB_HEADER_SIZE - 2, white());
	//Engine::DrawQuad(v.r, v.g, v.b, v.a, white());
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilMask(0x0);
	glStencilFunc(GL_EQUAL, 0, 0xFF);

	glStencilFunc(GL_EQUAL, 1, 0xFF);
	Vec2 v2 = Vec2(Display::width, Display::height)*0.03f;
	Engine::DrawQuad(0, 0, Display::width, Display::height, white(1, 0.2f));//editor->checkers->pointer, Vec2(), Vec2(v2.x, 0), Vec2(0, v2.y), v2, true, white(0.05f));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float ww1 = editor->xPoss[x1];
	float hh1 = editor->yPoss[y1];
	float ww = editor->xPoss[x2] - ww1;
	float hh = editor->yPoss[y2] - hh1;
	//if (!persp) {
		glMultMatrixf(glm::value_ptr(glm::ortho(-20 * ww - 40 * ww1, 40.0f - 20 * ww - 40 * ww1, -40.0f + 20 * hh + 40 * hh1, 20 * hh + 40 * hh1, 0.0f, 1000.0f)));
		glScalef(-1, 1, 1);
	//}
	//else {
		//glMultMatrixf(glm::value_ptr(glm::perspective(30.0f, 1.0f, 0.01f, 1000.0f)));
		//glScalef(1, -1, 1);
	//}
	glTranslatef(0, 0, -30);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	float mww = max(ww, 0.3f)*scale;
	glScalef(mww, mww, mww);
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
	//glPopMatrix();
	glPopMatrix();

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);

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
	if (Engine::Button(v.x + 5, v.y + EB_HEADER_SIZE + 10, 20, 20, editor->tooltipTexs[editor->selectedTooltip], white(0.7f), white(), white(1, 0.5f)) && MOUSE_HOVER_FLAG) {
		drawDescLT = 1;
		switch (editor->selectedTooltip) {
		case 0:
			descLabelLT = "Selected Axes: Transform(T)";
			break;
		case 1:
			descLabelLT = "Selected Axes: Rotate(R)";
			break;
		case 2:
			descLabelLT = "Selected Axes: Scale(S)";
			break;
		}
	}
	if (Engine::Button(v.x + 5, v.y + EB_HEADER_SIZE + 32, 20, 20, editor->shadingTexs[editor->selectedShading], white(0.7f), white(), white(1, 0.5f)) && MOUSE_HOVER_FLAG) {
		drawDescLT = 2;
		switch (editor->selectedShading) {
		case 0:
			descLabelLT = "Shading: Solid(Z)";
			break;
		case 1:
			descLabelLT = "Shading: Wire(Z)";
			break;
		}
	}

	if (drawDescLT > 0) {
		Engine::DrawQuad(v.x + 28, v.y + EB_HEADER_SIZE + 10 + 22 * (drawDescLT - 1), 200, 20, white(1, 0.6f));
		Engine::Label(v.x + 30, v.y + EB_HEADER_SIZE + 15 + 22 * (drawDescLT - 1), 12, descLabelLT, editor->font, black());
	}
}

void EB_Viewer::OnMouseM(Vec2 d) {
	if (editor->mousePressType == 1) {
		rz += d.x;
		rw += d.y;
		MakeMatrix();
	}
}

void EB_Viewer::OnMouseScr(bool up) {
	scale += (up?0.1f:-0.1f);
	scale = min(max(scale, 0.01f), 1000);
}

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
		*/
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
			Engine::Button(v->r + v->b*0.3f + 1, v->g + EB_HEADER_SIZE + 1 + 26 * allH, v->b*0.7f - 2, 25, white(1, 0.7f), to_string(((EBI_Value<int>*)val)->a), 22, e->font, black());
			break;
		case 2:
			Engine::Button(v->r + v->b*0.3f + 1, v->g + EB_HEADER_SIZE + 1 + 26 * allH, v->b*0.35f - 1, 25, white(1, 0.7f), to_string(((EBI_Value<float>*)val)->a), 22, e->font, black());
			Engine::Button(v->r + v->b*0.65f + 1, v->g + EB_HEADER_SIZE + 1 + 26 * allH, v->b*0.35f - 2, 25, white(1, 0.7f), to_string(((EBI_Value<float>*)val)->b), 22, e->font, black());
			break;
		}

		allH += val->sizeH;
	}
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
		Engine::Button(v.r + 20, v.g + 2 + EB_HEADER_SIZE, v.b - 21, 18, grey2());
		Engine::Label(v.r + 22, v.g + 6 + EB_HEADER_SIZE, 12, editor->selected->name, editor->font, white());
		//TRS
		Engine::Label(v.r, v.g + 23 + EB_HEADER_SIZE, 12, "Position", editor->font, white());
		Engine::Button(v.r + v.b*0.19f, v.g + 21 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.4f, 0.2f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.19f + 2, v.g + 23 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.position.x), editor->font, white());
		Engine::Button(v.r + v.b*0.46f, v.g + 21 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.4f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.46f + 2, v.g + 23 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.position.y), editor->font, white());
		Engine::Button(v.r + v.b*0.73f, v.g + 21 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.2f, 0.4f, 1));
		Engine::Label(v.r + v.b*0.73f + 2, v.g + 23 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.position.z), editor->font, white());

		Engine::Label(v.r, v.g + 40 + EB_HEADER_SIZE, 12, "Rotation", editor->font, white());
		Engine::Button(v.r + v.b*0.19f, v.g + 38 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.4f, 0.2f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.19f + 2, v.g + 40 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.eulerRotation.x), editor->font, white());
		Engine::Button(v.r + v.b*0.46f, v.g + 38 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.4f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.46f + 2, v.g + 40 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.eulerRotation.y), editor->font, white());
		Engine::Button(v.r + v.b*0.73f, v.g + 38 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.2f, 0.4f, 1));
		Engine::Label(v.r + v.b*0.73f + 2, v.g + 40 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.eulerRotation.z), editor->font, white());

		Engine::Label(v.r, v.g + 57 + EB_HEADER_SIZE, 12, "Scale", editor->font, white());
		Engine::Button(v.r + v.b*0.19f, v.g + 55 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.4f, 0.2f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.19f + 2, v.g + 57 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.scale.x), editor->font, white());
		Engine::Button(v.r + v.b*0.46f, v.g + 55 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.4f, 0.2f, 1));
		Engine::Label(v.r + v.b*0.46f + 2, v.g + 57 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.scale.y), editor->font, white());
		Engine::Button(v.r + v.b*0.73f, v.g + 55 + EB_HEADER_SIZE, v.b*0.27f - 1, 16, Color(0.2f, 0.2f, 0.4f, 1));
		Engine::Label(v.r + v.b*0.73f + 2, v.g + 57 + EB_HEADER_SIZE, 12, to_string(editor->selected->transform.scale.z), editor->font, white());
	}
	else
		Engine::Label(v.r + 2, v.g + 2 + EB_HEADER_SIZE, 12, "Select object to inspect.", editor->font, white());
}

void Editor::LoadDefaultAssets() {
	buttonX = new Texture("F:\\xbutton.bmp", false);
	buttonExt = new Texture("F:\\extbutton.bmp", false);
	buttonExtArrow = new Texture("F:\\extbutton arrow.bmp", false);
	background = new Texture("F:\\lines.bmp", false);

	placeholder = new Texture("F:\\placeholder.bmp");
	checkers = new Texture("F:\\checkers.bmp", false, true);
	expand = new Texture("F:\\expand.bmp", false);
	collapse = new Texture("F:\\collapse.bmp", false);
	object = new Texture("F:\\object.bmp", false);

	shadingTexs.push_back(new Texture("F:\\shading_solid.bmp"));
	shadingTexs.push_back(new Texture("F:\\shading_trans.bmp"));

	tooltipTexs.push_back(new Texture("F:\\tooltip_tr.bmp"));

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
		if (Engine::Button(v.r + 1, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonExt, white(1, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) { //splitter top left
			if (b->headerStatus == 1)
				b->headerStatus = 0;
			else b->headerStatus = (b->headerStatus == 0 ? 1 : 0);
		}
		if (b->headerStatus == 1) {
			//Engine::RotateUI(180, Vec2(v.r + 2 + 1.5f*EB_HEADER_PADDING, v.g + 1 + 0.5f*EB_HEADER_PADDING));
			if (Engine::Button(v.r + 2 + EB_HEADER_PADDING, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonExtArrow, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {
				b->headerStatus = 0;
				xPoss.push_back(xPoss.at(b->x1));
				xLerper.push_back(xPossLerper(this, xPoss.size() - 1, xPoss.at(b->x1), 0.5f*(xPoss.at(b->x1) + xPoss.at(b->x2))));
				//xPoss.at(xPoss.size()-1) = 0.5f*(xPoss.at(b->x1) + xPoss.at(b->x2));
				xLimits.push_back(xLimits.at(xLimits.size()-1));
				blocks.push_back(new EB_Empty(this, b->x1, b->y1, xPoss.size() - 1, b->y2));

				b->x1 = xPoss.size() - 1;
				break;
			}
			//Engine::ResetUIMatrix();
			Engine::RotateUI(-90, Vec2(v.r + 1 + 0.5f*EB_HEADER_PADDING, v.g + 2 + 1.5f*EB_HEADER_PADDING));
			if (Engine::Button(v.r + 1, v.g + 2 + EB_HEADER_PADDING, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonExtArrow, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {
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
		if (Engine::Button(v.b - EB_HEADER_PADDING, v.g + 1, EB_HEADER_PADDING, EB_HEADER_PADDING, buttonX, white(0.7f, 0.8f), white(), white(1, 0.3f)) == MOUSE_RELEASE) {//window mod
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
			b = Engine::Button(xPoss.at(q)*Display::width - 2, Display::height*yPoss.at(lim.x), 4, Display::height*yPoss.at(lim.y), black(0), white(0.3f, 1), white(0.7f, 1));
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
			b = Engine::Button(xPoss.at(lim.x)*Display::width, yPoss.at(q)*Display::height - 2, xPoss.at(lim.y)*Display::width, 4, black(0), white(0.3f, 1), white(0.7f, 1));
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
}