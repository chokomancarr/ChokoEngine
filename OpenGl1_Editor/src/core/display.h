#pragma once
#include "Engine.h"

/*! Display window properties
[av] */
class Display {
public:
	static int width, height;
	const static uint dpi = 96;
	static glm::mat3 uiMatrix;

	static void Resize(int x, int y, bool maximize = false);

	friend int main(int argc, char **argv);
	//move all functions in here please
	friend void MouseGL(GLFWwindow* window, int button, int state, int mods);
	friend void MotionGL(GLFWwindow* window, double x, double y);
	friend void FocusGL(GLFWwindow* window, int focus);
	friend class PopupSelector;
	friend class ChokoLait;
	//protected:
	static NativeWindow* window;
};