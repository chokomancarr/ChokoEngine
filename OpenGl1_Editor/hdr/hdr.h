#pragma once

#define _ITERATOR_DEBUG_LEVEL 0
#include <iostream>
#include <fstream>
#include <vector>

class hdr {
public:
	static const char* szSignature, *szFormat;
	static unsigned char * read_hdr(const char *filename, unsigned int *w, unsigned int *h);
	static std::vector<float> to_float(unsigned char imagergbe[], int w, int h);
};