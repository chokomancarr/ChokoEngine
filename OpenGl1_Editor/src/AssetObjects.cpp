#include "Engine.h"
#include "Editor.h"
#include "hdr.h"
#include <GL/glew.h>
#include <gl/GLUT.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <climits>
#include <shellapi.h>
#include <Windows.h>
#include <math.h>
#include <thread>
#include <jpeglib.h>
#include <jerror.h>
#include <lodepng.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Defines.h"
#include "SceneScriptResolver.h"

bool LoadJPEG(string fileN, uint &x, uint &y, byte& channels, byte** data)
{
	//unsigned int texture_id;
	unsigned long data_size;     // length
	unsigned char * rowptr[1];
	struct jpeg_decompress_struct info; //for our jpeg info
	struct jpeg_error_mgr err;          //the error handler

	FILE* file;
	fopen_s(&file, fileN.c_str(), "rb");  //open the file

	info.err = jpeg_std_error(&err);
	jpeg_create_decompress(&info);   //fills info structure

									 //if the jpeg file doesn't load
	if (!file) {
		return false;
	}

	jpeg_stdio_src(&info, file);
	jpeg_read_header(&info, TRUE);   // read jpeg file header

	jpeg_start_decompress(&info);    // decompress the file

	x = info.output_width;
	y = info.output_height;
	channels = info.num_components;

	data_size = x * y * 3;

	*data = (unsigned char *)malloc(data_size);
	while (info.output_scanline < y) // loop
	{
		// Enable jpeg_read_scanlines() to fill our jdata array
		rowptr[0] = (unsigned char *)*data + (3 * x * (y - info.output_scanline - 1));

		jpeg_read_scanlines(&info, rowptr, 1);
	}
	//---------------------------------------------------

	jpeg_finish_decompress(&info);   //finish decompressing
	jpeg_destroy_decompress(&info);
	fclose(file);
	return true;
}

//slow!!
void InvertPNG(std::vector<byte>& data, uint x, uint y) {
	for (uint a = 0; a <= y*0.5f; a++) {
		for (uint b = 0; b < x; b++) {
			for (uint c = 0; c < 4; c++) {
				byte t = data[(a*x + b) * 4 + c];
				data[(a*x + b) * 4 + c] = data[((y - a - 1)*x + b) * 4 + c];
				data[((y - a - 1)*x + b) * 4 + c] = t;
			}
		}
	}
}

bool LoadPNG(string fileN, uint &x, uint &y, byte& channels, std::vector<byte>& data) {
	channels = 4;
	uint err = lodepng::decode(data, x, y, fileN.c_str());
	if (err) {
		Debug::Error("PNG reader", "Read PNG error: " + string(lodepng_error_text(err)));
		return false;
	}
	InvertPNG(data, x, y);
	return true;
}

bool LoadBMP(string fileN, uint &x, uint &y, byte& channels, byte** data) {

	char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int imageSize;   // = width*height*3
	unsigned short bpi;

	std::ifstream strm(fileN.c_str(), std::ios::in | std::ios::binary);

	if (!strm.is_open()) {
		printf("Image could not be opened\n");
		return false;
	}
	strm.read(header, 54);
	if (strm.bad()) { // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return false;
	}
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return false;
	}
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	x = *(int*)&(header[0x12]);
	y = *(int*)&(header[0x16]);
	bpi = *(short*)&(header[0x1c]);
	if (bpi != 24 && bpi != 32)
		return false;
	else channels = (bpi == 24) ? 3 : 4;
	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = x * y * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	*data = new unsigned char[imageSize];
	// Read the actual data from the file into the buffer
	strm.read(*(char**)data, imageSize);
	return true;
}

Texture::Texture(const string& path) : Texture(path, true, TEX_FILTER_TRILINEAR, 5) {}
Texture::Texture(const string& path, bool mipmap) : Texture(path, mipmap, mipmap ? TEX_FILTER_TRILINEAR : TEX_FILTER_BILINEAR, 5) {}
Texture::Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso) : AssetObject(ASSETTYPE_TEXTURE), _mipmap(mipmap), _filter(filter), _aniso(aniso) {
	string sss = path.substr(path.find_last_of('.'), string::npos);
	byte *data;
	std::vector<byte> dataV;
	byte chn;
	//std::cout << "opening image at " << path << std::endl;
	GLenum rgb = GL_RGB, rgba = GL_RGBA;
	if (sss == ".bmp") {
		if (!LoadBMP(path, width, height, chn, &data)) {
			std::cout << "load bmp failed! " << path << std::endl;
			return;
		}
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	else if (sss == ".jpg") {
		if (!LoadJPEG(path, width, height, chn, &data)) {
			std::cout << "load jpg failed! " << path << std::endl;
			return;
		}
	}
	//*
	else if (sss == ".png") {
		if (!LoadPNG(path, width, height, chn, dataV)) {
			std::cout << "load png failed! " << path << std::endl;
			return;
		}
		data = &dataV[0];
	}
	//*/
	else {
		std::cout << "Image extension invalid! " << path << std::endl;
		return;
	}

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, data);
	else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
	if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap && (filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	glBindTexture(GL_TEXTURE_2D, 0);
	if (dataV.size() == 0) delete[](data);
	loaded = true;
	//std::cout << "image loaded: " << width << "x" << height << std::endl;
}

Texture::Texture(int i, Editor* e) : AssetObject(ASSETTYPE_TEXTURE) {
	string p = e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_TEXTURE][i] + ".meta";
	std::ifstream strm(p, std::ios::in | std::ios::binary);
	if (strm.is_open()) {
		byte chn;
		GLenum rgb = GL_RGB, rgba = GL_RGBA;
		byte* data;
		TEX_TYPE t = _ReadStrm(this, strm, chn, rgb, rgba);
		if (t == TEX_TYPE_RENDERTEXTURE) {
			((RenderTexture*)this)->Load(e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_TEXTURE][i]);
			return;
		}
		data = new byte[chn*width*height];
		strm.read((char*)data, chn*width*height);
		strm.close();

		glGenTextures(1, &pointer);
		glBindTexture(GL_TEXTURE_2D, pointer);
		if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, data);
		else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
		if (_mipmap) glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (_mipmap && (_filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _aniso);
		glBindTexture(GL_TEXTURE_2D, 0);
		delete[](data);
		loaded = true;
	}
}

Texture::Texture(std::ifstream& strm, uint offset) : AssetObject(ASSETTYPE_TEXTURE) {
	if (strm.is_open()) {
		strm.seekg(offset);
		byte chn;
		GLenum rgb = GL_RGB, rgba = GL_RGBA;
		byte* data;
		TEX_TYPE t = _ReadStrm(this, strm, chn, rgb, rgba);
		if (t == TEX_TYPE_RENDERTEXTURE) {
			((RenderTexture*)this)->Load(strm);
			return;
		}
		data = new byte[chn*width*height];
		strm.read((char*)data, chn*width*height);
		//strm.close();

		glGenTextures(1, &pointer);
		glBindTexture(GL_TEXTURE_2D, pointer);
		if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, data);
		else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
		if (_mipmap) glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (_mipmap && (_filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _aniso);
		glBindTexture(GL_TEXTURE_2D, 0);
		delete[](data);
		loaded = true;
	}
}

TEX_TYPE Texture::_ReadStrm(Texture* tex, std::ifstream& strm, byte& chn, GLenum& rgb, GLenum& rgba) {
	std::vector<char> hd(4);
	strm.read((&hd[0]), 3);
	if (hd[0] == 'I' && hd[1] == 'M' && hd[2] == 'R') {
		return TEX_TYPE_RENDERTEXTURE;
	}
	if (hd[0] != 'I' || hd[1] != 'M' || hd[2] != 'G') {
		Debug::Error("Image Cacher", "Image cache header wrong!");
		return TEX_TYPE_NORMAL;
	}
	byte bb;
	_Strm2Val<byte>(strm, chn);
	_Strm2Val<uint>(strm, tex->width);
	_Strm2Val<uint>(strm, tex->height);
	_Strm2Val<byte>(strm, bb);
	if (bb == 1) {
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	_Strm2Val<byte>(strm, tex->_aniso);
	_Strm2Val<byte>(strm, bb);
	tex->_filter = (TEX_FILTERING)bb;
	_Strm2Val<byte>(strm, bb);
	tex->_mipmap = (bb & 0xf0) == 0xf0;
	tex->_repeat = (bb & 0x0f) == 0x0f;
	strm.read((&hd[0]), 4);
	if (hd[0] != 'D' || hd[1] != 'A' || hd[2] != 'T' || hd[3] != 'A') {
		Debug::Error("Image Cacher", "Image cache data tag wrong!");
		return TEX_TYPE_NORMAL;
	}
	return TEX_TYPE_NORMAL;
}

//IMG [channels] [xxxx] [yyyy] [rgb=0, bgr=1] [aniso] [filter] [mipmap 0xf0 | repeat 0x0f] DATA [data]
bool Texture::Parse(Editor* e, string path) {
	byte ans = 5, flt = 2, mnr = 0xf0;
	string sss = path.substr(path.find_last_of('.'), string::npos);
	byte *data = nullptr;
	std::vector<byte> dataV;
	byte chn;
	uint width, height;
	GLenum rgb = GL_RGB, rgba = GL_RGBA;
	if (sss == ".bmp") {
		if (!LoadBMP(path, width, height, chn, &data)) {
			std::cout << "load bmp failed! " << path << std::endl;
			return false;
		}
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	else if (sss == ".jpg") {
		if (!LoadJPEG(path, width, height, chn, &data)) {
			std::cout << "load jpg failed! " << path << std::endl;
			return false;
		}
	}
	else if (sss == ".png") {
		if (!LoadPNG(path, width, height, chn, dataV)) {
			std::cout << "load png failed! " << path << std::endl;
			return false;
		}
		data = &dataV[0];
	}
	else {
		std::cout << "Image extension invalid! " << path << std::endl;
		return false;
	}
	if (data == nullptr)
		return false;
	string ss(path + ".meta");
	std::ifstream iStrm(ss, std::ios::in | std::ios::binary); //if exists, read old prefs
	if (iStrm.is_open()) {
		char* c = new char[16];
		iStrm.read(c, 16);
		if (c[0] == 'I' && c[1] == 'M' && c[2] == 'G') {
			byte* cb = (byte*)c;
			ans = cb[13];
			flt = cb[14];
			mnr = cb[15];
		}
		delete[](c);
	}
	iStrm.close();
	std::ofstream str(ss, std::ios::out | std::ios::trunc | std::ios::binary);
	str << "IMG";
	str << chn;
	_StreamWrite(&width, &str, 4);
	_StreamWrite(&height, &str, 4);
	str << (byte)((rgb == GL_RGB) ? 0 : 1);
	str << ans << flt << mnr;
	str << "DATA";
	_StreamWrite(data, &str, width*height*chn);
	str.close();
	if (dataV.size() == 0) delete[](data);
	return true;
}

void Texture::_ApplyPrefs(const string& p) {
	std::ifstream iStrm(p, std::ios::in | std::ios::binary | std::ios::ate);
	if (iStrm.is_open()) {
		uint sz((uint)iStrm.tellg());
		std::vector<byte> data(sz);
		iStrm.seekg(0);
		iStrm.read((char*)(&data[0]), sz);
		iStrm.close();

		data[13] = _aniso;
		data[14] = _filter;
		data[15] = (_mipmap ? 0xf0 : 0) | (_repeat ? 0x0f : 0);

		remove(p.c_str());
		std::ofstream strm(p, std::ios::out | std::ios::binary | std::ios::trunc);
		if (strm.is_open()) {
			strm.write((char*)(&data[0]), sz);
			strm.close();
			SetFileAttributes(p.c_str(), FILE_ATTRIBUTE_HIDDEN);
		}
	}
}

bool Texture::DrawPreview(uint x, uint y, uint w, uint h) {
	Engine::DrawTexture((float)x, (float)y, (float)w, (float)h, this, DrawTex_Fit);
	return true;
}

Background::Background(const string& path) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
	if (path.size() < 5 || path.substr(path.size() - 4, string::npos) != ".hdr") {
		printf("HDRI path invalid!");
		return;
	}
	//std::cout << "opening hdr image at " << path << std::endl;

	byte* data2 = hdr::read_hdr(path.c_str(), &width, &height);
	if (data2 == NULL)
		return;
	auto data = hdr2float(data2, width, height);
	delete[](data2);

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &data[0]);

	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;
	while (mips < 6 && height > 16) {
		//std::cout << "Downsampling " << mips << std::endl;
		mips++;
		data = Downsample(data, width_1, height_1, width_2, height_2);
		glTexImage2D(GL_TEXTURE_2D, mips, GL_RGB, width_2, height_2, 0, GL_RGB, GL_FLOAT, &data[0]);
		width_1 = width_2 + 0;
		height_1 = height_2 + 0;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
	//std::cout << "HDR Image loaded: " << width << "x" << height << std::endl;
}

void Background::B_DS(Background* b, std::vector<float> data) {
	uint width_1 = b->width, height_1 = b->height, width_2, height_2, mips = 0;
	while (mips < 6 && b->height > 16) {
		//std::cout << "Downsampling " << mips << std::endl;
		mips++;
		data = Downsample(data, width_1, height_1, width_2, height_2);

		glBindTexture(GL_TEXTURE_2D, b->pointer);

		glTexImage2D(GL_TEXTURE_2D, mips, GL_RGB, width_2, height_2, 0, GL_RGB, GL_FLOAT, &data[0]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
		glBindTexture(GL_TEXTURE_2D, 0);
		width_1 = width_2 + 0;
		height_1 = height_2 + 0;
	}
	b->loaded = true;
}

Background::Background(int i, Editor* editor) : width(0), height(0), AssetObject(ASSETTYPE_HDRI), loaded(false) {
	string path = editor->projectFolder + "Assets\\" + editor->normalAssets[ASSETTYPE_HDRI][i] + ".meta";
	std::ifstream strm(path.c_str(), std::ios::in | std::ios::binary);
	std::vector<char> hd(6);
	strm.read((&hd[0]), 4);
	if (hd[0] != 'I' || hd[1] != 'M' || hd[2] != 'G' || hd[3] != (char)4) {
		Debug::Error("HDR Cacher", "HDR cache header wrong!");
		return;
	}
	_Strm2Val<uint>(strm, width);
	_Strm2Val<uint>(strm, height);
	strm.read((&hd[0]), 5);
	if (hd[0] != (char)0 || hd[1] != 'D' || hd[2] != 'A' || hd[3] != 'T' || hd[4] != 'A') {
		Debug::Error("HDR Cacher", "Data tag missing!");
		return;
	}

	byte* data2 = new byte[width*height * 4];
	strm.read((char*)data2, width*height * 4);
	auto data = hdr2float(data2, width, height);
	delete[](data2);

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &data[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	B_DS(this, data);
	//thread t = thread(B_DS, this, data);
	//t.detach();
	//std::cout << "HDR Image loaded: " << width << "x" << height << std::endl;
}

Background::Background(std::ifstream& strm, uint offset) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
	std::vector<char> hd(6);
	strm.read((&hd[0]), 4);
	if (hd[0] != 'I' || hd[1] != 'M' || hd[2] != 'G' || hd[3] != (char)4) {
		Debug::Error("HDR Cacher", "HDR cache header wrong!");
		return;
	}
	_Strm2Val<uint>(strm, width);
	_Strm2Val<uint>(strm, height);
	strm.read((&hd[0]), 5);
	if (hd[0] != (char)0 || hd[1] != 'D' || hd[2] != 'A' || hd[3] != 'T' || hd[4] != 'A') {
		Debug::Error("HDR Cacher", "Data tag missing!");
		return;
	}

	byte* data2 = new byte[width*height * 4];
	strm.read((char*)data2, width*height * 4);
	auto data = hdr2float(data2, width, height);
	delete[](data2);

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &data[0]);

	uint width_1, height_1, mips = 0;
	while (mips < 6 && height > 16) {
		mips++;
		//std::cout << "Downsampling " << mips << std::endl;
		data = Downsample(data, width, height, width_1, height_1);
		glTexImage2D(GL_TEXTURE_2D, mips, GL_RGB, width_1, height_1, 0, GL_RGB, GL_FLOAT, &data[0]);
		width = width_1 + 0;
		height = height_1 + 0;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
	//std::cout << "HDR Image loaded: " << width << "x" << height << std::endl;
}

/*
std::vector<float> Background::Downsample(std::vector<float>& data, uint w, uint h, uint& w2, uint& h2) {
w2 = floor(w / 2);
h2 = floor(h / 2);
RenderTexture rt = RenderTexture(w2, h2, false);
GLuint pointer;
glGenTextures(1, &pointer);
glBindTexture(GL_TEXTURE_2D, pointer);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w2, h2, 0, GL_RGB, GL_FLOAT, &data[0]);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);
glUseProgram(Engine::blurProgram);
GLint loc = glGetUniformLocation(Engine::blurProgram, "screenSize");
GLint loc2 = glGetUniformLocation(Engine::blurProgram, "isY");
glUniform2f(loc, w2, h2);
glUniform1f(loc2, 0);
RenderTexture::Blit(pointer, &rt, Engine::blurProgram, "tex");
glDeleteTextures(1, &pointer);
glViewport(0, 0, Display::width, Display::height);
return rt.pixels();
}
*/
//*
std::vector<float> Background::Downsample(std::vector<float>& data, uint w, uint h, uint& w2, uint& h2) {
	if (w % 2 != 0) w--;
	if (h % 2 != 0) h--;
	w2 = w / 2;
	h2 = h / 2;

	//half the size
	std::vector<float> hImg(w2*h2 * 3);
	for (uint x = 0; x < w2; x++) {
		for (uint y = 0; y < h2; y++) {
			hImg[x * 3 + y * 3 * w2] = 0.25f*(data[x * 6 + y * 6 * w] + data[(x * 6 + 3) + y * 6 * w] + data[x * 6 + (y * 6 + 3) * w] + data[x * 6 + 3 + (y * 6 + 3) * w]);
			hImg[x * 3 + y * 3 * w2 + 1] = 0.25f*(data[x * 6 + y * 6 * w + 1] + data[(x * 6 + 3) + y * 6 * w + 1] + data[x * 6 + (y * 6 + 3) * w + 1] + data[x * 6 + 3 + (y * 6 + 3) * w + 1]);
			hImg[x * 3 + y * 3 * w2 + 2] = 0.25f*(data[x * 6 + y * 6 * w + 2] + data[(x * 6 + 3) + y * 6 * w + 2] + data[x * 6 + (y * 6 + 3) * w + 2] + data[x * 6 + 3 + (y * 6 + 3) * w + 2]);
		}
	}

	//sigma 5
	float kernal[21] = { 0.011f, 0.0164f, 0.023f, 0.031f, 0.04f, 0.05f, 0.06f, 0.07f, 0.076f, 0.08f, 0.0852f, 0.08f, 0.076f, 0.07f, 0.06f, 0.05f, 0.04f, 0.031f, 0.023f, 0.0164f, 0.011f };
	//blur 20 pixels x
	std::vector<float> xImg(w2*h2 * 3);
	for (uint x = 0; x < w2; x++) {
		for (uint y = 0; y < h2; y++) {
			for (uint a = 0; a < 21; a++) {
				int xx = x + (a - 10);
				if (xx < 0) xx = w2 + xx;
				else if ((uint)xx >= w2) xx -= w2;
				xImg[x * 3 + y * 3 * w2] += (hImg[xx * 3 + y * 3 * w2] * kernal[a]);
				xImg[x * 3 + y * 3 * w2 + 1] += (hImg[xx * 3 + y * 3 * w2 + 1] * kernal[a]);
				xImg[x * 3 + y * 3 * w2 + 2] += (hImg[xx * 3 + y * 3 * w2 + 2] * kernal[a]);
			}
		}
	}

	//blur 20 pixels y
	std::vector<float> oImg(w2*h2 * 3);
	for (uint x = 0; x < w2; x++) {
		for (uint y = 0; y < h2; y++) {
			for (uint a = 0; a < 21; a++) {
				int yy = y + (a - 10);
				int xx = w2 - x - 1;
				if (yy < 0) yy = -yy;
				else if ((uint)yy >= h2) yy = h2 - (yy - h2 + 1) - 1;
				else xx = x;
				oImg[x * 3 + y * 3 * w2] += (xImg[xx * 3 + yy * 3 * w2] * kernal[a]);
				oImg[x * 3 + y * 3 * w2 + 1] += (xImg[xx * 3 + yy * 3 * w2 + 1] * kernal[a]);
				oImg[x * 3 + y * 3 * w2 + 2] += (xImg[xx * 3 + yy * 3 * w2 + 2] * kernal[a]);
			}
		}
	}

	return oImg;
}
//*/

bool Background::Parse(string path) {
	uint width, height;
	byte* data = hdr::read_hdr(path.c_str(), &width, &height);
	if (data == NULL)
		return false;
	string ss(path + ".meta");
	std::ofstream str(ss, std::ios::out | std::ios::trunc | std::ios::binary);
	str << "IMG";
	str << (byte)4;
	_StreamWrite(&width, &str, 4);
	_StreamWrite(&height, &str, 4);
	str << (byte)0;
	str << "DATA";
	_StreamWrite(data, &str, width*height * 4);
	str.close();
	delete[](data);
	return true;
}

CubeMap::CubeMap(ushort size, bool mips, GLenum type, byte dataSize, GLenum format, GLenum dataType) : size(size), AssetObject(ASSETTYPE_HDRI), loaded(false) {
	if (size != 64 && size != 128 && size != 256 && size != 512 && size != 1024 && size != 2048) {
		Debug::Error("Cubemap", "CubeMaps must be sized POT between 64 and 2048! (" + to_string(size) + ")");
		abort();
	}
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pointer);
	//std::vector<byte> data = std::vector<byte>(size*size*dataSize, 0);
	glGenTextures(6, facePointers);
	for (byte aa = 0; aa < 6; aa++) {
		glBindTexture(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, facePointers[aa]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, 0, type, size, size, 0, format, dataType, NULL);
		if (mips) {
			for (byte aaa = 1; aaa < 7; aaa++) {
				facePointerMips[aa].push_back(0);
				glGenTextures(1, &facePointerMips[aa][aaa - 1]);
				glBindTexture(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, facePointers[aa]);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, aaa, type, size / (2 * aaa), size / (2 * aaa), 0, format, dataType, NULL);
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, GL_TEXTURE_MAX_LEVEL, 6);
		}
		else {
			glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + aa, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
}


CameraEffect::CameraEffect(Material* mat) : AssetObject(ASSETTYPE_CAMEFFECT) {
	material = mat;
}

CameraEffect::CameraEffect(string path) : AssetObject(ASSETTYPE_CAMEFFECT) {
	string p = Editor::instance->projectFolder + "Assets\\" + path;
	std::ifstream stream(p.c_str());
	if (!stream.good()) {
		std::cout << "cameffect not found!" << std::endl;
		return;
	}
	char* c = new char[4];
	stream.read(c, 3);
	c[3] = char0;
	string ss(c);
	if (ss != "KEF") {
		std::cerr << "file not supported" << std::endl;
		return;
	}
	delete[](c);
	char* nmm = new char[100];
	stream.getline(nmm, 100, char0);
	string shp(nmm);
	if (shp == "") {
		delete[](nmm);
		return;
	}
	ASSETTYPE t;
	Editor::instance->GetAssetInfo(shp, t, _material);
	material = _GetCache<Material>(ASSETTYPE_MATERIAL, _material);
	if (_material != -1) {
		stream.getline(nmm, 100, char0);
		mainTex = string(nmm);
	}
	delete[](nmm);
}

void CameraEffect::Save(string path) {
	std::ofstream strm(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	strm << "KEF";
	ASSETTYPE st;
	ASSETID si = Editor::instance->GetAssetId(material, st);
	_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_MATERIAL, si);
	if (si != -1) strm << mainTex << char0;
	strm.close();
}


//-----------------mesh class------------------------
Mesh::Mesh(Editor* e, int i) : AssetObject(ASSETTYPE_MESH) {
	Mesh* m2 = _GetCache<Mesh>(type, i);
	vertices = m2->vertices;
	vertexCount = m2->vertexCount;
	normals = m2->normals;
	tangents = m2->tangents;
	bitangents = m2->bitangents;
	triangles = m2->triangles;
	triangleCount = m2->triangleCount;
	boundingBox = m2->boundingBox;
	loaded = m2->loaded;
}

Mesh::Mesh(std::ifstream& stream, uint offset) : AssetObject(ASSETTYPE_MESH), loaded(false), vertexCount(0), triangleCount(0), materialCount(0) {
	if (stream.is_open()) {
		stream.seekg(offset);

		char* c = new char[100];
		stream.read(c, 6);
		c[6] = char0;
		if (string(c) != "KTO123") {
			Debug::Error("Mesh importer", "file not supported");
			return;
		}
		stream.getline(c, 100, 0);
		name += string(c);
		delete[](c);

		char cc;
		cc = stream.get();

		while (cc != 0) {
			if (cc == 'V') {
				_Strm2Val(stream, vertexCount);
				for (uint vc = 0; vc < vertexCount; vc++) {
					Vec3 v;
					_Strm2Val(stream, v.x);
					_Strm2Val(stream, v.y);
					_Strm2Val(stream, v.z);
					vertices.push_back(v);
					_Strm2Val(stream, v.x);
					_Strm2Val(stream, v.y);
					_Strm2Val(stream, v.z);
					normals.push_back(v);
				}
			}
			else if (cc == 'F') {
				_Strm2Val(stream, triangleCount);
				for (uint fc = 0; fc < triangleCount; fc++) {
					byte m;
					_Strm2Val(stream, m);
					while (materialCount <= m) {
						_matTriangles.push_back(std::vector<int>());
						materialCount++;
					}
					uint i;
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
					triangles.push_back(i);
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
					triangles.push_back(i);
					_Strm2Val(stream, i);
					_matTriangles[m].push_back(i);
					triangles.push_back(i);
				}
			}
			else if (cc == 'U') {
				byte c;
				_Strm2Val(stream, c);
				for (uint vc = 0; vc < vertexCount; vc++) {
					Vec2 i;
					_Strm2Val(stream, i.x);
					_Strm2Val(stream, i.y);
					uv0.push_back(i);
				}
				if (c > 1) {
					for (uint vc = 0; vc < vertexCount; vc++) {
						Vec2 i;
						_Strm2Val(stream, i.x);
						_Strm2Val(stream, i.y);
						uv1.push_back(i);
					}
				}
			}
			else {
				Debug::Error("Mesh Importer", "Unknown char: " + to_string(cc));
			}
			cc = stream.get();
		}

		if (vertexCount > 0) {
			if (normals.size() != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (normals incomplete)!");
				return;
			}
			else if (uv0.size() == 0)
				uv0.resize(vertexCount);
			else if (uv0.size() != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (uv0 incomplete)!");
				return;
			}
			if (uv1.size() == 0)
				uv1.resize(vertexCount);
			else if (uv1.size() != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (uv1 incomplete)!");
				return;
			}
			CalcTangents();
			RecalculateBoundingBox();
			loaded = true;
		}
		return;
	}
}

Mesh::Mesh(string path) : AssetObject(ASSETTYPE_MESH), loaded(false), vertexCount(0), triangleCount(0), materialCount(0) {
	string p = Editor::instance->projectFolder + "Assets\\" + path + ".mesh.meta";
	std::ifstream stream(p.c_str(), std::ios::in | std::ios::binary);
	if (!stream.good()) {
		std::cout << "mesh file not found!" << std::endl;
		return;
	}

	char* c = new char[100];
	stream.read(c, 6);
	c[6] = char0;
	if (string(c) != "KTO123") {
		Debug::Error("Mesh importer", "file not supported");
		return;
	}
	stream.getline(c, 100, 0);
	name += string(c);
	delete[](c);

	char cc;
	stream.read(&cc, 1);

	std::cout << path << std::endl;
	while (cc != 0) {
		if (cc == 'V') {
			_Strm2Val(stream, vertexCount);
			for (uint vc = 0; vc < vertexCount; vc++) {
				Vec3 v;
				_Strm2Val(stream, v.x);
				_Strm2Val(stream, v.y);
				_Strm2Val(stream, v.z);
				vertices.push_back(v);
				_Strm2Val(stream, v.x);
				_Strm2Val(stream, v.y);
				_Strm2Val(stream, v.z);
				normals.push_back(v);
			}
		}
		else if (cc == 'F') {
			_Strm2Val(stream, triangleCount);
			for (uint fc = 0; fc < triangleCount; fc++) {
				byte m;
				_Strm2Val(stream, m);
				while (materialCount <= m) {
					_matTriangles.push_back(std::vector<int>());
					materialCount++;
				}
				uint i;
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
				triangles.push_back(i);
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
				triangles.push_back(i);
				_Strm2Val(stream, i);
				_matTriangles[m].push_back(i);
				triangles.push_back(i);
			}
		}
		else if (cc == 'U') {
			byte c;
			_Strm2Val(stream, c);
			for (uint vc = 0; vc < vertexCount; vc++) {
				Vec2 i;
				_Strm2Val(stream, i.x);
				_Strm2Val(stream, i.y);
				uv0.push_back(i);
			}
			if (c > 1) {
				for (uint vc = 0; vc < vertexCount; vc++) {
					Vec2 i;
					_Strm2Val(stream, i.x);
					_Strm2Val(stream, i.y);
					uv1.push_back(i);
				}
			}
		}
		else {
			Debug::Error("Mesh Importer", "Unknown char: " + to_string(cc));
		}
		stream.read(&cc, 1);
	}

	if (vertexCount > 0) {
		if (normals.size() != vertexCount) {
			Debug::Error("Mesh Importer", "mesh metadata corrupted (normals incomplete)!");
			return;
		}
		else if (uv0.size() == 0)
			uv0.resize(vertexCount);
		else if (uv0.size() != vertexCount) {
			Debug::Error("Mesh Importer", "mesh metadata corrupted (uv0 incomplete)!");
			return;
		}
		if (uv1.size() == 0)
			uv1.resize(vertexCount);
		else if (uv1.size() != vertexCount) {
			Debug::Error("Mesh Importer", "mesh metadata corrupted (uv1 incomplete)!");
			return;
		}
		CalcTangents();
		RecalculateBoundingBox();
		loaded = true;
	}
}

void Mesh::RecalculateBoundingBox() {
	uint sz = vertices.size();
	if (sz == 0) {
		boundingBox = BBox();
		return;
	}
	boundingBox = BBox(vertices[0].x, vertices[0].x, vertices[0].x, vertices[0].x, vertices[0].x, vertices[0].x);
	for (uint i = 1; i < sz; i++) {
		Vec3& v = vertices[i];
		boundingBox.x0 = min(boundingBox.x0, v.x);
		boundingBox.x1 = max(boundingBox.x1, v.x);
		boundingBox.y0 = min(boundingBox.y0, v.y);
		boundingBox.y1 = max(boundingBox.y1, v.y);
		boundingBox.z0 = min(boundingBox.z0, v.z);
		boundingBox.z1 = max(boundingBox.z1, v.z);
	}
}

void Mesh::CalcTangents() {
	std::vector<bool> found(vertices.size());
	tangents = std::vector<Vec3>(vertices.size());
	bitangents = std::vector<Vec3>(vertices.size());
	for (uint a = 0; a < triangleCount; a++) {
		Vec2 u0, u1, u2;
		u0 = uv0[a * 3];
		u1 = uv0[a * 3 + 1];
		u2 = uv0[a * 3 + 2];
		if (u1 != u0 && u2 != u0) {
			if ((glm::normalize(u1 - u0) != glm::normalize(u2 - u0)) && (glm::normalize(u0 - u1) != glm::normalize(u2 - u0))) {
				if (!found[triangles[a * 3]]) {
					found[triangles[a * 3]] = true;
					Vec2 _v1 = u1 - u0;
					Vec2 _v2 = u2 - u0;
					float _b = 1.0f / (_v2.x - (_v1.x*_v2.y / _v1.y));
					float _a = -_b*_v2.y / _v1.y;
					tangents[a * 3] = Normalize(_a*(vertices[a * 3 + 1] - vertices[a * 3]) + _b*(vertices[a * 3 + 2] - vertices[a * 3]));
					_b = 1.0f / (_v2.y - (_v1.y*_v2.x / _v1.x));
					_a = -_b*_v2.x / _v1.x;
					bitangents[a * 3] = Normalize(_a*(vertices[a * 3 + 1] - vertices[a * 3]) + _b*(vertices[a * 3 + 2] - vertices[a * 3]));

					_v1 = u0 - u1;
					_v2 = u2 - u1;
					_b = 1.0f / (_v2.x - (_v1.x*_v2.y / _v1.y));
					_a = -_b*_v2.y / _v1.y;
					tangents[a * 3 + 1] = Normalize(_a*(vertices[a * 3] - vertices[a * 3 + 1]) + _b*(vertices[a * 3 + 2] - vertices[a * 3 + 1]));
					_b = 1.0f / (_v2.y - (_v1.y*_v2.x / _v1.x));
					_a = -_b*_v2.x / _v1.x;
					bitangents[a * 3 + 1] = Normalize(_a*(vertices[a * 3] - vertices[a * 3 + 1]) + _b*(vertices[a * 3 + 2] - vertices[a * 3 + 1]));

					_v1 = u1 - u2;
					_v2 = u0 - u2;
					_b = 1.0f / (_v2.x - (_v1.x*_v2.y / _v1.y));
					_a = -_b*_v2.y / _v1.y;
					tangents[a * 3 + 2] = Normalize(_a*(vertices[a * 3 + 1] - vertices[a * 3 + 2]) + _b*(vertices[a * 3] - vertices[a * 3 + 2]));
					_b = 1.0f / (_v2.y - (_v1.y*_v2.x / _v1.x));
					_a = -_b*_v2.x / _v1.x;
					bitangents[a * 3 + 2] = Normalize(_a*(vertices[a * 3 + 1] - vertices[a * 3 + 2]) + _b*(vertices[a * 3] - vertices[a * 3 + 2]));
				}
			}
		}
	}
}

bool Mesh::ParseBlend(Editor* e, string s) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	HANDLE stdOutR, stdOutW, stdInR, stdInW;
	if (!CreatePipe(&stdInR, &stdInW, &sa, 0)) {
		std::cout << "failed to create pipe for stdin!";
		return false;
	}
	if (!SetHandleInformation(stdInW, HANDLE_FLAG_INHERIT, 0)) {
		std::cout << "failed to set handle for stdin!";
		return false;
	}
	if (!CreatePipe(&stdOutR, &stdOutW, &sa, 0)) {
		std::cout << "failed to create pipe for stdout!";
		return false;
	}
	if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0)) {
		std::cout << "failed to set handle for stdout!";
		return false;
	}
	STARTUPINFO startInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startInfo, sizeof(STARTUPINFO));
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
	startInfo.cb = sizeof(STARTUPINFO);
	startInfo.hStdInput = stdInR;
	startInfo.hStdOutput = stdOutW;
	startInfo.dwFlags |= STARTF_USESTDHANDLES;

	//create meta directory
	string ss = s.substr(0, s.find_last_of('.'));
	string sss = ss + "_blend";
	if (!CreateDirectory(sss.c_str(), NULL)) {
		for (string file : IO::GetFiles(sss))
			DeleteFile(file.c_str());
	}
	SetFileAttributes(sss.c_str(), FILE_ATTRIBUTE_HIDDEN);
	string ms(s + ".meta");
	DeleteFile(ms.c_str());

	bool failed = true;
	string cmd1(e->_blenderInstallationPath.substr(0, 2) + "\n"); //root
	string cmd2("cd " + e->_blenderInstallationPath.substr(0, e->_blenderInstallationPath.find_last_of("\\")) + "\n");
	string cmd3("blender \"" + s + "\" --background --python \"" + e->dataPath + "\\Python\\blend_exporter.py\" -- \"" + s.substr(0, s.find_last_of('\\')) + "?" + ss.substr(ss.find_last_of('\\') + 1, string::npos) + "\"\n");
	//outputs object list, and meshes in subdir
	if (CreateProcess("C:\\Windows\\System32\\cmd.exe", 0, NULL, NULL, true, CREATE_NO_WINDOW, NULL, "D:\\TestProject\\", &startInfo, &processInfo) != 0) {
		std::cout << "executing Blender..." << std::endl;
		bool bSuccess = false;
		DWORD dwWrite;
		bSuccess = WriteFile(stdInW, cmd1.c_str(), cmd1.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			std::cout << "can't get to root!" << std::endl;
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd2.c_str(), cmd2.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			std::cout << "can't navigate to blender dir!" << std::endl;
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd3.c_str(), cmd3.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			std::cout << "can't execute blender!" << std::endl;
			return false;
		}
		DWORD w;
		bool finish = false;
		do {
			w = WaitForSingleObject(processInfo.hProcess, DWORD(200));
			DWORD dwRead;
			CHAR chBuf[4096];
			string out = "";
			bSuccess = ReadFile(stdOutR, chBuf, 4096, &dwRead, NULL) != 0;
			if (bSuccess && dwRead > 0) {
				string s(chBuf, dwRead);
				out += s;
			}
			for (uint r = 0; r < out.size();) {
				int rr = out.find_first_of('\n', r);
				if (rr == string::npos)
					rr = out.size() - 1;
				string sss = out.substr(r, rr - r);
				std::cout << sss << std::endl;
				r = rr + 1;
				if (sss.size() > 1 && sss[0] == '!')
					e->_Message("Blender", sss.substr(1, string::npos));
				if (sss.size() > 12 && sss.substr(0, 12) == "Blender quit") {
					TerminateProcess(processInfo.hProcess, 0);
					failed = false;
					finish = true;
				}
			}
		} while (w == WAIT_TIMEOUT && !finish);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		if (failed)
			return false;
	}
	else {
		std::cout << "Cannot start Blender!" << std::endl;
		CloseHandle(stdOutR);
		CloseHandle(stdOutW);
		CloseHandle(stdInR);
		CloseHandle(stdInW);
		return false;
	}
	CloseHandle(stdOutR);
	CloseHandle(stdOutW);
	SetFileAttributes(ms.c_str(), FILE_ATTRIBUTE_HIDDEN);
	return true;
}


AnimClip::AnimClip(string path) : AssetObject(ASSETTYPE_ANIMCLIP) {
	string p = Editor::instance->projectFolder + "Assets\\" + path + ".animclip.meta";
	std::ifstream stream(p.c_str(), std::ios::in | std::ios::binary);
	if (!stream.good()) {
		std::cout << "animclip file not found!" << std::endl;
		return;
	}

	char* c = new char[100];
	stream.read(c, 4);
	c[4] = char0;
	if (string(c) != "ANIM") {
		Debug::Error("AnimClip importer", "file not supported");
		return;
	}

	_Strm2Val(stream, curveLength);
	for (ushort aa = 0; aa < curveLength; aa++) {
		if (stream.eof()) {
			Debug::Error("AnimClip", "Unexpected eof");
			return;
		}
		curves.push_back(FCurve());
		_Strm2Val(stream, curves[aa].type);
		stream.getline(c, 100, 0);
		curves[aa].name += string(c);
		_Strm2Val(stream, curves[aa].keyCount);
		for (ushort bb = 0; bb < curves[aa].keyCount; bb++) {
			float ax, ay, bx, by, cx, cy;
			_Strm2Val(stream, ax);
			_Strm2Val(stream, ay);
			_Strm2Val(stream, bx);
			_Strm2Val(stream, by);
			_Strm2Val(stream, cx);
			_Strm2Val(stream, cy);
			curves[aa].keys.push_back(FCurve_Key(Vec2(ax, ay), Vec2(bx, by), Vec2(cx, cy)));
		}
		curves[aa].startTime += curves[aa].keys[0].point.x;
		curves[aa].endTime += curves[aa].keys[curves[aa].keyCount - 1].point.x;
	}
}

Animator::Animator() : AssetObject(ASSETTYPE_ANIMATOR), activeState(0), nextState(0), stateTransition(0), states(), transitions() {

}

Animator::Animator(string path) : Animator() {
	string p = Editor::instance->projectFolder + "Assets\\" + path;
	std::ifstream stream(p.c_str());
	if (!stream.good()) {
		std::cout << "animator not found!" << std::endl;
		return;
	}
	char* c = new char[4];
	stream.read(c, 3);
	c[3] = char0;
	string ss(c);
	if (ss != "ANT") {
		std::cerr << "file not supported" << std::endl;
		return;
	}
	byte stateCount, transCount;
	_Strm2Val(stream, stateCount);
	for (byte a = 0; a < stateCount; a++) {
		states.push_back(new Anim_State());
		_Strm2Val(stream, states[a]->isBlend);
		ASSETTYPE t;
		if (!states[a]->isBlend) {
			_Strm2Asset(stream, Editor::instance, t, states[a]->_clip);
		}
		else {
			byte bc;
			_Strm2Val(stream, bc);
			for (byte c = 0; c < bc; c++) {
				states[a]->_blendClips.push_back(-1);
				_Strm2Asset(stream, Editor::instance, t, states[a]->_blendClips[c]);
			}
		}
		_Strm2Val(stream, states[a]->speed);
		_Strm2Val(stream, states[a]->editorPos.x);
		_Strm2Val(stream, states[a]->editorPos.y);
	}
	_Strm2Val(stream, transCount);

}
