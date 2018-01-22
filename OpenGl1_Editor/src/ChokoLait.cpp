#include "ChokoLait.h"

void ReshapeGL(GLFWwindow* window, int w, int h) {
	glfwMakeContextCurrent(window);
	glViewport(0, 0, w, h);
	Display::width = w;
	Display::height = h;
}

GLFWwindow* ChokoLait::window = nullptr;

void ChokoLait::Init(char** argv, int scrW, int scrH) {
	string path = argv[0];
	path = path.substr(0, path.find_last_of('\\') + 1);
	Debug::Init(path);
	DefaultResources::Init(path + "defaultresources.bin");

	if (!glfwInit()) {
		std::cerr << "Fatal: Cannot init glfw!" << std::endl;
		abort();
	}
	window = glfwCreateWindow(scrW, scrH, "ChokoLait Application", NULL, NULL);
	Display::window = window;
	if (!window) {
		std::cerr << "Fatal: Cannot create glfw window!" << std::endl;
		abort();
	}
	glfwMakeContextCurrent(window);

	string p;
	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << ("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		abort();
	}

	Engine::Init(string(argv[0]));
	Scene::active = new Scene();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFrontFace(GL_CW);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	Time::startMillis = milliseconds();

	glfwSetFramebufferSizeCallback(window, ReshapeGL);
	ReshapeGL(window, scrW, scrH);
	glfwShowWindow(window);

	/*
	glfwSetCursorPosCallback(window, MotionGL);
	glfwSetMouseButtonCallback(window, MouseGL);
	glfwSetScrollCallback(window, MouseScrGL);

	glfwSetWindowFocusCallback(window, FocusGL);

	auto mills = milliseconds();
	*/
}

bool ChokoLait::alive() {
	return !glfwWindowShouldClose(window);
}

void ChokoLait::Update(emptyCallbackFunc func) {
	func();
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

	func();

	glfwSwapBuffers(window);
	glfwPollEvents();
}