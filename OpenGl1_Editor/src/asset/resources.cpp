#include "Engine.h"

std::vector<string> DefaultResources::names = std::vector<string>();
std::vector<char*> DefaultResources::datas = std::vector<char*>();
std::vector<uint> DefaultResources::sizes = std::vector<uint>();
void DefaultResources::Init(string path) {
	std::ifstream strm(path, std::ios::binary);
	if (!strm.is_open()) {
		Debug::Error("Default Resources", "fatal: cannot open default resources at " + path + "!");
		abort();
	}
	char c[100];
	while (!strm.eof()) {
		strm.getline(c, 100, 0);
		if (c[0] == 0) break;
		names.push_back(c);
		uint sz;
		_Strm2Val<uint>(strm, sz);
		sizes.push_back(sz);
		char* cc = new char[sz + 1];
		strm.read(cc, sz);
		cc[sz] = 0;
		datas.push_back(cc);
	}
	Debug::Message("Default Resources", "OK (" + std::to_string(names.size()) + " files loaded)");
}

string DefaultResources::GetStr(string name) {
	for (uint a = names.size(); a > 0; a--) {
		if (names[a - 1] == name) {
			return string(datas[a - 1]);
		}
	}
	Debug::Error("Default Resources", "Fatal: resource \"" + name + "\" missing!");
	return "";
}