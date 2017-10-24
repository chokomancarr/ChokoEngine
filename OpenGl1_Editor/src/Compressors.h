#ifndef COMPRESS_H
#define COMPRESS_H

#include <vector>
#include <iostream>
#include <fstream>
#include "Engine.h"

class Compressors {
public:
	static void Compress_LZW(unsigned char* input, unsigned int inSize, std::vector<unsigned char>& output);
	static void Decompress_LZW(std::ifstream& strm, std::vector<unsigned char>& output);
	static std::vector<byte> Compress_Huffman(byte* input, uint inSize);
	static std::vector<byte> Decompress_Huffman(byte* input, uint inSize);
};

#endif