#define _USE_MATH_DEFINES
#define IS_EDITOR
#include <math.h>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <Windows.h>
#include <Psapi.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Engine.h"
#include "editor.h"
#include "SceneObjects.h"
#include <gl/GLUT.h>
#include <thread>
#include <mutex>
using namespace std;

void InitGL(int argc, char* argv[]);
void DisplayGL();
void InitGL(int i);
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

thread updateThread;
uint t;
uint fps;
bool redrawn = false;
bool die = false;

int q = 0;

string path;
Editor* editor;
HWND hwnd, hwnd2;
mutex lockMutex;

//PC loc: C:\Users\Pua Kai\Documents\GitHub\OpenGl_Engine\Debug

int main(int argc, char **argv)
{
	path = argv[0];
	hwnd = GetForegroundWindow();
	editor = new Editor();
	editor->dataPath = path.substr(0, path.find_last_of('\\') + 1);
	editor->lockMutex = &lockMutex;

	//editor->ParseAsset("D:\\test.blend");
	//editor->Compile();

	//*
	cout << "Enter project folder path" << endl;

	getline(cin, editor->projectFolder);
	if (editor->projectFolder == "")
		editor->projectFolder = "F:\\TestProject\\";
	else while (!IO::HasDirectory(editor->projectFolder.c_str())) {
		cout << "Invalid project folder path: " << editor->projectFolder << endl;
		getline(cin, editor->projectFolder);
	}
	//*/

	editor->hwnd = hwnd;
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
	editor->yPoss.push_back(0.5f);
	editor->yLimits.push_back(Int2(0, 1));
	editor->yLimits.push_back(Int2(0, 1));
	editor->yLimits.push_back(Int2(0, 2));
	editor->yLimits.push_back(Int2(2, 1));
	
	editor->NewScene();
	editor->activeScene.objects.push_back(new SceneObject("Main Camera"));
	editor->activeScene.objects.push_back(new SceneObject("Player"));
	editor->activeScene.objects[0]
		->AddChild(new SceneObject())
		->AddChild(new SceneObject("Sound"))
		->AddChild(new SceneObject("Particles"))
		->transform.Translate(0, 3, -5)
		->object->AddComponent(new Camera())
		->object->AddComponent(new TextureRenderer())
		->object->GetChild(1)
		->AddChild(new SceneObject("Child"))
		->AddComponent(new MeshFilter());
	editor->activeScene.objects[1]
		->AddComponent(new MeshFilter())
		->object->AddComponent(new MeshRenderer());

	HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	editor->scrW = info.rcMonitor.right - info.rcMonitor.left;
	editor->scrH = info.rcMonitor.bottom - info.rcMonitor.top;

	//EBI_Asset* e = new EBI_Asset("F:\\OpenGl1\\OpenGl1_Editor\\OpenGl1_Editor\\src\\TestScript.h", "TestScript.h");
	//((EB_Inspector*)(editor->blocks[0]))->SelectAsset(e, "F:\\OpenGl1\\OpenGl1_Editor\\OpenGl1_Editor\\src\\TestScript.h");

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
	glutInitWindowSize(1366, 768);
	glutCreateWindow("Engine (loading...)");
	hwnd2 = GetActiveWindow();
	ShowWindow(hwnd2, SW_MAXIMIZE);
	/*
	SendMessage(hwnd2, WM_SETICON, ICON_SMALL, hIcon);
	SendMessage(hwnd2, WM_SETICON, ICON_BIG, hIcon);
	SendMessage(GetWindow(hwnd2, GW_OWNER), WM_SETICON, ICON_SMALL, hIcon);
	SendMessage(GetWindow(hwnd2, GW_OWNER), WM_SETICON, ICON_BIG, hIcon);
	*/

	string p;
	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		cerr << ("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		getline(cin, p);
		return 0;
	}
	else {
		Engine::Init(path);
		editor->LoadDefaultAssets();
		editor->blocks = vector<EditorBlock*>({ new EB_Inspector(editor, 2, 0, 1, 3), new EB_Inspector(editor, 2, 3, 1, 1), new EB_Browser(editor, 0, 2, 4, 1, editor->projectFolder + "Assets\\"), new EB_Debug(editor, 4, 2, 2, 1), new EB_Viewer(editor, 0, 0, 3, 2), new EB_Hierarchy(editor, 3, 0, 2, 2) }); //path.substr(0, path.find_last_of('\\') + 1)
		font = editor->font;
		editor->ReloadAssets(editor->projectFolder + "Assets\\", true);
		//editor->activeScene.sky = new Background(editor->dataPath + "res\\bg_refl.hdr");
		editor->SetBackground(editor->dataPath + "res\\bg.jpg", 0.3f);

		//ShaderBase b("F:\\TestProject\\Assets\\test.shade.meta");
		//Material m(&b);
		//m.Save("F:\\TestProject\\Assets\\test.material");

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glutDisplayFunc(renderScene);
		//glutTimerFunc(10, InitGL, 0);
		glutTimerFunc(1000, TimerGL, 0);
		glutMouseFunc(MouseGL);
		glutReshapeFunc(ReshapeGL);
		glutMotionFunc(MotionGL);
		glutPassiveMotionFunc(MotionGLP);

		Time::startMillis = milliseconds();

		updateThread = thread(UpdateLoop);
		atexit(OnDie);

		glutMainLoop();
		//thread updateThread(glutMainLoop);
		//updateThread.detach();

		return 0;
	}
}

void closeCallback()
{

	std::cout << "GLUT:\t Finished" << std::endl;
}

void InitGL(int i) {
	
}

void CheckShortcuts() {
	if (editor->editorLayer == 0) {
		bool cDn = Input::KeyHold(Key_Control);
		bool aDn = Input::KeyHold(Key_Alt);
		bool sDn = Input::KeyHold(Key_Shift);
		for (auto g = editor->globalShorts.begin(); g != editor->globalShorts.end(); g++) {
			if (ShortcutTriggered(g->first, cDn, aDn, sDn)) {
				g->second(editor);
				return;
			}
			if ((g->first & 0xff00) == 0) {
				cout << hex << g->first;
			}
		}
		for (EditorBlock* e : editor->blocks) {
			Vec4 v = Vec4(Display::width*editor->xPoss[e->x1], Display::height*editor->yPoss[e->y1], Display::width*editor->xPoss[e->x2], Display::height*editor->yPoss[e->y2]);
			v.a = round(v.a - v.g) - 1;
			v.b = round(v.b - v.r) - 1;
			v.g = round(v.g) + 1;
			v.r = round(v.r) + 1;
			if (Engine::Button(v.r, v.g, v.b, v.a) && MOUSE_HOVER_FLAG) {
				for (auto g2 = e->shortcuts.begin(); g2 != e->shortcuts.end(); g2++) {
					if (ShortcutTriggered(g2->first, cDn, aDn, sDn)) {
						g2->second(e);
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
	if (editor->WAITINGBUILDSTARTFLAG) {
		editor->WAITINGBUILDSTARTFLAG = false;
		editor->DoCompile();
		return;
	}
	editor->WAITINGREFRESHFLAG = false;
	CheckShortcuts();
	int i = -1, k = 0;
	editor->mouseOn = 0;
	for (EditorBlock* e : editor->blocks) {
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
		k++;
		if (editor->WAITINGREFRESHFLAG) //deleted
			return;
	}
	if (i != -1) {
		if (editor->mousePressType == -1) {
			editor->mouseOnP = i;
			if (Input::mouse0State == MOUSE_DOWN) {
				editor->mousePressType = 0;
				//editor->mouseOn = i;
			}
			else if (Input::mouse1State == MOUSE_DOWN) {
				editor->mousePressType = 1;
				//editor->mouseOn = i;
			}
			else if (Input::mouse2State == MOUSE_DOWN) {
				editor->mousePressType = 2;
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
		while (!redrawn) {}
			//Sleep(1);
		{
			lock_guard<mutex> lock(lockMutex);
			t++;
			long long millis = milliseconds();
			Time::delta = (millis - Time::millis)*0.001f;
			Time::time = (millis - Time::startMillis)*0.001;
			Time::millis = millis;
			Input::UpdateMouseNKeyboard();
			if (Input::mouse0State == MOUSE_DOWN)
				editor->blocks[editor->mouseOnP]->OnMousePress(0);
			if (Input::mouse1State == MOUSE_DOWN)
				editor->blocks[editor->mouseOnP]->OnMousePress(1);
			if (Input::mouse2State == MOUSE_DOWN)
				editor->blocks[editor->mouseOnP]->OnMousePress(2);
			DoUpdate();
			redrawn = false;
			//glutPostRedisplay();
		}
	}
}

void OnDie() {
	die = true;
	updateThread.join();
}

bool hi;
void DrawOverlay() {
	editor->UpdateLerpers();
	if (editor->backgroundTex != nullptr)
		Engine::DrawTexture(0, 0, Display::width, Display::height, editor->backgroundTex, editor->backgroundAlpha*0.01f);
	for (int i = editor->blocks.size() - 1; i >= 0; i--) {
		editor->blocks[i]->Draw();
	}
	if (editor->dialogBlock) {
		editor->dialogBlock->Draw();
	}
	editor->DrawHandles();
}

void renderScene()
{
	lock_guard<mutex> lock(lockMutex);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0, 0, 0, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glMultMatrixf(glm::value_ptr(glm::perspective(60.0f, 1.0f, 0.1f, 100.0f))); //double fovy, double aspect, double zNear, double zFar
	//glMultMatrixf(glm::value_ptr(glm::ortho(-1.0f, -1.0f, 1.0f, 1.0f, 0.1f, 100.0f)));

	glDisable(GL_DEPTH_TEST);
	DrawOverlay();

	glutSwapBuffers();
	redrawn = true;
	glutPostRedisplay();
}

void TimerGL(int i)
{
	fps = t;
	t = 0;
	glutTimerFunc(1000, TimerGL, 1);
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	SIZE_T virtualMemUsedByMe = pmc.WorkingSetSize;
	string str("Engine (about " + to_string((byte)round(virtualMemUsedByMe*0.000001f)) + "Mb used, " + to_string(fps) + "fps)");
	SetWindowText(hwnd2, str.c_str());
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