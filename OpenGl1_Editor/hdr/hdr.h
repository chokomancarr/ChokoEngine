#ifndef HDR_H
#define HDR_H

class hdr {
public:
	static const char* szSignature, *szFormat;
	static unsigned char * read_hdr(const char *filename, unsigned int *w, unsigned int *h);
};

#endif