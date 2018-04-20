#include "Engine.h"
#include "Editor.h"
#if defined(PLATFORM_ADR)
#include <dirent.h>
#include <android/log.h>
#endif

void Debug::Message(string c, string s) {
#ifndef IS_EDITOR
	if (stream) *stream << "[i]" << c << ": " << s << std::endl;
#endif
#ifdef PLATFORM_ADR
	__android_log_print(ANDROID_LOG_INFO, "ChokoLait", &s[0]);
#else
	std::cout << "[i]" << c << ": " << s << std::endl;
#endif
}
void Debug::Warning(string c, string s) {
#ifndef IS_EDITOR
	if (stream) *stream << "[w]" << c << ": " << s << std::endl;
#else
	Editor::instance->_Warning(c, s);
#endif
#ifdef PLATFORM_ADR
	__android_log_print(ANDROID_LOG_WARN, "ChokoLait", &s[0]);
#endif
	std::cout << "[w]" << c << ": " << s << std::endl;
}
void Debug::Error(string c, string s) {
#ifndef IS_EDITOR
	if (stream) *stream << "[e]" << c << ": " << s << std::endl;
#endif
#ifdef PLATFORM_ADR
	__android_log_print(ANDROID_LOG_ERROR, "ChokoLait", &s[0]);
#endif
	std::cout << "[e]" << c << ": " << s << std::endl;
#ifdef PLATFORM_WIN
	__debugbreak();
	abort();
#endif
}

void Debug::DoDebugObjectTree(const std::vector<pSceneObject>& o, int i) {
	for (auto& oo : o) {
		string s("");
		for (int a = 0; a < i; a++)
			s += " ";
		s += "o \"" + oo->name + "\" (";
		for (auto& cc : oo->_components) {
			s += cc->name + ", ";
		}
		s += ")";
#ifndef IS_EDITOR
		*stream << s << std::endl;
#endif
		std::cout << s << std::endl;
		DoDebugObjectTree(oo->children, i + 1);
	}
}

void Debug::ObjectTree(const std::vector<pSceneObject>& o) {
	Message("ObjectTree", "Start");
	DoDebugObjectTree(o, 0);
	Message("ObjectTree", "End");
}

uint Debug::StackTrace(uint count, void** frames) {
	uint c;
#ifdef PLATFORM_WIN
	c = CaptureStackBackTrace(0, count, frames, NULL);
#elif defined(PLATFORM_LNX)
	c = backtrace(frames, count);
#endif
	return c;
}

/*
void Debug::InitStackTrace() {
#ifdef PLATFORM_WIN
winHandle = GetCurrentProcess();// glfwGetWin32Window(Display::window);
if (SymInitialize(winHandle, NULL, true)) {
Message("Debug", "Symbols loaded.");
}
else {
Message("Debug", "No symbols loaded.");
}
#endif
}

std::vector<string> Debug::StackTraceNames() {
uint count;
void** frames = StackTrace(&count);
auto names = std::vector<string>(count);
#ifdef PLATFORM_WIN
auto handle = GetCurrentProcess();
char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
PSYMBOL_INFO sym = (PSYMBOL_INFO)buffer;
sym->SizeOfStruct = sizeof(SYMBOL_INFO);
sym->MaxNameLen = MAX_SYM_NAME;
DWORD64 off = 0;
for (uint a = 0; a < count; a++) {
DWORD64 frame = (DWORD64)frames[a];
std::stringstream stream;
stream << std::hex << frame;
if (SymFromAddr(handle, frame, &off, sym)) {
names[a] = string((char*)sym->Name)
+ " (0x" + stream.str() + ")";
}
else {
names[a] = "0x" + stream.str() + " (no pdb loaded)";
}
}
#endif
return names;
}

void Debug::DumpStackTrace() {
auto res = Debug::StackTraceNames();
Message("Debug", "Dumping stack trace (" + std::to_string(res.size()) + " frames)...");
for (auto& r : res) {
Message("Debug", "   " + r);
}
}
*/

std::ofstream* Debug::stream = nullptr;
#ifdef PLATFORM_WIN
HANDLE Debug::winHandle = 0;
#endif
void Debug::Init(string s) {
#ifndef IS_EDITOR
	string ss = s + "/Log.txt";
	stream = new std::ofstream(ss.c_str(), std::ios::out | std::ios::trunc);
	Message("Debug", "Log Initialized");
#endif
}