#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <Windows.h>
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
using namespace std;

void InitGL(int argc, char* argv[]);
void DisplayGL();
void InitGL(int i);
void TimerGL(int i);
void KeyboardGL(unsigned char c, int x, int y);
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
HWND hwnd;

int main(int argc, char **argv)
{
	path = argv[0];
	hwnd = GetForegroundWindow();

	editor = new Editor();
	editor->dataPath = path.substr(0, path.find_last_of('\\') + 1);

	//Editor::ParseAsset("D:\\test.shade");
	//editor->Compile();

	//*
	cout << "Enter project folder path" << endl;

	getline(cin, editor->projectFolder);
	if (editor->projectFolder == "")
		editor->projectFolder = editor->dataPath;
	else while (!IO::HasDirectory(editor->projectFolder.c_str())) {
		cout << "Invalid project folder path: " << editor->projectFolder << endl;
		getline(cin, editor->projectFolder);
	}
	//*/

	editor->hwnd = hwnd;
	editor->xPoss.push_back(0);
	editor->xPoss.push_back(1);
	editor->xPoss.push_back(0.8f);
	editor->xPoss.push_back(0.7f);
	editor->xLimits.push_back(Int2(0, 1));
	editor->xLimits.push_back(Int2(0, 1));
	editor->xLimits.push_back(Int2(0, 1));
	editor->xLimits.push_back(Int2(0, 2));

	editor->yPoss.push_back(0);
	editor->yPoss.push_back(1);
	editor->yPoss.push_back(0.7f);
	editor->yLimits.push_back(Int2(0, 1));
	editor->yLimits.push_back(Int2(0, 1));
	editor->yLimits.push_back(Int2(0, 2));
	
	editor->NewScene();
	editor->activeScene.objects.push_back(new SceneObject("Main Camera"));
	editor->activeScene.objects.push_back(new SceneObject("Player"));
	editor->activeScene.objects[0]
		->AddChild(new SceneObject())
		->AddChild(new SceneObject("Sound"))
		->AddChild(new SceneObject("Particles"))
		->transform.Translate(0, 3, -5)
		->object->AddComponent(new Camera())
		->object->GetChild(1)
		->AddChild(new SceneObject("Child"));
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

	string p;
	LPARAM hIcon = (LPARAM)LoadImage(NULL, "F:\\1.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, hIcon);
	SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, hIcon);
	SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_BIG, hIcon);

	/*
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
	glutInitWindowSize(512, 512);
	glutCreateWindow("Application");
	HWND hwnd2 = GetActiveWindow();
	ShowWindow(hwnd2, SW_MAXIMIZE);
	SendMessage(hwnd2, WM_SETICON, ICON_SMALL, hIcon);
	SendMessage(hwnd2, WM_SETICON, ICON_BIG, hIcon);
	SendMessage(GetWindow(hwnd2, GW_OWNER), WM_SETICON, ICON_SMALL, hIcon);
	SendMessage(GetWindow(hwnd2, GW_OWNER), WM_SETICON, ICON_BIG, hIcon);

	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		cerr << ("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		getline(cin, p);
		return 0;
	}
	else {
		Engine::Init(path);
		font = new Font("F:\\ascii s2.font", "F:\\ascii 3.font", 17);
		editor->font = font;

		editor->LoadDefaultAssets();
		editor->blocks = vector<EditorBlock*>({ new EB_Inspector(editor, 2, 0, 1, 1), new EB_Browser(editor, 0, 2, 2, 1, "D:\\"), new EB_Viewer(editor, 0, 0, 3, 2), new EB_Hierarchy(editor, 3, 0, 2, 2) }); //path.substr(0, path.find_last_of('\\') + 1)

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glutDisplayFunc(renderScene);
		//glutTimerFunc(10, InitGL, 0);
		glutTimerFunc(1000, TimerGL, 0);
		glutKeyboardFunc(KeyboardGL);
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

void DoUpdate() {
	if (editor->WAITINGBUILDSTARTFLAG) {
		editor->WAITINGBUILDSTARTFLAG = false;
		editor->DoCompile();
		return;
	}
	int i = -1, k = 0;
	for each (EditorBlock* e in editor->blocks) {
		Color v = Color(Display::width*editor->xPoss[e->x1], Display::height*editor->yPoss[e->y1], Display::width*editor->xPoss[e->x2], Display::height*editor->yPoss[e->y2]);
		v.a = round(v.a - v.g) - 1;
		v.b = round(v.b - v.r) - 1;
		v.g = round(v.g) + 1;
		v.r = round(v.r) + 1;
		if (Engine::Button(v.r, v.g, v.b, v.a) && MOUSE_HOVER_FLAG) {
			i = k;
			break;
		}
		k++;
	}
	if (i != -1) {
		if (editor->mousePressType == -1) {
			editor->mouseOnP = i;
			if (Input::mouse0State == MOUSE_DOWN) {
				editor->mousePressType = 0;
				editor->mouseOn = i;
			}
			else if (Input::mouse1State == MOUSE_DOWN) {
				editor->mousePressType = 1;
				editor->mouseOn = i;
			}
			else if (Input::mouse2State == MOUSE_DOWN) {
				editor->mousePressType = 2;
				editor->mouseOn = i;
			}
		}
		else {
			if ((editor->mousePressType == 0 && !Input::mouse0) || (editor->mousePressType == 1 && !Input::mouse1) || (editor->mousePressType == 2 && !Input::mouse2)) {
				editor->mousePressType = -1;
				editor->mouseOn = 0;
			}
		}
	}
}

void UpdateLoop() {
	while (!die) {
		while (!redrawn)
			Sleep(1);
		t++;
		redrawn = false; 
		long long millis = milliseconds();
		Time::delta = (millis - Time::millis)*0.001f;
		Time::time = (millis - Time::startMillis)*0.001;
		Time::millis = millis;
		Input::UpdateMouse();
		DoUpdate();
	}
}

void OnDie() {
	die = true;
	updateThread.join();
}

bool hi;
void DrawOverlay() {
	editor->UpdateLerpers();
	for (int i = editor->blocks.size() - 1; i >= 0; i--) {
		editor->blocks[i]->Draw();
	}
	editor->DrawHandles();
}

void renderScene()
{
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
	//glutPostRedisplay();
}


void KeyboardGL(unsigned char c, int x, int y) {
	int mods = glutGetModifiers();
	if ((mods & 2) == 2 && c != 32) {
		c = (c | (3 << 5));
	}
	if (mods == 1 && c > 64)
		c += 32;

	if (editor->editorLayer == 0) {
		ShortcutMapGlobal::const_iterator got = editor->globalShorts.find(GetShortcutInt(c, mods));
		if (got != editor->globalShorts.end()) {
			(*got->second)(editor);
			return;
		}
		for each (EditorBlock* e in editor->blocks) {
			Color v = Color(Display::width*editor->xPoss[e->x1], Display::height*editor->yPoss[e->y1], Display::width*editor->xPoss[e->x2], Display::height*editor->yPoss[e->y2]);
			v.a = round(v.a - v.g) - 1;
			v.b = round(v.b - v.r) - 1;
			v.g = round(v.g) + 1;
			v.r = round(v.r) + 1;
			if (Engine::Button(v.r, v.g, v.b, v.a) && MOUSE_HOVER_FLAG) {
				ShortcutMap::const_iterator got = e->shortcuts.find(GetShortcutInt(c, mods));

				if (got != e->shortcuts.end())
					(*got->second)(e);

				//e->OnKey(c, glutGetModifiers());
				break;
			}
		}
	}
	else if (c == 27) //escape key
		editor->editorLayer = 0;
}

void MouseGL(int button, int state, int x, int y) {

	switch (button) {
	case 0:
		Input::mouse0 = (state == 0);
		return;
	case 1:
		Input::mouse1 = (state == 0);
		return;
	case 2:
		Input::mouse2 = (state == 0);
		return;
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
	Input::mousePos = Vec2(x, y);
	Input::mousePosRelative = Vec2(x*1.0f / Display::width, y*1.0f / Display::height);
}

void MotionGL(int x, int y) {
	if (editor->editorLayer == 0)
		editor->blocks[editor->mouseOn]->OnMouseM(Vec2(x, y) - Input::mousePos);
	Input::mousePos = Vec2(x, y);
	Input::mousePosRelative = Vec2(x*1.0f / Display::width, y*1.0f / Display::height);
}