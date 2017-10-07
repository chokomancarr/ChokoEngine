#include <iostream>
#include <fstream>
#include <string>
#include <vector>

void _Write(const void* val, std::ofstream* stream, int size) {
	stream->write(reinterpret_cast<char const *>(val), size);
}

int main(int argc, char **argv)
{
	if (argc < 1) return -1;
	std::string configPath = "";
	for (int a = 0; a < argc; a++) {
		configPath = std::string(argv[a]);
		if (configPath.length() > 8 && configPath.substr(configPath.length() - 7) == ".config") break;
		if (a == argc - 1) return -1;
	}
	std::cout << "reading " << configPath << std::endl;
	
	std::ifstream strm(configPath);
	if (!strm.is_open()) return -2;
	char c[100];
	std::string ofile = "";
	std::vector<std::string> ifiles;
	while (!strm.eof()) {
		strm.getline(c, 100);
		if (c[0] == '\0') continue;
		if (ofile == "") ofile = std::string(c);
		else ifiles.push_back(std::string(c));
	}
	std::cout << "writing " + std::to_string(ifiles.size()) + " files to " << ofile << std::endl;

	std::ofstream ostrm (ofile, std::ios::binary);
	for (std::string s : ifiles) {
		std::cout << " " << s << "... ";
		std::ifstream ifile(s, std::ios::binary);
		if (!ifile.is_open()) {
			std::cout << "FAIL" << std::endl;
			continue;
		}
		ostrm << s.substr(s.find_last_of('\\')+1) << (char)0;
		std::streampos pos = ostrm.tellp();
		ostrm << "0000" << ifile.rdbuf();
		std::streampos pos2 = ostrm.tellp();
		ostrm.seekp(pos);
		auto sz = (unsigned int)(pos2-pos-4);
		_Write(&sz, &ostrm, 4);
		ostrm.seekp(pos2);
		std::cout << "OK" << std::endl;
	}
	
	std::getline(std::cin, configPath);
	return 0;
}