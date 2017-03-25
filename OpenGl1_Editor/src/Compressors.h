#include <vector>
#include <iostream>
#include <fstream>

class Compressors {
public:
	static void Compress_LZW(unsigned char* input, unsigned int inSize, std::vector<unsigned char>& output);
	static void Decompress_LZW(std::ifstream& strm, std::vector<unsigned char>& output);
};