#include "Engine.h"
#include "Editor.h"

FT_Library Font::_ftlib = nullptr;
GLuint Font::fontProgram = 0;
uint Font::vaoSz = 0;
GLuint Font::vao = 0;
GLuint Font::vbos[] = { 0, 0, 0 };

void Font::Init() {
	int err = FT_Init_FreeType(&_ftlib);
	if (err != FT_Err_Ok) {
		Debug::Error("Font", "Fatal: Initializing freetype failed!");
		std::runtime_error("Fatal: Initializing freetype failed!");
	}

	string error;
	GLuint vs, fs;
#if defined(PLATFORM_WIN) || defined(PLATFORM_LNX)
	string frag = "#version 330\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = vec4(1, 1, 1, texture(sampler, UV).r)*col;\n}";
#elif defined(PLATFORM_ADR)
	string frag = "#version 300 es\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = vec4(1.0, 1.0, 1.0, texture(sampler, UV).r)*col;\n}";
	//string frag = "#version 300 es\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = vec4(1.0, 0.0, 0.0, 1.0);\n}";
#endif
	if (!Shader::LoadShader(GL_VERTEX_SHADER, DefaultResources::GetStr("fontVert.txt"), vs, &error)) {
		Debug::Error("Engine", "Fatal: Cannot init font shader(v)! " + error);
		abort();
	}
	if (!Shader::LoadShader(GL_FRAGMENT_SHADER, frag, fs, &error)) {
		Debug::Error("Engine", "Fatal: Cannot init font shader(f)! " + error);
		abort();
	}
	fontProgram = glCreateProgram();
	glAttachShader(fontProgram, vs);
	glAttachShader(fontProgram, fs);

	int link_result = 0;
	glLinkProgram(fontProgram);
	glGetProgramiv(fontProgram, GL_LINK_STATUS, &link_result);
	if (link_result == GL_FALSE)
	{
		int info_log_length = 0;
		glGetProgramiv(fontProgram, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector<char> program_log(info_log_length);
		glGetProgramInfoLog(fontProgram, info_log_length, NULL, &program_log[0]);
		std::cerr << "Font shader link error" << std::endl << &program_log[0] << std::endl;
		abort();
	}
	glDetachShader(fontProgram, vs);
	glDetachShader(fontProgram, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);

	InitVao(500);
}

void Font::InitVao(uint sz) {
#ifdef IS_EDITOR
	if (PopupSelector::drawing)
		glfwMakeContextCurrent(PopupSelector::mainWindow);
#endif
	vaoSz = sz;
	if (!!vao) {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(3, vbos);
	}
	glGenVertexArrays(1, &vao);
	glGenBuffers(3, vbos);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glBufferStorage(GL_ARRAY_BUFFER, vaoSz * sizeof(Vec3), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
	glBufferStorage(GL_ARRAY_BUFFER, vaoSz * sizeof(Vec2), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
	glBufferStorage(GL_ARRAY_BUFFER, vaoSz * sizeof(float), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
#ifdef IS_EDITOR
	if (PopupSelector::drawing)
		glfwMakeContextCurrent(PopupSelector::window);
#endif
}

Font::Font(const string& path, ALIGNMENT align) : vecSize(0), alignment(align) {
	Debug::Message("Font", "opening font at " + path);
	auto err = FT_New_Face(_ftlib, path.c_str(), 0, &_face);
	if (err != FT_Err_Ok) {
		Debug::Warning("Font", "Failed to load font! " + std::to_string(err));
		return;
	}
	//FT_Set_Char_Size(_face, 0, (FT_F26Dot6)(size * 64.0f), Display::dpi, 0); // set pixel size based on dpi
	FT_Set_Pixel_Sizes(_face, 0, 12); // set pixel size directly
	FT_Select_Charmap(_face, FT_ENCODING_UNICODE);
	CreateGlyph(12, true);
	SizeVec(20);
	loaded = true;
}

void Font::SizeVec(uint sz) {
	if (vecSize >= sz) return;
	poss.resize(sz * 4 + 1);
	cs.resize(sz * 4);
	for (; vecSize < sz; vecSize++) {
		uvs.push_back(Vec2(0, 1));
		uvs.push_back(Vec2(1, 1));
		uvs.push_back(Vec2(0, 0));
		uvs.push_back(Vec2(1, 0));
		ids.push_back(4 * vecSize);
		ids.push_back(4 * vecSize + 1);
		ids.push_back(4 * vecSize + 2);
		ids.push_back(4 * vecSize + 1);
		ids.push_back(4 * vecSize + 3);
		ids.push_back(4 * vecSize + 2);
	}
}

GLuint Font::CreateGlyph(uint sz, bool recalc) {
	//FT_Set_Char_Size(_face, 0, (FT_F26Dot6)(size * 64.0f), Display::dpi, 0); // set pixel size based on dpi
	FT_Set_Pixel_Sizes(_face, 0, sz); // set pixel size directly
	_glyphs.emplace(sz, 0);
	glGenTextures(1, &_glyphs[sz]);
	glBindTexture(GL_TEXTURE_2D, _glyphs[sz]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, (sz + 1) * 16, (sz + 1) * 16);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (ushort a = 0; a < 256; a++) {
		if (recalc) {
			w2h[a] = 0;
			w2s[a] = 0;
		}
		if (FT_Load_Char(_face, a, FT_LOAD_RENDER) != FT_Err_Ok) continue;
		byte x = a % 16, y = a / 16;
		FT_Bitmap bmp = _face->glyph->bitmap;
		glTexSubImage2D(GL_TEXTURE_2D, 0, (sz + 1) * x + 1, (sz + 1) * y, bmp.width, bmp.rows, GL_RED, GL_UNSIGNED_BYTE, bmp.buffer);
		if (recalc) {
			if (bmp.width == 0) {
				w2h[a] = 0;
				w2s[a] = 0.5f / sz;
			}
			else {
				w2h[a] = (float)(bmp.width) / bmp.rows;
				w2s[a] = (float)(bmp.width) / sz;
			}
			off[a] = Vec2((float)(_face->glyph->bitmap_left) / sz, 1 - ((float)(_face->glyph->bitmap_top) / sz));
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	return _glyphs[sz];
}

Font* Font::Align(ALIGNMENT a) {
	alignment = a;
	return this;
}