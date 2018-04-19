#include "Engine.h"

#ifdef PLATFORM_WIN
#pragma comment(lib, "jpeg_win.lib")
#endif
#include "jpeglib.h"
#include "jerror.h"
#include "lodepng.h"

bool LoadJPEG(string fileN, uint &x, uint &y, byte& channels, byte** data)
{
	//unsigned int texture_id;
	unsigned long data_size;     // length
	unsigned char * rowptr[1];
	struct jpeg_decompress_struct info; //for our jpeg info
	struct jpeg_error_mgr err;          //the error handler

	FILE* file;
#ifdef PLATFORM_WIN
	fopen_s(&file, fileN.c_str(), "rb");  //open the file
#else
	file = fopen(fileN.c_str(), "rb");
#endif
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
	if (imageSize == 0)    imageSize = x * y * channels; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	*data = new unsigned char[imageSize];
	// Read the actual data from the file into the buffer
	strm.read(*(char**)data, imageSize);
#ifdef PLATFORM_ADR
	for (uint a = 0; a < x * y; a++) {
		unsigned char b = (*data)[a];
		(*data)[a] = (*data)[a + 2];
		(*data)[a + 2] = b;
	}
#endif
	return true;
}

byte* Texture::LoadPixels(const string& path, byte& chn, uint& w, uint& h) {
	string sss = path.substr(path.find_last_of('.'), string::npos);
	byte *data;
	std::vector<byte> dataV;
	//std::cout << "opening image at " << path << std::endl;
	//GLenum rgb = GL_RGB, rgba = GL_RGBA;
	if (sss == ".bmp") {
		if (!LoadBMP(path, w, h, chn, &data)) {
			std::cout << "load bmp failed! " << path << std::endl;
			return nullptr;
		}
	}
	else if (sss == ".jpg") {
		if (!LoadJPEG(path, w, h, chn, &data)) {
			std::cout << "load jpg failed! " << path << std::endl;
			return nullptr;
		}
	}
	else if (sss == ".png") {
		if (!LoadPNG(path, w, h, chn, dataV)) {
			std::cout << "load png failed! " << path << std::endl;
			return nullptr;
		}
		data = new byte[w*h*chn];
		memcpy(data, &dataV[0], w*h*chn);
	}
	else {
		std::cout << "Image extension invalid! " << path << std::endl;
		return nullptr;
	}
	return data;
}

Texture::Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso, TEX_WARPING warp) : AssetObject(ASSETTYPE_TEXTURE), _mipmap(mipmap), _filter(filter), _aniso(aniso) {
	string sss = path.substr(path.find_last_of('.') + 1, string::npos);
	byte *data;
	std::vector<byte> dataV;
	byte chn;
	//std::cout << "opening image at " << path << std::endl;
	GLenum rgb = GL_RGB, rgba = GL_RGBA;
	if (sss == "bmp") {
		if (!LoadBMP(path, width, height, chn, &data)) {
			Debug::Warning("Texture", "load bmp failed! " + path);
			return;
		}
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	else if (sss == "jpg") {
		if (!LoadJPEG(path, width, height, chn, &data)) {
			Debug::Warning("Texture", "load jpg failed! " + path);
			return;
		}
	}
	else if (sss == "png") {
		if (!LoadPNG(path, width, height, chn, dataV)) {
			Debug::Warning("Texture", "load png failed! " + path);
			return;
		}
		data = &dataV[0];
	}
	else {
		Debug::Warning("Texture", "invalid extension! " + sss);
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

Texture::Texture(int i, Editor* e) : AssetObject(ASSETTYPE_TEXTURE) {
#ifdef IS_EDITOR
	string path = e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_TEXTURE][i] + ".meta";
	F2ISTREAM(strm, path);
	if (strm.good()) {
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

		uint mips = 0;
		std::vector<RenderTexture*> rts = std::vector<RenderTexture*>();
		std::vector<Vec2> szs = std::vector<Vec2>();
		if (_mipmap && _blurmips) {
			uint width_1 = width, height_1 = height, width_2, height_2;

			Shader shd = Shader(Camera::d_blurProgram);
			Material mat = Material(&shd);

			rts.push_back(new RenderTexture(width, height, RT_FLAG_NONE, data, (chn == 3) ? GL_RGB : GL_RGBA));
			szs.push_back(Vec2(width, height));

			while (mips < 6 && height_1 > 16) {
				width_2 = width_1 / 2;
				height_2 = height_1 / 2;

				mat.SetFloat("mul", 1 - mips*0.1f);
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

		//GenECache(data, chn, rgb == GL_RGB, &rts);

		glGenTextures(1, &pointer);
		glBindTexture(GL_TEXTURE_2D, pointer);
		if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, data);
		else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
		if (_mipmap) {
			if (_blurmips) {
				for (uint a = 1; a <= mips; a++) {
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _aniso);
		glBindTexture(GL_TEXTURE_2D, 0);
		delete[](data);
		loaded = true;
	}
#endif
}

Texture::Texture(std::istream& strm, uint offset) : AssetObject(ASSETTYPE_TEXTURE) {
	if (strm.good()) {
		strm.seekg(offset);
		byte chn;
		GLenum rgb = GL_RGB, rgba = GL_RGBA;
		byte* data;
		TEX_TYPE t = _ReadStrm(this, strm, chn, rgb, rgba);
		if (t == TEX_TYPE_UNDEF) {
			Debug::Warning("Texture", "Texture header wrong/missing!");
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

			Shader shd = Shader(Camera::d_blurProgram);
			Material mat = Material(&shd);

			rts.push_back(new RenderTexture(width, height, RT_FLAG_NONE, data, (chn == 3) ? GL_RGB : GL_RGBA));
			szs.push_back(Vec2(width, height));

			while (mips < 6 && height_1 > 16) {
				width_2 = width_1 / 2;
				height_2 = height_1 / 2;

				mat.SetFloat("mul", 1 - mips*0.1f);
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
				for (uint a = 1; a <= mips; a++) {
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

Texture::Texture(byte* mem) : AssetObject(ASSETTYPE_TEXTURE) {
#ifndef IS_EDITOR
#define RD(tar, sz) memcpy(tar, mem, sz); mem += sz;
	byte b, mips, chn;
	GLenum rgb, rgba;
	RD(&width, 4);
	RD(&height, 4);
	RD(&_filter, 1);
	RD(&_aniso, 1);
	RD(&b, 1);
#define SR(n) (!!((b & (1 << n)) >> n))
	mips = SR(7);
	_repeat = SR(6);
	_blurmips = SR(5);
	chn = 3 + SR(4);
	rgb = SR(3) ? GL_RGB : GL_BGR;
	rgba = SR(3) ? GL_RGBA : GL_BGRA;

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, mem);
	else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, mem);
	mem += width*height*chn;
	if (_mipmap) {
		if (_blurmips) {
			uint w2, h2;
			for (uint a = 1; a <= mips; a++) {
				RD(&w2, 4);
				RD(&h2, 4);
				if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w2, h2, 0, rgb, GL_UNSIGNED_BYTE, mem);
				else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2, h2, 0, rgba, GL_UNSIGNED_BYTE, mem);
				mem += w2*h2*chn;
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
		}
		else glGenerateMipmap(GL_TEXTURE_2D);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (_mipmap && (_filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _aniso);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
#undef SR
#undef RD
#endif
}

void Texture::GenECache(byte* b, byte chn, bool isrgb, std::vector<RenderTexture*>* rts) {
	/*
	width (uint)
	height (uint)
	filter (byte)
	aniso (byte)
	[mip repeat blurmip hasA isrgb, 000] (byte)
	pixels (byte*w*h)
	if blurmip:
	mipcount (byte)
	for mips [
	width (uint)
	height (uint)
	pixels (byte*w*h)
	]
	*/
#ifdef IS_EDITOR
#define CPY(mem, sz) memcpy(_eCache + off, mem, sz); off += sz;
	_eCacheSz = 3;
	uint off = 1;
	if (_blurmips) {
		for (auto& a : *rts) _eCacheSz += 8 + a->width*a->height * 4;
		_eCacheSz++;
	}
	else {
		_eCacheSz += 8 + width*height*chn;
	}
	_eCache = (byte*)malloc(_eCacheSz);
	*_eCache = 255;
	CPY(&width, 4);
	CPY(&height, 4);
	CPY(&_filter, 1);
	CPY(&_aniso, 1);
	byte bb = (_mipmap << 7) | (_repeat << 6) | (_blurmips << 5) | (!(chn & 1) << 4) | (isrgb << 3);
	CPY(&bb, 1);
	CPY(b, width*height*chn);
	if (_blurmips) {
		byte s = (byte)rts->size();
		CPY(&s, 1);
		for (byte m = 1; m < s; m++) {
			auto r = (*rts)[m];
			auto px = r->pixels<byte>(chn == 4);
			CPY(&r->width, 4);
			CPY(&r->height, 4);
			CPY(&px[0], r->width*r->height*chn);
		}
	}
	assert(off == _eCacheSz + 1);
#undef CPY
#endif
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
	_Strm2Val(strm, chn);
	_Strm2Val(strm, tex->width);
	_Strm2Val(strm, tex->height);
	_Strm2Val(strm, bb);
	if (bb == 1) {
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	_Strm2Val(strm, tex->_aniso);
	_Strm2Val(strm, bb);
	tex->_filter = (TEX_FILTERING)bb;
	_Strm2Val(strm, bb);
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

#ifdef IS_EDITOR
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
#endif

bool Texture::DrawPreview(uint x, uint y, uint w, uint h) {
	UI::Texture((float)x, (float)y, (float)w, (float)h, this, DRAWTEX_FIT);
	return true;
}