#include "Engine.h"
#include "Editor.h"

string IO::path = IO::InitPath();

std::vector<string> IO::GetFiles(const string& folder, string ext)
{
	if (folder == "") return std::vector<string>();
	std::vector<string> names;
#ifdef PLATFORM_WIN
	string search_path = folder + "/*" + ext;
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(folder + "\\" + fd.cFileName);
			}
		} while (FindNextFile(hFind, &fd));
		FindClose(hFind);
	}
#elif defined(PLATFORM_ADR)
	DIR* dir = opendir(&folder[0]);
	dirent* ep;
	do {
		string nm(ep->d_name);
		if (nm != "." && nm != "..")
			names.push_back(nm);
	} while (ep = readdir(dir));
#endif
	return names;
}

#ifdef IS_EDITOR
std::vector<EB_Browser_File> IO::GetFilesE(Editor* e, const string& folder)
{
	std::vector<EB_Browser_File> names;
	string search_path = folder + "/*.*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all importable files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				string aa(fd.cFileName);
				if ((aa.length() > 5 && aa.substr(aa.length() - 5) == ".meta") &&
					(aa.length() < 7 || aa.substr(aa.length() - 7) != ".h.meta"))
					names.push_back(EB_Browser_File(e, folder, aa.substr(0, aa.length() - 5), aa));
				else if ((aa.length() > 4 && aa.substr(aa.length() - 4) == ".cpp"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 2 && aa.substr(aa.length() - 2) == ".h"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 4 && aa.substr(aa.length() - 4) == ".txt"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 9 && (aa.substr(aa.length() - 9) == ".material")))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 7 && (aa.substr(aa.length() - 7) == ".effect")))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 9 && (aa.substr(aa.length() - 9) == ".animator")))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 9 && (aa.substr(aa.length() - 9) == ".animclip")))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
			}
		} while (FindNextFile(hFind, &fd));
		FindClose(hFind);
	}
	return names;
}
#endif

void IO::GetFolders(const string& folder, std::vector<string>* names, bool hidden)
{
#ifdef PLATFORM_WIN
	//std::vector<string> names;
	string search_path = folder + "/*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (hidden || !(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))) {
				names->push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
#endif
}

bool IO::HasDirectory(string szPath)
{
#ifdef PLATFORM_WIN
	DWORD dwAttrib = GetFileAttributes(&szPath[0]);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif
}

bool IO::HasFile(string szPath)
{
#ifdef PLATFORM_WIN
	DWORD dwAttrib = GetFileAttributes(&szPath[0]);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES);// && (dwAttrib & FILE_ATTRIBUTE_NORMAL));
#endif
}

string IO::ReadFile(const string& path) {
	std::ifstream stream(path.c_str());
	if (!stream.good()) {
		std::cout << "not found! " << path << std::endl;
		return "";
	}
	std::stringstream buffer;
	buffer << stream.rdbuf();
	return buffer.str();
}

#if defined(IS_EDITOR) || defined(PLATFORM_WIN)
std::vector<string> IO::GetRegistryKeys(HKEY key) {
	TCHAR    achKey[255];
	TCHAR    achClass[MAX_PATH] = TEXT("");
	DWORD    cchClassName = MAX_PATH;
	DWORD	 size;
	std::vector<string> res;

	if (RegQueryInfoKey(key, achClass, &cchClassName, NULL, &size, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
		DWORD cbName = 255;
		for (uint i = 0; i < size; i++) {
			if (RegEnumKeyEx(key, i, achKey, &cbName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
				res.push_back(achKey);
			}
		}
	}
	return res;
}

std::vector<std::pair<string, string>> IO::GetRegistryKeyValues(HKEY hKey, DWORD numValues)
{
	std::vector<std::pair<string, string>> vals;
	for (DWORD i = 0; i < numValues; i++)
	{
		char valueName[201];
		DWORD valNameLen = 200;
		DWORD dataType;
		byte data[501];
		DWORD dataSize = 500;

		auto val = RegEnumValue(hKey,
			i,
			valueName,
			&valNameLen,
			NULL,
			&dataType,
			data, &dataSize);

		if (!!val) break;
		vals.push_back(std::pair<string, string>(string(valueName), string((char*)data)));
	}

	return vals;
}
#endif

string IO::GetText(const string& path) {
	std::ifstream strm(path);
	std::stringstream ss;
	ss << strm.rdbuf();
	return ss.str();
}

string IO::InitPath() {
	//auto path2 = path;
	//if (path == "") {
	char cpath[200];
#ifdef PLATFORM_WIN
	GetModuleFileName(NULL, cpath, 200);
	string path2 = cpath;
	path2 = path2.substr(0, path2.find_last_of('\\') + 1);
#elif defined(PLATFORM_LNX)
	getcwd(cpath, 199);
	string path2 = cpath;
#endif
	path = path2;
	Debug::Message("IO", "Path set to " + path);
	//}
	return path2;
}