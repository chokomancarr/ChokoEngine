#pragma once
#include "Engine.h"

#ifdef PLATFORM_WIN
#pragma comment(lib, "freetype_win.lib")
#endif
#include <ft2build.h>
#include FT_FREETYPE_H

/*! Dynamic fonts constructed with FreeType (.ttf files)
[av]*/
class Font {
public:
	Font(const string& path, ALIGNMENT align = ALIGN_TOPLEFT);
	bool loaded = false;
	//bool useSubpixel; //glyphs are rgba if true, else r (does it look good in games?)
	GLuint glyph(uint size) { if (_glyphs.count(size) == 1) return _glyphs[size]; else return CreateGlyph(size); }

	ALIGNMENT alignment;

	Font* Align(ALIGNMENT a);

	friend class Engine;
	friend class UI;
#ifdef IS_EDITOR
	friend class PopupSelector;
#endif
protected:
	static FT_Library _ftlib;
	static void Init(), InitVao(uint sz);
	FT_Face _face;
	static GLuint fontProgram;

	float w2h[256];
	float w2s[256];
	Vec2 off[256];

	uint vecSize;
	std::vector<Vec3> poss;
	std::vector<Vec2> uvs;
	std::vector<uint> ids;
	std::vector<float> cs;
	void SizeVec(uint sz);

	static uint vaoSz;
	static GLuint vao, vbos[3];

	std::unordered_map<uint, GLuint> _glyphs; //each glyph size is fontSize*16
	GLuint CreateGlyph(uint size, bool recalcW2h = false);
};