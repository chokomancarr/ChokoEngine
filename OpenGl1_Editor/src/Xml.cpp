#include "Xml.h"

void RemoveNewLine(string& s) {
	for (int a = s.size() - 1; a > 0; a--) {
		if (s[a] == '\t' || s[a] == '\r' || s[a] == '\n') {
			s.replace(a, 1, " ");
		}
	}
}

XmlNode* Xml::Parse(const string& path) {
	auto str = IO::GetText(path);
	RemoveNewLine(str);
	
	XmlNode* node = new XmlNode();
	uint pos = 0, sz = str.size();
	while (pos < sz) {
		if (!Read(str, pos, node)) {
			delete(node);
			return nullptr;
		}
	}
	return node;
}

bool Xml::Read(string& s, uint& pos, XmlNode* parent) {
	uint off, off2;
	off = s.find_first_of('<', pos);
	off2 = s.find_first_of('>', off);
	if (off2 < off) return false;
	if (off == string::npos) {
		pos = s.size();
		return true;
	}
	pos = off2 + 1;
	if (s[off + 1] == '?' && s[off2 - 1] == '?') {
		return true;
	}

	XmlNode n = {};
	auto ss = string_split(s.substr(off + 1, off2 - off - 1), ' ');
	if (!ss.size()) return false;
	n.name = ss[0];
	for (uint a = 1; a < ss.size(); a++) {
		if (ss[a] == "") continue;
		auto ss2 = string_split(ss[a], '=');
		n.params.push_back(std::pair<string, string>(ss2[0], ss2[1].substr(1, ss2[1].size() - 2)));
	}

	if (s[off2-1] == '/') {
		auto& lp = n.params.back().second;
		lp = lp.substr(0, lp.size()-1);
	}
	else {
		auto ep = string_find(s, "</" + n.name + ">", off2 + 1);
		if (ep == -1) return false;
		if (s[off2 + 1] != ' ') {
			n.value = s.substr(off2 + 1, ep - off2 - 1);
		}
		else {
			while (pos < (uint)ep) {
				std::cout << pos << " " << ep << std::endl;
				Read(s, pos, &n);
			}
		}
		pos = ep + n.name.size() + 2;
	}
	parent->children.push_back(n);
	return true;
}