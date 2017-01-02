#include "Engine.h"
#include "Editor.h"
#include "Shader.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <climits>
#include <Windows.h>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

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

uint Engine::unlitProgram = 0;
uint Engine::unlitProgramA = 0;
uint Engine::unlitProgramC = 0;
Font* Engine::defaultFont;//&Font("F:\\ascii 2.font");
bool Input::mouse0 = false;
bool Input::mouse1 = false;
bool Input::mouse2 = false;
byte Input::mouse0State = 0;
byte Input::mouse1State = 0;
byte Input::mouse2State = 0;

void Engine::Init() {
	string vertcode = "#version 330 core\nlayout(location = 0) in vec3 pos;\nlayout(location = 1) in vec2 uv;\nout vec2 UV;\nvoid main(){ \ngl_Position.xyz = pos;\ngl_Position.w = 1.0;\nUV = uv;\n}";
	string fragcode = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nvoid main(){\ngl_FragColor = texture(sampler, UV)*col;\n}"; //out vec3 color;\n
	string fragcode2 = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nvoid main(){\ngl_FragColor = vec4(1, 1, 1, texture(sampler, UV).r)*col;\n}"; //out vec3 color;\n
	string fragcode3 = "#version 330 core\nin vec2 UV;\nuniform vec4 col;\nvoid main(){\ngl_FragColor = col;\n}"; //out vec3 color;\n

	unlitProgram = glCreateProgram();
	GLuint vertex_shader = Shader::LoadShader(GL_VERTEX_SHADER, vertcode);
	glAttachShader(unlitProgram, vertex_shader);
	GLuint fragment_shader = Shader::LoadShader(GL_FRAGMENT_SHADER, fragcode);
	glAttachShader(unlitProgram, fragment_shader);

	int link_result = 0;

	glLinkProgram(unlitProgram);
	glGetProgramiv(unlitProgram, GL_LINK_STATUS, &link_result);
	if (link_result == GL_FALSE)
	{
		int info_log_length = 0;
		glGetProgramiv(unlitProgram, GL_INFO_LOG_LENGTH, &info_log_length);
		vector<char> program_log(info_log_length);
		glGetProgramInfoLog(unlitProgram, info_log_length, NULL, &program_log[0]);
		cerr << "Default Shader link error" << endl << &program_log[0] << endl;
		return;
	}
	glDetachShader(unlitProgram, vertex_shader);
	glDetachShader(unlitProgram, fragment_shader);

	unlitProgramA = glCreateProgram();
	glAttachShader(unlitProgramA, vertex_shader);
	GLuint fragment_shaderA = Shader::LoadShader(GL_FRAGMENT_SHADER, fragcode2);
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

	unlitProgramC = glCreateProgram();
	glAttachShader(unlitProgramC, vertex_shader);
	GLuint fragment_shaderC = Shader::LoadShader(GL_FRAGMENT_SHADER, fragcode3);
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
		cerr << "Default Shader (Alpha) link error" << endl << &program_log[0] << endl;
		return;
	}
	glDetachShader(unlitProgramC, vertex_shader);
	glDeleteShader(vertex_shader);
	glDetachShader(unlitProgramC, fragment_shaderC);
	glDeleteShader(fragment_shader);
	//defaultFont = &Font("F:\\ascii 2.font");
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

void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture) {
	if (texture->loaded) DrawQuad(x, y, w, h, texture->pointer);
}
void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, float alpha) {
	if (texture->loaded) DrawQuad(x, y, w, h, texture->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, Color(1, 1, 1, alpha));
}
void Engine::DrawTexture(float x, float y, float w, float h, Texture* texture, Color tint) {
	if (texture->loaded) DrawQuad(x, y, w, h, texture->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, tint);
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
	Label(x, y, s, st, font, Color(0, 0, 0, 1));
}
void Engine::Label(float x, float y, float s, string st, Font* font, Color color) {
	Label(x, y, s, st, font, color, _FMAX);
}
void Engine::Label(float x, float y, float s, string st, Font* font, Color color, float maxw) {
	const char* str = st.c_str();
	
	vector<Vec3> quadPoss;
	vector<uint> indexes;
	vector<Vec2> uvs;

	if (font->loaded) {
		for (int a = 0; a < strlen(str); a++) {
			int o = str[a];
			float h = (o*1.0f / font->gchars(s));
			float w = (1 - (font->gpadding(s)*1.0f / font->gwidth(s)));

			if ((a+3)*s*font->gw2h(s)*w > maxw) {
				o = '.';
				h = (o*1.0f / font->gchars(s));
				w = (1 - (font->gpadding(s)*1.0f / font->gwidth(s)));
				DrawQuad(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f), y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f), s*font->gw2h(s)*w, s, font->getpointer(s), Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), true, color);
				a++;
				DrawQuad(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f), y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f), s*font->gw2h(s)*w, s, font->getpointer(s), Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), true, color);
				break;
			}

			float r = Display::height * 1.0f / Display::width; //(x2 + a*s2*font->w2h*w*r) * 2 - 1, (1 - y2) * 2 - 1, s2*font->w2h * 2 * w*r, -s2 * 2
			//DrawQuad(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f), y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f), s*font->gw2h(s)*w, s, font->getpointer(s), Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), true, color);//Vec2(0, 0.49), Vec2(1, 0.49));//(1.0f / font->chars)), Vec2(1, h - (1.0f / font->chars)));
			AddQuad(x + a*s*font->gw2h(s)*w - (s*font->gw2h(s)*w*st.size()*(font->alignment & 0x0f)*0.5f), y - s*(1 - ((font->alignment & 0xf0) >> 4)*0.5f), s*font->gw2h(s)*w, s, Vec2(0, h + (1.0f / font->gchars(s))), Vec2(w, h + (1.0f / font->gchars(s))), Vec2(0, h), Vec2(w, h), &quadPoss, &indexes, &uvs, a * 4);
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
		glUniform4f(baseColLoc, color.r, color.g, color.b, color.a);
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
byte Engine::Button(float x, float y, float w, float h, Color normalColor) {
	return Button(x, y, w, h, normalColor, LerpColor(normalColor, white(), 0.5f), LerpColor(normalColor, black(), 0.5f));
}
byte Engine::Button(float x, float y, float w, float h, Color normalColor, string label, float labelSize, Font* labelFont, Color labelColor) {
	return Button(x, y, w, h, normalColor, LerpColor(normalColor, white(), 0.5f), LerpColor(normalColor, black(), 0.5f), label, labelSize, labelFont, labelColor);
}
byte Engine::Button(float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor) {
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
	switch (Input::mouse0State) {
	case 0:
	case MOUSE_UP:
		DrawQuad(x, y, w, h, inside ? highlightColor : normalColor);
		break;
	case MOUSE_DOWN:
	case MOUSE_HOLD:
		DrawQuad(x, y, w, h, inside ? pressColor : normalColor);
		break;
	}
	return inside ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
}
byte Engine::Button(float x, float y, float w, float h, Texture* texture, Color normalColor, Color highlightColor, Color pressColor) {
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
	if (texture->loaded) {
		switch (Input::mouse0State) {
		case 0:
		case MOUSE_UP:
			DrawQuad(x, y, w, h, texture->pointer, Vec2(), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), false, inside ? highlightColor : normalColor);
			break;
		case MOUSE_DOWN:
		case MOUSE_HOLD:
			DrawQuad(x, y, w, h, texture->pointer, Vec2(), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), false, inside ? pressColor : normalColor);
			break;
		}
	}
	return inside ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
}
byte Engine::Button(float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor, string label, float labelSize, Font* labelFont, Color labelColor) {
	byte b = Button(x, y, w, h, normalColor, LerpColor(normalColor, white(), 0.5f), LerpColor(normalColor, black(), 0.5f));
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDCENTER;
	Label(x + 0.5f*w, y + 0.5f*h, labelSize, label, labelFont);
	labelFont->alignment = al;
	return b;
}

byte Engine::EButton(bool a, float x, float y, float w, float h, Color normalColor) {
	return EButton(a, x, y, w, h, normalColor, LerpColor(normalColor, white(), 0.5f), LerpColor(normalColor, black(), 0.5f));
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor) {
	if (a) {
		bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
		switch (Input::mouse0State) {
		case 0:
		case MOUSE_UP:
			DrawQuad(x, y, w, h, inside ? highlightColor : normalColor);
			break;
		case MOUSE_DOWN:
		case MOUSE_HOLD:
			DrawQuad(x, y, w, h, inside ? pressColor : normalColor);
			break;
		}
		return inside ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
	}
	else {
		DrawQuad(x, y, w, h, normalColor);
		return false;
	}
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Color normalColor, string label, float labelSize, Font* labelFont, Color labelColor) {
	return EButton(a, x, y, w, h, normalColor, LerpColor(normalColor, white(), 0.5f), LerpColor(normalColor, black(), 0.5f), label, labelSize, labelFont, labelColor);
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Texture* texture, Color normalColor, Color highlightColor, Color pressColor) {
	if (a) {
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
	if (texture->loaded) {
		switch (Input::mouse0State) {
		case 0:
		case MOUSE_UP:
			DrawQuad(x, y, w, h, texture->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, inside ? highlightColor : normalColor);
			break;
		case MOUSE_DOWN:
		case MOUSE_HOLD:
			DrawQuad(x, y, w, h, texture->pointer, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, inside ? pressColor : normalColor);
			break;
		}
	}
	return inside ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
	}
	else {
		DrawQuad(x, y, w, h, normalColor);
		return false;
	}
}
byte Engine::EButton(bool a, float x, float y, float w, float h, Color normalColor, Color highlightColor, Color pressColor, string label, float labelSize, Font* labelFont, Color labelColor) {
	byte b = EButton(a, x, y, w, h, normalColor, LerpColor(normalColor, white(), 0.5f), LerpColor(normalColor, black(), 0.5f));
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDCENTER;
	Label(x + 0.5f*w, y + 0.5f*h, labelSize, label, labelFont);
	labelFont->alignment = al;
	return b;
}

void Engine::RotateUI(float aa, Vec2 point) {
	float a = 3.1415926535f*aa / 180.0f;
	//Display::uiMatrix = glm::mat3(1, 0, 0, 0, 1, 0, point2.x * 2 - 1, point2.y * 2 - 1, 1)*glm::mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)*glm::mat3(1, 0, 0, 0, 1, 0, -point2.x * 2 + 1, -point2.y * 2 + 1, 1)*Display::uiMatrix;
	Display::uiMatrix = glm::mat3(1, 0, 0, 0, 1, 0, point.x, point.y, 1)*glm::mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)*glm::mat3(1, 0, 0, 0, 1, 0, -point.x, -point.y, 1)*Display::uiMatrix;
}
void Engine::ResetUIMatrix() {
	Display::uiMatrix = glm::mat3();
}

void Engine::DrawQuad(float x, float y, float w, float h, uint texture) {
	DrawQuad(x, y, w, h, texture, Vec2(0, 1), Vec2(1, 1), Vec2(0, 0), Vec2(1, 0), false, Color(1, 1, 1, 1));
}

void Engine::DrawQuad(float x, float y, float w, float h, Color color) {
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
	glUniform4f(baseColLoc, color.r, color.g, color.b, color.a);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableVertexAttribArray(0);
	glUseProgram(0);

	/*
	glColor3f(1.0f, 0.0f, 0.0f);
	glLineWidth(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);
	*/

	glDisableClientState(GL_VERTEX_ARRAY);
}

void Engine::DrawQuad(float x, float y, float w, float h, GLuint texture, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, bool single, Color color) {
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
	glUniform4f(baseColLoc, color.r, color.g, color.b, color.a);

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
	glColor3f(1.0f, 0.0f, 0.0f);
	glLineWidth(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);
	*/

	glDisableClientState(GL_VERTEX_ARRAY);
	//glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	//glDisable(GL_DEPTH_TEST);
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

void Engine::DrawLine(Vec2 v1, Vec2 v2, Color col, float width) {
	DrawLine(Vec3(v1.x, v1.y, 1), Vec3(v2.x, v2.y, 1), col, width);
}
void Engine::DrawLine(Vec3 v1, Vec3 v2, Color col, float width) {
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
void Engine::DrawLineW(Vec3 v1, Vec3 v2, Color col, float width) {
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

void Input::UpdateMouse() {
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
vector<EB_Browser_File> IO::GetFiles (const string& folder)
{
	vector<EB_Browser_File> names;
	string search_path = folder + "/*.*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(EB_Browser_File(folder, fd.cFileName));
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

//-----------------time class---------------------
long long Time::startMillis = 0;
long long Time::millis = 0;
double Time::time = 0;
float Time::delta = 0;

//-----------------colors--------------------------
Color red() { return red(1, 1); }
Color green() { return green(1, 1); }
Color blue() { return blue(1, 1); }
Color cyan() { return cyan(1, 1); }
Color black() { return black(1); }
Color white() { return white(1, 1); }

Color red(float f) { return red(f, 1); }
Color green(float f) { return green(f, 1); }
Color blue(float f) { return blue(f, 1); }
Color cyan(float f) { return cyan(f, 1); }
Color black(float f) { return Color(0, 0, 0, f); }
Color white(float f) { return white(f, 1); }

Color red(float f, float i) { return Color(i, 0, 0, f); }
Color green(float f, float i) { return Color(0, i, 0, f); }
Color blue(float f, float i) { return Color(0, 0, i, f); }
Color cyan(float f, float i) { return Color(i*0.09f, i*0.706f, i, f); }
Color white(float f, float i) { return Color(i, i, i, f); }
Color LerpColor(Color a, Color b, float f) {
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
	cout << "opening image at " << path << endl;
	unsigned char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int imageSize;   // = width*height*3
	unsigned char *data;

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (nearest) ? GL_NEAREST : GL_LINEAR);
	if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap) ? GL_LINEAR_MIPMAP_LINEAR : (nearest)? GL_NEAREST : GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
	cout << "image loaded: " << width << "x" << height << endl;
}

Texture::Texture(HBITMAP bmp, int width, int height) {
	cout << "reading image from hbitmap not implemented" << endl;
	return;
	/*
	unsigned char data = 0;
	if (GetDIBits(dc_mem, hbitmap, 0, 0, nullptr, &bmi, DIB_RGB_COLORS) == 0) {
		cout << "image load from hbitmap failed" << endl;
		return;
	}

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, &data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
	cout << "image loaded from hbitmap: " << width << "x" << height << endl;
	*/
}


//-----------------font class---------------------
Font::Font(const string& path) : Font("", path, -1) {}
Font::Font(const string& paths, const string& pathb, int size) : loaded(false), alignment(ALIGN_TOPLEFT), sizeToggle(size) {
	if (paths != "") {
		cout << "opening font at " << paths << endl;
		unsigned char header[6];
		unsigned int charSize;
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
		int ii = 2;
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
	unsigned int charSize;
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
	int ii = 2;
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
	shader = Engine::unlitProgram;
}

Material::Material(Shader * shad) {
	shader = shad->pointer;
}

void Material::SetSampler(string name, Texture * texture) {
	SetSampler(name, texture, 0);
}
void Material::SetSampler(string name, Texture * texture, int id) {
	GLint imageLoc = glGetUniformLocation(shader, name.c_str());
	glUseProgram(shader);

	glUniform1i(imageLoc, 0);
	glActiveTexture(GL_TEXTURE0 + id);
	glBindTexture(GL_TEXTURE_2D, texture->pointer);
}