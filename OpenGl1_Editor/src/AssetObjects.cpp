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

using namespace ChokoEngine;

#define F2ISTREAM(_varname, _pathname) std::ifstream _f2i_ifstream(_pathname.c_str(), std::ios::in | std::ios::binary); \
std::istream _varname(_f2i_ifstream.rdbuf());

std::stringstream StreamFromBuffer(const std::vector<char>& buf) {
	std::stringstream strm;
	strm.write(&buf[0], buf.size());
	return strm;
}

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

Texture::Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso, TEX_WARPING warp) : AssetObject(ASSETTYPE_TEXTURE), _mipmap(mipmap), _filter(filter), _aniso(aniso) {
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (warp == TEX_WARP_CLAMP) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (warp == TEX_WARP_CLAMP) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	if (dataV.size() == 0) delete[](data);
	loaded = true;
	//std::cout << "image loaded: " << width << "x" << height << std::endl;
}

/*
Texture::Texture(int i, Editor* e) : AssetObject(ASSETTYPE_TEXTURE) {
	string p = e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_TEXTURE][i] + ".meta";
	std::ifstream strm(p, std::ios::in | std::ios::binary);
	//if (strm.is_open()) {
		byte chn;
		GLenum rgb = GL_RGB, rgba = GL_RGBA;
		byte* data;
		TEX_TYPE t = _ReadStrm(this, strm, chn, rgb, rgba);
		if (t == TEX_TYPE_UNDEF) {
			Debug::Error("Texture", "Texture header wrong/missing!");
			return;
		}
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
	//}
}
*/
Texture::Texture(int i, Editor* e) : Texture(std::ifstream(e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_TEXTURE][i] + ".meta", std::ios::in | std::ios::binary), 0) {}

Texture::Texture(std::istream& strm, uint offset) : AssetObject(ASSETTYPE_TEXTURE) {
	if (strm.good()) {
		strm.seekg(offset);
		byte chn;
		GLenum rgb = GL_RGB, rgba = GL_RGBA;
		byte* data;
		TEX_TYPE t = _ReadStrm(this, strm, chn, rgb, rgba);
		if (t == TEX_TYPE_UNDEF) {
			Debug::Error("Texture", "Texture header wrong/missing!");
			return;
		}
		if (t == TEX_TYPE_RENDERTEXTURE) {
			((RenderTexture*)this)->Load(strm);
			return;
		}
		data = new byte[chn*width*height];
		strm.read((char*)data, chn*width*height);
		//strm.close();

		uint mips = 0;
		std::vector<RenderTexture*> rts = std::vector<RenderTexture*>();
		std::vector<Vec2> szs = std::vector<Vec2>();
		if (_mipmap && _blurmips) {
			uint width_1 = width, height_1 = height, width_2, height_2;

			ShaderBase shd = ShaderBase(Camera::d_blurProgram);
			Material mat = Material(&shd);

			rts.push_back(new RenderTexture(width, height, RT_FLAG_NONE, data, GL_RGBA));
			szs.push_back(Vec2(width, height));

			while (mips < 6 && height_1 > 16) {
				width_2 = width_1 / 2;
				height_2 = height_1 / 2;

				mat.SetFloat("mul", 1-mips*0.1f);
				mat.SetFloat("mul", 1);
				mat.SetVec2("screenSize", Vec2(width_2, height_2));
				RenderTexture rx = RenderTexture(width_2, height_2, RT_FLAG_HDR);
				mat.SetFloat("isY", 0);
				RenderTexture::Blit(rts[mips], &rx, &mat);
				rts.push_back(new RenderTexture(width_2, height_2, RT_FLAG_HDR));
				mat.SetFloat("isY", 1);
				RenderTexture::Blit(&rx, rts[mips + 1], &mat);

				szs.push_back(Vec2(width_2, height_2));
				width_1 = width_2 + 0;
				height_1 = height_2 + 0;
				mips++;
			}
		}

		glGenTextures(1, &pointer);
		glBindTexture(GL_TEXTURE_2D, pointer);
		if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, data);
		else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
		if (_mipmap) {
			if (_blurmips) {
				for (uint a = 0; a <= mips; a++) {
					glBindFramebuffer(GL_READ_FRAMEBUFFER, rts[a]->d_fbo);
					glCopyTexImage2D(GL_TEXTURE_2D, a, GL_RGBA, 0, 0, (GLsizei)szs[a].x, (GLsizei)szs[a].y, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
					delete(rts[a]);
				}
				
			}
			else glGenerateMipmap(GL_TEXTURE_2D);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (_mipmap && (_filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _aniso);
		glBindTexture(GL_TEXTURE_2D, 0);
		delete[](data);
		loaded = true;
	}
}

TEX_TYPE Texture::_ReadStrm(Texture* tex, std::istream& strm, byte& chn, GLenum& rgb, GLenum& rgba) {
	std::vector<char> hd(4);
	strm.read((&hd[0]), 3);
	if (hd[0] == 'I' && hd[1] == 'M' && hd[2] == 'R') {
		return TEX_TYPE_RENDERTEXTURE;
	}
	else if (hd[0] != 'I' || hd[1] != 'M' || hd[2] != 'G') {
		Debug::Error("Image Cacher", "Image cache header wrong!");
		return TEX_TYPE_UNDEF;
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
	tex->_mipmap = !!(bb & 0x80);
	tex->_repeat = !!(bb & 0x01);
	tex->_blurmips = !(bb & 0x10);
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
		data[15] = (_mipmap ? 0x80 : 0) | (_repeat ? 0x01 : 0) | (_blurmips ? 0 : 0x10);

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

	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;

	ShaderBase shd = ShaderBase(Camera::d_blurSBProgram);
	Material mat = Material(&shd);

	std::vector<RenderTexture*> rts = std::vector<RenderTexture*>();
	std::vector<Vec2> szs = std::vector<Vec2>();
	rts.push_back(new RenderTexture(width, height, RT_FLAG_HDR, &data[0], GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT));
	szs.push_back(Vec2(width, height));

	while (mips < 6 && height_1 > 16) {
		width_2 = width_1 / 2;
		height_2 = height_1 / 2;

		mat.SetVec2("screenSize", Vec2(width_2, height_2));
		mat.SetFloat("mul", 1 + mips*0.1f);
		RenderTexture rx = RenderTexture(width_2, height_2, RT_FLAG_HDR, NULL, GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT);
		mat.SetFloat("isY", 0);
		RenderTexture::Blit(rts[mips], &rx, &mat);
		rts.push_back(new RenderTexture(width_2, height_2, RT_FLAG_HDR, NULL, GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT));
		mat.SetFloat("isY", 1);
		RenderTexture::Blit(&rx, rts[mips + 1], &mat);

		szs.push_back(Vec2(width_2, height_2));
		width_1 = width_2 + 0;
		height_1 = height_2 + 0;
		mips++;
	}

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	for (uint a = 0; a <= mips; a++) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, rts[a]->d_fbo);
		glCopyTexImage2D(GL_TEXTURE_2D, a, GL_RGB, 0, 0, (GLsizei)szs[a].x, (GLsizei)szs[a].y, 0);
		delete(rts[a]);
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
}

Background::Background(int i, Editor* editor) : width(0), height(0), AssetObject(ASSETTYPE_HDRI), loaded(false) {
	string path = editor->projectFolder + "Assets\\" + editor->normalAssets[ASSETTYPE_HDRI][i] + ".meta";
	//std::ifstream strm(path.c_str(), std::ios::in | std::ios::binary);
	F2ISTREAM(strm, path);
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

	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;

	ShaderBase shd = ShaderBase(Camera::d_blurSBProgram);
	Material mat = Material(&shd);

	std::vector<RenderTexture*> rts = std::vector<RenderTexture*>();
	std::vector<Vec2> szs = std::vector<Vec2>();
	rts.push_back(new RenderTexture(width, height, RT_FLAG_HDR, &data[0], GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT));
	szs.push_back(Vec2(width, height));

	while (mips < 6 && height_1 > 16) {
		width_2 = width_1 / 2;
		height_2 = height_1 / 2;

		mat.SetVec2("screenSize", Vec2(width_2, height_2));
		mat.SetFloat("mul", 1 + mips*0.1f);
		RenderTexture rx = RenderTexture(width_2, height_2, RT_FLAG_HDR, NULL, GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT);
		mat.SetFloat("isY", 0);
		RenderTexture::Blit(rts[mips], &rx, &mat);
		rts.push_back(new RenderTexture(width_2, height_2, RT_FLAG_HDR, NULL, GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT));
		mat.SetFloat("isY", 1);
		RenderTexture::Blit(&rx, rts[mips+1], &mat);

		szs.push_back(Vec2(width_2, height_2));
		width_1 = width_2 + 0;
		height_1 = height_2 + 0;
		mips++;
	}

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	for (uint a = 0; a <= mips; a++) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, rts[a]->d_fbo);
		glCopyTexImage2D(GL_TEXTURE_2D, a, GL_RGB, 0, 0, (GLsizei)szs[a].x, (GLsizei)szs[a].y, 0);
		delete(rts[a]);
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
}

Background::Background(std::istream& strm, uint offset) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
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

	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;

	ShaderBase shd = ShaderBase(Camera::d_blurSBProgram);
	Material mat = Material(&shd);

	std::vector<RenderTexture*> rts = std::vector<RenderTexture*>();
	std::vector<Vec2> szs = std::vector<Vec2>();
	rts.push_back(new RenderTexture(width, height, RT_FLAG_HDR, &data[0], GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT));
	szs.push_back(Vec2(width, height));

	while (mips < 6 && height_1 > 16) {
		width_2 = width_1 / 2;
		height_2 = height_1 / 2;

		mat.SetVec2("screenSize", Vec2(width_2, height_2));
		mat.SetFloat("mul", 1 + mips*0.1f);
		RenderTexture rx = RenderTexture(width_2, height_2, RT_FLAG_HDR, NULL, GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT);
		mat.SetFloat("isY", 0);
		RenderTexture::Blit(rts[mips], &rx, &mat);
		rts.push_back(new RenderTexture(width_2, height_2, RT_FLAG_HDR, NULL, GL_RGB, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_MIRRORED_REPEAT));
		mat.SetFloat("isY", 1);
		RenderTexture::Blit(&rx, rts[mips + 1], &mat);

		szs.push_back(Vec2(width_2, height_2));
		width_1 = width_2 + 0;
		height_1 = height_2 + 0;
		mips++;
	}

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	for (uint a = 0; a <= mips; a++) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, rts[a]->d_fbo);
		glCopyTexImage2D(GL_TEXTURE_2D, a, GL_RGB, 0, 0, (GLsizei)szs[a].x, (GLsizei)szs[a].y, 0);
		delete(rts[a]);
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
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
/*
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
*/

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


//-------------------Material class--------------
Material::Material() : _shader(-1), AssetObject(ASSETTYPE_MATERIAL), writeMask(4, true) {
	shader = nullptr;// Engine::unlitProgram;
}

Material::Material(ShaderBase * shad) : _shader(-1), AssetObject(ASSETTYPE_MATERIAL), writeMask(4, true) {
	shader = shad;
	if (shad == nullptr)
		return;
	if (Editor::instance != nullptr)
		_shader = Editor::instance->GetAssetId(shader);
	_ReloadParams();
}

ShaderBase* Material::Shader() {
	return shader;
}
void Material::Shader(ShaderBase* shad) {
	shader = shad;
	_ReloadParams();
}

void Material::_ReloadParams() {
	ResetVals();
	for (ShaderVariable* v : shader->vars) {
		void* l = nullptr;
		if (v->type == SHADER_INT)
			l = new int();
		else if (v->type == SHADER_FLOAT)
			l = new float();
		else if (v->type == SHADER_SAMPLER) {
			l = new MatVal_Tex();
			((MatVal_Tex*)l)->defTex = defTexs[v->def.i];
		}
		valNames[v->type].push_back(v->name);
		GLint ii = glGetUniformLocation(shader->pointer, v->name.c_str());
		if (ii > -1) {
			vals[v->type].emplace(ii, l);
			valOrders.push_back(v->type);
			valOrderIds.push_back((byte)(valNames[v->type].size() - 1));
			valOrderGLIds.push_back(ii);
		}
		else
			Editor::instance->_Warning("Material", "Shader parameter " + v->name + " not used!");
	}
}

Material::Material(string p) : AssetObject(ASSETTYPE_MATERIAL), writeMask(4, true) {
	//string p = Editor::instance->projectFolder + "Assets\\" + path;
	//std::ifstream stream(p.c_str());
	F2ISTREAM(stream, p);
	if (!stream.good()) {
		std::cout << "material not found!" << std::endl;
		return;
	}
	char* c = new char[4];
	stream.read(c, 3);
	c[3] = (char)0;
	string ss(c);
	if (ss != "KTC") {
		std::cerr << "file not supported" << std::endl;
		return;
	}
	delete[](c);
	char* nmm = new char[100];
	stream.getline(nmm, 100, (char)0);
	string shp(nmm);
	if (shp == "") {
		delete[](nmm);
		return;
	}
	ASSETTYPE t;
	Editor::instance->GetAssetInfo(shp, t, _shader);
	shader = _GetCache<ShaderBase>(ASSETTYPE_SHADER, _shader);

	if (shader == nullptr) {
		delete[](nmm);
		return;
	}
	ResetVals();
	std::unordered_map<string, GLint> nMap;
	for (ShaderVariable* v : shader->vars) {
		void* l = nullptr;
		if (v->type == SHADER_INT)
			l = new int();
		else if (v->type == SHADER_FLOAT)
			l = new float();
		else if (v->type == SHADER_SAMPLER) {
			l = new MatVal_Tex();
			((MatVal_Tex*)l)->defTex = defTexs[v->def.i];
		}
		valNames[v->type].push_back(v->name);
		GLint loc = glGetUniformLocation(shader->pointer, v->name.c_str());
		if (loc > -1) {
			vals[v->type].emplace(loc, l);
			nMap.emplace(v->name, loc);
			valOrders.push_back(v->type);
			valOrderIds.push_back((byte)(valNames[v->type].size() - 1));
			valOrderGLIds.push_back(loc);
		}
		else
			Editor::instance->_Warning("Material", "Shader parameter " + v->name + " not used!");
	}

	int vs;
	_Strm2Val(stream, vs);
	for (int r = 0; r < vs; r++) {
		char ii;
		stream >> ii;
		nmm = new char[100];
		stream.getline(nmm, 100, (char)0);
		string nm(nmm);
		switch (ii) {
		case SHADER_FLOAT:
			for (int x = vals[SHADER_FLOAT].size() - 1; x >= 0; x--) {
				if (valNames[SHADER_FLOAT][x] == nm) {
					float f;
					_Strm2Val(stream, f);
					(*(float*)vals[SHADER_FLOAT][nMap[nm]]) += f;
					break;
				}
			}
			break;
		case SHADER_INT:
			for (int x = vals[SHADER_INT].size() - 1; x >= 0; x--) {
				if (valNames[SHADER_INT][x] == nm) {
					int f;
					_Strm2Val(stream, f);
					(*(int*)vals[SHADER_INT][nMap[nm]]) += f;
					break;
				}
			}
			break;
		case SHADER_SAMPLER:
			for (int x = vals[SHADER_SAMPLER].size() - 1; x >= 0; x--) {
				if (valNames[SHADER_SAMPLER][x] == nm) {
					char* nmm2 = new char[100];
					stream.getline(nmm2, 100, (char)0);
					string nm2(nmm2);
					ASSETTYPE t;
					if (vals[SHADER_SAMPLER][nMap[nm]] == nullptr) {
						delete[](nmm2);
						break;
					}
					Editor::instance->GetAssetInfo(nm2, t, ((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id);
					((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, ((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id);
					delete[](nmm2);
					break;
				}
			}
			break;
		}
	}
	delete[](nmm);
	//stream.close();
}

Material::Material(std::istream& stream, uint offset) : AssetObject(ASSETTYPE_MATERIAL), writeMask(4, true) {
	if (stream.good()) {
		stream.seekg(offset);
		char* c = new char[4];
		stream.read(c, 3);
		c[3] = (char)0;
		string ss(c);
		if (ss != "KTC") {
			std::cerr << "file not supported" << std::endl;
			return;
		}
		delete[](c);
		char* nmm = new char[100];
		stream.getline(nmm, 100, (char)0);
		string shp(nmm);
		if (shp == "") {
			delete[](nmm);
			return;
		}
		offset = (uint)stream.tellg();
		ASSETTYPE t;
		_shader = AssetManager::GetAssetId(shp, t);
		shader = _GetCache<ShaderBase>(ASSETTYPE_SHADER, _shader);
		stream.seekg(offset);

		if (shader == nullptr) {
			delete[](nmm);
			return;
		}
		ResetVals();
		std::unordered_map<string, GLint> nMap;
		for (ShaderVariable* v : shader->vars) {
			void* l = nullptr;
			if (v->type == SHADER_INT)
				l = new int();
			else if (v->type == SHADER_FLOAT)
				l = new float();
			else if (v->type == SHADER_SAMPLER)
				l = new MatVal_Tex();
			valNames[v->type].push_back(v->name);
			GLint loc = glGetUniformLocation(shader->pointer, v->name.c_str());
			if (loc > -1) {
				vals[v->type].emplace(loc, l);
				nMap.emplace(v->name, loc);
				valOrders.push_back(v->type);
				valOrderIds.push_back((byte)(valNames[v->type].size() - 1));
				valOrderGLIds.push_back(loc);
			}
			else
				Debug::Warning("Material", "Shader parameter " + v->name + " not used!");
		}
		int vs;
		_Strm2Val(stream, vs);
		for (int r = 0; r < vs; r++) {
			char ii;
			stream >> ii;
			nmm = new char[100];
			stream.getline(nmm, 100, (char)0);
			string nm(nmm);
			switch (ii) {
			case SHADER_FLOAT:
				for (int x = vals[SHADER_FLOAT].size() - 1; x >= 0; x--) {
					if (valNames[SHADER_FLOAT][x] == nm) {
						float f;
						_Strm2Val(stream, f);
						(*(float*)vals[SHADER_FLOAT][nMap[nm]]) += f;
						break;
					}
				}
				break;
			case SHADER_INT:
				for (int x = vals[SHADER_INT].size() - 1; x >= 0; x--) {
					if (valNames[SHADER_INT][x] == nm) {
						int f;
						_Strm2Val(stream, f);
						(*(int*)vals[SHADER_INT][nMap[nm]]) += f;
						break;
					}
				}
				break;
			case SHADER_SAMPLER:
				for (int x = vals[SHADER_SAMPLER].size() - 1; x >= 0; x--) {
					if (valNames[SHADER_SAMPLER][x] == nm) {
						char* nmm2 = new char[100];
						stream.getline(nmm2, 100, (char)0);
						string nm2(nmm2);
						ASSETTYPE t;
						if (vals[SHADER_SAMPLER][nMap[nm]] == nullptr) {
							delete[](nmm2);
							break;
						}
						offset = (uint)stream.tellg();
						((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id = AssetManager::GetAssetId(nm2, t);
						((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, ((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id);
						stream.seekg(offset);
						delete[](nmm2);
						break;
					}
				}
				break;
			}
		}
		delete[](nmm);
	}
}

Material::~Material() {
	for (auto& a : vals) {
		for (auto& b : a.second)
			delete(b.second);
	}
}

void Material::ResetVals() {
	vals[SHADER_INT] = std::unordered_map <GLint, void*>();
	vals[SHADER_FLOAT] = std::unordered_map <GLint, void*>();
	vals[SHADER_VEC2] = std::unordered_map <GLint, void*>();
	vals[SHADER_VEC3] = std::unordered_map <GLint, void*>();
	vals[SHADER_SAMPLER] = std::unordered_map <GLint, void*>();
	vals[SHADER_MATRIX] = std::unordered_map <GLint, void*>();
	valNames[SHADER_INT] = std::vector<string>();
	valNames[SHADER_FLOAT] = std::vector<string>();
	valNames[SHADER_VEC2] = std::vector<string>();
	valNames[SHADER_VEC3] = std::vector<string>();
	valNames[SHADER_SAMPLER] = std::vector<string>();
	valNames[SHADER_MATRIX] = std::vector<string>();
	valOrders = std::vector<SHADER_VARTYPE>();
}

void Material::SetTexture(string name, Texture * texture) {
	SetTexture(glGetUniformLocation(shader->pointer, name.c_str()), texture);
}
void Material::SetTexture(GLint id, Texture * texture) {
	if (id > -1) {
		if (vals[SHADER_SAMPLER].find(id) == vals[SHADER_SAMPLER].end()) vals[SHADER_SAMPLER][id] = new MatVal_Tex();
		((MatVal_Tex*)vals[SHADER_SAMPLER][id])->tex = texture;
	}
}

void Material::SetFloat(string name, float val) {
	SetFloat(glGetUniformLocation(shader->pointer, name.c_str()), val);
}
void Material::SetFloat(GLint id, float val) {
	if (id > -1)
		vals[SHADER_FLOAT][id] = new float(val);
}

void Material::SetInt(string name, int val) {
	SetInt(glGetUniformLocation(shader->pointer, name.c_str()), val);
}
void Material::SetInt(GLint id, int val) {
	if (id > -1)
		vals[SHADER_INT][id] = new int(val);
}

void Material::SetVec2(string name, Vec2 val) {
	SetVec2(glGetUniformLocation(shader->pointer, name.c_str()), val);
}
void Material::SetVec2(GLint id, Vec2 val) {
	if (id > -1)
		vals[SHADER_VEC2][id] = new Vec2(val);
}

std::vector<GLuint> Material::defTexs = std::vector<GLuint>(7);

void Material::LoadOris() {
	std::vector<byte> data(12, 0x00);

	glGenTextures(1, &defTexs[0]);
	glBindTexture(GL_TEXTURE_2D, defTexs[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);
	
	for (int a = 0; a < 12; a++) data[a] = 0x80;
	glGenTextures(1, &defTexs[1]);
	glBindTexture(GL_TEXTURE_2D, defTexs[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = 0xFF;
	glGenTextures(1, &defTexs[2]);
	glBindTexture(GL_TEXTURE_2D, defTexs[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = (a / 3 == 0) ? 0xFF : 0x00;
	glGenTextures(1, &defTexs[3]);
	glBindTexture(GL_TEXTURE_2D, defTexs[3]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = ((a - 1) / 3 == 0) ? 0xFF : 0x00;
	glGenTextures(1, &defTexs[4]);
	glBindTexture(GL_TEXTURE_2D, defTexs[4]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = ((a + 1) / 3 == 0) ? 0xFF : 0x00;
	glGenTextures(1, &defTexs[5]);
	glBindTexture(GL_TEXTURE_2D, defTexs[5]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	for (int a = 0; a < 12; a++) data[a] = ((a + 1) / 3 == 0) ? 0xFF : 0x80;
	glGenTextures(1, &defTexs[6]);
	glBindTexture(GL_TEXTURE_2D, defTexs[6]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Material::_UpdateTexCache(void* v) {
	Material* mat = (Material*)v;
	for (auto& a : mat->vals[SHADER_SAMPLER]) {
		if (a.second == nullptr)
			continue;
		MatVal_Tex* tx = (MatVal_Tex*)a.second;
		tx->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, tx->id);
	}
}

void Material::Save(string path) {
	std::ofstream strm(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	strm << "KTC";
	ASSETTYPE st;
	ASSETID si = Editor::instance->GetAssetId(shader, st);
	_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_SHADER, si);
	int i = 0, j = 0;
	SHADER_VARTYPE t = SHADER_INT;
	long long p1 = strm.tellp();
	strm << "0000";
	//_StreamWrite(&i, &strm, 4);
	for (auto& v : vals[SHADER_INT]) {
		t = SHADER_INT;
		strm << (char)t;
		strm << valNames[SHADER_INT][j] << (char)0;
		int ii(*(int*)v.second);
		_StreamWrite(&ii, &strm, 4);
		i++;
		j++;
	}
	j = 0;
	for (auto& v : vals[SHADER_FLOAT]) {
		t = SHADER_FLOAT;
		strm << (char)t;
		strm << valNames[SHADER_FLOAT][j] << (char)0;
		float ff(*(float*)v.second);
		_StreamWrite(&ff, &strm, 4);
		i++;
		j++;
	}
	j = 0;
	for (auto& v : vals[SHADER_SAMPLER]) {
		if (v.second == nullptr)
			continue;
		t = SHADER_SAMPLER;
		strm << (char)t;
		strm << valNames[SHADER_SAMPLER][j] << (char)0;
		_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_TEXTURE, ((MatVal_Tex*)v.second)->id);
		i++;
		j++;
	}
	strm.seekp(p1);
	_StreamWrite(&i, &strm, 4);
	strm.close();
}

void Material::ApplyGL(Mat4x4& _mv, Mat4x4& _p) {
	if (shader == nullptr || !shader->loaded) {
		glUseProgram(0);
		return;
	}
	else {
		glUseProgram(shader->pointer);
		GLint mv = glGetUniformLocation(shader->pointer, "_M");
		GLint mvp = glGetUniformLocation(shader->pointer, "_MVP");
		glUniformMatrix4fv(mv, 1, GL_FALSE, glm::value_ptr(_mv));
		glUniformMatrix4fv(mvp, 1, GL_FALSE, glm::value_ptr(_p*_mv));
		//glUniformMatrix4fv(p, 1, GL_FALSE, matrix2);
		for (auto& a : vals[SHADER_INT])
			if (a.second != nullptr)
				glUniform1i(a.first, *(int*)a.second);
		for (auto& a : vals[SHADER_FLOAT])
			if (a.second != nullptr)
				glUniform1f(a.first, *(float*)a.second);
		for (auto& a : vals[SHADER_VEC2]) {
			if (a.second == nullptr)
				continue;
			Vec2* v2 = (Vec2*)a.second;
			glUniform2f(a.first, v2->x, v2->y);
		}
		int ti = -1;
		for (auto& a : vals[SHADER_SAMPLER]) {
			ti++;
			if (a.second == nullptr)
				continue;
			MatVal_Tex* tx = (MatVal_Tex*)a.second;
			glUniform1i(a.first, ti);
			glActiveTexture(GL_TEXTURE0 + ti);
			if (tx->tex == nullptr)
				glBindTexture(GL_TEXTURE_2D, tx->defTex);
			else
				glBindTexture(GL_TEXTURE_2D, tx->tex->pointer);
		}
		bool wm;
		for (uint m = 0; m < GBUFFER_NUM_TEXTURES - 1; m++) {
			wm = writeMask[m];
			glColorMaski(m, wm, wm, wm, wm);
		}
	}
}


CameraEffect::CameraEffect(Material* mat) : AssetObject(ASSETTYPE_CAMEFFECT) {
	material = mat;
}

CameraEffect::CameraEffect(string p) : AssetObject(ASSETTYPE_CAMEFFECT) {
	//string p = Editor::instance->projectFolder + "Assets\\" + path;
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
Mesh::Mesh(const std::vector<Vec3>& verts, const std::vector<Vec3>& norms, const std::vector<int>& tris, std::vector<Vec2> uvs) : AssetObject(ASSETTYPE_MESH) {
	vertices = std::vector<Vec3>(verts);
	vertexCount = verts.size();
	normals = std::vector<Vec3>(norms);
	triangles = std::vector<int>(tris);
	triangleCount = tris.size() / 3;
	uv0 = std::vector<Vec2>(uvs);
	materialCount = 1;
	_matTriangles.push_back(std::vector<int>(tris));
	CalcTangents();
	RecalculateBoundingBox();
	loaded = (vertexCount > 0) && (normals.size() == vertexCount) && (triangleCount > 0);
}

Mesh::Mesh(Editor* e, int i) : AssetObject(ASSETTYPE_MESH) {
	Mesh* m2 = _GetCache<Mesh>(type, i);
	vertices = m2->vertices;
	vertexCount = m2->vertexCount;
	normals = m2->normals;
	tangents = m2->tangents;
	//bitangents = m2->bitangents;
	triangles = m2->triangles;
	triangleCount = m2->triangleCount;
	boundingBox = m2->boundingBox;
	loaded = m2->loaded;
}

Mesh::Mesh(std::istream& stream, uint offset) : AssetObject(ASSETTYPE_MESH), loaded(false), vertexCount(0), triangleCount(0), materialCount(0) {
	if (stream.good()) {
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

Mesh::Mesh(string p) : AssetObject(ASSETTYPE_MESH), loaded(false), vertexCount(0), triangleCount(0), materialCount(0) {
	F2ISTREAM(stream, p);
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

	//std::cout << path << std::endl;
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

		GenECache();
		loaded = true;
	}
}

Mesh::Mesh(byte* mem) : AssetObject(ASSETTYPE_MESH) {
	memcpy(&vertexCount, mem, sizeof(uint));
	memcpy(&triangleCount, mem + sizeof(uint), sizeof(uint));
	memcpy(&materialCount, mem + sizeof(uint) * 2, sizeof(uint));
	mem += sizeof(uint) * 3;
	vertices = std::vector<Vec3>(mem, mem + sizeof(Vec3)*vertexCount);
	mem += sizeof(Vec3) * vertexCount;
	normals = std::vector<Vec3>(mem, mem + sizeof(Vec3)*vertexCount);
	mem += sizeof(Vec3) * vertexCount;
	tangents = std::vector<Vec3>(mem, mem + sizeof(Vec3)*vertexCount);
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
	tangents = std::vector<Vec3>(vertices.size(), Vec3(0, 0, 0));
	std::vector<int> tC = std::vector<int>(vertices.size(), 0);

	Vec2 u0, u1, u2, r1, r2;
	Vec3 p0, p1, p2;
	float a, b, db;
	int n0, n1, n2;
	for (uint n = 0; n < triangleCount; n++) {
		n0 = triangles[n * 3];
		n1 = triangles[n * 3 + 1];
		n2 = triangles[n * 3 + 2];

		u0 = uv0[n0];
		u1 = uv0[n1];
		u2 = uv0[n2];
		p0 = vertices[n0];
		p1 = vertices[n1];
		p2 = vertices[n2];

		if ((u0 == u1) || (u1 == u2) || (u0 == u2)) {
			//Debug::Warning("Tangent calculator", "Triangle " + to_string(n) + " does not have well-defined UVs!");
			continue;
		}
		r1 = u1 - u0;
		r2 = u2 - u0;

		//t = ar + br';
		db = r1.y*r2.x - r1.x*r2.y;
		if (db == 0) {
			//Debug::Warning("Tangent calculator", "Triangle " + to_string(n) + " does not have well-defined UVs!");
			continue;
		}
		b = r1.y / db;
		if (r1.y == 0) a = (1 - b*r2.x) / r1.x;
		else a = -b*r2.y / r1.y;
		Vec3 t = (p1 - p0)*a + (p2 - p0)*b;
		tangents[n0] += t;
		tangents[n1] += t;
		tangents[n2] += t;
		tC[n0]++;
		tC[n1]++;
		tC[n2]++;
	}
	for (uint a = 0; a < vertexCount; a++) {
		if (tC[a] > 0) tangents[a] = Normalize(tangents[a] / ((float)tC[a]));
	}
}

void Mesh::GenECache() {
	if (_eCache.first) return;
	_eCache.second = sizeof(uint) * 3 + sizeof(Vec3)*vertexCount * 3 + sizeof(Vec2)*vertexCount * 2 + sizeof(uint)*triangleCount * 3;
	_eCache.first = (byte*)malloc(_eCache.second);
	memcpy(_eCache.first, &vertexCount, sizeof(uint));
	memcpy(_eCache.first + sizeof(uint), &triangleCount, sizeof(uint));
	memcpy(_eCache.first + sizeof(uint) * 2, &materialCount, sizeof(uint));
	uint off = 3 * sizeof(uint);
	memcpy(_eCache.first + off, &vertices[0], sizeof(Vec3)*vertexCount);
	off += sizeof(Vec3)*vertexCount;
	memcpy(_eCache.first + off, &normals[0], sizeof(Vec3)*vertexCount);
	off += sizeof(Vec3)*vertexCount;
	memcpy(_eCache.first + off, &tangents[0], sizeof(Vec3)*vertexCount);
	off += sizeof(Vec3)*vertexCount;
	memcpy(_eCache.first + off, &triangles[0], sizeof(uint)*triangleCount * 3);
	off += sizeof(uint)*triangleCount * 3;
	memcpy(_eCache.first + off, &uv0[0], sizeof(Vec2)*vertexCount);
	off += sizeof(Vec2)*vertexCount;
	memcpy(_eCache.first + off, &uv1[0], sizeof(Vec2)*vertexCount);
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


AnimClip::AnimClip(string p) : AssetObject(ASSETTYPE_ANIMCLIP) {
	//string p = Editor::instance->projectFolder + "Assets\\" + path + ".animclip.meta";
	//std::ifstream stream(p.c_str(), std::ios::in | std::ios::binary);
	F2ISTREAM(stream, p);
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

Animator::Animator(string p) : Animator() {
	F2ISTREAM(stream, p);
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
