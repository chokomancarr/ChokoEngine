#include "Xml.h"
#include <iostream>
#include <fstream>
#include "Engine.h"

uint _spacecount(char* c) {
	uint i = 0;
	while (*(c++) == ' ') i++;
	return i;
}

char* ParseSection(std::ifstream& strm, XmlTable* table, uint offset, char* c = 0) {
	char cc[500];
	std::string line;
	while (1) {
		if (!c) strm.getline(cc, 500, '\n');
		else {
			memcpy(cc, c, 500);
			delete[](c);
			c = 0;
		}
		if (strm.eof()) return 0;
		line = std::string(cc);
		auto sc = _spacecount(cc);
		if (sc != offset) {
			if (sc > offset) {
				c = new char[500];
				memcpy(c, cc, 500);
				if (sc - offset > 3) {
					table->children.back().children.push_back(XmlTable());
					c = ParseSection(strm, &table->children.back().children.back(), offset + 4, c);
				}
				else {
					table->children.push_back(XmlTable());
					c = ParseSection(strm, &table->children.back(), offset + 2, c);
				}
			}
			else {
				c = new char[500];
				memcpy(c, cc, 500);
				return c;
			}
		}
		else if (!!table->name.size() && line.substr(sc, 2) != "</") {
			c = new char[500];
			memcpy(c, cc, 500);
			table->children.push_back(XmlTable());
			c = ParseSection(strm, &table->children.back(), offset, c);
		}
		else {
			auto bp = line.find_first_of('>');
			bool nc = false;
			if (cc[bp - 1] == '/') {
				bp--;
				nc = true;
			}
			auto nm = line.substr(sc + 1, bp - sc - 1);
			auto ss = string_split(nm, ' ');
			auto ep = line.find('/');
			if (ep > sc + 1 && cc[ep + 1] != '>' && !table->name.size()) table->name = ss[0];
			for (uint ds = 1; ds < ss.size(); ds++) {
				auto eq = ss[ds].find('=');
				auto pn = ss[ds].substr(0, eq);
				auto pv = ss[ds].substr(eq + 2, ss[ds].size() - eq - 3);
				table->params.push_back(std::pair<std::string, std::string>(pn, pv));
			}
			if (ep != std::string::npos && cc[ep - 1] == '<') {
				table->value = line.substr(bp + 1, ep - bp - 2);
				return 0;
			}
			if (nc) return 0;
		}
	}
}

XmlTable* Xml::Parse(const std::string& path) {
	std::ifstream strm(path, std::ios::binary);
	if (!strm.is_open()) return nullptr;
	XmlTable* table = new XmlTable();
	char cc[500];
	strm.getline(cc, 500); //first line is garbage
	ParseSection(strm, table, 0);
	return table;
}