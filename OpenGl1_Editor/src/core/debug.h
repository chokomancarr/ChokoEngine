#pragma once
#include "Engine.h"

/*! Debugging functions. Output is saved in Log.txt beside the executable.
[av] */
class Debug {
public:
	static void Message(string c, string s);
	static void Warning(string c, string s);
	static void Error(string c, string s);
	static void ObjectTree(const std::vector<pSceneObject>& o);

	//*
	//static void InitStackTrace();
	static uint StackTrace(uint count, void** buffer);
	//static std::vector<string> StackTraceNames();
	//static void DumpStackTrace();
	//*/

	friend int main(int argc, char **argv);
	friend class ChokoLait;
protected:
	static std::ofstream* stream;
	static void Init(string path);

private:
	static void DoDebugObjectTree(const std::vector<pSceneObject>& o, int i);

#ifdef PLATFORM_WIN
	static HANDLE winHandle;
#endif
};