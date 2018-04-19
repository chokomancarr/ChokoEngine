#pragma once
#include "AssetObjects.h"

class DefaultResources { //loads binary created by DefaultResourcesBuild.exe
public:
	static void Init(string path);

	static string GetStr(string name);
	static std::vector<byte> GetBin(string name);
private:
	static std::vector<string> names;
	static std::vector<char*> datas;
	static std::vector<uint> sizes;
};