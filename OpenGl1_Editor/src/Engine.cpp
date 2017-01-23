#include "Engine.h"
#include "Editor.h"
#include "hdr.h"
#include "KTMModel.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>
#include <shellapi.h>
#include <Windows.h>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

void Engine::Init(string path) {
	fallbackTex = new Texture(path.substr(0, path.find_last_of('\\') + 1) + "fallback.bmp");
	if (!fallbackTex->loaded)
		cout << "cannot load fallback texture!" << endl;

	string vertcode = "#version 330 core\nlayout(location = 0) in vec3 pos;\nlayout(location = 1) in vec2 uv;\nout vec2 UV;\nvoid main(){ \ngl_Position.xyz = pos;\ngl_Position.w = 1.0;\nUV = uv;\n}";
	string fragcode = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\color = texture(sampler, UV)*col;\n}"; //out vec3 Vec4;\n
	string fragcode2 = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\color = vec4(1, 1, 1, texture(sampler, UV).r)*col;\n}"; //out vec3 Vec4;\n
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

void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture) {
	DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer);
}
void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, float alpha) {
	DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, Vec4(1, 1, 1, alpha));
}
void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, Vec4 tint) {
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
			int x0 = (x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f));
			if (x0 > Display::width)
				break;

			float r = Display::height * 1.0f / Display::width; //(x2 + a*s2*font->w2h*w*r) * 2 - 1, (1 - y2) * 2 - 1, s2*font->w2h * 2 * w*r, -s2 * 2
			//DrawQuad(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f), y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f), s*font->gw2h(s)*w, s, font->getpointer(s), Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), true, Vec4);//Vec2(0, 0.49), Vec2(1, 0.49));//(1.0f / font->chars)), Vec2(1, h - (1.0f / font->chars)));
			AddQuad(x0, (int)(y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f)), s*font->gw2h(s)*w, s, Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), &quadPoss, &indexes, &uvs, a * 4);
		}
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
	Label((int)(x + 2), (int)(y + 0.5f*h), labelSize, label, labelFont, labelVec4);
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
	for (int a = 1; a < 106; a++) {
		keyStatusNew[a] = ((GetAsyncKeyState(a) >> 8) == -128);
	}

	if (mouse0)
		mouse0State = min(mouse0State+1, MOUSE_HOLD);
	else
		mouse0State = (mouse0State == MOUSE_UP | mouse0State == 0) ? 0 : MOUSE_UP;
	if (mouse1)
		mouse1State = min(mouse1State + 1, MOUSE_HOLD);
	else
		mouse1State = (mouse1State == MOUSE_UP | mouse1State == 0) ? 0 : MOUSE_UP;
	if (mouse2)
		mouse2State = min(mouse2State + 1, MOUSE_HOLD);
	else
		mouse2State = (mouse2State == MOUSE_UP | mouse2State == 0) ? 0 : MOUSE_UP;
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

}
void Debug::Warning(string c, string s) {

}
void Debug::Error(string c, string s) {

}
//-----------------io class-----------------------
vector<string> IO::GetFiles(const string& folder)
{
	if (folder == "") return vector<string>();
	vector<string> names;
	string search_path = folder + "/*";
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
					names.push_back(EB_Browser_File(e, folder, aa.substr(0, aa.length() - 5)));
				else if ((aa.length() > 2 && aa.substr(aa.length() - 2, string::npos) == ".h"))
					names.push_back(EB_Browser_File(e, folder, aa));
				else if ((aa.length() > 4 && aa.substr(aa.length() - 4, string::npos) == ".cpp"))
					names.push_back(EB_Browser_File(e, folder, aa));
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}

void IO::GetFolders(const string& folder, vector<string>* names)
{
	//vector<string> names;
	string search_path = folder + "/*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
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
		for (int i = 0; i < size; i++) {
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
Vec4 black() { return black(1); }
Vec4 white() { return white(1, 1); }

Vec4 red(float f) { return red(f, 1); }
Vec4 green(float f) { return green(f, 1); }
Vec4 blue(float f) { return blue(f, 1); }
Vec4 cyan(float f) { return cyan(f, 1); }
Vec4 black(float f) { return Vec4(0, 0, 0, f); }
Vec4 white(float f) { return white(f, 1); }

Vec4 red(float f, float i) { return Vec4(i, 0, 0, f); }
Vec4 green(float f, float i) { return Vec4(0, i, 0, f); }
Vec4 blue(float f, float i) { return Vec4(0, 0, i, f); }
Vec4 cyan(float f, float i) { return Vec4(i*0.09f, i*0.706f, i, f); }
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
	return glm::mat4(q.w, q.x, q.y, q.z, -q.x, q.w, q.z, -q.y, -q.y, -q.z, q.w, q.x, -q.z, q.y, -q.x, q.w);
}

//-----------------texture class---------------------
Texture::Texture(const string& path) : Texture(path, true, false) {}
Texture::Texture(const string& path, bool mipmap) : Texture(path, mipmap, false) {}
Texture::Texture(const string& path, bool mipmap, bool nearest) {
	if (path.size() < 5 || path.substr(path.size() - 4, string::npos) != ".bmp") {
		printf("Image path invalid!");
		return;
	}
	cout << "opening image at " << path << endl;
	char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int imageSize;   // = width*height*3
	unsigned char *data;
	unsigned short bpi;

	FILE *file;

	fopen_s(&file, path.c_str(), "rb");

	if (!file){
		printf("Image could not be opened\n");
		return;
	}
	if (fread(header, 1, 54, file) != 54){ // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return;
	}
	if (header[0] != 'B' || header[1] != 'M'){
		printf("Not a correct BMP file\n");
		return;
	}
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);
	bpi = *(short*)&(header[0x1c]);
	if (bpi != 24 && bpi != 32)
		return;
	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	data = new unsigned char[imageSize];
	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);
	//Everything is in memory now, the file can be closed
	fclose(file);
	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	if (bpi == 24)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (nearest) ? GL_NEAREST : GL_LINEAR);
	if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap) ? GL_LINEAR_MIPMAP_LINEAR : (nearest)? GL_NEAREST : GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
	cout << "image loaded: " << width << "x" << height << endl;
}
Texture::Texture(ifstream& stream, long pos) {
	
	char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int imageSize;   // = width*height*3
	char *data;
	unsigned short bpi;

	stream.seekg(pos);

	stream.get((char*)&header[0], 54);
	if (header[0] != 'B' || header[1] != 'M'){
		printf("Not a correct BMP file\n");
		return;
	}
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);
	bpi = *(short*)&(header[0x1c]);
	if (bpi != 24 && bpi != 32)
		return;
	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	data = new char[imageSize];
	// Read the actual data from the file into the buffer
	stream.get(data, imageSize);

	stream.seekg(pos-1);
	byte header1;
	stream >> header1;

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	if (bpi == 24)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (header1 & 1 == 1) ? GL_NEAREST : GL_LINEAR);
	if (header1 & 2 == 2) glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (header1 & 2 == 2) ? GL_LINEAR_MIPMAP_LINEAR : (header1 & 1 == 1) ? GL_NEAREST : GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
	cout << "image loaded: " << width << "x" << height << endl;
}

Background::Background(const string& path) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
	if (path.size() < 5 || path.substr(path.size() - 4, string::npos) != ".hdr") {
		printf("HDRI path invalid!");
		return;
	}
	cout << "opening hdr image at " << path << endl;
	
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
	cout << "HDR Image loaded: " << width << "x" << height << endl;
}

//-----------------font class---------------------
Font::Font(const string& path) : Font("", path, -1) {}
Font::Font(const string& paths, const string& pathb, int size) : loaded(false), alignment(ALIGN_TOPLEFT), sizeToggle(size) {
	if (paths != "") {
		cout << "opening font at " << paths << endl;
		unsigned char header[6];
		//unsigned int charSize;
		unsigned char *data;

		FILE *file;

		fopen_s(&file, paths.c_str(), "rb");

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
			printf("font data incomplete\n");
			return;
		}
		fclose(file);

		glGenTextures(1, &pointer);
		glBindTexture(GL_TEXTURE_2D, pointer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height*chars, 0, GL_RED, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		cout << "font loaded: " << width << "x" << height << endl;
		//loaded = true;
	}
	cout << "opening font at " << pathb << endl;
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
	loaded = true;
	cout << "font loaded: " << width2 << "x" << height2 << endl;
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

//-------------------Material class--------------
Material::Material() {
	shader = nullptr;// Engine::unlitProgram;
}

Material::Material(ShaderBase * shad) {
	shader = shad;
}

void Material::SetTexture(string name, Texture * texture) {
	SetTexture(glGetUniformLocation(shader->pointer, name.c_str()), texture);
}
void Material::SetTexture(GLint id, Texture * texture) {
	shader->Set(SHADER_SAMPLER, id, texture);
}

void _StreamWrite(void* val, ofstream* stream, int size) {
	stream->write(reinterpret_cast<char const *>(val), size);
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
			float v = (1.0f / 256.0f) * pow(2, (float)(exponent - 128));
			image[i * 3 + 0] = (imagergbe[i * 4 + 0] + 0.5f) * v;
			image[i * 3 + 1] = (imagergbe[i * 4 + 1] + 0.5f) * v;
			image[i * 3 + 2] = (imagergbe[i * 4 + 2] + 0.5f) * v;
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
	stream->write("O", 1);
	_StreamWrite(&o->name, stream, o->name.size());
	*stream << "\n";
	_StreamWrite(&o->transform.position.x, stream, 4);
	_StreamWrite(&o->transform.position.y, stream, 4);
	_StreamWrite(&o->transform.position.z, stream, 4);
	_StreamWrite(&o->transform.eulerRotation.x, stream, 4);
	_StreamWrite(&o->transform.eulerRotation.y, stream, 4);
	_StreamWrite(&o->transform.eulerRotation.z, stream, 4);
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
	stream->write("o", 1);
}

void Deserialize(ifstream& stream, SceneObject* obj) {
	char c;
	stream >> c;
	while (c != '\n') {
		obj->name += c;
		stream >> c;
	}
	stream >> obj->transform.position.x;
	stream >> obj->transform.position.y;
	stream >> obj->transform.position.z;
	stream >> obj->transform.eulerRotation.x;
	stream >> obj->transform.eulerRotation.y;
	stream >> obj->transform.eulerRotation.z;
	stream >> obj->transform.scale.x;
	stream >> obj->transform.scale.y;
	stream >> obj->transform.scale.z;
	stream >> c;
	if (c == 'o')
		return;
	if (c != 'C')
		throw runtime_error("scene data corrupted(2)");
	stream >> c; //component type
	switch (c) {
	case COMP_CAM:
		obj->AddComponent(new Camera(stream, -1));
	case COMP_TRD:
		obj->AddComponent(new TextureRenderer(stream, -1));
	}
}

Scene::Scene(ifstream& stream, long pos) : sceneName(""), sky(nullptr) {
	stream.seekg(pos);
	char h1, h2;
	stream >> h1 >> h2;
	if (h1 != 'S' && h2 != 'N')
		throw runtime_error("scene data corrupted");
	char o;
	stream >> o;
	while (o == 'O') {
		SceneObject* sc = new SceneObject();
		objects.push_back(sc);
		Deserialize(stream, sc);
	}
}

void Scene::Save(Editor* e) {
	ofstream sw(e->projectFolder + "Assets\\test.scene", ios::out);
	sw << "SN";
	for (SceneObject* sc : objects) {
		Serialize(e, sc, &sw);
	}
	sw.close();

	//
	e->includedScenes.clear();
	e->includedScenes.push_back(e->projectFolder + "Assets\\test.scene");
}