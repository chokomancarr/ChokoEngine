#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <Windows.h>
#include <Psapi.h>
#include <typeinfo>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gl/GLUT.h>
#include <thread>
#include <mutex>
#include "Engine.h"
#include "Editor.h"
#include "SceneObjects.h"
#include "Compressors.h"
#include <sstream>
#include <type_traits>
//#include <signal.h>

using namespace ChokoEngine;

void TimerGL(int i);
void MouseGL(int button, int state, int x, int y);
void MotionGL(int x, int y);
void MotionGLP(int x, int y);
void ReshapeGL(int w, int h);

void UpdateLoop();
void OnDie();

void renderScene();
void closeCallback();

Font *font;

std::thread updateThread;
uint fps;
bool redrawn = false;
bool die = false;

string path;
Editor* editor;
std::mutex lockMutex;

HWND splashHwnd;
void ShowSplash(string bitmap, uint cx, uint cy, uint sw, uint sh);
void KillSplash();

//extern "C" void abort_func(int signum)
//{
//	MessageBox(hwnd, "aaa", "title", MB_OK);
//}

char Get(std::istream& strm) {
	char c;
	strm.read(&c, 1);
	return c;
}

int main(int argc, char **argv)
{
	path = argv[0];
	//signal(SIGABRT, &abort_func);

	UI::EditText(0, 0, 0, 0, 0, "", 0, Vec4());

	editor = new Editor();
	editor->hwnd = GetForegroundWindow();
	editor->dataPath = path.substr(0, path.find_last_of('\\') + 1);
	editor->lockMutex = &lockMutex;

	HMONITOR monitor = MonitorFromWindow(editor->hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	
	ShowSplash(editor->dataPath + "res\\splash.png", (info.rcMonitor.right - info.rcMonitor.left) / 2, (info.rcMonitor.bottom - info.rcMonitor.top) / 2, 600, 300);
	SetForegroundWindow(editor->hwnd);

	editor->scrW = info.rcMonitor.right - info.rcMonitor.left;
	editor->scrH = info.rcMonitor.bottom - info.rcMonitor.top;
	editor->scrW = 1024;
	editor->scrH = 600;


	//editor->ParseAsset("D:\\test.blend");
	//editor->Compile();

	//*

	std::cout << "Enter project folder path" << std::endl;
	std::getline(std::cin, editor->projectFolder);
	editor->projectFolder = "D:\\TestProject2\\";

	//ShowWindow(editor->hwnd, SW_HIDE);

	DefaultResources::Init(editor->dataPath + "res\\defaultresources.bin");
	editor->xPoss.push_back(0);
	editor->xPoss.push_back(1);
	editor->xPoss.push_back(0.75f);
	editor->xPoss.push_back(0.63f);
	editor->xPoss.push_back(0.4f);
	editor->xLimits.push_back(Int2(0, 1));
	editor->xLimits.push_back(Int2(0, 1));
	editor->xLimits.push_back(Int2(0, 1));
	editor->xLimits.push_back(Int2(0, 2));
	editor->xLimits.push_back(Int2(2, 1));

	editor->yPoss.push_back(0);
	editor->yPoss.push_back(1);
	editor->yPoss.push_back(0.65f);
	editor->yLimits.push_back(Int2(0, 1));
	editor->yLimits.push_back(Int2(0, 1));
	editor->yLimits.push_back(Int2(0, 2));
	editor->yLimits.push_back(Int2(2, 1));

	//EBI_Asset* e = new EBI_Asset("D:\\OpenGl1\\OpenGl1_Editor\\OpenGl1_Editor\\src\\TestScript.h", "TestScript.h");
	//((EB_Inspector*)(editor->blocks[0]))->SelectAsset(e, "D:\\OpenGl1\\OpenGl1_Editor\\OpenGl1_Editor\\src\\TestScript.h");

	//TestScript* ts = new TestScript();

	/*
	string ip = editor->dataPath + "1.ico";
	LPARAM hIcon = (LPARAM)LoadImage(NULL, ip.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, hIcon);
	SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, hIcon);
	SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_BIG, hIcon);

	long style = GetWindowLong(hwnd, GWL_STYLE);
	style &= ~(WS_VISIBLE);    // this works - window become invisible

	style |= WS_EX_TOOLWINDOW;
	style &= ~(WS_EX_APPWINDOW);

	ShowWindow(hwnd, SW_HIDE); // hide the window
	SetWindowLong(hwnd, GWL_STYLE, style); // set the style
	ShowWindow(hwnd, SW_SHOW); // show the window for the new style to come into effect
	//SetWindowLong(hwnd, GWL_STYLE, style);
	ShowWindow(hwnd, SW_HIDE); // hide the window so we can't see it
	*/
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(editor->scrW, editor->scrH);
	glutInitWindowPosition(0, GetSystemMetrics(SM_CYVIRTUALSCREEN));
	glutCreateWindow("ChokoEngine");
	editor->hwnd2 = GetActiveWindow();
	glutHideWindow();
	//ShowWindow(editor->hwnd2, SW_MAXIMIZE);

	//SendMessage(hwnd2, WM_SETICON, ICON_SMALL, hIcon);
	//SendMessage(hwnd2, WM_SETICON, ICON_BIG, hIcon);
	//SendMessage(GetWindow(hwnd2, GW_OWNER), WM_SETICON, ICON_SMALL, hIcon);
	//SendMessage(GetWindow(hwnd2, GW_OWNER), WM_SETICON, ICON_BIG, hIcon);

	//editor->RegisterPopup(new PB_ColorPicker(editor, &_col), Vec2(200, 200));

	string p;
	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << ("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		std::getline(std::cin, p);
		return 0;
	}

	Engine::Init(path);
	editor->LoadDefaultAssets();
	editor->ReloadAssets(editor->projectFolder + "Assets\\", true);
	editor->ReadPrefs();
	editor->blocks = std::vector<EditorBlock*>({ new EB_Inspector(editor, 2, 0, 1, 1), new EB_Browser(editor, 0, 2, 4, 1, editor->projectFolder + "Assets\\"), new EB_AnimEditor(editor, 0, 2, 4, 1), new EB_Debug(editor, 4, 2, 2, 1), new EB_Viewer(editor, 0, 0, 3, 2), new EB_Hierarchy(editor, 3, 0, 2, 2), new EB_Previewer(editor, 4, 2, 2, 1), new EB_Console(editor, 0, 2, 4, 1) }); //path.substr(0, path.find_last_of('\\') + 1)
	editor->blockCombos.push_back(new BlockCombo());
	editor->blockCombos[0]->blocks.push_back(editor->blocks[1]);
	editor->blockCombos[0]->blocks.push_back(editor->blocks[2]);
	editor->blockCombos[0]->blocks.push_back(editor->blocks[7]);
	editor->blockCombos[0]->Set();
	editor->blockCombos.push_back(new BlockCombo());
	editor->blockCombos[1]->blocks.push_back(editor->blocks[6]);
	editor->blockCombos[1]->blocks.push_back(editor->blocks[3]);
	editor->blockCombos[1]->Set();
	editor->SetBackground(editor->dataPath + "res\\bg.jpg", 0.3f);
	font = editor->font;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFrontFace(GL_CW);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glutDisplayFunc(renderScene);
	glutTimerFunc(1000, TimerGL, 0);
	glutMouseFunc(MouseGL);
	glutReshapeFunc(ReshapeGL);
	glutMotionFunc(MotionGL);
	glutPassiveMotionFunc(MotionGLP);

	Time::startMillis = milliseconds();

	updateThread = std::thread(UpdateLoop);
	atexit(OnDie);

	KillSplash();
	glutPositionWindow(info.rcMonitor.left, info.rcMonitor.top);
	glutReshapeWindow(editor->scrW, editor->scrH);
	glutShowWindow();
	//SetWindowText(editor->hwnd2, TEXT("ChokoEngine"));
	//AnimateWindow(editor->hwnd2, 300, AW_ACTIVATE | AW_BLEND);
	glutMainLoop();
	string str;
	std::getline(std::cin, str);
	return 0;
}

void closeCallback()
{

	std::cout << "GLUT:\t Finished" << std::endl;
}

void CheckShortcuts() {
	if (editor->editorLayer == 0) {
		bool cDn = Input::KeyHold(Key_Control);
		bool aDn = Input::KeyHold(Key_Alt);
		bool sDn = Input::KeyHold(Key_Shift);
		for (auto& g : editor->globalShorts) {
			if (ShortcutTriggered(g.first, cDn, aDn, sDn)) {
				g.second(editor);
				return;
			}
			if ((g.first & 0xff00) == 0) {
				std::cout << std::hex << g.first;
			}
		}
		for (EditorBlock* e : editor->blocks) {
			if (e->hidden)
				continue;
			Vec4 v = Vec4(Display::width*editor->xPoss[e->x1], Display::height*editor->yPoss[e->y1], Display::width*editor->xPoss[e->x2], Display::height*editor->yPoss[e->y2]);
			v.a = round(v.a - v.g) - 1;
			v.b = round(v.b - v.r) - 1;
			v.g = round(v.g) + 1;
			v.r = round(v.r) + 1;
			if (Engine::Button(v.r, v.g, v.b, v.a) && MOUSE_HOVER_FLAG) {
				for (auto& g2 : e->shortcuts) {
					if (ShortcutTriggered(g2.first, cDn, aDn, sDn)) {
						g2.second(e);
						//return;
					}
				}
				break;
			}
		}
	}
	else if (Input::KeyDown(Key_Escape)) {
		if (editor->editorLayer != 4 || editor->buildEnd) {
			editor->editorLayer = 0;
		}
	}
	else if (Input::KeyDown(Key_Enter) && editor->editorLayer == 4 && editor->buildEnd && editor->buildErrorPath != "") { //enter
		string cmd = " " + editor->buildErrorPath + " -n" + to_string(editor->buildErrorLine);
		ShellExecute(NULL, "open", "C:\\Program Files (x86)\\Notepad++\\Notepad++.exe", cmd.c_str(), NULL, SW_SHOW);
	}
}

void DoUpdate() {
	if (!!(editor->flags & WAITINGBUILDSTARTFLAG)) {
		editor->flags &= ~WAITINGBUILDSTARTFLAG;
		editor->DoCompile();
		return;
	}
	std::lock_guard<std::mutex> lock(lockMutex);
	editor->flags &= ~WAITINGREFRESHFLAG;
	CheckShortcuts();
	int i = -1, k = 0;
	editor->mouseOn = 0;
	for (EditorBlock* e : editor->blocks) {
		if (!e->hidden) {
			Vec4 v = Vec4(Display::width*editor->xPoss[e->x1], Display::height*editor->yPoss[e->y1], Display::width*editor->xPoss[e->x2], Display::height*editor->yPoss[e->y2]);
			v.a = round(v.a - v.g) - 1;
			v.b = round(v.b - v.r) - 1;
			v.g = round(v.g) + 1;
			v.r = round(v.r) + 1;
			if (Engine::Button(v.r, v.g, v.b, v.a) && MOUSE_HOVER_FLAG) {
				i = k;
				editor->mouseOn = i;
				break;
			}
			if (!!(editor->flags & WAITINGREFRESHFLAG)) //deleted
				return;
		}
		k++;
	}
	if (i != -1) {
		if (editor->mousePressType == -1) {
			editor->mouseOnP = i;
			if (Input::mouse0State == MOUSE_DOWN) {
				editor->mousePressType = 0;
				editor->blocks[editor->mouseOnP]->OnMousePress(0);
				//editor->mouseOn = i;
			}
			if (Input::mouse1State == MOUSE_DOWN) {
				editor->mousePressType = 1;
				editor->blocks[editor->mouseOnP]->OnMousePress(1);
				//editor->mouseOn = i;
			}
			if (Input::mouse2State == MOUSE_DOWN) {
				editor->mousePressType = 2;
				editor->blocks[editor->mouseOnP]->OnMousePress(2);
				//editor->mouseOn = i;
			}
		}
		else {
			if ((editor->mousePressType == 0 && !Input::mouse0) || (editor->mousePressType == 1 && !Input::mouse1) || (editor->mousePressType == 2 && !Input::mouse2)) {
				editor->mousePressType = -1;
				//editor->mouseOn = 0;
			}
		}
	}
}

void UpdateLoop() {
	while (!die) {
		if (redrawn)
		{
			//lock_guard<mutex> lock(lockMutex);
			redrawn = false;
			long long millis = milliseconds();
			Time::delta = (millis - Time::millis)*0.001f;
			Time::time = (millis - Time::startMillis)*0.001;
			Time::millis = millis;
			Input::UpdateMouseNKeyboard();
			DoUpdate();
		}
	}
}

void OnDie() {
	die = true;
	updateThread.join();
	editor->playSyncer.Terminate();
}

bool hi;
void DrawOverlay() {
	std::lock_guard<std::mutex> lock(lockMutex);
	editor->UpdateLerpers();
	editor->playSyncer.Update();
	if (editor->backgroundTex != nullptr)
		UI::Texture(0, 0, (float)Display::width, (float)Display::height, editor->backgroundTex, editor->backgroundAlpha*0.01f, DrawTex_Crop);
	for (int i = editor->blocks.size() - 1; i >= 0; i--) {
		if (!editor->blocks[i]->hidden && !(editor->hasMaximize && !editor->blocks[i]->maximize)) {
			editor->blocks[i]->Draw();
		}
	}
	if (editor->dialogBlock) {
		editor->dialogBlock->Draw();
	}
	if (!!(editor->flags & WAITINGPLAYFLAG)) {
		if (editor->playSyncer.status == Editor_PlaySyncer::EPS_Offline) editor->playSyncer.Connect();
		else if (editor->playSyncer.status == Editor_PlaySyncer::EPS_Running) editor->playSyncer.Disconnect();

		editor->flags &= ~WAITINGPLAYFLAG;
	}
	editor->DrawHandles();
}

void renderScene()
{
	if (!redrawn || editor->editorLayer == 6) {
		fps++;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0, 0, 0, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_BLEND);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		DrawOverlay();

		//Engine::DrawQuad(0, 0, 512, 512, rt->pointer);
		//Engine::DrawQuad(513, 0, 512, 512, rt->pointer);
		//Engine::DrawQuad(1026, 0, 512, 512, rt2->pointer);

		glutSwapBuffers();
		redrawn = true;
	}
	glutPostRedisplay();
}

void TimerGL(int i)
{
	glutTimerFunc(1000, TimerGL, 1);
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	SIZE_T virtualMemUsedByMe = pmc.WorkingSetSize;
	string str("ChokoEngine (about " + to_string((uint)round(virtualMemUsedByMe/1024/1024)) + "Mb used, " + to_string(fps) + "fps)");
	SetWindowText(editor->hwnd2, str.c_str());
	fps = 0;
	//glutPostRedisplay();
}

void MouseGL(int button, int state, int x, int y) {
	switch (button) {
	case 0:
		Input::mouse0 = (state == 0);
		break;
	case 1:
		Input::mouse1 = (state == 0);
		break;
	case 2:
		Input::mouse2 = (state == 0);
		break;
	case 3:
		if (state == GLUT_DOWN && editor->editorLayer == 0) editor->blocks[editor->mouseOnP]->OnMouseScr(true);
		return;
	case 4:
		if (state == GLUT_DOWN && editor->editorLayer == 0) editor->blocks[editor->mouseOnP]->OnMouseScr(false);
		return;
	}
}

void ReshapeGL(int w, int h) {
	glViewport(0, 0, w, h);
	Display::width = w;
	Display::height = h;
	glutPostRedisplay();
}

//void MouseGL(int button, int state, int x, int y);
void MotionGLP(int x, int y) {
	if (editor->editorLayer == 0)
		editor->blocks[editor->mouseOn]->OnMouseM(Vec2(x, y) - Input::mousePos);
	Input::mousePos = Vec2(x, y);
	Input::mousePosRelative = Vec2(x*1.0f / Display::width, y*1.0f / Display::height);
}

void MotionGL(int x, int y) {
	if (editor->editorLayer == 0)
		editor->blocks[editor->mouseOn]->OnMouseM(Vec2(x, y) - Input::mousePos);
	Input::mousePos = Vec2(x, y);
	Input::mousePosRelative = Vec2(x*1.0f / Display::width, y*1.0f / Display::height);
}

void ShowSplash(string bitmap, uint cx, uint cy, uint sw, uint sh) {
	auto inst = GetModuleHandle(NULL);
	//register window
	WNDCLASS wc = {};
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = inst;
	//wc.hIcon = LoadIcon(inst, MAKEINTRESOURCE(IDI_SPLASHICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = TEXT("ChokoEngineSplash");
	RegisterClass(&wc);

	splashHwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, TEXT("ChokoEngineSplash"), NULL, WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, inst, NULL);
	//cx - sw / 2, cy - sh / 2, sw, sh
	
	HDC dcScr = GetDC(NULL);
	HDC dcMem = CreateCompatibleDC(dcScr);

	byte chn;
	uint pw, ph;
	byte* px = Texture::LoadPixels(bitmap, chn, pw, ph);
	byte tmp = 0;
	for (uint a = 0; a < pw*ph; a++) {
		px[a * 4] *= px[a * 4 + 3] * 1.0f/255;
		px[a * 4 + 1] *= px[a * 4 + 3] * 1.0f / 255;
		px[a * 4 + 2] *= px[a * 4 + 3] * 1.0f / 255;
		tmp = px[a * 4];
		px[a * 4] = px[a * 4 + 2];
		px[a * 4 + 2] = tmp;
	}
	
	BITMAPINFO binfo = {};
	binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	binfo.bmiHeader.biWidth = pw;
	binfo.bmiHeader.biHeight = (LONG)ph;
	binfo.bmiHeader.biPlanes = 1;
	binfo.bmiHeader.biBitCount = 32;
	binfo.bmiHeader.biCompression = BI_RGB;

	byte* pvb;
	HBITMAP hbmp = CreateDIBSection(dcScr, &binfo, DIB_RGB_COLORS, (void**)&pvb, NULL, 0);
	memcpy(pvb, px, chn*pw*ph);

	HBITMAP bmpo = (HBITMAP)SelectObject(dcMem, hbmp);

	POINT pt, pt0 = {};
	pt.x = cx - pw/2;
	pt.y = cy - ph/2;
	SIZE sz = { pw, ph };
	BLENDFUNCTION func = {};
	func.BlendOp = AC_SRC_OVER;
	func.SourceConstantAlpha = 255;
	func.AlphaFormat = AC_SRC_ALPHA;
	UpdateLayeredWindow(splashHwnd, dcScr, &pt, &sz, dcMem, &pt0, RGB(0, 0, 0), &func, ULW_ALPHA);
	
	SelectObject(dcMem, bmpo);
	DeleteDC(dcMem);
	ReleaseDC(NULL, dcScr);
}

void KillSplash() {
	DestroyWindow(splashHwnd);
}