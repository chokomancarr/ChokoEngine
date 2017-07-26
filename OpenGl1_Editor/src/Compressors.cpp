#include "Compressors.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

int IndexInDic (std::string& str, std::vector<std::string>& dic) {
    for (unsigned short x = dic.size(); x > 0; x--) {
        if (strcmp(dic[x-1].c_str(), str.c_str()) == 0)
            return x-1;
    }
    return -1;
}

void Compressors::Compress_LZW (unsigned char* input, unsigned int inSize, std::vector<unsigned char>& output) {
    output.clear();
	std::vector<std::string> dictionary = std::vector<std::string>();
	dictionary.push_back(""); //0=end
	for (unsigned short x = 0; x < 256; x++) {
		char xx[] = { (char)x, '\0' };
		dictionary.push_back(std::string(xx));
	}
	unsigned int outSize = 0;

    bool odd = false;
    std::string buffer = "";
	for (unsigned int a = 0; a < inSize; a++) {
		char xx[] = { (char)input[a], '\0' };
		std::string k = std::string(xx);
        int i = IndexInDic(buffer + k, dictionary);
        if (i > -1) {
            buffer += k;
			if (a == inSize - 1) {
				unsigned short o = (unsigned short)IndexInDic(buffer, dictionary);
				//cout << hex << o << endl;
				output.push_back((unsigned char)(o & 255));
				if (!odd) {
					output.push_back((unsigned char)((o & (15 << 8)) >> 4));
					outSize++;
				}
				else
					output[outSize - 1] |= ((unsigned char)(o >> 8));
				outSize++;
				odd = !odd;
			}
        }
        else {
			unsigned short o = (unsigned short)IndexInDic(buffer, dictionary);
			//cout << hex << o << endl;
			output.push_back((unsigned char)(o & 255));
			if (!odd) {
				output.push_back((unsigned char)((o & (15 << 8)) >> 4));
				outSize++;
			}
			else
				output[outSize - 1] |= ((unsigned char)(o >> 8));
			odd = !odd;
			outSize++;

			dictionary.push_back(buffer + k);
			buffer = k;
        }
    }

	output.push_back((unsigned char)(0));
	outSize++;
	if (!odd) {
		output.push_back((unsigned char)(0));
		outSize++;
	}

	//for (std::string sss : dictionary)
		//cout << sss << endl;
}

void Compressors::Decompress_LZW(std::ifstream& strm, std::vector<unsigned char>& output) {
	std::string buffer = "";
	std::vector<std::string> dictionary = std::vector<std::string>();
	dictionary.push_back(""); //0=end
	for (unsigned short x = 0; x < 256; x++) {
		char xx[] = { (char)x, '\0' };
		dictionary.push_back(std::string(xx));
	}
	unsigned short dicSize = 257;
	output.clear();
	unsigned short prevCode, currCode;
	bool isOdd = true;
	unsigned char byte0, byte1;
	std::string s = "";

	byte0 = strm.get();
	byte1 = strm.get();
	prevCode = byte0 | ((byte1 & 0xf0) >> 4);
	s = dictionary[prevCode];
	for (auto c : s) {
		output.push_back((unsigned char)c);
	}

	while (!strm.eof()) {
		if (!isOdd) {
			byte0 = strm.get();
			byte1 = strm.get();
			currCode = byte0 | (((unsigned short)(byte1 & 0xf0)) << 4);
		}
		else {
			byte0 = strm.get();
			currCode = byte0 | (((unsigned short)(byte1 & 0x0f)) << 8);
		}
		isOdd = !isOdd;
		//cout << hex << currCode << endl;
		if (currCode == 0) return;

		if (currCode == dicSize) {
			s = dictionary[prevCode];
			s = s + s.substr(0, 1);
			for (auto c : s) {
				output.push_back((unsigned char)c);
			}
			dictionary.push_back(s);
		}
		else {
			s = dictionary[currCode];
			for (auto c : s) {
				output.push_back((unsigned char)c);
			}
			dictionary.push_back(dictionary[prevCode] + s.substr(0, 1));
		}
		prevCode = currCode + 0;
		dicSize++;
	}
}