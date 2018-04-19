#pragma once
#include "Engine.h"

/*! File/folder reading/writing functions.
[av] */
class IO {
public:
	static std::vector<string> GetFiles(const string& path, string ext = "");
#ifdef IS_EDITOR
	static std::vector<EB_Browser_File> GetFilesE(Editor* e, const string& path);
#endif
	static void GetFolders(const string& path, std::vector<string>* names, bool hidden = false);
	static bool HasDirectory(string szPath);
	static bool HasFile(string szPath);
	static string ReadFile(const string& path);
#if defined(IS_EDITOR) || defined(PLATFORM_WIN)
	static std::vector<string> GetRegistryKeys(HKEY key);
	static std::vector<std::pair<string, string>> GetRegistryKeyValues(HKEY hKey, DWORD numValues = 5);
#endif
	static string GetText(const string& path);
	static std::vector<byte> GetBytes(const string& path);

	static string path;

	friend class ChokoLait;
	friend class Engine;
	//protected:
	static string InitPath();
};