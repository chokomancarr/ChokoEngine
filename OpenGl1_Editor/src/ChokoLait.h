#pragma once

#include "Defines.h"

#ifndef CHOKO_LAIT_BUILD
#define CHOKO_LAIT
#if defined(PLATFORM_WIN)
//#pragma comment(lib, "../bin/chokolait_win.lib")
#elif defined(PLATFORM_ADR)
//#pragma comment(lib, "../bin/chokolait_adr.so")
#endif
#pragma comment(lib, "legacy_stdio_definitions.lib")
#pragma comment(lib, "opengl32.lib")
#endif

#include "Engine.h"

typedef void(*emptyCallbackFunc)(void);

class ChokoLait {
public:
	static void Init(char** argv, int scrW, int scrH);

	static bool alive();

	static void Update(emptyCallbackFunc);
	static void Paint(emptyCallbackFunc);

protected:
	static GLFWwindow* window;
};