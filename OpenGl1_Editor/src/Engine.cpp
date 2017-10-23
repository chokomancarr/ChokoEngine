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

string to_string(float f) { return std::to_string(f); }
string to_string(double f) { return std::to_string(f); }
string to_string(ulong f) { return std::to_string(f); }
string to_string(long f) { return std::to_string(f); }
string to_string(uint f) { return std::to_string(f); }
string to_string(int f) { return std::to_string(f); }
string to_string(Vec2 v) {
	return "(" + to_string(v.x) + ", " + to_string(v.y) + ")";
}
string to_string(Vec3 v) {
	return "(" + to_string(v.x) + ", " + to_string(v.y) + ", " + to_string(v.z) + ")";
}
string to_string(Vec4 v) {
	return "(" + to_string(v.w) + ", " + to_string(v.x) + ", " + to_string(v.y) + ", " + to_string(v.z) + ")";
}
string to_string(Quat v) {
	return "(" + to_string(v.w) + ", " + to_string(v.x) + ", " + to_string(v.y) + ", " + to_string(v.z) + ")";
}

std::vector<string> string_split(string s, char c) {
	std::vector<string> o = std::vector<string>();
	size_t pos = -1;
	do {
		s = s.substr(pos + 1);
		pos = s.find_first_of(c);
		o.push_back(s.substr(0, pos));
	} while (pos != string::npos);
	return o;
}

Vec3 to_vec3(Vec4 v) {
	if (v.w != 0) v /= v.w;
	return Vec3(v.x, v.y, v.z);
}

float Dw(float f) {
	return (f / Display::width);
}
float Dh(float f) {
	return (f / Display::height);
}
Vec3 Ds(Vec3 v) {
	return Vec3(Dw(v.x) * 2 - 1, 1 - Dh(v.y) * 2, 1);
}

GLuint Color::pickerProgH = 0;
GLuint Color::pickerProgSV = 0;

string Color::hex() {
	std::stringstream ss;
	ss << "#";
	ss << std::hex << std::setfill('0') << std::setw(2) << (int)r;
	ss << std::hex << std::setfill('0') << std::setw(2) << (int)g;
	ss << std::hex << std::setfill('0') << std::setw(2) << (int)b;
	ss << std::hex << std::setfill('0') << std::setw(2) << (int)a;
	return ss.str();
}

void Color::Rgb2Hsv(byte r, byte g, byte b, float& h, float& s, float& v) {
	float R = r * 0.0039216f;
	float G = g * 0.0039216f;
	float B = b * 0.0039216f;

	float mn = min(min(R, G), B);
	float mx = max(max(R, G), B);

	v = mx;// 0.5f*(mn + mx);
	if (mx > 0) {
		s = (mx - mn) / mx;// (1 - abs(2 * v - 1));
		if (mn != mx) {
			/*
			if (v < 0.5f)
				s = (mx - mn) / (mx + mn);
			else
				s = (mx - mn) / (2.0f - mx - mn);
			*/
			if (R == mx) {
				h = (G - B) / (mx - mn);
			}
			else if (G == mx) {
				h = 2.0f + (B - R) / (mx - mn);
			}
			else {
				h = 4.0f + (R - G) / (mx - mn);
			}
			h /= 6;
			if (h < 0) h += 1;
		}
	}
	//else s = 0;
}

void Color::Hsv2Rgb(float h, float s, float v, byte& r, byte& g, byte& b) {
	Vec4 cb = HueBaseCol(h);
	Vec4 c(LerpVec4(LerpVec4(cb, Vec4(1, 1, 1, 1), 1 - s), Vec4(), 1-v));
	r = (byte)round(c.r*255);
	g = (byte)round(c.g*255);
	b = (byte)round(c.b*255);
}

Vec3 Color::Rgb2Hsv(Vec4 col) {
	Vec3 o = Vec3();
	Rgb2Hsv(col.r, col.g, col.b, o.r, o.g, o.b);
	return o;
}

string Color::Col2Hex(Vec4 col) {
	std::stringstream ss;
	ss << "#";
	ss << std::hex << std::setfill('0') << std::setw(2) << (int)(col.r * 255);
	ss << std::hex << std::setfill('0') << std::setw(2) << (int)(col.g * 255);
	ss << std::hex << std::setfill('0') << std::setw(2) << (int)(col.b * 255);
	ss << std::hex << std::setfill('0') << std::setw(2) << (int)(col.a * 255);
	return ss.str();
}

void Color::DrawPicker(float x, float y, Color& c) {
	Engine::DrawQuad(x, y, 270, c.useA ? 335 : 318, white(0.8f, 0.1f));
	Vec2 v2 = Engine::DrawSliderFill2D(x + 10, y + 10, 220, 220, Vec2(), Vec2(1, 1), Vec2(1 - c.s, c.v), black(), grey1());
	c.s = 1 - v2.x;
	c.v = v2.y;
	DrawSV(x + 10, y + 10, 220, 220, c.h);
	Engine::DrawCircle(Vec2(x + 10 + 220 * (1 - c.s), y + 10 + 220 * (1 - c.v)), 8, 24, black(), 4);
	Engine::DrawCircle(Vec2(x + 10 + 220 * (1 - c.s), y + 10 + 220 * (1 - c.v)), 8, 24, white(), 2);
	//Engine::DrawQuad(x + 244, y + 10, 15, 220, white());
	c.h = Engine::DrawSliderFillY(x + 244, y + 10, 15, 220, 0, 1, c.h, black(), white());
	DrawH(x + 244, y + 10, 15, 220);
	Engine::DrawLine(Vec3(x + 242, y + 10 + 220*c.h, 0), Vec3(x + 261, y + 10 + 220*c.h, 0), white(), 2);
	
	c.RecalcRGB();

	y += 242;

	Engine::Label(x + 10, y, 12, "HEX", Editor::instance->font, white());
	Engine::Label(x + 10, y + 18, 12, "R", Editor::instance->font, white());
	Engine::Label(x + 10, y + 35, 12, "G", Editor::instance->font, white());
	Engine::Label(x + 10, y + 52, 12, "B", Editor::instance->font, white());

	Engine::DrawQuad(x + 40, y - 2, 170, 16, grey2());
	Engine::Label(x + 42, y, 12, c.hex(), Editor::instance->font, white());
	c.r = (byte)round(Engine::DrawSliderFill(x + 40, y + 16, 170, 16, 0, 255, c.r, grey2(), red()));
	c.g = (byte)round(Engine::DrawSliderFill(x + 40, y + 33, 170, 16, 0, 255, c.g, grey2(), green()));
	c.b = (byte)round(Engine::DrawSliderFill(x + 40, y + 50, 170, 16, 0, 255, c.b, grey2(), blue()));

	Engine::DrawQuad(x + 212, y + 16, 47, 16, grey2());
	Engine::Label(x + 214, y + 18, 12, to_string(c.r), Editor::instance->font, white());
	Engine::DrawQuad(x + 212, y + 33, 47, 16, grey2());
	Engine::Label(x + 214, y + 35, 12, to_string(c.g), Editor::instance->font, white());
	Engine::DrawQuad(x + 212, y + 50, 47, 16, grey2());
	Engine::Label(x + 214, y + 52, 12, to_string(c.b), Editor::instance->font, white());

	if (c.useA) {
		Engine::Label(x + 10, y + 69, 12, "A", Editor::instance->font, white());
		c.a = (byte)Engine::DrawSliderFill(x + 40, y + 67, 170, 16, 0, 255, c.a, grey2(), white());
		Engine::DrawQuad(x + 212, y + 67, 47, 16, grey2());
		Engine::Label(x + 214, y + 69, 12, to_string(c.a), Editor::instance->font, white());
	}

	c.RecalcHSV();
	std::cout << to_string(c.vec4()) << std::endl << to_string(c.hsv()) << std::endl;
}

Vec4 Color::HueBaseCol(float hue) {
	hue *= 6;
	Vec4 v;
	v.r = clamp(abs(hue - 3) - 1, 0, 1);
	v.g = 1 - clamp(abs(hue - 2) - 1, 0, 1);
	v.b = 1 - clamp(abs(hue - 4) - 1, 0, 1);
	v.a = 1;
	return v;
}

void Color::RecalcRGB() {
	Hsv2Rgb(h, s, v, r, g, b);
}

void Color::RecalcHSV() {
	Rgb2Hsv(r, g, b, h, s, v);
}

void Color::DrawSV(float x, float y, float w, float h, float hue) {
	Vec3 quadPoss[4];
	Vec2 quadUvs[4] {Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0)};
	quadPoss[0].x = x;
	quadPoss[0].y = y;
	quadPoss[1].x = x + w;
	quadPoss[1].y = y;
	quadPoss[2].x = x;
	quadPoss[2].y = y + h;
	quadPoss[3].x = x + w;
	quadPoss[3].y = y + h;
	for (int y = 0; y < 4; y++) {
		quadPoss[y].z = 1;
		//Vec3 v = quadPoss[y];
		quadPoss[y] = Ds(Display::uiMatrix*quadPoss[y]);
	}
	uint quadIndexes[4] = { 0, 1, 3, 2 };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glUseProgram(pickerProgSV);

	Vec4 bc = HueBaseCol(hue);
	GLint baseColLoc = glGetUniformLocation(pickerProgSV, "col");
	glUniform3f(baseColLoc, bc.r, bc.g, bc.b);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &quadUvs[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glUseProgram(0);

	glDisableClientState(GL_VERTEX_ARRAY);
}

void Color::DrawH(float x, float y, float w, float h) {
	Vec3 quadPoss[4];
	Vec2 quadUvs[4]{ Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0) };
	quadPoss[0].x = x;
	quadPoss[0].y = y;
	quadPoss[1].x = x + w;
	quadPoss[1].y = y;
	quadPoss[2].x = x;
	quadPoss[2].y = y + h;
	quadPoss[3].x = x + w;
	quadPoss[3].y = y + h;
	for (int y = 0; y < 4; y++) {
		quadPoss[y].z = 1;
		//Vec3 v = quadPoss[y];
		quadPoss[y] = Ds(Display::uiMatrix*quadPoss[y]);
	}
	uint quadIndexes[4] = { 0, 1, 3, 2 };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glUseProgram(pickerProgH);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &quadUvs[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glUseProgram(0);

	glDisableClientState(GL_VERTEX_ARRAY);
}

Mesh* Procedurals::Plane(uint xCount, uint yCount) {
	std::vector<Vec3> norms = std::vector<Vec3>();
	std::vector<Vec3> verts = std::vector<Vec3>();
	std::vector<Vec2> uvs = std::vector<Vec2>();
	std::vector<int> tris = std::vector<int>();
	for (uint a = 0; a <= xCount; a++) {
		for (uint b = 0; b <= yCount; b++) {
			verts.push_back(Vec3(xCount * (a*1.0f / xCount - 0.5f), 0, xCount * (b*1.0f / yCount - 0.5f)));
			norms.push_back(Vec3(0, 1, 0));
			uvs.push_back(Vec2(a*1.0f / xCount, b*1.0f / yCount));
		}
	}
	int va1, va2, vb1, vb2;
	for (uint a = 0; a < xCount; a++) {
		for (uint b = 0; b < yCount; b++) {
			va1 = (yCount + 1)*a + b;
			va2 = va1 + 1;
			vb1 = (yCount + 1)*(a + 1) + b;
			vb2 = vb1 + 1;
			/*
			if (b == uCount - 1) {
			va2 -= uCount;
			vb2 -= uCount;
			}
			*/
			tris.push_back(va1);
			tris.push_back(vb2);
			tris.push_back(vb1);
			tris.push_back(va1);
			tris.push_back(va2);
			tris.push_back(vb2);
		}
	}
	return new Mesh(verts, norms, tris, uvs);
}

Mesh* Procedurals::UVSphere(uint uCount, uint vCount) {
	std::vector<Vec3> verts, norms;
	std::vector<Vec2> uvs = std::vector<Vec2>();
	std::vector<int> tris = std::vector<int>();
	verts = std::vector<Vec3>();
	for (uint a = 0; a <= vCount; a++) {
		double theta1 = 0.5f * PI - PI*a / (vCount - 1);
		for (uint b = 0; b <= uCount; b++) {
			double theta2 = 2 * PI * b / uCount;
			verts.push_back(Vec3(cos(theta2)*cos(theta1), sin(theta1), sin(theta2)*cos(theta1)));
			uvs.push_back(Vec2(b * 1.0f / (uCount), a * 1.0f / (vCount)));
		}
	}
	norms = std::vector<Vec3>(verts);
	int va1, va2, vb1, vb2;
	for (uint a = 0; a < vCount - 1; a++) {
		for (uint b = 0; b < uCount; b++) {
			va1 = (uCount+1)*a + b;
			va2 = va1 + 1;
			vb1 = (uCount+1)*(a + 1) + b;
			vb2 = vb1 + 1;
			/*
			if (b == uCount - 1) {
				va2 -= uCount;
				vb2 -= uCount;
			}
			*/
			tris.push_back(va1);
			tris.push_back(vb2);
			tris.push_back(vb1);
			tris.push_back(va1);
			tris.push_back(va2);
			tris.push_back(vb2);
		}
	}
	return new Mesh(verts, norms, tris, uvs);
}

bool Rect::Inside(const Vec2& v) {
	return ((w > 0) ? (v.x > x && v.x < (x + w)) : (v.x >(x + w) && v.x < x)) && ((h > 0) ? (v.y > y && v.y < (y + h)) : (v.y >(y + h) && v.y < y));
}

Rect Rect::Intersection(const Rect& r2) {
	float ox = max(x, r2.x);
	float oy = max(y, r2.y);
	float p2x = min(x + w, r2.x + r2.w);
	float p2y = min(y + h, r2.y + r2.h);
	return Rect(ox, oy, p2x - ox, p2y - oy);
}

int Random::seed = 1;

float Random::Value() {
	return ((float)rand()) / ((float)RAND_MAX);
}

float Random::Range(float min, float max) {
	return min + Random::Value()*(max-min);
}

long long milliseconds() {
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
	}
	else {
		return GetTickCount64();
	}
}

Texture* Engine::fallbackTex = nullptr;

uint Engine::unlitProgram = 0;
uint Engine::unlitProgramA = 0;
uint Engine::unlitProgramC = 0;
uint Engine::skyProgram = 0;
Font* Engine::defaultFont;
Rect* Engine::stencilRect = nullptr;
bool Input::mouse0 = false;
bool Input::mouse1 = false;
bool Input::mouse2 = false;
byte Input::mouse0State = 0;
byte Input::mouse1State = 0;
byte Input::mouse2State = 0;
string Input::inputString = "";

void Engine::Init(string path) {
	if (path != "") {
		fallbackTex = new Texture(path.substr(0, path.find_last_of('\\') + 1) + "fallback.bmp");
		if (!fallbackTex->loaded)
			std::cout << "cannot load fallback texture!" << std::endl;
	}

	Material::LoadOris();
	Light::InitShadow();
	Camera::InitShaders();
	Font::Init();
#ifdef IS_EDITOR
	Editor::InitShaders();
	Editor::instance->InitMaterialPreviewer();
#endif
	string vertcode = "#version 330 core\nlayout(location = 0) in vec3 pos;\nlayout(location = 1) in vec2 uv;\nout vec2 UV;\nvoid main(){ \ngl_Position.xyz = pos;\ngl_Position.w = 1.0;\nUV = uv;\n}";
	string fragcode = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nuniform float level;\nout vec4 color;void main(){\ncolor = textureLod(sampler, UV, level)*col;\n}"; //out vec3 Vec4;\n
	string fragcode2 = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nuniform float level;\nout vec4 color;void main(){\ncolor = vec4(1, 1, 1, textureLod(sampler, UV, level).r)*col;\n}"; //out vec3 Vec4;\n
	string fragcode3 = "#version 330 core\nin vec2 UV;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = col;\n}";
	string fragcodeSky = "in vec2 UV;uniform sampler2D sampler;uniform vec2 dir;uniform float length;out vec4 color;void main(){float ay = asin((UV.y) / length);float l2 = length*cos(ay);float ax = asin((dir.x + UV.x) / l2);color = textureLod(sampler, vec2((dir.x + ax / 3.14159)*sin(dir.y + ay / 3.14159) + 0.5, (dir.y + ay / 3.14159)), 0);color.a = 1;}";

	unlitProgram = ShaderBase::FromVF(vertcode, fragcode);
	unlitProgramA = ShaderBase::FromVF(vertcode, fragcode2);
	unlitProgramC = ShaderBase::FromVF(vertcode, fragcode3);
	skyProgram = ShaderBase::FromVF(vertcode, fragcodeSky);
	ScanQuadParams();

#ifdef IS_EDITOR
	//string colorPickerV = "#version 330 core\nlayout(location = 0) in vec3 pos;\nlayout(location = 1) in vec2 uv;\nout vec2 UV;\nvoid main(){ \ngl_Position.xyz = pos;\ngl_Position.w = 1.0;\nUV = uv;\n}";
	//string colorPickerF = "#version 330 core\nin vec2 UV;\nuniform vec3 col;\nout vec4 color;void main(){\ncolor = vec4(mix(mix(col, vec3(1, 1, 1), UV.x), vec3(0, 0, 0), 1-UV.y), 1);\n}";

	std::vector<string> s2 = string_split(DefaultResources::GetStr("e_colorPickerSV.txt"), '$');
	Color::pickerProgSV = ShaderBase::FromVF(s2[0], s2[1]);

	std::vector<string> s22 = string_split(DefaultResources::GetStr("e_colorPickerH.txt"), '$');
	Color::pickerProgH = ShaderBase::FromVF(s22[0], s22[1]);
#endif
}

std::vector<std::ifstream*> Engine::assetStreams = std::vector<std::ifstream*>();
std::unordered_map<byte, std::vector<string>> Engine::assetData = std::unordered_map<byte, std::vector<string>>();
//std::unordered_map<string, byte[]> Engine::assetDataLoaded = std::unordered_map<string, byte[]>();

bool Engine::LoadDatas(string path) {
	std::ifstream* d0 = new std::ifstream(path + "\\data0");
	assetStreams.push_back(d0);
	if (!d0->is_open())
		return false;
	char header[2];
	char levelCount;
	(*d0) >> header[0] >> header[1] >> levelCount;
	if (header[0] != 'D' || header[1] != '0' || levelCount <= 0)
		return false;


	return true;
}

void Engine::BeginStencil(float x, float y, float w, float h) {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glStencilFunc(GL_NEVER, 1, 0xFF);
	glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP); // draw 1s on test fail (always)
	glStencilMask(0xFF); // draw stencil pattern
	glClear(GL_STENCIL_BUFFER_BIT); // needs mask=0xFF
	Engine::DrawQuad(x, y, w, h, white());
	//Engine::DrawQuad(v.r, v.g, v.b, v.a, white());
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilMask(0x0);
	glStencilFunc(GL_EQUAL, 1, 0xFF);

	stencilRect = new Rect(x, y, w, h);
}

void Engine::EndStencil() {
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);

	delete(stencilRect);
	stencilRect = nullptr;
}

void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, DrawTex_Scaling scl, float miplevel) {
	DrawTexture(x, y, w, h, texture, white(), scl, miplevel);
}
void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, float alpha, DrawTex_Scaling scl, float miplevel) {
	DrawTexture(x, y, w, h, texture, white(alpha), scl, miplevel);
}
void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, Vec4 tint, DrawTex_Scaling scl, float miplevel) {
	GLuint tex = (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer;
	if (scl == DrawTex_Stretch)
		DrawQuad(x, y, w, h, tex, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, tint, miplevel);
	else if (scl == DrawTex_Fit) {
		float w2h = ((float)texture->width) / texture->height;
		if (w / h > w2h)
			DrawQuad(x + 0.5f*(w - h*w2h), y, h*w2h, h, tex, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, tint, miplevel);
		else
			DrawQuad(x, y + 0.5f*(h - w / w2h), w, w / w2h, tex, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, tint, miplevel);
	}
	else {
		DrawQuad(x, y, w, h, tex, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, tint, miplevel);
	}
}

void Engine::Label(float x, float y, float s, string st, Font* font) {
	Label(x, y, s, st, font, Vec4(0, 0, 0, 1));
}
void Engine::Label(float x, float y, float s, string st, Font* font, Vec4 Vec4) {
	Label(x, y, s, st, font, Vec4, -1);
}
void Engine::Label(float x, float y, float s, string st, Font* font, Vec4 Vec4, float maxw) {
	if (s <= 0) return;
	uint sz = st.size();
	GLuint tex = font->glyph((uint)round(s));
	font->SizeVec(sz);

	byte align = (byte)font->alignment;
	if ((align & 15) > 0) {
		float totalW = 0;
		for (uint i = 0; i < sz * 4; i += 4) {
			char c = st[i / 4];
			totalW = ceil(totalW + font->w2s[c] * s + s*0.1f);
		}
		x -= totalW * (align & 15) * 0.5f;
	}

	y -= (1-(0.5f * (align >> 4))) * s;

	y = Display::height - y;
	float w = 0;
	Vec3 ds = Vec3(1.0f/Display::width, 1.0f / Display::height, 0.5f);
	x = round(x);
	for (uint i = 0; i < sz * 4; i += 4) {
		char c = st[i / 4];
		Vec3 off = -Vec3(font->off[c].x, font->off[c].y, 0)*s*ds;
		w = font->w2h[c] * s;
		font->poss[i] = Vec3(x, y - s, 1)*ds + off;
		font->poss[i + 1] = Vec3(x + s, y - s, 1)*ds + off;
		font->poss[i + 2] = Vec3(x, y, 1)*ds + off;
		font->poss[i + 3] = Vec3(x + s, y, 1)*ds + off;
		font->cs[i] = c;
		font->cs[i + 1] = c;
		font->cs[i + 2] = c;
		font->cs[i + 3] = c;
		x = ceil(x + font->w2s[c]*s + s*0.1f);
	}

	uint prog = font->fontProgram;
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &font->poss[0]);
	glUseProgram(prog);
	GLint baseColLoc = glGetUniformLocation(prog, "col");
	glUniform4f(baseColLoc, Vec4.r, Vec4.g, Vec4.b, Vec4.a);
	GLint baseImageLoc = glGetUniformLocation(prog, "sampler");
	glUniform1i(baseImageLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &font->poss[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &font->uvs[0]);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, 0, &font->cs[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6*sz, GL_UNSIGNED_INT, &font->ids[0]);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glUseProgram(0);

	glDisableClientState(GL_VERTEX_ARRAY);
}

byte Engine::Button(float x, float y, float w, float h) {
	if (stencilRect) {
		if (!stencilRect->Intersection(Rect(x, y, w, h)).Inside(Input::mousePos)) return 0;
	}
	return Rect(x, y, w, h).Inside(Input::mousePos) ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4) {
	return Button(x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f));
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4, bool labelCenter) {
	return Button(x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f), label, labelSize, labelFont, labelVec4, labelCenter);
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4) {
	if (Input::mouse0State != 0 && !Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		DrawQuad(x, y, w, h, normalVec4);
		return 0;
	}
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
	if (stencilRect) {
		if (!stencilRect->Intersection(Rect(x, y, w, h)).Inside(Input::mousePos))
			inside = false;
	}
	switch (Input::mouse0State) {
	case 0:
	case MOUSE_UP:
		DrawQuad(x, y, w, h, inside ? highlightVec4 : normalVec4);
		break;
	case MOUSE_DOWN:
	case MOUSE_HOLD:
		DrawQuad(x, y, w, h, inside ? pressVec4 : normalVec4);
		break;
	}
	return inside ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
}
byte Engine::Button(float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, float uvx, float uvy, float uvw, float uvh) {
	if (Input::mouse0State != 0 && !Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(uvx, 1 - uvy), Vec2(uvx + uvw, 1 - uvy), Vec2(uvx, 1 - uvy - uvh), Vec2(uvx + uvw, 1 - uvy - uvh), false, normalVec4);
		return 0;
	}
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
	if (stencilRect) {
		if (!stencilRect->Intersection(Rect(x, y, w, h)).Inside(Input::mousePos))
			inside = false;
	}
	switch (Input::mouse0State) {
	case 0:
	case MOUSE_UP:
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(uvx, 1 - uvy), Vec2(uvx + uvw, 1 - uvy), Vec2(uvx, 1 - uvy - uvh), Vec2(uvx + uvw, 1 - uvy - uvh), false, inside ? highlightVec4 : normalVec4);
		break;
	case MOUSE_DOWN:
	case MOUSE_HOLD:
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(uvx, 1 - uvy), Vec2(uvx + uvw, 1 - uvy), Vec2(uvx, 1 - uvy - uvh), Vec2(uvx + uvw, 1 - uvy - uvh), false, inside ? pressVec4 : normalVec4);
		break;
	}
	return inside ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4, bool labelCenter) {
	byte b = Button(x, y, w, h, normalVec4, highlightVec4, pressVec4);
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = labelCenter? ALIGN_MIDCENTER : ALIGN_MIDLEFT;
	Label(round(x + (labelCenter? w*0.5f : 2)), round(y + 0.4f*h), labelSize, label, labelFont, labelVec4);
	labelFont->alignment = al;
	return b;
}

byte Engine::EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4) {
	return EButton(a, x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f));
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4) {
	if (Input::mouse0State != 0 && !Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		DrawQuad(x, y, w, h, normalVec4);
		return 0;
	}
	if (a) {
		bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
		if (stencilRect) {
			if (!stencilRect->Intersection(Rect(x, y, w, h)).Inside(Input::mousePos))
				inside = false;
		}
		switch (Input::mouse0State) {
		case 0:
		case MOUSE_UP:
			DrawQuad(x, y, w, h, inside ? highlightVec4 : normalVec4);
			break;
		case MOUSE_DOWN:
		case MOUSE_HOLD:
			DrawQuad(x, y, w, h, inside ? pressVec4 : normalVec4);
			break;
		}
		return inside ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
	}
	else {
		DrawQuad(x, y, w, h, normalVec4);
		return false;
	}
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4) {
	return EButton(a, x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f), label, labelSize, labelFont, labelVec4);
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Texture* texture, Vec4 Vec4) {
	return EButton(a, x, y, w, h, texture, Vec4, LerpVec4(Vec4, white(), 0.5f), LerpVec4(Vec4, black(), 0.5f));
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4) {
	if (Input::mouse0State != 0 && !Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, normalVec4);
		return 0;
	}
	if (a) {
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
	if (stencilRect) {
		if (!stencilRect->Intersection(Rect(x, y, w, h)).Inside(Input::mousePos))
			inside = false;
	}
	switch (Input::mouse0State) {
	case 0:
	case MOUSE_UP:
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, inside ? highlightVec4 : normalVec4);
		break;
	case MOUSE_DOWN:
	case MOUSE_HOLD:
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, inside ? pressVec4 : normalVec4);
		break;
	}
	return inside ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
	}
	else {
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, normalVec4);
		return false;
	}
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4) {
	byte b = EButton(a, x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f));
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDLEFT;
	Label(round(x + 2), round(y + 0.4f*h), labelSize, label, labelFont, labelVec4);
	labelFont->alignment = al;
	return b;
}

bool Engine::DrawToggle(float x, float y, float s, Vec4 col, bool t) {
	byte b = Button(x, y, s, s, col);
	if (b == MOUSE_RELEASE)
		t = !t;
	return t;
}
bool Engine::DrawToggle(float x, float y, float s, Texture* texture, bool t, Vec4 col, ORIENTATION o) {
	byte b;
	if (o == 0)
		b = Button(x, y, s, s, texture, col, LerpVec4(col, white(), 0.5f), LerpVec4(col, black(), 0.5f));
	else if (o == 1)
		b = Button(x, y, s, s, texture, col, LerpVec4(col, white(), 0.5f), LerpVec4(col, black(), 0.5f), t ? 0.5f : 0, 0, 0.5f, 1);
	else
		b = Button(x, y, s, s, texture, col, LerpVec4(col, white(), 0.5f), LerpVec4(col, black(), 0.5f), 0, t ? 0.5f : 0, 1, 0.5f);
	if (b == MOUSE_RELEASE)
		t = !t;
	return t;
}

float Engine::DrawSliderFill(float x, float y, float w, float h, float min, float max, float val, Vec4 background, Vec4 foreground) {
	DrawQuad(x, y, w, h, background);
	val = clamp(val, min, max);
	float v = val, vv = (val - min)/(max-min);
	if (Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		if (Input::mouse0) {
			vv = clamp((Input::mousePos.x - (x+1)) / (w-2), 0, 1);
			v = vv*(max - min) + min;
			DrawQuad(x + 1, y + 1, (w - 2)*vv, h - 2, foreground*white(1, 0.4f));
			return v;
		}
	}
	else DrawQuad(x + 1, y + 1, (w - 2)*vv, h - 2, foreground*(Rect(x, y, w, h).Inside(Input::mousePos)? 1 : 0.8f));
	return v;
}
float Engine::DrawSliderFillY(float x, float y, float w, float h, float min, float max, float val, Vec4 background, Vec4 foreground) {
	DrawQuad(x, y, w, h, background);
	val = clamp(val, min, max);
	float v = val, vv = (val - min) / (max - min);
	if (Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		if (Input::mouse0) {
			vv = clamp((Input::mousePos.y - (y + 1)) / (h - 2), 0, 1);
			v = vv*(max - min) + min;
			DrawQuad(x + 1, y + 1 + (h-2)*(1-vv), w - 2, (h - 2)*vv, foreground*white(1, 0.4f));
			return v;
		}
	}
	else DrawQuad(x + 1, y + 1 + (h - 2)*(1 - vv), w - 2, (h - 2)*vv, foreground*(Rect(x, y, w, h).Inside(Input::mousePos) ? 1 : 0.8f));
	return v;
}

Vec2 Engine::DrawSliderFill2D(float x, float y, float w, float h, Vec2 min, Vec2 max, Vec2 val, Vec4 background, Vec4 foreground) {
	DrawQuad(x, y, w, h, background);
	val = clamp(val, min, max);
	Vec2 v = val, vv = (val - min) / (max - min);
	if (Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		if (Input::mouse0) {
			vv = clamp((Input::mousePos - Vec2(x + 1, y + 1)) / Vec2(w - 2, h - 2), Vec2(0, 0), Vec2(1, 1));
			vv.y = 1 - vv.y;
			v = vv*(max - min) + min;
			DrawQuad(x + 1, y + 1 + (h - 2)*(1 - vv.y), (w - 2)*vv.x, (h - 2)*vv.y, foreground*white(1, 0.4f));
			return v;
		}
	}
	else DrawQuad(x + 1, y + 1 + (h - 2)*(1 - vv.y), (w - 2)*vv.x, (h - 2)*vv.y, foreground*(Rect(x, y, w, h).Inside(Input::mousePos) ? 1 : 0.8f));
	return v;
}

void Engine::DrawProgressBar(float x, float y, float w, float h, float progress, Vec4 background, Texture* foreground, Vec4 tint, int padding, byte clip) {
	DrawQuad(x, y, w, h, background);
	progress = clamp(progress, 0, 100)*0.01f;
	float tx = (clip == 0) ? 1 : ((clip == 1) ? progress : w*progress / h);
	DrawQuad(x + padding, y + padding, w*progress - 2 * padding, h - 2 * padding, foreground->pointer, Vec2(0, 1), Vec2(tx, 1), Vec2(0, 0), Vec2(tx, 0), false, tint);
}

void Engine::RotateUI(float aa, Vec2 point) {
	float a = -3.1415926535f*aa / 180.0f;
	//Display::uiMatrix = glm::mat3(1, 0, 0, 0, 1, 0, point2.x * 2 - 1, point2.y * 2 - 1, 1)*glm::mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)*glm::mat3(1, 0, 0, 0, 1, 0, -point2.x * 2 + 1, -point2.y * 2 + 1, 1)*Display::uiMatrix;
	Display::uiMatrix = glm::mat3(1, 0, 0, 0, 1, 0, point.x, point.y, 1)*glm::mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)*glm::mat3(1, 0, 0, 0, 1, 0, -point.x, -point.y, 1)*Display::uiMatrix;
}
void Engine::ResetUIMatrix() {
	Display::uiMatrix = glm::mat3();
}

GLint Engine::drawQuadLocs[] = { 0, 0, 0 };
GLint Engine::drawQuadLocsA[] = { 0, 0, 0 };
GLint Engine::drawQuadLocsC[] = { 0 };

void Engine::ScanQuadParams() {
	drawQuadLocs[0] = glGetUniformLocation(unlitProgram, "sampler");
	drawQuadLocs[1] = glGetUniformLocation(unlitProgram, "col");
	drawQuadLocs[2] = glGetUniformLocation(unlitProgram, "level");

	drawQuadLocsA[0] = glGetUniformLocation(unlitProgramA, "sampler");
	drawQuadLocsA[1] = glGetUniformLocation(unlitProgramA, "col");
	drawQuadLocsA[2] = glGetUniformLocation(unlitProgramA, "level");

	drawQuadLocsC[0] = glGetUniformLocation(unlitProgramC, "col");
}

void Engine::DrawQuad(float x, float y, float w, float h, uint texture, float miplevel) {
	DrawQuad(x, y, w, h, texture, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, Vec4(1, 1, 1, 1), miplevel);
}
void Engine::DrawQuad(float x, float y, float w, float h, uint texture, Vec4 Vec4) {
	DrawQuad(x, y, w, h, texture, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, Vec4);
}
void Engine::DrawQuad(float x, float y, float w, float h, Vec4 Vec4) {
	Vec3 quadPoss[4];
	quadPoss[0].x = x;
	quadPoss[0].y = y;
	quadPoss[1].x = x + w;
	quadPoss[1].y = y;
	quadPoss[2].x = x;
	quadPoss[2].y = y + h;
	quadPoss[3].x = x + w;
	quadPoss[3].y = y + h;
	for (int y = 0; y < 4; y++) {
		quadPoss[y].z = 1;
		//Vec3 v = quadPoss[y];
		quadPoss[y] = Ds(Display::uiMatrix*quadPoss[y]);
	}
	uint quadIndexes[4] = { 0, 1, 3, 2 };
	uint prog = unlitProgramC;
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glUseProgram(prog);

	glUniform4f(drawQuadLocsC[0], Vec4.r, Vec4.g, Vec4.b, Vec4.a);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableVertexAttribArray(0);
	glUseProgram(0);

	/*
	glVec43f(1.0f, 0.0f, 0.0f);
	glLineWidth(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);
	*/

	glDisableClientState(GL_VERTEX_ARRAY);
}

void Engine::DrawQuad(float x, float y, float w, float h, GLuint texture, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, bool single, Vec4 Vec4, float miplevel) {
	Vec3 quadPoss[4];
	quadPoss[0].x = x;
	quadPoss[0].y = y;
	quadPoss[1].x = x + w;
	quadPoss[1].y = y;
	quadPoss[2].x = x;
	quadPoss[2].y = y + h;
	quadPoss[3].x = x + w;
	quadPoss[3].y = y + h;
	for (int y = 0; y < 4; y++) {
		quadPoss[y].z = 1;
		//Vec3 v = quadPoss[y];
		quadPoss[y] = Ds(Display::uiMatrix*quadPoss[y]);
	}
	Vec2 quadUv[4]{uv0, uv1, uv2, uv3};
	uint quadIndexes[4] = { 0, 1, 3, 2 };
	uint prog = single ? unlitProgramA : unlitProgram;
	glEnableClientState(GL_VERTEX_ARRAY);
	
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	//glTexCoordPointer(2, GL_FLOAT, 0, &quadUv);
	glUseProgram(prog);

	glUniform1i(single? drawQuadLocsA[0] : drawQuadLocs[0], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glUniform4f(single ? drawQuadLocsA[1] : drawQuadLocs[1], Vec4.r, Vec4.g, Vec4.b, Vec4.a);
	glUniform1f(single ? drawQuadLocsA[2] : drawQuadLocs[2], miplevel);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &quadUv[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glUseProgram(0);

	/*
	glVec43f(1.0f, 0.0f, 0.0f);
	glLineWidth(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);
	*/

	glDisableClientState(GL_VERTEX_ARRAY);
	//glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	//glDisable(GL_DEPTH_TEST);
}

void Engine::DrawCube(Vec3 pos, float dx, float dy, float dz, Vec4 Vec4) {
	Vec3 quadPoss[8];
	quadPoss[0] = pos + Vec3(-dx, -dy, -dz);
	quadPoss[1] = pos + Vec3(dx, -dy, -dz);
	quadPoss[2] = pos + Vec3(-dx, dy, -dz);
	quadPoss[3] = pos + Vec3(dx, dy, -dz);
	quadPoss[4] = pos + Vec3(-dx, -dy, dz);
	quadPoss[5] = pos + Vec3(dx, -dy, dz);
	quadPoss[6] = pos + Vec3(-dx, dy, dz);
	quadPoss[7] = pos + Vec3(dx, dy, dz);
	uint quadIndexes[24] = { 0, 1, 3, 2, 4, 5, 7, 6, 0, 2, 6, 4, 1, 3, 7, 5, 0, 1, 5, 4, 2, 3, 7, 6 };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glColor4f(Vec4.r, Vec4.g, Vec4.b, Vec4.a);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableClientState(GL_VERTEX_ARRAY);
}

void Engine::DrawIndices(const Vec3* poss, const int* is, int length, float r, float g, float b) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, poss);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor4f(r, g, b, 1);
	glDrawElements(GL_TRIANGLES, length, GL_UNSIGNED_INT, is);
	glDisableClientState(GL_VERTEX_ARRAY);
}
void Engine::DrawIndicesI(const Vec3* poss, const int* is, int length, float r, float g, float b) {
	glVertexPointer(3, GL_FLOAT, 0, poss);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor4f(r, g, b, 1);
	glDrawElements(GL_TRIANGLES, length, GL_UNSIGNED_INT, is);
}

void Engine::DrawLine(Vec2 v1, Vec2 v2, Vec4 col, float width) {
	DrawLine(Vec3(v1.x, v1.y, 1), Vec3(v2.x, v2.y, 1), col, width);
}
void Engine::DrawLine(Vec3 v1, Vec3 v2, Vec4 col, float width) {
	Vec3 quadPoss[2];
	quadPoss[0] = v1;
	quadPoss[1] = v2;
	for (int y = 0; y < 2; y++) {
		quadPoss[y] = Ds(Display::uiMatrix*quadPoss[y]);
	}
	uint quadIndexes[2] = { 0, 1 };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);
	glColor4f(col.r, col.g, col.b, col.a);
	glLineWidth(width);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, &quadIndexes[0]);
	//glDisableVertexAttribArray(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}
void Engine::DrawLineW(Vec3 v1, Vec3 v2, Vec4 col, float width) {
	Vec3 quadPoss[2];
	quadPoss[0] = v1;
	quadPoss[1] = v2;
	uint quadIndexes[2] = { 0, 1 };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glColor4f(col.r, col.g, col.b, col.a);
	glLineWidth(width);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, &quadIndexes[0]);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Engine::DrawLineWDotted(Vec3 v1, Vec3 v2, Vec4 col, float width, float dotSz, bool app) {
	std::vector<Vec3> quadPoss = std::vector<Vec3>();
	std::vector<uint> quadIndexes = std::vector<uint>();
	Vec3 p2 = v1;
	uint aa = 0;
	bool ad = true;
	while (Distance(p2, v1) < Distance(v2, v1)) {
		quadPoss.push_back(p2);
		if (Distance(p2, v2) < dotSz) {
			quadPoss.push_back(v2);
			ad = false;
		}
		else
			quadPoss.push_back(p2 + Normalize(v2-v1)*dotSz);
		quadIndexes.push_back(aa++);
		quadIndexes.push_back(aa++);
		p2 += Normalize(v2 - v1)*dotSz*2.0f;
	}

	if (quadPoss.size() == 0) return;
	if (ad) {
		quadPoss.push_back(v2);
		quadPoss.push_back(v2 + Normalize(v1 - v2)*dotSz);
		quadIndexes.push_back(aa++);
		quadIndexes.push_back(aa++);
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glColor4f(col.r, col.g, col.b, col.a);
	glLineWidth(width);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_LINES, quadIndexes.size(), GL_UNSIGNED_INT, &quadIndexes[0]);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Engine::DrawTriangle(Vec2 v1, Vec2 v2, Vec2 v3, Vec4 col, bool fill, float width) {
	Vec3 quadPoss[] = { Vec3(v1.x, v1.y, 1), Vec3(v2.x, v2.y, 1), Vec3(v3.x, v3.y, 1) };
	for (int y = 0; y < 3; y++) {
		quadPoss[y] = Ds(Display::uiMatrix*quadPoss[y]);
	}
	uint quadIndexes[3] = {0, 1, 2};
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glColor4f(col.r, col.g, col.b, col.a);
	glLineWidth(width);
	glPolygonMode(GL_FRONT_AND_BACK, fill? GL_FILL : GL_LINE);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &quadIndexes[0]);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Engine::DrawTriangle(Vec2 centre, Vec2 dir, Vec4 col, bool fill, float width) {
	Vec2 x1(centre + dir);
	float a120 = PI*0.6666667f;
	float a240 = PI*1.3333333f;
	Vec2 x2(centre + glm::mat2(cos(a120), sin(a120), -sin(a120), cos(a120))*dir);
	Vec2 x3(centre + glm::mat2(cos(a240), sin(a240), -sin(a240), cos(a240))*dir);
	DrawTriangle(x1, x2, x3, col, fill, width);
}

void Engine::DrawCircle(Vec2 c, float r, uint n, Vec4 col, float width) {
	if (n < 3) {
		Debug::Warning("DrawCircle", "only n of 3 and above allowed!");
		return;
	}
	std::vector<Vec3> poss = std::vector<Vec3>(n);
	std::vector<uint> ids = std::vector<uint>(n * 2 - 1);
	for (uint y = 0; y < n; y++) {
		Vec3 vv = Vec3(sin(y * 6.283f / (n + 1)) * r + c.x, cos(y * 6.283f / (n + 1)) * r + c.y, 0);
		poss[y] += Ds(vv);
		ids[y*2] += y;
		if (y != (n-1)) ids[y*2+1] += y+1;
	}
	ids.push_back(0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &poss[0]);
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);
	glColor4f(col.r, col.g, col.b, col.a);
	glLineWidth(width);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_LINES, ids.size(), GL_UNSIGNED_INT, &ids[0]);
	//glDisableVertexAttribArray(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Engine::DrawCircleW(Vec3 c, Vec3 x, Vec3 y, float r, uint n, Vec4 col, float width, bool dotted) {
	if (n < 3) {
		Debug::Warning("DrawCircle", "only n of 3 and above allowed!");
		return;
	}
	std::vector<Vec3> poss = std::vector<Vec3>(n);
	std::vector<uint> ids = std::vector<uint>(dotted? n : n * 2 - 1);
	for (uint a = 0; a < n; a++) {
		poss[a] += c + sin(a * 6.283f / (n)) * x * r + cos(a * 6.283f / (n)) * y * r;
		if (dotted)
			ids[a] += a;
		else {
			ids[a * 2] += a;
			if (a != (n - 1)) ids[a * 2 + 1] += a + 1;
		}
	}
	ids.push_back(0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &poss[0]);
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);
	glColor4f(col.r, col.g, col.b, col.a);
	glLineWidth(width);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_LINES, ids.size(), GL_UNSIGNED_INT, &ids[0]);
	//glDisableVertexAttribArray(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Engine::DrawCubeLinesW(float x0, float x1, float y0, float y1, float z0, float z1, float width, Vec4 col) {
	Vec3 quadPoss[8];
	quadPoss[0] = Vec3(x0, y0, z0);
	quadPoss[1] = Vec3(x1, y0, z0);
	quadPoss[2] = Vec3(x0, y1, z0);
	quadPoss[3] = Vec3(x1, y1, z0);
	quadPoss[4] = Vec3(x0, y0, z1);
	quadPoss[5] = Vec3(x1, y0, z1);
	quadPoss[6] = Vec3(x0, y1, z1);
	quadPoss[7] = Vec3(x1, y1, z1);
	uint quadIndexes[24] = { 0, 1, 3, 2, 4, 5, 7, 6, 0, 2, 6, 4, 1, 3, 7, 5, 0, 1, 5, 4, 2, 3, 7, 6 };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glColor4f(col.r, col.g, col.b, col.a);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(width);
	glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableClientState(GL_VERTEX_ARRAY);
}

bool Input::keyStatusOld[] = {};
bool Input::keyStatusNew[] = {};

bool Input::KeyDown(InputKey k) {
	return keyStatusNew[k] && !keyStatusOld[k];
}

bool Input::KeyHold(InputKey k) {
	return keyStatusNew[k];
}


bool Input::KeyUp(InputKey k) {
	return !keyStatusNew[k] && keyStatusOld[k];
}

void Input::UpdateMouseNKeyboard() {
	std::swap(keyStatusOld, keyStatusNew);
	for (byte a = 1; a < 112; a++) {
		keyStatusNew[a] = ((GetAsyncKeyState(a) >> 8) == -128);
	}
	for (byte a = 187; a < 191; a++) {
		keyStatusNew[a] = ((GetAsyncKeyState(a) >> 8) == -128);
	}
	
	inputString = "";
	bool shift = KeyHold(Key_Shift);
	for (byte c = Key_0; c <= Key_9; c++) {
		if (KeyDown((InputKey)c)) {
			inputString += char(c);
		}
	}
	for (byte b = Key_A; b <= Key_Z; b++) {
		if (KeyDown((InputKey)b)) {
			inputString += shift ? char(b) : char(b + 32);
		}
	}

	if (mouse0)
		mouse0State = min(mouse0State+1, MOUSE_HOLD);
	else
		mouse0State = ((mouse0State == MOUSE_UP) | (mouse0State == 0)) ? 0 : MOUSE_UP;
	if (mouse1)
		mouse1State = min(mouse1State + 1, MOUSE_HOLD);
	else
		mouse1State = ((mouse1State == MOUSE_UP) | (mouse1State == 0)) ? 0 : MOUSE_UP;
	if (mouse2)
		mouse2State = min(mouse2State + 1, MOUSE_HOLD);
	else
		mouse2State = ((mouse2State == MOUSE_UP) | (mouse2State == 0)) ? 0 : MOUSE_UP;

	if (mouse0State == MOUSE_DOWN)
		mouseDownPos = mousePos;
	else if (mouse0State == 0)
		mouseDownPos = Vec2(-1, -1);

	mouseDelta = mousePos - mousePosOld;
	mousePosOld = mousePos;
}

void Display::Resize(int x, int y, bool maximize) {
	ShowWindow(GetActiveWindow(), maximize? SW_MAXIMIZE : SW_NORMAL);
	glutReshapeWindow(x, y);
}

ulong Engine::idCounter = 0;
ulong Engine::GetNewId() {
	if (++idCounter >= ULONG_MAX) {
		idCounter = 0;
		Debug::Warning("Engine", "max id count reached! (" + to_string(idCounter) + ")");
	}
	return idCounter;
}

//std::vector<Camera*> Engine::sceneCameras();

//-----------------debug class-----------------------
void Debug::Message(string c, string s) {
#ifndef IS_EDITOR
	*stream << "[i]" << c << ": " << s << std::endl;
#endif
	std::cout << "[i]" << c << ": " << s << std::endl;
}
void Debug::Warning(string c, string s) {
#ifndef IS_EDITOR
	*stream << "[w]" << c << ": " << s << std::endl;
#endif
	std::cout << "[w]" << c << ": " << s << std::endl;
}
void Debug::Error(string c, string s) {
#ifndef IS_EDITOR
	*stream << "[e]" << c << ": " << s << std::endl;
#endif
	std::cout << "[e]" << c << " says: " << s << std::endl;
	abort();
}

void Debug::DoDebugObjectTree(std::vector<SceneObject*> o, int i) {
	for (SceneObject* oo : o) {
		string s("");
		for (int a = 0; a < i; a++)
			s += " ";
		s += "o \"" + oo->name + "\" (";
		for (Component* cc : oo->_components) {
			s += cc->name + ", ";
		}
		s += ")";
#ifndef IS_EDITOR
		*stream << s << std::endl;
#endif
		std::cout << s << std::endl;
		DoDebugObjectTree(oo->children, i + 1);
	}
}

void Debug::ObjectTree(std::vector<SceneObject*> o) {
	Message("ObjectTree", "Start");
	DoDebugObjectTree(o, 0);
	Message("ObjectTree", "End");
}

std::ofstream* Debug::stream = nullptr;
void Debug::Init(string s) {
#ifndef IS_EDITOR
	string ss = s + "Log.txt";
	stream = new std::ofstream(ss.c_str(), std::ios::out | std::ios::trunc);
	Message("Debug", "Log init'd");
#endif
}
//-----------------io class-----------------------
std::vector<string> IO::GetFiles(const string& folder, string ext)
{
	if (folder == "") return std::vector<string>();
	std::vector<string> names;
	string search_path = folder + "/*" + ext;
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(folder + "\\" + fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}
std::vector<EB_Browser_File> IO::GetFilesE (Editor* e, const string& folder)
{
#ifndef IS_EDITOR
//#error you cannot call GetFilesE! (Editor Functions only)
	abort(); //fuck you
#endif
	std::vector<EB_Browser_File> names;
	string search_path = folder + "/*.*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all importable files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				string aa(fd.cFileName);
				if ((aa.length() > 5 && aa.substr(aa.length() - 5) == ".meta") &&
					(aa.length() < 7 || aa.substr(aa.length() - 7) != ".h.meta"))
						names.push_back(EB_Browser_File(e, folder, aa.substr(0, aa.length() - 5), aa));
				else if ((aa.length() > 4 && aa.substr(aa.length() - 4) == ".cpp"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 2 && aa.substr(aa.length() - 2) == ".h"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 4 && aa.substr(aa.length() - 4) == ".txt"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 9 && (aa.substr(aa.length() - 9) == ".material")))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 9 && (aa.substr(aa.length() - 7) == ".effect")))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}

void IO::GetFolders(const string& folder, std::vector<string>* names, bool hidden)
{
	//std::vector<string> names;
	string search_path = folder + "/*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (hidden || !(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))) {
				names->push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
}

bool IO::HasDirectory (string szPath)
{
	DWORD dwAttrib = GetFileAttributes(&szPath[0]);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool IO::HasFile(string szPath)
{
	DWORD dwAttrib = GetFileAttributes(&szPath[0]);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES);// && (dwAttrib & FILE_ATTRIBUTE_NORMAL));
}

string IO::ReadFile(const string& path) {
	std::ifstream stream(path.c_str());
	if (!stream.good()) {
		std::cout << "not found! " << path << std::endl;
		return "";
	}
	std::stringstream buffer;
	buffer << stream.rdbuf();
	return buffer.str();
}

std::vector<string> IO::GetRegistryKeys(HKEY key) {
	TCHAR    achKey[255];
	TCHAR    achClass[MAX_PATH] = TEXT("");
	DWORD    cchClassName = MAX_PATH;
	DWORD	 size;
	std::vector<string> res;

	if (RegQueryInfoKey(key, achClass, &cchClassName, NULL, &size, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
		DWORD cbName = 255;
		for (uint i = 0; i < size; i++) {
			if (RegEnumKeyEx(key, i, achKey, &cbName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
				res.push_back(achKey);
			}
		}
	}
	return res;
}

string IO::GetText(const string& path) {
	std::ifstream strm(path);
	std::stringstream ss;
	ss << strm.rdbuf();
	return ss.str();
}

BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM lParam) {
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	auto v = (std::pair<DWORD, HWND>*)lParam;
	if (lpdwProcessId == v->first)
	{
		v->second = hwnd;
		return FALSE;
	}
	return TRUE;
}

HWND WinFunc::GetHwndFromProcessID(DWORD id) {
	auto var = std::pair<DWORD, HWND>(id, 0);
	EnumWindows(EnumWindowsProcMy, (LPARAM)&var);
	return var.second;
}

//-----------------time class---------------------
long long Time::startMillis = 0;
long long Time::millis = 0;
double Time::time = 0;
float Time::delta = 0;

//-----------------Vec4s--------------------------
Vec4 black(float f) { return Vec4(0, 0, 0, f); }
Vec4 red(float f, float i) { return Vec4(i, 0, 0, f); }
Vec4 green(float f, float i) { return Vec4(0, i, 0, f); }
Vec4 blue(float f, float i) { return Vec4(0, 0, i, f); }
Vec4 cyan(float f, float i) { return Vec4(i*0.09f, i*0.706f, i, f); }
Vec4 yellow(float f, float i) { return Vec4(i, i, 0, f); }
Vec4 white(float f, float i) { return Vec4(i, i, i, f); }
Vec4 LerpVec4(Vec4 a, Vec4 b, float f) {
	if (f > 1)
		return b;
	else if (f < 0)
		return a;
	else return a*(1 - f) + b*f;
}

float clamp(float f, float a, float b) {
	return min(b, max(f, a));
}

float repeat(float f, float a, float b) {
	assert(b > a);
	float fn = f;
	while (f >= b) {
		fn -= (b - a);
		if (fn == f) {
			Debug::Warning("Repeat", "Floating-point accuracy exceeded. Returning max.");
			return b;
		}
	}
	while (f < a) {
		fn += (b - a);
		if (fn == f) {
			Debug::Warning("Repeat", "Floating-point accuracy exceeded. Returning min.");
			return a;
		}
	}
	return fn;
}

//-----------------font class---------------------
FT_Library Font::_ftlib = nullptr;
GLuint Font::fontProgram = 0;
void Font::Init() {
	int err = FT_Init_FreeType(&_ftlib);
	if (err != FT_Err_Ok) {
		Debug::Error("Font", "Fatal: Initializing freetype failed!");
		std::runtime_error("Fatal: Initializing freetype failed!");
	}

	string error;
	GLuint vs, fs;
	string frag = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = vec4(1, 1, 1, texture(sampler, UV).r)*col;\n}";
	if (!ShaderBase::LoadShader(GL_VERTEX_SHADER, DefaultResources::GetStr("fontVert.txt"), vs, &error)) {
		Debug::Error("Engine", "Fatal: Cannot init font shader(v)! " + error);
		abort();
	}
	if (!ShaderBase::LoadShader(GL_FRAGMENT_SHADER, frag, fs, &error)) {
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
}

Font::Font(const string& path, int size) {
	if (FT_New_Face(_ftlib, path.c_str(), 0, &_face) != FT_Err_Ok) {
		Debug::Error("Font", "Failed to load font!");
		return;
	}
	//FT_Set_Char_Size(_face, 0, (FT_F26Dot6)(size * 64.0f), Display::dpi, 0); // set pixel size based on dpi
	FT_Set_Pixel_Sizes(_face, 0, size); // set pixel size directly
	FT_Select_Charmap(_face, FT_ENCODING_UNICODE);
	CreateGlyph(size, true);
	SizeVec(20);
}

void Font::SizeVec(uint sz) {
	if (vecSize >= sz) return;
	poss.resize(sz * 4);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (sz + 1) * 16, (sz + 1) * 16, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
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
				w2s[a] = 0.5f/sz;
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

//--------------------Display class--------------
int Display::width = 512;
int Display::height = 512;
glm::mat3 Display::uiMatrix = glm::mat3();

//--------------------Input class--------------
Vec2 Input::mousePos = Vec2(0, 0);
Vec2 Input::mousePosRelative = Vec2(0, 0);
Vec2 Input::mousePosOld = Vec2(0, 0);
Vec2 Input::mouseDelta = Vec2(0, 0);
Vec2 Input::mouseDownPos = Vec2(0, 0);

float BezierSolveApprox(Vec2& v1, Vec2& v2, Vec2& v3, Vec2& v4, float x, float acc = 0.5f) {
	if (x < v1.x)
		return v1.y;
	else if (x > v4.x)
		return v4.y;

	Vec2 a = v4 - 3.0f * v3 + 3.0f * v2 - v1;
	Vec2 b = 3.0f * v3 - 6.0f * v2 + 3.0f * v1;
	Vec2 c = 3.0f * v2 - 3.0f * v1;
	Vec2 d = v1;
	Vec2 val;
	float t = 0.5f;
	float incre = 0.25f;
	do {
		val = t*t*t*a + t*t*b + t*c + d;
		if (val.x > x)
			t -= incre;
		else
			t += incre;
		incre *= 0.5f;
	} while (abs(val.x - x) > acc);
	return val.y;
}

float BezierSolve(Vec2& v1, Vec2& v2, Vec2& v3, Vec2& v4, float x) { //x is between v1.x & v2.x
	if (x < v1.x)
		return v1.y;
	else if (x > v4.x)
		return v4.y;
	float a = v4.x - 3 * v3.x + 3 * v2.x - v1.x;
	float b = 3 * v3.x - 6 * v2.x + 3 * v1.x;
	float c = 3 * v2.x - 3 * v1.x;
	float d = v1.x - x;
	b /= a;
	c /= a;
	d /= a;
	float disc, q, r, dum1, t, term1, r13;
	q = (3 * c - b*b) / 9.0f;
	r = (-27 * d + b*(9 * c - 2 * b*b)) / 54.0f;
	disc = q*q*q + r*r;
	float t1, t2, t3;
	q = -q;
	term1 = b / 3.0f;
	dum1 = q*q*q;
	dum1 = acos(r / sqrt(dum1));
	r13 = 2 * sqrt(q);
	t1 = -term1 + r13*cos(dum1 / 3.0f);
	t2 = -term1 + r13*cos((dum1 + 2 * PI) / 3.0f);
	t3 = -term1 + r13*cos((dum1 + 4 * PI) / 3.0f);

	t = 0;
	if (t1 > 0 && t1 < 1)
		t += t1;
	else if (t2 > 0 && t2 < 1)
		t += t2;
	else if (t3 > 0 && t3 < 1)
		t += t3;
	else {
		Debug::Error("Bezier solver", "Cannot solve for t!");
		return 0;
	}

	a = v4.y - 3 * v3.y + 3 * v2.y - v1.y;
	b = 3 * v3.y - 6 * v2.y + 3 * v1.y;
	c = 3 * v2.y - 3 * v1.y;
	d = v1.y;
	return t*t*t*a + t*t*b + t*c + d;
}

float FCurve::Eval(float t, float repeat) {
	if (!repeat) {
		if (t <= startTime)
			return keys[0].point.y;
		else if (t >= endTime)
			return keys[keyCount-1].point.y;
	}
	while (t > endTime)
		t -= (endTime - startTime);
	while (t < startTime)
		t += (endTime - startTime);

	for (uint i = 0; i < keyCount; i++) {
		if (keys[i].point.x >= t) {
			return BezierSolveApprox(keys[i - 1].point, keys[i-1].right, keys[i].left, keys[i].point, t, 0.2f);
		}
	}
	return 0;
}

void _StreamWrite(const void* val, std::ofstream* stream, int size) {
	stream->write(reinterpret_cast<char const *>(val), size);
}

void _StreamWriteAsset(Editor* e, std::ofstream* stream, ASSETTYPE t, ASSETID i) {
	if (i < 0) {
		(*stream) << (char)0;
		return;
	}
	string p;
	if (t == ASSETTYPE_SCRIPT_H)
		p = e->headerAssets[i];
	else
		p = e->normalAssets[t][i];
	(*stream) << p << char0;
}

ASSETID _Strm2H(std::istream& strm) {
	return -1;
}
string _Strm2Asset(std::istream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int max) {
	char* c = new char[max];
	strm.getline(c, max, (char)0);
	string s(c);
#ifdef IS_EDITOR
	e->GetAssetInfo(s, t, i);
#else
	i = AssetManager::GetAssetId(s, t);
#endif
	delete[](c);
	return s;
}

std::vector<float> hdr2float(byte imagergbe[], int w, int h) {
	std::vector<float> image(w * h * 3 * sizeof(float));
	for (int i = 0; i < w*h; i++) {
		unsigned char exponent = imagergbe[i * 4 + 3];
		if (exponent == 0) {
			image[i * 3 + 0] = 0.0f;
			image[i * 3 + 1] = 0.0f;
			image[i * 3 + 2] = 0.0f;
		}
		else {
			double v = (1.0f / 256.0f) * pow(2, (float)(exponent - 128));
			image[i * 3 + 0] = (float)((imagergbe[i * 4 + 0] + 0.5f) * v);
			image[i * 3 + 1] = (float)((imagergbe[i * 4 + 1] + 0.5f) * v);
			image[i * 3 + 2] = (float)((imagergbe[i * 4 + 2] + 0.5f) * v);
		}
	}
	return image;
}

float B2F(char c[]) {
	float ff;
	*((char*)(&ff) + 0) = c[0];
	*((char*)(&ff) + 1) = c[1];
	*((char*)(&ff) + 2) = c[2];
	*((char*)(&ff) + 3) = c[3];
	return ff;
}

void Serialize(Editor* e, SceneObject* o, std::ofstream* stream) {
	/*
	Object data
	O[tx][ty][tz][rx][ry][rz][sx][sy][sz] (trs=float32)
	(components)
	C[type][...]c (type=byte)
	(child objects...)
	o
	*/
	*stream << "O";
	//_StreamWrite(&o->name, stream, o->name.size());
	*stream << o->name;
	*stream << (char)0;
	_StreamWrite(&o->transform.position.x, stream, 4);
	_StreamWrite(&o->transform.position.y, stream, 4);
	_StreamWrite(&o->transform.position.z, stream, 4);
	const Vec3& rot = o->transform.eulerRotation();
	_StreamWrite(&rot.x, stream, 4);
	_StreamWrite(&rot.y, stream, 4);
	_StreamWrite(&rot.z, stream, 4);
	_StreamWrite(&o->transform.scale.x, stream, 4);
	_StreamWrite(&o->transform.scale.y, stream, 4);
	_StreamWrite(&o->transform.scale.z, stream, 4);
	for (Component* c : o->_components)
	{
		stream->write("C", 1);
		byte t = c->componentType;
		_StreamWrite(&t, stream, 1);
		c->Serialize(e, stream);
		stream->write("c", 1);
	}
	for (SceneObject* oo : o->children)
	{
		Serialize(e, oo, stream);
	}
	*stream << "o";
}

void Deserialize(std::ifstream& stream, SceneObject* obj) {
	char cc[100];
	stream.getline(cc, 100, 0);
	obj->name = string(cc);
	Debug::Message("Object Deserializer", "Deserializing object " + obj->name);
	char c;
	_Strm2Val(stream, obj->transform.position.x);
	_Strm2Val(stream, obj->transform.position.y);
	_Strm2Val(stream, obj->transform.position.z);
	Vec3 r;
	_Strm2Val(stream, r.x);
	_Strm2Val(stream, r.y);
	_Strm2Val(stream, r.z);
	_Strm2Val(stream, obj->transform.scale.x);
	_Strm2Val(stream, obj->transform.scale.y);
	_Strm2Val(stream, obj->transform.scale.z);
	obj->transform.eulerRotation(r); //so matrix will be updated
	c = stream.get();
	while (c != 'o') {
		//Debug::Message("Object Deserializer", to_string(c) + " " + to_string(stream.tellg()));
		if (c == 'O') {
			SceneObject* sc = new SceneObject();
			obj->AddChild(sc);
			Deserialize(stream, sc);
		}
		else if (c == 'C') {
			c = stream.get(); //component type
			Debug::Message("Object Deserializer", "component " + to_string(c) + " " + to_string((int)c));
			switch (c) {
			case COMP_CAM:
				obj->AddComponent(new Camera(stream, obj));
				break;
			case COMP_MFT:
				obj->AddComponent(new MeshFilter(stream, obj));
				break;
			case COMP_MRD:
				obj->AddComponent(new MeshRenderer(stream, obj));
				break;
			case COMP_TRD:
				obj->AddComponent(new TextureRenderer(stream, obj));
				break;
			case (char)COMP_LHT:
				obj->AddComponent(new Light(stream, obj));
				break;
			case (char)COMP_RDP:
				obj->AddComponent(new ReflectionProbe(stream, obj));
				break;
			case (char)COMP_SCR:
#ifdef IS_EDITOR
				obj->AddComponent(new SceneScript(stream, obj));
#else
				obj->AddComponent(SceneScriptResolver::instance->Resolve(stream, obj));
#endif
				break;
			default:
				std::cout << "unknown component " << (int)c << "!" << std::endl;
				char cc;
				cc = stream.get();
				while (cc != 'c')
					stream >> cc;
				long long ss = stream.tellg();
				stream.seekg(ss - 1);
				break;
			}
			//Debug::Message("Object Deserializer", "2 " + to_string(stream.tellg()));
			c = stream.get();
			if (c != 'c') {
				Debug::Error("Object Deserializer", "scene data corrupted(component)");
				abort();
			}
		}
		else {
			Debug::Error("Object Deserializer", "scene data corrupted(2)");
			abort();
		}
		c = stream.get();
	}
	Debug::Message("Object Deserializer", "-- End --");
}

void Scene::ReadD0() {
	ushort num;
	_Strm2Val(*strm, num);
	for (ushort a = 0; a < num; a++) {
		uint sz;
		char c[100];
#ifndef IS_EDITOR
		if (_pipemode) {
			strm->getline(c, 100);
			sceneNames.push_back(c);
			sceneEPaths.push_back(AssetManager::eBasePath + sceneNames[a]);
			Debug::Message("AssetManager", "Registered scene " + sceneNames[a]);
		} else {
#else
		{
#endif
			_Strm2Val(*strm, sz);
			*strm >> c[0];
			long p1 = (long)strm->tellg();
			scenePoss.push_back(p1);
			*strm >> c[0] >> c[1];
			if (c[0] != 'S' || c[1] != 'N') {
				Debug::Error("Scene Importer", "fatal: Scene Signature wrong!");
				return;
			}
			strm->getline(c, 100, 0);
			sceneNames.push_back(c);
			strm->seekg(p1 + sz + 1);
			Debug::Message("AssetManager", "Registered scene " + sceneNames[a] + " (" + to_string(sz) + " bytes)");
		}
	}
}

void Scene::Unload() {
	
}

Scene::Scene(std::ifstream& stream, long pos) : sceneName("") {
	Debug::Message("SceneLoader", "Begin Loading Scene...");
	std::vector<SceneObject*>().swap(objects);
	stream.seekg(pos);
	char h1, h2;
	stream >> h1 >> h2;
	if (h1 != 'S' || h2 != 'N')
		Debug::Error("Scene Loader", "scene data corrupted");
	char* cc = new char[100];
	stream.getline(cc, 100, 0);
	sceneName += string(cc);
	ASSETTYPE t;

	_Strm2Asset(stream, Editor::instance, t, settings.skyId);
	if (settings.skyId >= 0 && t != ASSETTYPE_HDRI) {
		Debug::Error("Scene", "Sky asset invalid!");
		return;
	}

	Debug::Message("SceneLoader", "Loading SceneObjects...");
	char o;
	stream >> o;
	while (!stream.eof() && o == 'O') {
		SceneObject* sc = new SceneObject();
		objects.push_back(sc);
		Deserialize(stream, sc);
		sc->Refresh();
		stream >> o;
	}

	Debug::Message("SceneLoader", "Loading Background...");
	settings.sky = _GetCache<Background>(t, settings.skyId);
	Debug::Message("SceneLoader", "Scene Loaded.");
}

/*
Scene::~Scene() {
	for (SceneObject* o : objects)
		delete(o);
	objects.resize(0);
}
*/

void Scene::AddObject(SceneObject* object, SceneObject* parent) {
	if (active == nullptr)
		return;
	if (parent == nullptr)
		active->objects.push_back(object);
	else
		parent->AddChild(object);
}

void Scene::DeleteObject(SceneObject* o) {
	if (active == nullptr)
		return;
	o->_pendingDelete = true;
}

std::shared_ptr<Scene> Scene::active = nullptr;
std::ifstream* Scene::strm = nullptr;
#ifndef IS_EDITOR
std::vector<string> Scene::sceneEPaths = {};
#endif
std::vector<string> Scene::sceneNames = {};
std::vector<long> Scene::scenePoss = {};

void Scene::Load(string name) {
	for (uint a = sceneNames.size(); a > 0; a--) {
		if (sceneNames[a-1] == name) {
			Load(a-1);
			return;
		}
	}
}

void Scene::Load(uint i) {
	if (i >= sceneNames.size()) {
		Debug::Error("Scene Loader", "Scene ID (" + to_string(i) + ") out of range!");
		return;
	}
	Debug::Message("Scene Loader", "Loading scene " + to_string(i) + "...");
#ifndef IS_EDITOR
	if (_pipemode) {
		std::ifstream strm2(sceneEPaths[i], std::ios::binary);
		active = std::make_shared<Scene>(strm2, 0);
	} else {
#else
	{
#endif
		active = std::make_shared<Scene>(*strm, scenePoss[i]);
	}
	active->sceneId = i;
	Debug::Message("Scene Loader", "Loaded scene " + to_string(i) + "(" + sceneNames[i] + ")");
}

void Scene::Save(Editor* e) {
	std::ofstream sw(e->projectFolder + "Assets\\" + sceneName + ".scene", std::ios::out | std::ios::binary);
	sw << "SN";
	sw << sceneName << (char)0;
	_StreamWriteAsset(e, &sw, ASSETTYPE_HDRI, settings.skyId);
	for (SceneObject* sc : objects) {
		Serialize(e, sc, &sw);
	}
	sw.close();
	int a = 0;
	for (void* v : e->normalAssetCaches[ASSETTYPE_MATERIAL]) {
		if (v != nullptr) {
			Material* m = (Material*)v;
			//if (m->_changed) {
				m->Save(e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_MATERIAL][a]);
			//}
		}
		a++;
	}
	//
	//e->includedScenes.clear();
	//e->includedScenes.push_back(e->projectFolder + "Assets\\test.scene");
}

std::unordered_map<ASSETTYPE, std::vector<string>> AssetManager::names = {};
std::unordered_map<ASSETTYPE, std::vector<std::pair<byte, uint>>> AssetManager::dataLocs = {};
std::unordered_map<ASSETTYPE, std::vector<AssetObject*>> AssetManager::dataCaches = {};
std::vector<std::ifstream*> AssetManager::streams = {};
#ifndef IS_EDITOR
string AssetManager::eBasePath = "";
std::unordered_map<ASSETTYPE, std::vector<string>> AssetManager::dataELocs = {};
#endif
void AssetManager::Init(string dpath) {
#ifndef IS_EDITOR
	names.clear();
	dataLocs.clear();
	std::vector<std::ifstream*>().swap(streams);
	if (_pipemode) {
		string pp = dpath + "0_";
		Scene::strm = new std::ifstream(pp.c_str(), std::ios::in | std::ios::binary);
		std::ifstream* strm = Scene::strm;
		if (!strm->is_open()) {
			Debug::Error("AssetManager", "Fatal: cannot open data file 0!");
			abort();
		}

		ushort num;
		ASSETTYPE type;
		char c[100];
		*strm >> c[0] >> c[1];
		if (c[0] != 'D' || c[1] != '0') {
			Debug::Error("AssetManager", "Fatal: file 0 has wrong signature!");
			abort();
		}
		strm->getline(c, 100, char0);
		string path(c);
		_Strm2Val(*strm, num);
		for (ushort a = 0; a < num; a++) {
			_Strm2Val(*strm, type);
			strm->getline(c, 100, (char)0);
			string nm = path + string(c);
			strm->getline(c, 100, (char)0);
			std::cout << (int)type << ": " << nm << std::endl;
			dataCaches[type].push_back(nullptr);
			names[type].push_back(c);
			dataELocs[type].push_back(nm);
		}
	}
	else {
		string pp = dpath + "0";
		Scene::strm = new std::ifstream(pp.c_str(), std::ios::in | std::ios::binary);
		std::ifstream* strm = Scene::strm;
		if (!strm->is_open()) {
			Debug::Error("AssetManager", "Fatal: cannot open data file 0!");
			abort();
		}

		byte numDat = 0, id;
		ushort num;
		ASSETTYPE type;
		uint pos;
		char c[100];
		*strm >> c[0] >> c[1];
		if (c[0] != 'D' || c[1] != '0') {
			Debug::Error("AssetManager", "Fatal: file 0 has wrong signature!");
			abort();
		}
		_Strm2Val(*strm, num);
		for (ushort a = 0; a < num; a++) {
			_Strm2Val(*strm, type);
			_Strm2Val(*strm, id);
			_Strm2Val(*strm, pos);
			strm->getline(c, 100, (char)0);
			std::cout << (int)type << " " << (int)id << " " << pos << ": " << string(c) << std::endl;
			numDat = max(numDat, id);
			dataLocs[type].push_back(std::pair<byte, uint>(id - 1, pos));
			dataCaches[type].push_back(nullptr);
			names[type].push_back(c);
		}

		for (int a = 0; a < numDat; a++) {
			string pp = dpath + to_string(a + 1);
			streams.push_back(new std::ifstream(pp.c_str(), std::ios::in | std::ios::binary));
			if (streams[a]->is_open())
				Debug::Message("AssetManager", "Streaming data" + to_string(a + 1));
			else {
				Debug::Error("AssetManager", "Fatal: Failed to open data" + to_string(a + 1) + "!");
				abort();
			}
		}

		for (auto& aa : dataLocs) {
			ushort s = 0;
			for (auto& bb : aa.second) {
				streams[bb.first]->seekg(bb.second);
				uint sz;
				_Strm2Val(*streams[bb.first], sz);
				Debug::Message("AssetManager", "Registered asset " + names[aa.first][s] + " (" + to_string(sz) + " bytes)");
				streams[bb.first]->seekg(0);
				s++;
			}
		}
		//long long ppos = strm->tellg();
	}
	Scene::ReadD0();
#endif
}

AssetObject* AssetManager::CacheFromName(string nm) {
	//ASSETTYPE t;
	for (auto& t : names) {
		for (uint a = names[t.first].size(); a > 0; a--) {
			if (names[t.first][a-1] == nm) {
				return GetCache(t.first, a-1);
			}
		}
	}
	Debug::Warning("Asset Finder", "Asset not found for " + nm + "!");
	return nullptr;
}

ASSETID AssetManager::GetAssetId(string nm, ASSETTYPE &t) {
	for (auto& q : names) {
		for (uint a = q.second.size(); a > 0; a--) {
			if (q.second[a-1] == nm) {
				t = q.first;
				return a-1;
			}
		}
	}
	t = ASSETTYPE_UNDEF;
	return -1;
}

AssetObject* AssetManager::GetCache(ASSETTYPE t, ASSETID i) {
	if (i == -1)
		return nullptr;
	if (dataCaches[t][i] != nullptr)
		return dataCaches[t][i];
	else
		return GenCache(t, i);
}
AssetObject* AssetManager::GenCache(ASSETTYPE t, ASSETID i) {
#ifndef IS_EDITOR
	std::ifstream* strm = _pipemode? new std::ifstream(dataELocs[t][i], std::ios::in | std::ios::binary) : streams[dataLocs[t][i].first];
	uint off = _pipemode ? 0 : dataLocs[t][i].second + 4;
	//strm->seekg(off);
	//uint sz;
	//_Strm2Val(*strm, sz);
	//off += 4;
	switch (t) {
	case ASSETTYPE_TEXTURE:
		dataCaches[t][i] = new Texture(*strm, off);
		break;
	case ASSETTYPE_HDRI:
		dataCaches[t][i] = new Background(*strm, off);
		break;
	case ASSETTYPE_MESH:
		dataCaches[t][i] = new Mesh(*strm, off);
		break;
	case ASSETTYPE_MATERIAL:
		dataCaches[t][i] = new Material(*strm, off);
		break;
	case ASSETTYPE_SHADER:
		dataCaches[t][i] = new ShaderBase(*strm, off);
		break;
	default:
		Debug::Error("AssetManager", "No operation suits asset type " + to_string(t) + "!");
		return nullptr;
	}
	if (_pipemode) delete(strm);
	return dataCaches[t][i];
#else
	return nullptr;
#endif
}

std::vector<string> DefaultResources::names = std::vector<string>();
std::vector<char*> DefaultResources::datas = std::vector<char*>();
std::vector<uint> DefaultResources::sizes = std::vector<uint>();
void DefaultResources::Init(string path) {
	std::ifstream strm(path, std::ios::binary);
	if (!strm.is_open()) {
		Debug::Error("Default Resources", "fatal: cannot open default resources at " + path + "!");
		abort();
	}
	char c[100];
	while (!strm.eof()) {
		strm.getline(c, 100, 0);
		if (c[0] == 0) break;
		names.push_back(c);
		uint sz;
		_Strm2Val<uint>(strm, sz);
		sizes.push_back(sz);
		char* cc = new char[sz+1];
		strm.read(cc, sz);
		cc[sz] = 0;
		datas.push_back(cc);
	}
	std::cout << "Default Resources OK (" << to_string(names.size()) << " files loaded)" << std::endl;
}

string DefaultResources::GetStr(string name) {
	for (uint a = names.size(); a > 0; a--) {
		if (names[a-1] == name) {
			return string(datas[a-1]);
		}
	}
	Debug::Error("Default Resources", "Fatal: resource \"" + name + "\" missing!");
	return "";
}