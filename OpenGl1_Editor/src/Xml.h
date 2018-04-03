#pragma once
#define _ITERATOR_DEBUG_LEVEL 0
#include "Engine.h"

struct XmlNode {
	std::string name = "", value = "";
	std::vector<std::pair<string, string>> params;
	std::vector<XmlNode> children;
};

class Xml {
public:
	static XmlNode* Parse(const string& path);
protected:
	static bool Read(string& s, uint& pos, XmlNode* parent);
};