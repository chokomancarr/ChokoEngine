#pragma once
#include "Engine.h"

/* cmake for msvc does not have to_string */
#if defined(PLATFORM_ADR)
namespace std {
	template <typename T> string to_string(T val) {
		std::ostringstream strm;
		strm << val;
		return strm.str();
	}

	int stoi(const string& s);
	float stof(const string& s);
	unsigned long stoul(const string& s);
}
#endif

namespace std {
	string to_string(Vec2 v);
	string to_string(Vec3 v);
	string to_string(Vec4 v);
	string to_string(Quat v);
}

std::vector<string> string_split(string s, char c, bool removeBlank = false);
int string_find(const string& s, const string& s2, int start = -1);

int TryParse(string str, int defVal);
uint TryParse(string str, uint defVal);
float TryParse(string str, float defVal);
