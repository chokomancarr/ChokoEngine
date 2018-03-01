#include "ChokoLait.h"

GLFWwindow* ChokoLait::window = nullptr;
int ChokoLait::initd = 0;
rCamera ChokoLait::mainCamera = rCamera();

void _dieded(int i) {
#ifdef PLATFORM_WIN
	MessageBox(glfwGetWin32Window(Display::window), "Beep Boop, I Crashed.", "fuck", MB_OK);
#endif
}

void ChokoLait::_InitVars() {
#ifdef PLATFORM_WIN
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	signal(SIGABRT, &_dieded);
#elif defined(PLATFORM_LNX)

#endif

	const string& path = IO::InitPath();
	Debug::Init(path);
	DefaultResources::Init(path + "/defaultresources.bin");

	if (!glfwInit()) {
		Debug::Error("System", "Fatal: Cannot init glfw!");
		abort();
	}
	glfwWindowHint(GLFW_VISIBLE, 0);
#ifdef PLATFORM_LNX
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	window = glfwCreateWindow(10, 10, "ChokoLait Application", NULL, NULL);
	Display::window = window;
	if (!window) {
		Debug::Error("System", "Fatal: Cannot create glfw window!");
		abort();
	}
	glfwMakeContextCurrent(window);

	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		Debug::Error("System", "Glew error: " + string((const char*)glewGetErrorString(GlewInitResult)));
		abort();
	}
	Engine::Init(path);
}

void ChokoLait::Init(int scrW, int scrH) {
	if (!initd) {
		ChokoLait();
	}
	if (initd == 1) {
		Engine::_mainThreadId = std::this_thread::get_id();

		Scene::active = new Scene();
		pSceneObject cam = SceneObject::New(Vec3());
		Scene::AddObject(cam);
		mainCamera(cam->AddComponent<Camera>());

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glFrontFace(GL_CW);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		Time::startMillis = milliseconds();

		glfwSetWindowSize(window, scrW, scrH);
		ReshapeGL(window, scrW, scrH);
		glfwShowWindow(window);

		glfwSetFramebufferSizeCallback(window, ReshapeGL);
		glfwSetCursorPosCallback(window, MotionGL);
		glfwSetMouseButtonCallback(window, MouseGL);
		glfwSetScrollCallback(window, MouseScrGL);

		glClearColor(0, 0, 0, 1.0f);
		/*
		glfwSetCursorPosCallback(window, MotionGL);
		glfwSetMouseButtonCallback(window, MouseGL);
		glfwSetScrollCallback(window, MouseScrGL);

		glfwSetWindowFocusCallback(window, FocusGL);

		auto mills = milliseconds();
		*/

		initd = 2;
		Debug::Message("ChokoLait", "Init finished.");
	}
}

bool ChokoLait::alive() {
	return !glfwWindowShouldClose(window);
}

void ChokoLait::Update(emptyCallbackFunc func) {
	long long millis = milliseconds();
	Time::delta = (millis - Time::millis)*0.001f;
	Time::time = (millis - Time::startMillis)*0.001;
	Time::millis = millis;
	Input::UpdateMouseNKeyboard();
	if (func) func();
}

void ChokoLait::Paint(emptyCallbackFunc rendFunc, emptyCallbackFunc paintFunc) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	mainCamera->Render(nullptr, rendFunc);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

#ifdef PLATFORM_WIN
	if (!UI::drawFuncLoc) {
		__trace(funcLoc);
		UI::drawFuncLoc = funcLoc;
	}
#endif
	UI::PreLoop();

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glEnable(GL_BLEND);

	if (paintFunc) paintFunc();

	glfwSwapBuffers(window);
	glfwPollEvents();
}

void ChokoLait::MouseGL(GLFWwindow* window, int button, int state, int mods) {
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

void ChokoLait::MouseScrGL(GLFWwindow* window, double xoff, double yoff) {
	
}

void ChokoLait::MotionGL(GLFWwindow* window, double x, double y) {
	Input::mousePos = Vec2(x, y);
	Input::mousePosRelative = Vec2(x*1.0f / Display::width, y*1.0f / Display::height);
}

void ChokoLait::ReshapeGL(GLFWwindow* window, int w, int h) {
	glViewport(0, 0, w, h);
	Display::width = w;
	Display::height = h;
}
