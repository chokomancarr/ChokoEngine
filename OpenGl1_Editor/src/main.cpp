#ifdef IS_EDITOR

#define _ITERATOR_DEBUG_LEVEL 0

#include "Engine.h"
#include "Editor.h"
#include <ctime>
#include <Psapi.h>
#include <typeinfo>
#include <thread>
#include <mutex>
#include <type_traits>
#include "Compressors.h"
#include <shellapi.h>

void MouseGL(GLFWwindow* window, int button, int state, int mods);
void MouseScrGL(GLFWwindow* window, double xoff, double yoff);
void MotionGL(GLFWwindow* window, double x, double y);
void ReshapeGL(GLFWwindow* window, int w, int h);
void FocusGL(GLFWwindow* window, int focus);

void UpdateLoop();
void OnDie();

void renderScene();

Font *font;

//std::thread updateThread;
uint fps;
//bool redrawn = false;
//bool die = false;

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

int main(int argc, char **argv)
{
	editor = new Editor();
	editor->dataPath = path = IO::InitPath();// path.substr(0, path.find_last_of('\\') + 1);
	editor->lockMutex = &lockMutex;
	editor->projectFolder = "D:\\TestProject2\\";

	HMONITOR monitor = MonitorFromWindow(editor->hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	ShowSplash(path + "res\\splash.png", (info.rcMonitor.right - info.rcMonitor.left) / 2, (info.rcMonitor.bottom - info.rcMonitor.top) / 2, 600, 300);
	
	editor->scrW = info.rcMonitor.right - info.rcMonitor.left;
	editor->scrH = info.rcMonitor.bottom - info.rcMonitor.top;

	//std::cout << "Enter project folder path" << std::endl;
	//std::getline(std::cin, editor->projectFolder);

	DefaultResources::Init(path + "res\\defaultresources.bin");
	editor->xPoss.push_back(0);
	editor->xPoss.push_back(1);
	//editor->xPoss.push_back(0.95f);
	//editor->xPoss.push_back(0.9f);
	//editor->xPoss.push_back(0.1f);
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
	//editor->yPoss.push_back(0.95f);
	editor->yLimits.push_back(Int2(0, 1));
	editor->yLimits.push_back(Int2(0, 1));
	editor->yLimits.push_back(Int2(0, 2));
	editor->yLimits.push_back(Int2(2, 1));

	if (!glfwInit()) {
		std::cerr << "Fatal: Cannot init glfw!" << std::endl;
		abort();
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_VISIBLE, 0);
	auto* window = glfwCreateWindow(1024, 600, "ChokoEngine", NULL, NULL);
	Display::window = window;
	if (!window) {
		std::cerr << "Fatal: Cannot create glfw window!" << std::endl;
		abort();
	}
	glfwSetWindowPos(window, info.rcMonitor.left + editor->scrW / 2 - 512, info.rcMonitor.top + editor->scrH / 2 - 300);
	glfwMakeContextCurrent(window);
	editor->hwnd2 = glfwGetWin32Window(window);

	auto ver = string((const char*)glGetString(GL_VERSION));

	//SendMessage(hwnd2, WM_SETICON, ICON_SMALL, hIcon);
	//SendMessage(hwnd2, WM_SETICON, ICON_BIG, hIcon);
	//SendMessage(GetWindow(hwnd2, GW_OWNER), WM_SETICON, ICON_SMALL, hIcon);
	//SendMessage(GetWindow(hwnd2, GW_OWNER), WM_SETICON, ICON_BIG, hIcon);
	glewExperimental = true;
	string p;
	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << ("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		std::getline(std::cin, p);
		return 0;
	}

	Engine::Init(path);
	Editor::instance->activeScene = new Scene();
	Scene::active = Editor::instance->activeScene;
	Scene::active->settings.sky = 0;
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
	editor->SetBackground(path + "res\\bg.jpg", 0.3f);
	font = editor->font;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFrontFace(GL_CW);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glClearColor(0, 0, 0, 1.0f);

	Time::startMillis = milliseconds();

	//updateThread = std::thread(UpdateLoop);
	atexit(OnDie);
	
	PopupSelector::Init();
	glfwMakeContextCurrent(window);

	KillSplash();

	glfwSetFramebufferSizeCallback(window, ReshapeGL);
	ReshapeGL(window, 1024, 600);
	glfwShowWindow(window);

	glfwSetCursorPosCallback(window, MotionGL);
	glfwSetMouseButtonCallback(window, MouseGL);
	glfwSetScrollCallback(window, MouseScrGL);

	glfwSetWindowFocusCallback(window, FocusGL);

	glfwSetCursorPosCallback(PopupSelector::window, MotionGL);
	glfwSetMouseButtonCallback(PopupSelector::window, MouseGL);
	glfwSetScrollCallback(PopupSelector::window, MouseScrGL);

	glfwSetWindowFocusCallback(PopupSelector::window, FocusGL);

	auto millis = Time::startMillis = milliseconds();

	AudioEngine::Start(&Editor::AudioCallback);

	while (!glfwWindowShouldClose(window)) {
		if (PopupSelector::show) {
			glfwMakeContextCurrent(PopupSelector::window);

			PopupSelector::Draw();

			if (Input::KeyDown(Key_Escape)) PopupSelector::Close();

			glfwSwapBuffers(PopupSelector::window);
			glfwPollEvents();
			glfwMakeContextCurrent(window);
		}

		UpdateLoop();
		renderScene();

		glfwSwapBuffers(window);
		glfwPollEvents();

		if (Time::millis - millis > 1000) {
			millis = Time::millis;

			PROCESS_MEMORY_COUNTERS pmc;
			GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
			SIZE_T virtualMemUsedByMe = pmc.WorkingSetSize;
			string str("ChokoEngine (about " + std::to_string((uint)round(virtualMemUsedByMe / 1024 / 1024)) + "Mb used, " + std::to_string(fps) + "fps)");
			glfwSetWindowTitle(window, &str[0]);
			fps = 0;
		}
	}
	return 0;
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
		string cmd = " " + editor->buildErrorPath + " -n" + std::to_string(editor->buildErrorLine);
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
	//while (!die) {
	//	if (redrawn)
	//	{
			//lock_guard<mutex> lock(lockMutex);
			//redrawn = false;
			long long millis = milliseconds();
			Time::delta = (millis - Time::millis)*0.001f;
			Time::time = (millis - Time::startMillis)*0.001f;
			Time::millis = millis;
			Input::UpdateMouseNKeyboard();
			Editor::onFocus = GetForegroundWindow() == Editor::hwnd2;
			DoUpdate();
	//	}
	//}
}

void OnDie() {
	//die = true;
	//updateThread.join();
	editor->playSyncer.Terminate();
}

void DrawOverlay() {
	std::lock_guard<std::mutex> lock(lockMutex);
	if (!UI::drawFuncLoc) {
		__trace(funcLoc);
		UI::drawFuncLoc = funcLoc;
	}
	UI::PreLoop();
	if (Scene::active) {
		for (auto a : Scene::active->_preRenderComps) {
			a->OnPreRender();
		}
	}

	editor->UpdateLerpers();
	editor->playSyncer.Update();
	if (editor->backgroundTex != nullptr)
		UI::Texture(0, 0, (float)Display::width, (float)Display::height, editor->backgroundTex, editor->backgroundAlpha*0.01f, DRAWTEX_CROP);
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
	//if (!redrawn || editor->editorLayer == 6) {
		fps++;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);
		glEnable(GL_BLEND);

		MVP::Reset();

		DrawOverlay();
		//Engine::DrawQuad(10, 20, 500, 300, red());
		//Engine::DrawLine(Vec2(300, 500), Vec2(700, 0), green(), 1);

		//redrawn = true;
	//}
}

void MouseGL(GLFWwindow* window, int button, int state, int mods) {
	if (window == Display::window) {
		switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			Input::mouse0 = (state == GLFW_PRESS);
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			Input::mouse1 = (state == GLFW_PRESS);
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			Input::mouse2 = (state == GLFW_PRESS);
			break;
		}
	}
	else {
		switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			PopupSelector::mouse0 = (state == GLFW_PRESS);
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			PopupSelector::mouse1 = (state == GLFW_PRESS);
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			PopupSelector::mouse2 = (state == GLFW_PRESS);
			break;
		}
	}
}

void MouseScrGL(GLFWwindow* window, double xoff, double yoff) {
	if (window == Display::window) {
		if (yoff != 0 && editor->editorLayer == 0) {
			editor->blocks[editor->mouseOnP]->OnMouseScr(yoff > 0);
		}
	}
	else {
		PopupSelector::scrollPos += (yoff > 0 ? -20 : 20);
	}
}

void MotionGL(GLFWwindow* window, double x, double y) {
	if (window == Display::window) {
		if (editor->editorLayer == 0)
			editor->blocks[editor->mouseOn]->OnMouseM(Vec2(x, y) - Input::mousePos);
		Input::mousePos = Vec2(x, y);
		Input::mousePosRelative = Vec2(x*1.0f / Display::width, y*1.0f / Display::height);
	}
	else {
		PopupSelector::mousePos = Vec2(x, y);
	}
}

void ReshapeGL(GLFWwindow* window, int w, int h) {
	glfwMakeContextCurrent(window);
	glViewport(0, 0, w, h);
	Display::width = w;
	Display::height = h;
}


void FocusGL(GLFWwindow* window, int focus) {
	if (window == Display::window) {
		UI::focused = !!focus;
	}
	else {
		PopupSelector::focused = !!focus;
	}
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
		px[a * 4] = byte((px[a * 4] * px[a * 4 + 3])/255.0f);
		px[a * 4 + 1] = byte((px[a * 4 + 1] * px[a * 4 + 3]) / 255.0f);
		px[a * 4 + 2] = byte((px[a * 4 + 2] * px[a * 4 + 3]) / 255.0f);
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
	SIZE sz = { (LONG)pw, (LONG)ph };
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

#elif defined TEST_LAIT
//#define WATERY

#ifdef WATERY
#define PLATFORM_WIN
#include "ChokoLait.h"
#include "Water.h"
CHOKOLAIT_INIT_VARS;

Background* bg = new Background(IO::path + "/refl.hdr");
Font* font = new Font(IO::path + "/arimo.ttf");
Water* water;

double told = 0;
uint fpsc = 0;
uint fps;
ulong simtime;

float camz = 2.0f;

Material* mat;
Shader* shad;
Mesh* mesh;

void updateFunc() {
	if (Input::KeyHold(Key_W)) camz -= Time::delta;
	else if (Input::KeyHold(Key_S)) camz += Time::delta;
	camz = Clamp<float>(camz, 0.5f, 10);
}

void rendFunc() {
	MVP::Translate(-Water::me->wall / 2, -Water::me->wall / 2, -Water::me->wall / 2);
	Engine::DrawMeshInstanced(mesh, 0, mat, 256 * 3);
}

void paintfunc() {
	if (++fpsc == 100) {
		fps = (uint)roundf(100.0f / (Time::time - told));
		told = Time::time;
		fpsc = 0;
	}
	UI::Label(10, 10, 12, "fps: " + std::to_string(fps), font, white());
	UI::Label(10, 25, 12, "t: " + std::to_string(++simtime / 1000) + " ps", font, white());
	//UI::Label(10, 40, 12, "T: " + std::to_string((int)roundf(Water::me->res_tmp)) + " K", font, white());
	//UI::Label(10, 55, 12, "P: " + std::to_string(Water::me->res_prs) + " atm", font, white());

	//auto& set = Scene::active->settings;
	//set.skyStrength = Engine::DrawSliderFill(400, 10, 150, 20, 0, 3, set.skyStrength, white(0.5f, 0.3f), white());
}

int main(int argc, char **argv)
{
	ChokoLait::Init(500, 500);

	auto& set = Scene::active->settings;
	set.sky = bg;
	set.skyStrength = 2;

	shad = new Shader(IO::GetText(IO::path + "/instV.txt"), IO::GetText(IO::path + "/instF.txt"));
	mat = new Material(shad);
	mesh = Procedurals::UVSphere(20, 10);

	font->Align(ALIGN_TOPLEFT);
	water = new Water(IO::path + "/water.compute", 4, 1000.0f / 1.66f / 17.99f, 200);
	mat->SetBuffer(5, water->psb);

	while (ChokoLait::alive()) {
		ChokoLait::Update(updateFunc);
		ChokoLait::mainCamera->object->transform.localPosition(Vec3(0, 0, -camz));
		//if (Input::KeyHold(Key_D)) tr.Rotate(0, Time::delta * 60, 0);
		//if (Input::KeyHold(Key_A)) tr.Rotate(0, -Time::delta * 60, 0);
		water->Update();
		
		ChokoLait::Paint(rendFunc, paintfunc);
	}
}

#else

#define PLATFORM_WIN
#include "ChokoLait.h"
#include "Gromacs.h"
CHOKOLAIT_INIT_VARS;

Background* bg = new Background(IO::path + "/refl.hdr");
Font* font = new Font(IO::path + "/arimo.ttf", ALIGN_TOPLEFT);

float camz = 10;
float rw = 0, rz = 0;

Gromacs* gro;
Material* mat;
Shader* shad;
Mesh* mesh;

void updateFunc() {
	if (Input::KeyHold(Key_UpArrow)) camz -= 10*Time::delta;
	else if (Input::KeyHold(Key_DownArrow)) camz += 10*Time::delta;
	camz = max(camz, 0.0f);
	if (Input::KeyHold(Key_W)) rw -= 100 * Time::delta;
	else if (Input::KeyHold(Key_S)) rw += 100 * Time::delta;
	//camz = Clamp<float>(camz, 0.5f, 10);
	if (Input::KeyHold(Key_A)) rz -= 100 * Time::delta;
	else if (Input::KeyHold(Key_D)) rz += 100 * Time::delta;
	//camz = Clamp<float>(camz, 0.5f, 10);
}

void rendFunc() {
	MVP::Switch(false);
	MVP::Clear();
	float csz = cos(-rz*deg2rad);
	float snz = sin(-rz*deg2rad);
	float csw = cos(rw*deg2rad);
	float snw = sin(rw*deg2rad);
	Mat4x4 mMatrix = Mat4x4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1) * Mat4x4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
	MVP::Mul(mMatrix);
	MVP::Translate(-gro->boundingBox.x / 2, -gro->boundingBox.y / 2, -gro->boundingBox.z / 2);
	Engine::DrawMeshInstanced(mesh, 0, mat, gro->frames[0].count);
}

uint fpsc, fps;
float told;
void paintfunc() {
	if (++fpsc == 100) {
		fps = (uint)roundf(100.0f / (Time::time - told));
		told = Time::time;
		fpsc = 0;
	}
	UI::Label(10, 10, 12, "fps: " + std::to_string(fps), font, white());
}

int main(int argc, char **argv)
{
	ChokoLait::Init(500, 500);

	auto& set = Scene::active->settings;
	set.sky = bg;
	set.skyStrength = 2;
	set.skyBrightness = 0;

	gro = new Gromacs(IO::path + "/md.gro");
	shad = new Shader(IO::GetText(IO::path + "/groV.txt"), IO::GetText(IO::path + "/instF.txt"));
	mat = new Material(shad);
	mat->SetBuffer(5, gro->ubo_positions);
	mat->SetBuffer(6, gro->ubo_colors);
	mesh = Procedurals::UVSphere(16, 8);

	while (ChokoLait::alive()) {
		ChokoLait::Update(updateFunc);
		ChokoLait::mainCamera->object->transform.localPosition(Vec3(0, 0, -camz));
		
		ChokoLait::Paint(rendFunc, paintfunc);
	}
}


#endif
#endif