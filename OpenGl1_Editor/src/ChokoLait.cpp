#include "ChokoLait.h"

GLFWwindow* ChokoLait::window = nullptr;
int ChokoLait::initd = 0;

ChokoLait::ChokoLait() {
	if (!initd) {
		char cpath[200];
		GetModuleFileName(NULL, cpath, 200);
		string path = cpath;
		path = path.substr(0, path.find_last_of('\\') + 1);

		Debug::Init(path);
		DefaultResources::Init(path + "defaultresources.bin");

		if (!glfwInit()) {
			Debug::Error("System", "Fatal: Cannot init glfw!");
			abort();
		}
		glfwWindowHint(GLFW_VISIBLE, 0);
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

		initd = 1;
	}
}

void ChokoLait::Init(int scrW, int scrH) {
	if (!initd) {
		ChokoLait();
	}
	if (initd == 1) {
		Engine::_mainThreadId = std::this_thread::get_id();

		Scene::active = new Scene();

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

		/*
		glfwSetCursorPosCallback(window, MotionGL);
		glfwSetMouseButtonCallback(window, MouseGL);
		glfwSetScrollCallback(window, MouseScrGL);

		glfwSetWindowFocusCallback(window, FocusGL);

		auto mills = milliseconds();
		*/

		initd = 2;
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

void ChokoLait::Paint(emptyCallbackFunc func) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0, 0, 0, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (!UI::drawFuncLoc) {
		__trace(funcLoc);
		UI::drawFuncLoc = funcLoc;
	}
	UI::PreLoop();

	if (func) func();

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
