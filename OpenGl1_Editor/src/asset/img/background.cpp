#include "Engine.h"
#include "hdr.h"
#include "Editor.h"

Background::Background(const string& path) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
	if (path.size() < 5 || path.substr(path.size() - 4, string::npos) != ".hdr") {
		printf("HDRI path invalid!");
		return;
	}
	//std::cout << "opening hdr image at " << path << std::endl;

	byte* data2 = hdr::read_hdr(path.c_str(), &width, &height);
	if (data2 == NULL)
		return;
	auto data = hdr::to_float(data2, width, height);
	delete[](data2);

	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;

	Shader shd = Shader(Camera::d_blurSBProgram);
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
#ifdef IS_EDITOR
	string path = editor->projectFolder + "Assets\\" + editor->normalAssets[ASSETTYPE_HDRI][i] + ".meta";
	//std::ifstream strm(path.c_str(), std::ios::in | std::ios::binary);
	F2ISTREAM(strm, path);
	std::vector<char> hd(6);
	strm.read((&hd[0]), 4);
	if (hd[0] != 'I' || hd[1] != 'M' || hd[2] != 'G' || hd[3] != (char)4) {
		Debug::Error("HDR Cacher", "HDR cache header wrong!");
		return;
	}
	_Strm2Val(strm, width);
	_Strm2Val(strm, height);
	strm.read((&hd[0]), 5);
	if (hd[0] != (char)0 || hd[1] != 'D' || hd[2] != 'A' || hd[3] != 'T' || hd[4] != 'A') {
		Debug::Error("HDR Cacher", "Data tag missing!");
		return;
	}

	byte* data2 = new byte[width*height * 4];
	strm.read((char*)data2, width*height * 4);
	auto data = hdr::to_float(data2, width, height);
	delete[](data2);

	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;

	Shader shd = Shader(Camera::d_blurSBProgram);
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

	GenECache(szs, rts);

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
#endif
}

Background::Background(std::istream& strm, uint offset) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
	std::vector<char> hd(6);
	strm.read((&hd[0]), 4);
	if (hd[0] != 'I' || hd[1] != 'M' || hd[2] != 'G' || hd[3] != (char)4) {
		Debug::Error("HDR Cacher", "HDR cache header wrong!");
		return;
	}
	_Strm2Val(strm, width);
	_Strm2Val(strm, height);
	strm.read((&hd[0]), 5);
	if (hd[0] != (char)0 || hd[1] != 'D' || hd[2] != 'A' || hd[3] != 'T' || hd[4] != 'A') {
		Debug::Error("HDR Cacher", "Data tag missing!");
		return;
	}

	byte* data2 = new byte[width*height * 4];
	strm.read((char*)data2, width*height * 4);
	auto data = hdr::to_float(data2, width, height);
	delete[](data2);

	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;

	Shader shd = Shader(Camera::d_blurSBProgram);
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

	//GenECache(szs, rts);

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

Background::Background(byte* mem) : AssetObject(ASSETTYPE_HDRI) {
#define RD(tar, sz) memcpy(tar, mem, sz); mem += sz;

	byte mips = 0;
	RD(&mips, 1);
	RD(&width, 4);
	RD(&height, 4);
	mips--;

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, mem);
	mem += 4 * width*height * 3;
	for (uint a = 1; a <= mips; a++) {
		uint w2, h2;
		RD(&w2, 4);
		RD(&h2, 4);
		//glTexSubImage2D(GL_TEXTURE_2D, a, 0, 0, (GLsizei)w2, (GLsizei)h2, GL_RGB, GL_FLOAT, mem);
		glTexImage2D(GL_TEXTURE_2D, a, GL_RGB32F, w2, h2, 0, GL_RGB, GL_FLOAT, mem);
		mem += 4 * w2*h2 * 3;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
#undef RD
}

void Background::GenECache(const std::vector<Vec2>& szs, const std::vector<RenderTexture*>& rts) {
	/*
	mips (byte)
	for each mip [
	width (uint)
	height (uint)
	data (w*h*3*float)
	]
	*/
#ifdef IS_EDITOR
#define CPY(pt, sz) memcpy(_eCache + off, pt, sz); off += sz;
	if (_eCache) return;
	byte mipn = (byte)rts.size();
	_eCacheSz = sizeof(byte) + sizeof(uint) * 2 * mipn;
	for (auto& a : rts) _eCacheSz += sizeof(float)*a->width*a->height * 3;
	_eCache = (byte*)malloc(_eCacheSz + 1);
	*_eCache = 255;
	uint off = 1;
	CPY(&mipn, 1);
	for (auto& a : rts) {
		auto px = a->pixels<float>(false);
		CPY(&a->width, 4);
		CPY(&a->height, 4);
		CPY(&px[0], 4 * a->width*a->height * 3);
	}
	assert(off == _eCacheSz + 1);
#undef CPY
#endif
}

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