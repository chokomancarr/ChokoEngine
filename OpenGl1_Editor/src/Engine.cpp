#include "Engine.h"
#include "Editor.h"
#include "hdr.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <climits>
#include <shellapi.h>
#include <Windows.h>
#include <math.h>
#include <jpeglib.h>
#include <jerror.h>
//#include <png.h>
//#include <zlib.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Defines.h"
#include "SceneScriptResolver.h"

using namespace std;

bool Rect::Inside(Vec2 v) {
	return ((w > 0) ? (v.x > x && v.x < (x + w)) : (v.x >(x + w) && v.x < x)) && ((h > 0) ? (v.y > y && v.y < (y + h)) : (v.y >(y + h) && v.y < y));
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
		return GetTickCount();
	}
}

Texture* Engine::fallbackTex = nullptr;

uint Engine::unlitProgram = 0;
uint Engine::unlitProgramA = 0;
uint Engine::unlitProgramC = 0;
uint Engine::skyProgram = 0;
Font* Engine::defaultFont;//&Font("F:\\ascii 2.font");
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
			cout << "cannot load fallback texture!" << endl;
	}

	string vertcode = "#version 330 core\nlayout(location = 0) in vec3 pos;\nlayout(location = 1) in vec2 uv;\nout vec2 UV;\nvoid main(){ \ngl_Position.xyz = pos;\ngl_Position.w = 1.0;\nUV = uv;\n}";
	string fragcode = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = texture(sampler, UV)*col;\n}"; //out vec3 Vec4;\n
	string fragcode2 = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = vec4(1, 1, 1, texture(sampler, UV).r)*col;\n}"; //out vec3 Vec4;\n
	string fragcode3 = "#version 330 core\nin vec2 UV;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = col;\n}"; //out vec3 Vec4;\n
	//string fragcodeSky = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec2 dir;\nuniform float angle;\nout vec4 Vec4;\nvoid main(){\nvec4 col = texture(sampler, UV);\nVec4.rgb = col.rgb*pow(2, col.a*255-128);\nVec4.a = 1;\n}"; //(1.0f / 256.0f) * pow(2, (float)(exponent - 128));
	//string fragcodeSky = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec2 dir;\nuniform float length;\nout vec4 Vec4;\nvoid main(){\nfloat ay = asin((dir.y + UV.y)/length);\nfloat l2 = length*cos(ay);\nfloat ax = asin((dir.x + UV.x)/l2);\nVec4 = texture(sampler, vec2(0.5, 0.5) + vec2(ax, ay));\nVec4.a = 1;\n}";
	string fragcodeSky = "in vec2 UV;uniform sampler2D sampler;uniform vec2 dir;uniform float length;out vec4 color;void main(){float ay = asin((UV.y) / length);float l2 = length*cos(ay);float ax = asin((dir.x + UV.x) / l2);color = texture(sampler, vec2((dir.x + ax / 3.14159)*sin(dir.y + ay / 3.14159) + 0.5, (dir.y + ay / 3.14159)));color.a = 1;}";

	unlitProgram = glCreateProgram();
	GLuint vertex_shader, fragment_shader;
	int link_result = 0;
	int info_log_length = 0;
	if (ShaderBase::LoadShader(GL_VERTEX_SHADER, vertcode, vertex_shader)
		&& ShaderBase::LoadShader(GL_FRAGMENT_SHADER, fragcode, fragment_shader)) {
		glAttachShader(unlitProgram, vertex_shader);
		glAttachShader(unlitProgram, fragment_shader);


		glLinkProgram(unlitProgram);
		glGetProgramiv(unlitProgram, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			glGetProgramiv(unlitProgram, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(unlitProgram, info_log_length, NULL, &program_log[0]);
			cerr << "Default Shader link error" << endl << &program_log[0] << endl;
			return;
		}
		glDetachShader(unlitProgram, vertex_shader);
		glDetachShader(unlitProgram, fragment_shader);
	}
	GLuint fragment_shaderA;
	if (ShaderBase::LoadShader(GL_FRAGMENT_SHADER, fragcode2, fragment_shaderA)) {
		unlitProgramA = glCreateProgram();
		glAttachShader(unlitProgramA, vertex_shader);
		glAttachShader(unlitProgramA, fragment_shaderA);

		link_result = 0;

		glLinkProgram(unlitProgramA);
		glGetProgramiv(unlitProgramA, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			int info_log_length = 0;
			glGetProgramiv(unlitProgramA, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(unlitProgramA, info_log_length, NULL, &program_log[0]);
			cerr << "Default Shader (Alpha) link error" << endl << &program_log[0] << endl;
			return;
		}
		glDetachShader(unlitProgramA, vertex_shader);
		glDetachShader(unlitProgramA, fragment_shaderA);
	}
	GLuint fragment_shaderC;
	if (ShaderBase::LoadShader(GL_FRAGMENT_SHADER, fragcode3, fragment_shaderC)) {
		unlitProgramC = glCreateProgram();
		glAttachShader(unlitProgramC, vertex_shader);
		glAttachShader(unlitProgramC, fragment_shaderC);

		link_result = 0;

		glLinkProgram(unlitProgramC);
		glGetProgramiv(unlitProgramC, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			int info_log_length = 0;
			glGetProgramiv(unlitProgramC, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(unlitProgramC, info_log_length, NULL, &program_log[0]);
			cerr << "Default Shader (Vec4) link error" << endl << &program_log[0] << endl;
			return;
		}
		glDetachShader(unlitProgramC, vertex_shader);
		glDetachShader(unlitProgramC, fragment_shaderC);
		//defaultFont = &Font("F:\\ascii 2.font");
	}
	GLuint fragment_shaderS;
	if (ShaderBase::LoadShader(GL_FRAGMENT_SHADER, fragcodeSky, fragment_shaderS)) {
		skyProgram = glCreateProgram();
		glAttachShader(skyProgram, vertex_shader);
		glAttachShader(skyProgram, fragment_shaderS);

		link_result = 0;

		glLinkProgram(skyProgram);
		glGetProgramiv(skyProgram, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			int info_log_length = 0;
			glGetProgramiv(skyProgram, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(skyProgram, info_log_length, NULL, &program_log[0]);
			cerr << "Sky shader link error" << endl << &program_log[0] << endl;
			return;
		}
		glDetachShader(unlitProgramC, vertex_shader);
		glDeleteShader(vertex_shader);
		glDetachShader(unlitProgramC, fragment_shaderC);
		glDeleteShader(fragment_shader);
		//defaultFont = &Font("F:\\ascii 2.font");
	}
}

vector<ifstream*> Engine::assetStreams = vector<ifstream*>();
unordered_map<byte, vector<string>> Engine::assetData = unordered_map<byte, vector<string>>();
unordered_map<string, byte[]> Engine::assetDataLoaded = unordered_map<string, byte[]>();

bool Engine::LoadDatas(string path) {
	ifstream* d0 = new ifstream(path + "\\data0");
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

float Dw(float f) {
	return (f / Display::width);
}
float Dh(float f) {
	return (f / Display::height);
}
Vec3 Ds(Vec3 v) {
	return Vec3(Dw(v.x) * 2 - 1, 1 - Dh(v.y) * 2, 1);
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
}

void Engine::EndStencil() {
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
}

void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, DrawTex_Scaling scl) {
	if (scl == DrawTex_Stretch)
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer);
	else if (scl == DrawTex_Fit) {
		float w2h = ((float)texture->width) / texture->height;
		if (w/h > w2h)
			DrawQuad(x + 0.5f*(w - h*w2h), y, h*w2h, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer);
		else
			DrawQuad(x, y + 0.5f*(h - w/w2h), w, w/w2h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer);
	}
	else {
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer);
	}
}
void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, float alpha, DrawTex_Scaling scl) {
	DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, Vec4(1, 1, 1, alpha));
}
void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, Vec4 tint, DrawTex_Scaling scl) {
	DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, tint);
}

void AddQuad(float x, float y, float w, float h, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, vector<Vec3>* poss, vector<uint>* indexes, vector<Vec2>* uvs, uint i) {
	poss->push_back(Vec3(x, y, 0));
	poss->push_back(Vec3(x + w, y, 0));
	poss->push_back(Vec3(x, y + h, 0));
	poss->push_back(Vec3(x + w, y + h, 0));
	indexes->push_back(i);
	indexes->push_back(i + 1);
	indexes->push_back(i + 3);
	indexes->push_back(i + 2);
	uvs->push_back(uv0);
	uvs->push_back(uv1);
	uvs->push_back(uv2);
	uvs->push_back(uv3);
}

void Engine::Label(float x, float y, float s, string st, Font* font) {
	Label(x, y, s, st, font, Vec4(0, 0, 0, 1));
}
void Engine::Label(float x, float y, float s, string st, Font* font, Vec4 Vec4) {
	Label(x, y, s, st, font, Vec4, -1);
}
void Engine::Label(float x, float y, float s, string st, Font* font, Vec4 Vec4, float maxw) {
	if (st == "")
		return;
	const char* str = st.c_str();
	
	vector<Vec3> quadPoss;
	vector<uint> indexes;
	vector<Vec2> uvs;

	if (font->loaded) {
		for (uint a = 0; a < strlen(str); a++) {
			int o = str[a];
			float h = (o*1.0f / font->gchars(s));
			float w = (1 - (font->gpadding(s)*1.0f / font->gwidth(s)));

			if (maxw > 0) {
				if ((a + 3)*s*font->gw2h(s)*w > maxw) {
					o = '.';
					h = (o*1.0f / font->gchars(s));
					w = (1 - (font->gpadding(s)*1.0f / font->gwidth(s)));
					DrawQuad(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f), y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f), s*font->gw2h(s)*w, s, font->getpointer(s), Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), true, Vec4);
					a++;
					DrawQuad(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f), y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f), s*font->gw2h(s)*w, s, font->getpointer(s), Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), true, Vec4);
					break;
				}
			}
			int x0 = (int)(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f));
			if (x0 > Display::width)
				break;

			float r = Display::height * 1.0f / Display::width; //(x2 + a*s2*font->w2h*w*r) * 2 - 1, (1 - y2) * 2 - 1, s2*font->w2h * 2 * w*r, -s2 * 2
			//DrawQuad(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f), y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f), s*font->gw2h(s)*w, s, font->getpointer(s), Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), true, Vec4);//Vec2(0, 0.49), Vec2(1, 0.49));//(1.0f / font->chars)), Vec2(1, h - (1.0f / font->chars)));
			AddQuad((float)x0, round(y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f)), s*font->gw2h(s)*w, s, Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), &quadPoss, &indexes, &uvs, a * 4);
		}
		if (quadPoss.size() == 0) //happens if click showdesktop
			return;
		for (int y = quadPoss.size()-1; y >= 0; y--) {
			quadPoss[y] = Ds(Display::uiMatrix*quadPoss[y]);
		}
		uint prog = unlitProgramA;
		GLint baseImageLoc = glGetUniformLocation(prog, "sampler");
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
		glUseProgram(prog);
		glUniform1i(baseImageLoc, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, font->getpointer(s));
		GLint baseColLoc = glGetUniformLocation(prog, "col");
		glUniform4f(baseColLoc, Vec4.r, Vec4.g, Vec4.b, Vec4.a);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, &uvs[0]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_QUADS, indexes.size(), GL_UNSIGNED_INT, &indexes[0]);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glUseProgram(0);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

byte Engine::Button(float x, float y, float w, float h) {
	return Rect(x, y, w, h).Inside(Input::mousePos) ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4) {
	return Button(x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f));
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4) {
	return Button(x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f), label, labelSize, labelFont, labelVec4);
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4) {
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
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
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
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
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4) {
	byte b = Button(x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f));
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDLEFT;
	Label(round(x + 2), round(y + 0.5f*h), labelSize, label, labelFont, labelVec4);
	labelFont->alignment = al;
	return b;
}

byte Engine::EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4) {
	return EButton(a, x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f));
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4) {
	if (a) {
		bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
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
	if (a) {
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
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
	labelFont->alignment = ALIGN_MIDCENTER;
	Label(x + 0.5f*w, y + 0.5f*h, labelSize, label, labelFont, labelVec4);
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
	else if (o == 2)
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
			vv = clamp((Input::mousePos.x - x) / w, 0, 1);
			v = vv*(max - min) + min;
			DrawQuad(x + 1, y + 1, (w - 2)*vv, h - 2, foreground);
			return v;
		}
	}
	DrawQuad(x + 1, y + 1, (w - 2)*vv, h - 2, foreground*(Rect(x, y, w, h).Inside(Input::mousePos)? 1 : 0.8f));
	return v;
}

void Engine::DrawProgressBar(float x, float y, float w, float h, float progress, Vec4 background, Texture* foreground, Vec4 tint, int padding, byte clip) {
	DrawQuad(x, y, w, h, background);
	progress = clamp(progress, 0, 100)*0.01f;
	float tx = (clip == 0) ? 1 : ((clip == 1) ? progress : w*progress / h);
	DrawQuad(x + padding, y + padding, w*progress - 2 * padding, h - 2 * padding, foreground->pointer, Vec2(0, 1), Vec2(tx, 1), Vec2(0, 0), Vec2(tx, 0), false, tint);
}

void Engine::DrawSky(float x, float y, float w, float h, Background* sky, float forX, float forY, float angleW, float rot) { //forward: x (-90~90), y (0~360), can repeat
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
	float w2h = h / w;
	void* uvP;
	if (w2h < 1) {
		Vec2 quadUv[4]{Vec2(-0.5f, w2h / 2), Vec2(0.5f, w2h / 2), Vec2(-0.5f, -w2h / 2), Vec2(0.5f, -w2h / 2)};
		uvP = &quadUv[0];
	}
	else {
		Vec2 quadUv[4]{Vec2(-0.5f / w2h, 0.5f), Vec2(0.5f / w2h, 0.5f), Vec2(-0.5f / w2h, -0.5f), Vec2(0.5f / w2h, -0.5f)};
		uvP = &quadUv[0];
	}
	uint quadIndexes[4] = { 0, 1, 3, 2 };
	uint prog = skyProgram;

	GLint baseImageLoc = glGetUniformLocation(prog, "sampler");
	glUniform1i(baseImageLoc, 0);
	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glUseProgram(prog);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sky->pointer);

	GLint dirLoc = glGetUniformLocation(prog, "dir");
	glUniform2f(dirLoc, forX, -forY);
	GLint lLoc = glGetUniformLocation(prog, "length");
	glUniform1f(lLoc, 0.5f/sin(angleW*3.14159f/360));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 0, uvP);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glUseProgram(0);

	glDisableClientState(GL_VERTEX_ARRAY);

}

void Engine::RotateUI(float aa, Vec2 point) {
	float a = -3.1415926535f*aa / 180.0f;
	//Display::uiMatrix = glm::mat3(1, 0, 0, 0, 1, 0, point2.x * 2 - 1, point2.y * 2 - 1, 1)*glm::mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)*glm::mat3(1, 0, 0, 0, 1, 0, -point2.x * 2 + 1, -point2.y * 2 + 1, 1)*Display::uiMatrix;
	Display::uiMatrix = glm::mat3(1, 0, 0, 0, 1, 0, point.x, point.y, 1)*glm::mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)*glm::mat3(1, 0, 0, 0, 1, 0, -point.x, -point.y, 1)*Display::uiMatrix;
}
void Engine::ResetUIMatrix() {
	Display::uiMatrix = glm::mat3();
}

void Engine::DrawQuad(float x, float y, float w, float h, uint texture) {
	DrawQuad(x, y, w, h, texture, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, Vec4(1, 1, 1, 1));
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

	GLint baseColLoc = glGetUniformLocation(prog, "col");
	glUniform4f(baseColLoc, Vec4.r, Vec4.g, Vec4.b, Vec4.a);

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

void Engine::DrawQuad(float x, float y, float w, float h, GLuint texture, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, bool single, Vec4 Vec4) {
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
	//glLinkProgram(prog);
	GLint baseImageLoc = glGetUniformLocation(prog, "sampler");
	glEnableClientState(GL_VERTEX_ARRAY);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	//glEnable(GL_DEPTH_TEST);

	
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	//glTexCoordPointer(2, GL_FLOAT, 0, &quadUv);
	glUseProgram(prog);

	glUniform1i(baseImageLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	GLint baseColLoc = glGetUniformLocation(prog, "col");
	glUniform4f(baseColLoc, Vec4.r, Vec4.g, Vec4.b, Vec4.a);

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
	uint quadIndexes[4] = { 0, 1 };
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
	uint quadIndexes[4] = { 0, 1 };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);
	glColor4f(col.r, col.g, col.b, col.a);
	glLineWidth(width);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, &quadIndexes[0]);
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
	for (byte a = 1; a < 106; a++) {
		keyStatusNew[a] = ((GetAsyncKeyState(a) >> 8) == -128);
	}

	inputString = "";
	bool shift = KeyDown(Key_Shift);
	for (byte c = Key_0; c <= Key_9; c++) {
		if (KeyDown((InputKey)c)) {
			inputString += char(c);
		}
	}
	for (byte b = Key_A; b <= Key_Z; b++) {
		if (KeyDown((InputKey)b)) {
			inputString += shift ? char(b + 32) : char(b);
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
}

ulong Engine::idCounter = 0;
ulong Engine::GetNewId() {
	if (++idCounter >= ULONG_MAX) {
		idCounter = 0;
		Debug::Warning("Engine", "max id count reached! (" + to_string(idCounter) + ")");
	}
	return idCounter;
}

//vector<Camera*> Engine::sceneCameras();

//-----------------debug class-----------------------
void Debug::Message(string c, string s) {
#ifndef IS_EDITOR
	*stream << "[i]" << c << ": " << s << endl;
#endif
	cout << "[i]" << c << ": " << s << endl;
}
void Debug::Warning(string c, string s) {
#ifndef IS_EDITOR
	*stream << "[w]" << c << ": " << s << endl;
#endif
	cout << "[w]" << c << ": " << s << endl;
}
void Debug::Error(string c, string s) {
#ifndef IS_EDITOR
	*stream << "[e]" << c << ": " << s << endl;
#endif
	cout << "[e]" << c << " says: " << s << endl;
	abort();
}
ofstream* Debug::stream = nullptr;
void Debug::Init(string s) {
#ifndef IS_EDITOR
	string ss = s + "Log.txt";
	stream = new ofstream(ss.c_str(), ios::out | ios::trunc);
	Message("Debug", "Log init'd");
#endif
}
//-----------------io class-----------------------
vector<string> IO::GetFiles(const string& folder, string ext)
{
	if (folder == "") return vector<string>();
	vector<string> names;
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
vector<EB_Browser_File> IO::GetFilesE (Editor* e, const string& folder)
{
	vector<EB_Browser_File> names;
	string search_path = folder + "/*.*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all importable files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				string aa(fd.cFileName);
				if ((aa.length() > 5 && aa.substr(aa.length() - 5, string::npos) == ".meta"))
					names.push_back(EB_Browser_File(e, folder, aa.substr(0, aa.length() - 5), aa));
				else if ((aa.length() > 2 && aa.substr(aa.length() - 2, string::npos) == ".h"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 4 && aa.substr(aa.length() - 4, string::npos) == ".cpp"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 9 && aa.substr(aa.length() - 9, string::npos) == ".material"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}

void IO::GetFolders(const string& folder, vector<string>* names, bool hidden)
{
	//vector<string> names;
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

bool IO::HasDirectory (LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool IO::HasFile(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES);// && (dwAttrib & FILE_ATTRIBUTE_NORMAL));
}

string IO::ReadFile(const string& path) {
	ifstream stream(path.c_str());
	if (!stream.good()) {
		cout << "not found! " << path << endl;
		return "";
	}
	stringstream buffer;
	buffer << stream.rdbuf();
	return buffer.str();
}

vector<string> IO::GetRegistryKeys(HKEY key) {
	TCHAR    achKey[255];
	TCHAR    achClass[MAX_PATH] = TEXT("");
	DWORD    cchClassName = MAX_PATH;
	DWORD	 size;
	vector<string> res;

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

//-----------------time class---------------------
long long Time::startMillis = 0;
long long Time::millis = 0;
double Time::time = 0;
float Time::delta = 0;

//-----------------Vec4s--------------------------
Vec4 red() { return red(1, 1); }
Vec4 green() { return green(1, 1); }
Vec4 blue() { return blue(1, 1); }
Vec4 cyan() { return cyan(1, 1); }
Vec4 yellow() { return yellow(1, 1); }
Vec4 black() { return black(1); }
Vec4 white() { return white(1, 1); }

Vec4 red(float f) { return red(f, 1); }
Vec4 green(float f) { return green(f, 1); }
Vec4 blue(float f) { return blue(f, 1); }
Vec4 cyan(float f) { return cyan(f, 1); }
Vec4 yellow(float f) { return yellow(f, 1); }
Vec4 black(float f) { return Vec4(0, 0, 0, f); }
Vec4 white(float f) { return white(f, 1); }

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

Vec3 rotate(Vec3 v, Quat q) {
	// Extract the vector part of the quaternion
	Vec3 u(q.x, q.y, q.z);
	// Extract the scalar part of the quaternion
	float s = q.w;
	// Do the math
	return (2.0f * dot(u, v) * u + (s*s - dot(u, u)) * v + 2.0f * s * cross(u, v));
}

glm::mat4 Quat2Mat(Quat q) {
	//return glm::mat4(q.w, q.x, q.y, q.z, -q.x, q.w, q.z, -q.y, -q.y, -q.z, q.w, q.x, -q.z, q.y, -q.x, q.w);
	return glm::mat4_cast(q);
}

//-----------------transform class-------------------
Transform::Transform(SceneObject* sc, Vec3 pos, Quat rot, Vec3 scl) : object(sc), rotation(rot), scale(scl) {
	Translate(pos);
}

Vec3 Transform::worldPosition() {
	Vec3 w = position;
	SceneObject* o = object;
	while (o->parent != nullptr) {
		o = o->parent;
		w = w*o->transform.scale + o->transform.position;
	}
	return w;
}

void Transform::eulerRotation(Vec3 r) {
	_eulerRotation = r;
	_eulerRotation.x = clamp(_eulerRotation.x, 0, 360);
	_eulerRotation.y = clamp(_eulerRotation.y, 0, 360);
	_eulerRotation.z = clamp(_eulerRotation.z, 0, 360);
	_UpdateQuat();
}

void Transform::_UpdateQuat() {
	rotation = Quat(deg2rad*_eulerRotation);
}


Transform* Transform::Translate(Vec3 v) {
	position += v;
	return this;
}
Transform* Transform::Rotate(Vec3 v) {
	eulerRotation(_eulerRotation + v);
	return this;
}
Transform* Transform::Scale(Vec3 v) {
	scale += v;
	return this;
}

//-----------------mesh class------------------------
Mesh::Mesh(Editor* e, int i) : AssetObject(ASSETTYPE_MESH) {
	Mesh* m2 = _GetCache<Mesh>(type, i);
	vertices = m2->vertices;
	vertexCount = m2->vertexCount;
	normals = m2->normals;
	triangles = m2->triangles;
	triangleCount = m2->triangleCount;
	loaded = m2->loaded;
}

Mesh::Mesh(string path) : AssetObject(ASSETTYPE_MESH), loaded(false) {
	string p = Editor::instance->projectFolder + "Assets\\" + path + ".mesh.meta";
	ifstream stream(p.c_str());
	if (!stream.good()) {
		cout << "mesh file not found!" << endl;
		return;
	}
	string a;
	string junk;
	uint x;
	stream >> a;
	if (a != "KTO123") {
		Debug::Error("Mesh Importer", "mesh metadata corrupted (wrong header)!");
	}
	stream >> a;
	if (a == "obj") {
		stream >> name >> junk;
	}
	else {
		Debug::Error("Mesh Importer", "mesh metadata corrupted (obj tag not found)!");
		return;
	}
	stream >> a;
	while (!stream.eof()) {
		if (a == "vrt") {
			Vec3 v;
			stream >> x;
			if (x != vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (vertex id not in order)!");
				return;
			}
			vertexCount++;
			stream >> v.x >> v.y >> v.z;
			vertices.push_back(v);
		}
		else if (a == "nrm") {
			stream >> x;
			if (x >= vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (normal id exceed vertex id)!");
				return;
			}
			Vec3 n;
			stream >> n.x >> n.y >> n.z;
			normals.push_back(n);
			//cout << "vn" << x << " " << s(mesh.vertices[x].normal) << endl;
		}
		/*
		else if (a == "vcl") {
			stream >> x;
			if (x >= vertCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (vertex color id exceed vertex id)!");
				return false;
			}
			stream >> mesh.vertices[x].Vec4.x >> mesh.vertices[x].Vec4.y >> mesh.vertices[x].Vec4.z;
			//cout << "vc" << x << " " << s(mesh.vertices[x].normal) << endl;
		}
		*/
		else if (a == "tri") {
			uint i;
			stream >> i;
			int mati = i+0;
			while (_matTriangles.size() <= i) {
				_matTriangles.push_back({});
				materialCount++;
			}
			stream >> i;
			triangles.push_back(i+0);
			_matTriangles[mati].push_back(i + 0);
			stream >> i;
			triangles.push_back(i + 0);
			_matTriangles[mati].push_back(i + 0);
			stream >> i;
			triangles.push_back(i + 0);
			_matTriangles[mati].push_back(i + 0);
			triangleCount++;
			//cout << "tri" << faceCount - 1 << endl;
		}
		else if (a == "uv0") {
			Vec2 v;
			stream >> x;
			if (x >= vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (uv id exceed vertex id)!");
				return;
			}
			vertexCount++;
			stream >> v.x >> v.y;
			uv0.push_back(v);
		}
		else if (a == "uv1") {
			Vec2 v;
			stream >> x;
			if (x >= vertexCount) {
				Debug::Error("Mesh Importer", "mesh metadata corrupted (uv id exceed vertex id)!");
				return;
			}
			vertexCount++;
			stream >> v.x >> v.y;
			uv1.push_back(v);
		}
		else if (a == "]")
			break;
		else {
			cout << "what is this? [" << a << "]" << endl;
		}
		stream >> a;
	}
	int vs = vertices.size();
	if (vs > 0) {
		if (normals.size() != vs)
			Debug::Error("Mesh Importer", "mesh metadata corrupted (normals incomplete)!");
		else if (uv0.size() == 0)
			uv0.resize(vs);
		else if (uv0.size() != vs)
			Debug::Error("Mesh Importer", "mesh metadata corrupted (uv0 incomplete)!");
		if (uv1.size() == 0)
			uv1.resize(vs);
	}
	loaded = true;
	return;
}

bool Mesh::ParseBlend(Editor* e, string s) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	HANDLE stdOutR, stdOutW, stdInR, stdInW;
	if (!CreatePipe(&stdInR, &stdInW, &sa, 0)) {
		cout << "failed to create pipe for stdin!";
		return false;
	}
	if (!SetHandleInformation(stdInW, HANDLE_FLAG_INHERIT, 0)){
		cout << "failed to set handle for stdin!";
		return false;
	}
	if (!CreatePipe(&stdOutR, &stdOutW, &sa, 0)) {
		cout << "failed to create pipe for stdout!";
		return false;
	}
	if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0)){
		cout << "failed to set handle for stdout!";
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
	if (CreateProcess("C:\\Windows\\System32\\cmd.exe", 0, NULL, NULL, true, CREATE_NO_WINDOW, NULL, "F:\\TestProject\\", &startInfo, &processInfo) != 0) {
		cout << "executing Blender..." << endl;
		bool bSuccess = false;
		DWORD dwWrite;
		bSuccess = WriteFile(stdInW, cmd1.c_str(), cmd1.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			cout << "can't get to root!" << endl;
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd2.c_str(), cmd2.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			cout << "can't navigate to blender dir!" << endl;
			return false;
		}
		bSuccess = WriteFile(stdInW, cmd3.c_str(), cmd3.size(), &dwWrite, NULL) != 0;
		if (!bSuccess || dwWrite == 0) {
			cout << "can't execute blender!" << endl;
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
				cout << sss << endl;
				r = rr + 1;
				if (sss.size() > 1 && sss[0] == '!')
					e->_Message("Blender", sss.substr(1, string::npos));
				if (sss.size() > 12 && sss.substr(0, 12) == "Blender quit"){
					TerminateProcess(processInfo.hProcess, 0);
					failed = false;
					finish = true;
				}
			}
		} while (w == WAIT_TIMEOUT && !finish);
		if (failed)
			return false;
	}
	else {
		cout << "Cannot start Blender!" << endl;
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

/*
void Mesh::Draw(Material* mat) {
	if (mat == nullptr)
		glUseProgram(0);
	else {
		mat->ApplyGL();
	}
}
*/

//-----------------texture class---------------------
bool LoadJPEG(string fileN, uint &x, uint &y, byte& channels, byte** data)
{
	//unsigned int texture_id;
	unsigned long data_size;     // length of the 
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

/*
void DoReadPNG(png_structp pngPtr, png_bytep data, png_size_t length) {
	//Here we get our IO pointer back from the read struct.
	//This is the parameter we passed to the png_set_read_fn() function.
	//Our std::istream pointer.
	png_voidp a = png_get_io_ptr(pngPtr);
	//Cast the pointer to std::istream* and read 'length' bytes into 'data'
	((ifstream*)a)->read((char*)data, length);
}

bool LoadPNG(string fileN, uint &x, uint &y, byte& channels, byte** data) {
	ifstream strm(fileN, ios::in | ios::binary);
	png_byte pngsig[8]; //signature
	int is_png = 0;
	strm.read((char*)pngsig, 8);
	if (!strm.good()) return false;
	is_png = png_sig_cmp(pngsig, 0, 8);
	if (is_png != 0) {
		Debug::Warning("PNG reader", "Png file has wrong header!");
		return false;
	}

	//Here we create the png read struct. The 3 NULL's at the end can be used
	//for your own custom error handling functions, but we'll just use the default.
	//if the function fails, NULL is returned. Always check the return values!
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr) {
		Debug::Warning("PNG reader", "Couldn't initialize png read struct!");
		return false;
	}
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) {
		Debug::Warning("PNG reader", "Couldn't initialize png info struct!");
		png_destroy_read_struct(&pngPtr, (png_infopp)0, (png_infopp)0);
		return false;
	}

	png_set_read_fn(pngPtr, (png_voidp)&strm, DoReadPNG);

	//Set the amount signature bytes we've already read:
	png_set_sig_bytes(pngPtr, 8);
	//Now call png_read_info with our pngPtr as image handle, and infoPtr to receive the file info.
	png_read_info(pngPtr, infoPtr);
	x = (uint)png_get_image_width(pngPtr, infoPtr);
	y = (uint)png_get_image_height(pngPtr, infoPtr);
	//bits per channel
	png_uint_32 bitdepth = png_get_bit_depth(pngPtr, infoPtr);
	//Number of channels
	channels = png_get_channels(pngPtr, infoPtr);
	//Color type. (RGB, RGBA, Luminance, luminance alpha... palette... etc)
	png_uint_32 color_type = png_get_color_type(pngPtr, infoPtr);

	switch (color_type) {
	case PNG_COLOR_TYPE_PALETTE:
		png_set_palette_to_rgb(pngPtr);
		channels = 3;
		break;
	case PNG_COLOR_TYPE_GRAY:
		if (bitdepth > 8)
			png_set_expand_gray_1_2_4_to_8(pngPtr);
		//And the bitdepth info
		bitdepth = 8;
		break;
	}
	//if the image has a transperancy set.. convert it to a full Alpha channel..
	if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(pngPtr);
		channels += 1;
	}

	png_bytep* rowPtrs = new png_bytep[y];

	*data = new byte[x * y * bitdepth * channels / 8];
	//This is the length in bytes, of one row.
	const unsigned int stride = x * bitdepth * channels / 8;

	//A little for-loop here to set all the row pointers to the starting
	//Addresses for every row in the buffer
	for (size_t i = 0; i < y; i++) {
		//Set the pointer to the data pointer + i times the row stride.
		//Notice that the row order is reversed with q.
		//This is how at least OpenGL expects it,
		//and how many other image loaders present the data.
		png_uint_32 q = (y - i - 1) * stride;
		rowPtrs[i] = (png_bytep)data + q;
	}
	//And here it is! The actual reading of the image!
	//Read the imagedata and write it to the addresses pointed to
	//by rowptrs (in other words: our image databuffer)
	png_read_image(pngPtr, rowPtrs);

	delete[](png_bytep)rowPtrs;
	png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
}
*/

bool LoadBMP(string fileN, uint &x, uint &y, byte& channels, byte** data) {

	char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int imageSize;   // = width*height*3
	unsigned short bpi;

	FILE *file;

	fopen_s(&file, fileN.c_str(), "rb");

	if (!file){
		printf("Image could not be opened\n");
		return false;
	}
	if (fread(header, 1, 54, file) != 54){ // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return false;
	}
	if (header[0] != 'B' || header[1] != 'M'){
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
	fread(*data, 1, imageSize, file);
	//Everything is in memory now, the file can be closed
	fclose(file);
	return true;
}

Texture::Texture(const string& path) : Texture(path, true, TEX_FILTER_TRILINEAR, 5) {}
Texture::Texture(const string& path, bool mipmap) : Texture(path, mipmap, mipmap? TEX_FILTER_TRILINEAR : TEX_FILTER_BILINEAR, 5) {}
Texture::Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso) : AssetObject(ASSETTYPE_TEXTURE), _mipmap(mipmap), _filter(filter), _aniso(aniso) {
	string sss = path.substr(path.find_last_of('.'), string::npos);
	byte *data;
	byte chn;
	//cout << "opening image at " << path << endl;
	GLenum rgb = GL_RGB, rgba = GL_RGBA;
	if (sss == ".bmp") {
		if (!LoadBMP(path, width, height, chn, &data)) {
			cout << "load bmp failed! " << path << endl;
			return;
		}
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	else if (sss == ".jpg") {
		if (!LoadJPEG(path, width, height, chn, &data)) {
			cout << "load jpg failed! " << path << endl;
			return;
		}
	}
	/*
	else if (sss == ".png") {
		if (!LoadPNG(path, width, height, chn, &data)) {
			cout << "load png failed! " << path << endl;
			return;
		}
	}
	*/
	else  {
		cout << "Image extension invalid! " << path << endl;
		return;
	}

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, data);
	else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
	if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap && (filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (filter == TEX_FILTER_POINT)? GL_NEAREST : GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	glBindTexture(GL_TEXTURE_2D, 0);
	delete[](data);
	loaded = true;
	//cout << "image loaded: " << width << "x" << height << endl;
}

Texture::Texture(int i, Editor* e) : AssetObject(ASSETTYPE_TEXTURE) {
	string p = e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_TEXTURE][i] + ".meta";
	ifstream strm(p, ios::in | ios::binary);
	if (strm.is_open()) {
		byte chn;
		GLenum rgb = GL_RGB, rgba = GL_RGBA;
		vector<char> hd(4);
		strm.read((&hd[0]), 3);
		if (hd[0] != 'I' || hd[1] != 'M' || hd[2] != 'G') {
			e->_Error("Image Cacher", "Image cache header wrong! " + p);
			return;
		}
		byte bb;
		_Strm2Val<byte>(strm, chn);
		_Strm2Val<uint>(strm, width);
		_Strm2Val<uint>(strm, height);
		_Strm2Val<byte>(strm, bb);
		if (bb == 1) {
			rgb = GL_BGR;
			rgba = GL_BGRA;
		}
		_Strm2Val<byte>(strm, _aniso);
		_Strm2Val<byte>(strm, bb);
		_filter = (TEX_FILTERING)bb;
		_Strm2Val<byte>(strm, bb);
		_mipmap = (bb & 0xf0) == 0xf0;
		_repeat = (bb & 0x0f) == 0x0f;
		strm.read((&hd[0]), 4);
		if (hd[0] != 'D' || hd[1] != 'A' || hd[2] != 'T' || hd[3] != 'A') {
			e->_Error("Image Cacher", "Image cache data tag wrong! " + p);
			return;
		}
		byte* data = new byte[chn*width*height];
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

//IMG [channels] [xxxx] [yyyy] [rgb=0, bgr=1] [aniso] [filter] [mipmap 0xf0 | repeat 0x0f] DATA [data]
bool Texture::Parse(Editor* e, string path) {
	byte ans = 5, flt = 2, mnr = 0xf0;
	string sss = path.substr(path.find_last_of('.'), string::npos);
	byte *data = nullptr;
	byte chn;
	uint width, height;
	GLenum rgb = GL_RGB, rgba = GL_RGBA;
	if (sss == ".bmp") {
		if (!LoadBMP(path, width, height, chn, &data)) {
			cout << "load bmp failed! " << path << endl;
			return false;
		}
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	else if (sss == ".jpg") {
		if (!LoadJPEG(path, width, height, chn, &data)) {
			cout << "load jpg failed! " << path << endl;
			return false;
		}
	}
	else  {
		cout << "Image extension invalid! " << path << endl;
		return false;
	}
	if (data == nullptr)
		return false;
	string ss(path + ".meta");
	ifstream iStrm(ss, ios::in | ios::binary); //if exists, read old prefs
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
	ofstream str(ss, ios::out | ios::trunc | ios::binary);
	str << "IMG";
	str << chn;
	_StreamWrite(&width, &str, 4);
	_StreamWrite(&height, &str, 4);
	str << (byte)((rgb == GL_RGB)? 0 : 1);
	str << ans << flt << mnr;
	str << "DATA";
	_StreamWrite(data, &str, width*height*chn);
	str.close();
	delete[](data);
	return true;
}

void Texture::_ApplyPrefs(const string& p) {
	ifstream iStrm(p, ios::in | ios::binary | ios::ate);
	if (iStrm.is_open()) {
		uint sz((uint)iStrm.tellg());
		vector<byte> data(sz);
		iStrm.seekg(0);
		iStrm.read((char*)(&data[0]), sz);
		iStrm.close();

		data[13] = _aniso;
		data[14] = _filter;
		data[15] = (_mipmap ? 0xf0 : 0) | (_repeat ? 0x0f : 0);

		remove(p.c_str());
		ofstream strm(p, ios::out | ios::binary | ios::trunc);
		if (strm.is_open()) {
			strm.write((char*)(&data[0]), sz);
			strm.close();
			SetFileAttributes(p.c_str(), FILE_ATTRIBUTE_HIDDEN);
		}
	}
}

Background::Background(const string& path) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
	if (path.size() < 5 || path.substr(path.size() - 4, string::npos) != ".hdr") {
		printf("HDRI path invalid!");
		return;
	}
	//cout << "opening hdr image at " << path << endl;
	
	byte* data2 = hdr::read_hdr(path.c_str(), &width, &height);
	if (data2 == NULL)
		return;
	float* data = hdr2float(data2, width, height);

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
	delete[](data2);
	//cout << "HDR Image loaded: " << width << "x" << height << endl;
}

bool Background::Parse(string path) {
	uint width, height;
	byte* data = hdr::read_hdr(path.c_str(), &width, &height);
	if (data == NULL)
		return false;
	string ss(path + ".meta");
	ofstream str(ss, ios::out | ios::trunc | ios::binary);
	str << "IMG";
	str << (byte)4;
	_StreamWrite(&width, &str, 4);
	_StreamWrite(&height, &str, 4);
	str << (byte)0;
	str << "DATA";
	_StreamWrite(data, &str, width*height*4);
	str.close();
	delete[](data);
	return true;
}

//-----------------font class---------------------
Font::Font(const string& pathb, const string& paths, int size) : loaded(false), alignment(ALIGN_TOPLEFT), sizeToggle(size) {
	if (paths != "") {
		//cout << "opening font at " << paths << endl;
		unsigned char header[6];
		//unsigned int charSize;
		unsigned char *data;

		FILE *file;

		fopen_s(&file, paths.c_str(), "rb");

		if (!file){
			cout << "Font could not be opened! " << paths << endl;
			return;
		}
		if (fread(header, 1, 6, file) != 6){
			cout << "font header wrong! " << paths << endl;
			return;
		}
		if (header[0] != 'f' || header[1] != 'o' || header[2] != 'n' || header[3] != 't'){
			cout << "Not a font file! " << paths << endl;
			return;
		}
		width = header[4];
		uint ii = 2;
		while (ii < width)
			ii *= 2;
		padding = ii - width;
		width = ii;
		height = header[5];
		chars = 125;
		w2h = width * 1.0f / height;
		data = new unsigned char[chars * width*height];
		if (fread(data, 1, chars * width*height, file) != chars * width*height) {
			cout << "font data incomplete! " << paths << endl;
			return;
		}
		fclose(file);

		glGenTextures(1, &pointer);
		glBindTexture(GL_TEXTURE_2D, pointer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height*chars, 0, GL_RED, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		delete[](data);
		//cout << "font loaded: " << width << "x" << height << endl;
		//loaded = true;
	}
	//cout << "opening font at " << pathb << endl;
	unsigned char header[6];
	//unsigned int charSize;
	unsigned char *data;

	FILE *file;

	fopen_s(&file, pathb.c_str(), "rb");

	if (!file){
		printf("Font could not be opened\n");
		return;
	}
	if (fread(header, 1, 6, file) != 6){
		printf("font header wrong\n");
		return;
	}
	if (header[0] != 'f' || header[1] != 'o' || header[2] != 'n' || header[3] != 't'){
		printf("Not a font file\n");
		return;
	}
	width2 = header[4];
	uint ii = 2;
	while (ii < width2)
		ii *= 2;
	padding2 = ii - width2;
	width2 = ii;
	height2 = header[5];
	chars2 = 125;
	w2h2 = width2 * 1.0f / height2;
	data = new unsigned char[chars2 * width2*height2];
	if (fread(data, 1, chars2 * width2*height2, file) != chars2 * width2*height2) {
		printf("font data incomplete\n");
		return;
	}
	fclose(file);

	glGenTextures(1, &pointer2);
	glBindTexture(GL_TEXTURE_2D, pointer2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width2, height2*chars, 0, GL_RED, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	delete[](data);
	loaded = true;
	//cout << "font loaded: " << width2 << "x" << height2 << endl;
}

Font* Font::Align(ALIGNMENT a) {
	alignment = a;
	return this;
}

GLuint Font::getpointer(float f) {
	return (f > sizeToggle) ? pointer2 : pointer;
}

//--------------------Display class--------------
int Display::width = 512;
int Display::height = 512;
glm::mat3 Display::uiMatrix = glm::mat3();

//--------------------Input class--------------
Vec2 Input::mousePos = Vec2(0, 0);
Vec2 Input::mousePosRelative = Vec2(0, 0);
Vec2 Input::mouseDownPos = Vec2(0, 0);

//-------------------Material class--------------
Material::Material() : _shader(-1), AssetObject(ASSETTYPE_MATERIAL) {
	shader = nullptr;// Engine::unlitProgram;
}

Material::Material(ShaderBase * shad) : _shader(-1), AssetObject(ASSETTYPE_MATERIAL) {
	shader = shad;
	if (shad == nullptr)
		return;
	ResetVals();
	if (Editor::instance != nullptr)
		_shader = Editor::instance->GetAssetId(shader);
	for (ShaderVariable* v : shad->vars) {
		void* l = nullptr;
		if (v->type == SHADER_INT)
			l = new int();
		else if (v->type == SHADER_FLOAT)
			l = new float();
		else if (v->type == SHADER_SAMPLER)
			l = new MatVal_Tex();
		valNames[v->type].push_back(v->name);
		vals[v->type].emplace(glGetUniformLocation(shad->pointer, v->name.c_str()), l);
	}
}

Material::Material(string path) : AssetObject(ASSETTYPE_MATERIAL) {
	string p = Editor::instance->projectFolder + "Assets\\" + path;
	ifstream stream(p.c_str());
	if (!stream.good()) {
		cout << "material not found!" << endl;
		return;
	}
	char* c = new char[4];
	stream.read(c, 3);
	c[3] = (char)0;
	string ss(c);
	if (string(c) != "KTC") {
		cerr << "file not supported" << endl;
		return;
	}
	delete[](c);
	char* nmm = new char[100];
	stream.getline(nmm, 100, (char)0);
	string shp(nmm);
	if (shp == "")
		return;
	ASSETTYPE t;
	Editor::instance->GetAssetInfo(shp, t, _shader);
	shader = _GetCache<ShaderBase>(ASSETTYPE_SHADER, _shader);
	
	if (shader == nullptr)
		return;
	ResetVals();
	unordered_map<string, GLint> nMap;
	//if (Editor::instance != nullptr)
		//_shader = Editor::instance->GetAssetId(shader);
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
		vals[v->type].emplace(loc, l);
		nMap.emplace(v->name, loc);
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
					Editor::instance->GetAssetInfo(nm2, t, ((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id);
					((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->tex = _GetCache<Texture>(ASSETTYPE_TEXTURE, ((MatVal_Tex*)vals[SHADER_SAMPLER][nMap[nm]])->id);
					free(nmm2);
					break;
				}
			}
			break;
		}
	}
	delete[](nmm);
	stream.close();
}

Material::~Material() {
	for (auto a : vals) {
		for (auto b : a.second)
			delete(b.second);
	}
}

void Material::ResetVals() {
	vals[SHADER_INT] = unordered_map <GLint, void*>();
	vals[SHADER_FLOAT] = unordered_map <GLint, void*>();
	vals[SHADER_VEC2] = unordered_map <GLint, void*>();
	vals[SHADER_VEC3] = unordered_map <GLint, void*>();
	vals[SHADER_SAMPLER] = unordered_map <GLint, void*>();
	vals[SHADER_MATRIX] = unordered_map <GLint, void*>();
	valNames[SHADER_INT] = vector<string>();
	valNames[SHADER_FLOAT] = vector<string>();
	valNames[SHADER_VEC2] = vector<string>();
	valNames[SHADER_VEC3] = vector<string>();
	valNames[SHADER_SAMPLER] = vector<string>();
	valNames[SHADER_MATRIX] = vector<string>();
}

void Material::SetTexture(string name, Texture * texture) {
	SetTexture(glGetUniformLocation(shader->pointer, name.c_str()), texture);
}
void Material::SetTexture(GLint id, Texture * texture) {
	vals[SHADER_SAMPLER][id] = texture;
}

void Material::Save(string path) {
	ofstream strm(path, ios::out | ios::binary | ios::trunc);
	strm << "KTC";
	ASSETTYPE st;
	ASSETID si = Editor::instance->GetAssetId(shader, st);
	_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_SHADER, si);
	int i = 0, j = 0;
	SHADER_VARTYPE t = 0;
	long long p1 = strm.tellp();
	strm << "0000";
	//_StreamWrite(&i, &strm, 4);
	for (auto v : vals[SHADER_INT]) {
		t = SHADER_INT;
		strm << t;
		strm << valNames[SHADER_INT][j] << (char)0;
		int ii(*(int*)v.second);
		_StreamWrite(&ii, &strm, 4);
		i++;
		j++;
	}
	j = 0;
	for (auto v : vals[SHADER_FLOAT]) {
		t = SHADER_FLOAT;
		strm << t;
		strm << valNames[SHADER_FLOAT][j] << (char)0;
		float ff(*(float*)v.second);
		_StreamWrite(&ff, &strm, 4);
		i++;
		j++;
	}
	j = 0;
	for (auto v : vals[SHADER_SAMPLER]) {
		t = SHADER_SAMPLER;
		strm << t;
		strm << valNames[SHADER_SAMPLER][j] << (char)0;
		_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_TEXTURE, ((MatVal_Tex*)v.second)->id);
		i++;
		j++;
	}
	strm.seekp(p1);
	_StreamWrite(&i, &strm, 4);
	strm.close();
}

void Material::ApplyGL(glm::mat4& matrix) {
	if (shader == nullptr) {
		glUseProgram(0);
		return;
	}
	else {
		glUseProgram(shader->pointer);
		GLint mvp = glGetUniformLocation(shader->pointer, "_MVP");
		//GLint p = glGetUniformLocation(shader->pointer, "_P");
		glUniformMatrix4fv(mvp, 1, GL_FALSE, glm::value_ptr(matrix));
		//glUniformMatrix4fv(p, 1, GL_FALSE, matrix2);
		for (auto a : vals[SHADER_INT])
			if (a.second != nullptr)
				glUniform1i(a.first, *(int*)a.second);
		for (auto a : vals[SHADER_FLOAT])
			if (a.second != nullptr)
				glUniform1f(a.first, *(float*)a.second);
		for (auto a : vals[SHADER_VEC2]) {
			if (a.second == nullptr)
				continue;
			Vec2* v2 = (Vec2*)a.second;
			glUniform2f(a.first, v2->x, v2->y);
		}
		int ti = 0;
		for (auto a : vals[SHADER_SAMPLER]) {
			if (a.second == nullptr)
				continue;
			MatVal_Tex* tx = (MatVal_Tex*)a.second;
			if (tx->tex == nullptr)
				continue;
			glUniform1i(a.first, ti);
			glActiveTexture(GL_TEXTURE0 + ti);
			glBindTexture(GL_TEXTURE_2D, tx->tex->pointer);
		}

	}
}

void _StreamWrite(const void* val, ofstream* stream, int size) {
	stream->write(reinterpret_cast<char const *>(val), size);
}

void _StreamWriteAsset(Editor* e, ofstream* stream, ASSETTYPE t, ASSETID i) {
	if (i < 0) {
		(*stream) << (char)0;
		return;
	}
	string p;
	if (t == ASSETTYPE_SCRIPT_H)
		p = e->headerAssets[i];
	else
		p = e->normalAssets[t][i];
	(*stream) << p << (char)0;
}

ASSETID _Strm2H(ifstream& strm) {
	return -1;
}
string _Strm2Asset(ifstream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int max) {
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

float* hdr2float(byte imagergbe[], int w, int h) {
	float* image = (float *)malloc(w * h * 3 * sizeof(float));
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

void Serialize(Editor* e, SceneObject* o, ofstream* stream) {
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

void Deserialize(ifstream& stream, SceneObject* obj) {
	char* cc = new char[100];
	stream.getline(cc, 100, 0);
	obj->name = string(cc);
	delete[](cc);
	Debug::Message("Object Deserializer", "Deserializing object " + obj->name);
	char c;
	_Strm2Val(stream, obj->transform.position.x);
	_Strm2Val(stream, obj->transform.position.y);
	_Strm2Val(stream, obj->transform.position.z);
	Vec3 r;
	_Strm2Val(stream, r.x);
	_Strm2Val(stream, r.y);
	_Strm2Val(stream, r.z);
	obj->transform.eulerRotation(r);
	_Strm2Val(stream, obj->transform.scale.x);
	_Strm2Val(stream, obj->transform.scale.y);
	_Strm2Val(stream, obj->transform.scale.z);
	stream >> c;
	while (c != 'o') {
		Debug::Message("Object Deserializer", to_string(c) + " " + to_string(stream.tellg()));
		if (c == 'O') {
			SceneObject* sc = new SceneObject();
			obj->AddChild(sc);
			Deserialize(stream, sc);
		}
		else if (c == 'C') {
			stream >> c; //component type
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
			case COMP_SCR:
#ifndef IS_EDITOR
				//obj->AddComponent(SceneScriptResolver::instance->Resolve(stream, obj));
#endif
				break;
			default:
				char cc;
				stream >> cc;
				while (cc != 'c')
					stream >> cc;
				long long ss = stream.tellg();
				stream.seekg(ss-1);
				break;
			}
			Debug::Message("Object Deserializer", "2 " + to_string(stream.tellg()));
			stream >> c;
			if (c != 'c')
				Debug::Error("Object Deserializer", "scene data corrupted(component)");
		}
		else Debug::Error("Object Deserializer", "scene data corrupted(2)");
		stream >> c;
	}
}

void Scene::ReadD0() {
	ushort num;
	_Strm2Val(*strm, num);
	for (ushort a = 0; a < num; a++) {
		uint sz;
		char *c = new char[100];
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

Scene::Scene(ifstream& stream, long pos) : sceneName("") {
	vector<SceneObject*>().swap(objects);
	stream.seekg(pos);
	char h1, h2;
	stream >> h1 >> h2;
	if (h1 != 'S' || h2 != 'N')
		throw runtime_error("scene data corrupted");
	char* cc = new char[100];
	stream.getline(cc, 100, 0);
	sceneName += string(cc);
	char o;
	stream >> o;
	while (!stream.eof() && o == 'O') {
		SceneObject* sc = new SceneObject();
		objects.push_back(sc);
		Deserialize(stream, sc);
		sc->Refresh();
		stream >> o;
	}
}

Scene* Scene::active = nullptr;
ifstream* Scene::strm = nullptr;
vector<string> Scene::sceneNames = {};
vector<long> Scene::scenePoss = {};

void Scene::Load(string name) {
	for (uint a = sceneNames.size() - 1; a >= 0; a--) {
		if (sceneNames[a] == name) {
			Load(a);
			return;
		}
	}
}

void Scene::Load(uint i) {
	if (i >= scenePoss.size()) {
		Debug::Error("Scene Loader", "Scene ID (" + to_string(i) + ") out of range!");
		return;
	}
	free(active);
	Debug::Message("Scene Loader", "Loading scene " + to_string(i) + "...");
	active = new Scene(*strm, scenePoss[i]);
	active->sceneId = i;
	Debug::Message("Scene Loader", "Loaded scene " + to_string(i) + "(" + sceneNames[i] + ")");
}

void Scene::Save(Editor* e) {
	ofstream sw(e->projectFolder + "Assets\\" + sceneName + ".scene", ios::out);
	sw << "SN";
	sw << sceneName << (char)0;
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
	e->includedScenes.clear();
	e->includedScenes.push_back(e->projectFolder + "Assets\\test.scene");
}

unordered_map<ASSETTYPE, vector<string>> AssetManager::names = {};
unordered_map<ASSETTYPE, vector<pair<byte, uint>>> AssetManager::dataLocs = {};
unordered_map<ASSETTYPE, vector<void*>> AssetManager::dataCaches = {};
vector<ifstream*> AssetManager::streams = {};
void AssetManager::Init(string dpath) {
	names.clear();
	dataLocs.clear();
	vector<ifstream*>().swap(streams);

	string pp = dpath + "0";
	Scene::strm = new ifstream(pp.c_str(), ios::in | ios::binary);
	ifstream* strm = Scene::strm;
	if (!strm->is_open()) {
		Debug::Error("AssetManager", "Fatal: cannot open data file 0!");
		return;
	}

	byte numDat = 0, id;
	ushort num;
	ASSETTYPE type;
	uint pos;
	char* c = new char[100];
	*strm >> c[0] >> c[1];
	if (c[0] != 'D' || c[1] != '0') {
		Debug::Error("AssetManager", "Fatal: file 0 has wrong signature!");
		return;
	}
	_Strm2Val(*strm, num);
	for (ushort a = 0; a < num; a++) {
		_Strm2Val(*strm, type);
		_Strm2Val(*strm, id);
		_Strm2Val(*strm, pos);
		strm->getline(c, 100, (char)0);
		numDat = max(numDat, id);
		dataLocs[type].push_back(pair<byte, uint>(id - 1, pos));
		dataCaches[type].push_back(nullptr);
		names[type].push_back(c);
	}

	for (int a = 0; a < numDat; a++) {
		string pp = dpath + to_string(a+1);
		streams.push_back(new ifstream(pp.c_str(), ios::in | ios::binary));
	}

	for (auto aa : dataLocs) {
		ushort s = 0;
		for (auto bb : aa.second) {
			streams[bb.first]->seekg(bb.second);
			uint sz;
			_Strm2Val(*streams[bb.first], sz);
			Debug::Message("AssetManager", "Registered asset " + names[aa.first][s] + " (" + to_string(sz) + " bytes)");
			streams[bb.first]->seekg(0);
			s++;
		}
	}
	long long ppos = strm->tellg();
	Scene::ReadD0();
}

void* AssetManager::CacheFromName(string nm) {
	ASSETTYPE t;
	
	for (uint a = names[t].size() - 1; a >= 0; a--) {
		if (names[t][a] == nm) {
			return GetCache(t, a);
		}
	}
	Debug::Warning("Asset Finder", "Asset not found for " + nm + "!");
	return nullptr;
}

ASSETID AssetManager::GetAssetId(string nm, ASSETTYPE &t) {
	for (auto q : names) {
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

void* AssetManager::GetCache(ASSETTYPE t, ASSETID i) {
	if (dataCaches[t][i] != nullptr)
		return dataCaches[t][i];
	else
		return GenCache(t, i);
}
void* AssetManager::GenCache(ASSETTYPE t, ASSETID i) {
	return nullptr;
}