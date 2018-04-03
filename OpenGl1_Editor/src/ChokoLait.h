#pragma once

/*
ChokoLait Interface for ChokoEngine (c) Chokomancarr 2018

See https://chokomancarr.github.io/ChokoLait/ for documentation and examples.
*/

#include "Defines.h"

#if defined(__ANDROID__)
#define PLATFORM_ADR
#elif defined(__APPLE__)
#define PLATFORM_IOS
#endif

#ifndef CHOKO_LAIT_BUILD
#ifndef CHOKO_LAIT
#define CHOKO_LAIT
#endif
#ifndef TEST_LAIT
#if defined(PLATFORM_WIN)
#pragma comment(lib, "chokolait_win.lib")
#elif defined(PLATFORM_ADR)
//#pragma comment(lib, "chokolait_adr.so")
#endif
#pragma comment(lib, "opengl32.lib")
#endif
#endif

#define CHOKOLAIT_INIT_VARS ChokoLait __chokolait_instance;

#include "Engine.h"

typedef void(*emptyCallbackFunc)(void);

class ChokoLait {
public:
	ChokoLait() {
		if (!initd) {
#ifdef FEATURE_AV_CODECS
			av_register_all();
#endif
			_InitVars();
			initd = 1;
		}
	}

	static rCamera mainCamera;

	static void Init(int scrW, int scrH);

	static bool alive();

	static void Update(emptyCallbackFunc func = 0);
	static void Paint(emptyCallbackFunc rendFunc = 0, emptyCallbackFunc paintFunc = 0);

protected:
	static int initd;

	static GLFWwindow* window;

	static void _InitVars();

	static void MouseGL(GLFWwindow* window, int button, int state, int mods);
	static void MouseScrGL(GLFWwindow* window, double xoff, double yoff);
	static void MotionGL(GLFWwindow* window, double x, double y);
	static void ReshapeGL(GLFWwindow* window, int w, int h);
};