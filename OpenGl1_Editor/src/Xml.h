#pragma once
#define _ITERATOR_DEBUG_LEVEL 0
#include <string>
#include <vector>

/*
This header is for parsing Doxygen output files.
It might not work as intended on other xml files.
*/

struct XmlTable {
	std::string name = "", value = "";
	std::vector<std::pair<std::string, std::string>> params;
	std::vector<XmlTable> children;
};

class Xml {
public:
	static XmlTable* Parse(const std::string& path);
};