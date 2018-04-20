#include "makegen.h"

void makegen_scan(string path, string relpath, std::vector<string>& hs, std::vector<string>& cs) {
	auto files = IO::GetFiles(path);
	for (auto f : files) {
		if (f.substr(f.size() - 2) == ".h") hs.push_back(relpath + f.substr(f.find_last_of('\\') + 1));
		else if (f.substr(f.size() - 4) == ".cpp") cs.push_back(relpath + f.substr(f.find_last_of('\\') + 1));
	}
	std::vector<string> folds;
	IO::GetFolders(path, &folds);
	for (auto f : folds) {
		if (f != "." && f != "..") {
			makegen_scan(path + "/" + f, relpath + f + "/", hs, cs);
		}
	}
}

void makegen(const string& path) {
	std::ifstream istrm(path + "/Makefile.txt");
	std::ofstream ostrm(path + "/Makefile");

	std::vector<string> headers, cpps, objs;

	makegen_scan(path, "", headers, cpps);
	
	ostrm << istrm.rdbuf();
	ostrm << std::endl << "OBJS =";
	
	for (auto c : cpps) {
		auto t = "obj/" + c.substr(0, c.size() - 3) + "o";
		objs.push_back(t);
		ostrm << " " << t;
	}
	ostrm << std::endl << std::endl << "HEADERS =";
	for (auto h : headers) {
		ostrm << " " << h;
	}
	ostrm << std::endl << std::endl;
	ostrm << "../build/.out: $(OBJS)" << std::endl << "\t$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)" << std::endl << std::endl;
	for (uint i = 0; i < cpps.size(); i++) {
		ostrm << objs[i] << ": " << cpps[i] << " $(HEADERS)" << std::endl;
		ostrm << "\t$(CXX) $(CXXFLAGS) -c -o $@ " << cpps[i] << std::endl << std::endl;
	}
	ostrm << "clean:" << std::endl << "\trm -f *.o" << std::endl << "\trm -f ../build/.out";
}