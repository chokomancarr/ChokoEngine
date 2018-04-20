#include "Engine.h"

#ifdef PLATFORM_ADR
namespace std {
	int stoi(const string& s) {
		return 1;
	}
	float stof(const string& s) {
		return 1.0f;
	}
	unsigned long stoul(const string& s) {
		return 0UL;
	}
}
#endif

namespace std {
	string to_string(Vec2 v) {
		return "(" + to_string(v.x) + ", " + to_string(v.y) + ")";
	}
	string to_string(Vec3 v) {
		return "(" + to_string(v.x) + ", " + to_string(v.y) + ", " + to_string(v.z) + ")";
	}
	string to_string(Vec4 v) {
		return "(" + to_string(v.w) + ", " + to_string(v.x) + ", " + to_string(v.y) + ", " + to_string(v.z) + ")";
	}
	string to_string(Quat v) {
		return "(" + to_string(v.w) + ", " + to_string(v.x) + ", " + to_string(v.y) + ", " + to_string(v.z) + ")";
	}
}

std::vector<string> string_split(string s, char c, bool rm) {
	std::vector<string> o = std::vector<string>();
	size_t pos = -1;
	do {
		s = s.substr(pos + 1);
		pos = s.find_first_of(c);
		if (!rm || pos > 0)
			o.push_back(s.substr(0, pos));
	} while (pos != string::npos);
	return o;
}

int string_find(const string& s, const string& s2, int start) {
	uint ss = s.size();
	uint s2s = s2.size();
	int p = start;
	for (;;) {
		p = s.find_first_of(s2[0], p + 1);
		if (p == -1) return -1;
		if (s.substr(p, s2s) == s2) return p;
	}
	return -1;
}

int TryParse(string str, int defVal) {
	try {
		return std::stoi(str);
	}
	catch (...) {
		return defVal;
	}
}
uint TryParse(string str, uint defVal) {
	try {
		if (str[0] == '-') return 0;
		else return std::stoul(str);
	}
	catch (...) {
		return defVal;
	}
}
float TryParse(string str, float defVal) {
	try {
		return std::stof(str);
	}
	catch (...) {
		return defVal;
	}
}