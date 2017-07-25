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

using namespace std;

Vec3 Quat2Euler(const Quat& q) {
	Vec3 out;
	double ysqr = q.y * q.y;

	// roll (x-axis rotation)
	double t0 = +2.0 * (q.w * q.x + q.y * q.z);
	double t1 = +1.0 - 2.0 * (q.x * q.x + ysqr);
	out.x = std::atan2(t0, t1);

	// pitch (y-axis rotation)
	double t2 = +2.0 * (q.w * q.y - q.z * q.x);
	t2 = ((t2 > 1.0) ? 1.0 : t2);
	t2 = ((t2 < -1.0) ? -1.0 : t2);
	out.y = std::asin(t2);

	// yaw (z-axis rotation)
	double t3 = +2.0 * (q.w * q.z + q.x * q.y);
	double t4 = +1.0 - 2.0 * (ysqr + q.z * q.z);
	out.z = std::atan2(t3, t4);

	return out;
}

Quat AxisAngle2Quat(const Vec3& axis, float angle) {
	float a = deg2rad*angle;
	double factor = sin(a / 2.0);

	// Calculate the x, y and z of the quaternion
	double x = axis.x * factor;
	double y = axis.y * factor;
	double z = axis.z * factor;

	// Calcualte the w value by cos( theta / 2 )
	double w = cos(a / 2.0);
	return Normalize(Quat(x, y, z, w));
}

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
	stringstream ss;
	ss << "#";
	ss << std::hex << setfill('0') << setw(2) << (int)r;
	ss << std::hex << setfill('0') << setw(2) << (int)g;
	ss << std::hex << setfill('0') << setw(2) << (int)b;
	ss << std::hex << setfill('0') << setw(2) << (int)a;
	return ss.str();
}

void Color::Rgb2Hsv(byte r, byte g, byte b, float& h, float& s, float& v) {
	float R = r * 0.0039216f;
	float G = g * 0.0039216f;
	float B = b * 0.0039216f;

	float mn = min(min(R, G), B);
	float mx = max(max(R, G), B);

	v = 0.5f*(mn + mx);
	if (mn != mx) {
		if (v < 0.5f)
			s = (mx - mn) / (mx + mn);
		else
			s = (mx - mn) / (2.0f - mx - mn);

		if (R == mx) {
			h = (G - B) / (mx - mn);
		}
		else if (G == mx) {
			h = 2.0f + (B - R) / (mx - mn);
		}
		else {
			h = 4.0f + (R - G) / (mx - mn);
		}
		h *= 60;
	}
}

string Color::Col2Hex(Vec4 col) {
	stringstream ss;
	ss << "#";
	ss << std::hex << setfill('0') << setw(2) << (int)(col.r * 255);
	ss << std::hex << setfill('0') << setw(2) << (int)(col.g * 255);
	ss << std::hex << setfill('0') << setw(2) << (int)(col.b * 255);
	ss << std::hex << setfill('0') << setw(2) << (int)(col.a * 255);
	return ss.str();
}

void Color::DrawPicker(float x, float y, Color& c) {
	Engine::DrawQuad(x, y, 270, 350, white(0.8f, 0.1f));
	DrawSV(x + 10, y + 25, 200, 200);
	Engine::DrawQuad(x + 220, y + 25, 15, 200, white());
	
	Engine::Label(x + 10, y + 235, 12, "HEX", Editor::instance->font, white());
	Engine::Label(x + 10, y + 235 + 18, 12, "R", Editor::instance->font, white());
	Engine::Label(x + 10, y + 235 + 35, 12, "G", Editor::instance->font, white());
	Engine::Label(x + 10, y + 235 + 52, 12, "B", Editor::instance->font, white());

	Engine::DrawQuad(x + 40, y + 233, 170, 16, grey2());
	Engine::Label(x + 42, y + 235, 12, c.hex(), Editor::instance->font, white());
	c.r = (byte)Engine::DrawSliderFill(x + 40, y + 235 + 16, 170, 16, 0, 255, c.r, grey2(), red());
	c.g = (byte)Engine::DrawSliderFill(x + 40, y + 235 + 33, 170, 16, 0, 255, c.g, grey2(), green());
	c.b = (byte)Engine::DrawSliderFill(x + 40, y + 235 + 50, 170, 16, 0, 255, c.b, grey2(), blue());

	Engine::DrawQuad(x + 212, y + 235 + 16, 47, 16, grey2());
	Engine::Label(x + 214, y + 235 + 18, 12, to_string(c.r), Editor::instance->font, white());
	Engine::DrawQuad(x + 212, y + 235 + 33, 47, 16, grey2());
	Engine::Label(x + 214, y + 235 + 35, 12, to_string(c.g), Editor::instance->font, white());
	Engine::DrawQuad(x + 212, y + 235 + 50, 47, 16, grey2());
	Engine::Label(x + 214, y + 235 + 52, 12, to_string(c.b), Editor::instance->font, white());

	if (c.useA) {
		Engine::Label(x + 10, y + 235 + 69, 12, "A", Editor::instance->font, white());
		c.a = (byte)Engine::DrawSliderFill(x + 40, y + 235 + 67, 170, 16, 0, 255, c.a, grey2(), white());
		Engine::DrawQuad(x + 212, y + 235 + 67, 47, 16, grey2());
		Engine::Label(x + 214, y + 235 + 69, 12, to_string(c.a), Editor::instance->font, white());
	}
}

void Color::DrawSV(float x, float y, float w, float h) {
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

	GLint baseColLoc = glGetUniformLocation(pickerProgSV, "col");
	glUniform3f(baseColLoc, 1, 0, 0);

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

bool Rect::Inside(Vec2 v) {
	return ((w > 0) ? (v.x > x && v.x < (x + w)) : (v.x >(x + w) && v.x < x)) && ((h > 0) ? (v.y > y && v.y < (y + h)) : (v.y >(y + h) && v.y < y));
}

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
uint Engine::blurProgram = 0;
Font* Engine::defaultFont;
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

	Light::InitShadow();
//#ifdef IS_EDITOR
	Camera::InitShaders();
//#endif
	Font::Init();

	string vertcode = "#version 330 core\nlayout(location = 0) in vec3 pos;\nlayout(location = 1) in vec2 uv;\nout vec2 UV;\nvoid main(){ \ngl_Position.xyz = pos;\ngl_Position.w = 1.0;\nUV = uv;\n}";
	string fragcode = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = textureLod(sampler, UV, 0)*col;\n}"; //out vec3 Vec4;\n
	string fragcode2 = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = vec4(1, 1, 1, textureLod(sampler, UV, 0).r)*col;\n}"; //out vec3 Vec4;\n
	string fragcode3 = "#version 330 core\nin vec2 UV;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = col;\n}"; //out vec3 Vec4;\n
	//string fragcodeSky = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec2 dir;\nuniform float angle;\nout vec4 Vec4;\nvoid main(){\nvec4 col = texture(sampler, UV);\nVec4.rgb = col.rgb*pow(2, col.a*255-128);\nVec4.a = 1;\n}"; //(1.0f / 256.0f) * pow(2, (float)(exponent - 128));
	//string fragcodeSky = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec2 dir;\nuniform float length;\nout vec4 Vec4;\nvoid main(){\nfloat ay = asin((dir.y + UV.y)/length);\nfloat l2 = length*cos(ay);\nfloat ax = asin((dir.x + UV.x)/l2);\nVec4 = texture(sampler, vec2(0.5, 0.5) + vec2(ax, ay));\nVec4.a = 1;\n}";
	string fragcodeSky = "in vec2 UV;uniform sampler2D sampler;uniform vec2 dir;uniform float length;out vec4 color;void main(){float ay = asin((UV.y) / length);float l2 = length*cos(ay);float ax = asin((dir.x + UV.x) / l2);color = textureLod(sampler, vec2((dir.x + ax / 3.14159)*sin(dir.y + ay / 3.14159) + 0.5, (dir.y + ay / 3.14159)), 0);color.a = 1;}";

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
	else {
		Debug::Error("Engine", "Fatal: Cannot init vert shader!");
		abort();
	}
	glDeleteShader(fragment_shader);

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
			abort();
		}
		glDetachShader(unlitProgramA, vertex_shader);
		glDetachShader(unlitProgramA, fragment_shaderA);
	}
	else {
		Debug::Error("Engine", "Fatal: Cannot init frag shader1!");
		abort();
	}
	glDeleteShader(fragment_shaderA);

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
			abort();
		}
		glDetachShader(unlitProgramC, vertex_shader);
		glDetachShader(unlitProgramC, fragment_shaderC);
		//defaultFont = &Font("D:\\ascii 2.font");
	}
	else {
		Debug::Error("Engine", "Fatal: Cannot init frag shader2!");
		abort();
	}
	glDeleteShader(fragment_shaderC);

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
			abort();
		}
		glDetachShader(skyProgram, vertex_shader);
		glDetachShader(skyProgram, fragment_shaderS);
		//defaultFont = &Font("D:\\ascii 2.font");
	}
	else {
		Debug::Error("Engine", "Fatal: Cannot init frag shader3!");
		abort();
	}
	glDeleteShader(fragment_shaderS);

	GLuint fragment_shaderB;
	ifstream strm("D:\\blurPassFrag.txt");
	stringstream ss;
	ss << strm.rdbuf();
	string error = "";
	if (ShaderBase::LoadShader(GL_FRAGMENT_SHADER, ss.str(), fragment_shaderB, &error)) {
		blurProgram = glCreateProgram();
		glAttachShader(blurProgram, vertex_shader);
		glAttachShader(blurProgram, fragment_shaderB);

		link_result = 0;

		glLinkProgram(blurProgram);
		glGetProgramiv(blurProgram, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			int info_log_length = 0;
			glGetProgramiv(blurProgram, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(blurProgram, info_log_length, NULL, &program_log[0]);
			cerr << "Blur shader link error" << endl << &program_log[0] << endl;
			abort();
		}
		glDetachShader(blurProgram, vertex_shader);
		glDetachShader(blurProgram, fragment_shaderB);
		//defaultFont = &Font("D:\\ascii 2.font");
	}
	else {
		Debug::Error("Engine", "Fatal: Cannot init blur shader! " + error);
		abort();
	}
	glDeleteShader(fragment_shaderB);

	glDeleteShader(vertex_shader);

#ifdef IS_EDITOR
	string colorPickerV = "#version 330 core\nlayout(location = 0) in vec3 pos;\nlayout(location = 1) in vec2 uv;\nout vec2 UV;\nvoid main(){ \ngl_Position.xyz = pos;\ngl_Position.w = 1.0;\nUV = uv;\n}";
	string colorPickerF = "#version 330 core\nin vec2 UV;\nuniform vec3 col;\nout vec4 color;void main(){\ncolor = vec4(mix(mix(col, vec3(1, 1, 1), UV.x), vec3(0, 0, 0), 1-UV.y), 1);\n}";

	Color::pickerProgSV = glCreateProgram();
	string err;
	if (ShaderBase::LoadShader(GL_VERTEX_SHADER, colorPickerV, vertex_shader, &err)
		&& ShaderBase::LoadShader(GL_FRAGMENT_SHADER, colorPickerF, fragment_shader, &err)) {
		glAttachShader(Color::pickerProgSV, vertex_shader);
		glAttachShader(Color::pickerProgSV, fragment_shader);


		glLinkProgram(Color::pickerProgSV);
		glGetProgramiv(Color::pickerProgSV, GL_LINK_STATUS, &link_result);
		if (link_result == GL_FALSE)
		{
			glGetProgramiv(Color::pickerProgSV, GL_INFO_LOG_LENGTH, &info_log_length);
			vector<char> program_log(info_log_length);
			glGetProgramInfoLog(Color::pickerProgSV, info_log_length, NULL, &program_log[0]);
			cerr << "ColorPicker SV Shader link error" << endl << &program_log[0] << endl;
			return;
		}
		glDetachShader(Color::pickerProgSV, vertex_shader);
		glDetachShader(Color::pickerProgSV, fragment_shader);
		glDeleteShader(fragment_shader);
		glDeleteShader(vertex_shader);
	}
	else cout << err << endl;
#endif
}

vector<ifstream*> Engine::assetStreams = vector<ifstream*>();
unordered_map<byte, vector<string>> Engine::assetData = unordered_map<byte, vector<string>>();
//unordered_map<string, byte[]> Engine::assetDataLoaded = unordered_map<string, byte[]>();

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

void Engine::Label(float x, float y, float s, string st, Font* font) {
	Label(x, y, s, st, font, Vec4(0, 0, 0, 1));
}
void Engine::Label(float x, float y, float s, string st, Font* font, Vec4 Vec4) {
	Label(x, y, s, st, font, Vec4, -1);
}
void Engine::Label(float x, float y, float s, string st, Font* font, Vec4 Vec4, float maxw) {
	uint sz = st.size();
	GLuint tex = font->glyph(s);
	font->SizeVec(sz);

	if ((font->alignment & 15) > 0) {
		float totalW = 0;
		for (uint i = 0; i < sz * 4; i += 4) {
			char c = st[i / 4];
			totalW = ceil(totalW + font->w2s[c] * s + s*0.1f);
		}
		x -= totalW * (font->alignment & 15) * 0.5f;
	}

	y -= (1-(0.5f * (font->alignment >> 4))) * s;

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
	return Rect(x, y, w, h).Inside(Input::mousePos) ? (MOUSE_HOVER_FLAG | Input::mouse0State) : 0;
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4) {
	return Button(x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f));
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4) {
	return Button(x, y, w, h, normalVec4, LerpVec4(normalVec4, white(), 0.5f), LerpVec4(normalVec4, black(), 0.5f), label, labelSize, labelFont, labelVec4);
}
byte Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4) {
	if (Input::mouse0State != 0 && !Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		DrawQuad(x, y, w, h, normalVec4);
		return 0;
	}
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
	if (Input::mouse0State != 0 && !Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		DrawQuad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, Vec2(uvx, 1 - uvy), Vec2(uvx + uvw, 1 - uvy), Vec2(uvx, 1 - uvy - uvh), Vec2(uvx + uvw, 1 - uvy - uvh), false, normalVec4);
		return 0;
	}
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
	byte b = Button(x, y, w, h, normalVec4, highlightVec4, pressVec4);
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = ALIGN_MIDLEFT;
	Label(round(x + 2), round(y + 0.4f*h), labelSize, label, labelFont, labelVec4);
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
	vector<Vec3> quadPoss = vector<Vec3>();
	vector<uint> quadIndexes = vector<uint>();
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
	vector<Vec3> poss = vector<Vec3>(n);
	vector<uint> ids = vector<uint>(n * 2 - 1);
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
	vector<Vec3> poss = vector<Vec3>(n);
	vector<uint> ids = vector<uint>(dotted? n : n * 2 - 1);
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
	//abort();
}

void Debug::DoDebugObjectTree(vector<SceneObject*> o, int i) {
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
		*stream << s << endl;
#endif
		cout << s << endl;
		DoDebugObjectTree(oo->children, i + 1);
	}
}

void Debug::ObjectTree(vector<SceneObject*> o) {
	Message("ObjectTree", "Start");
	DoDebugObjectTree(o, 0);
	Message("ObjectTree", "End");
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
				else if ((aa.length() > 4 && aa.substr(aa.length() - 4, string::npos) == ".cpp"))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 9 && (aa.substr(aa.length() - 9, string::npos) == ".material")))
					names.push_back(EB_Browser_File(e, folder, aa, aa));
				else if ((aa.length() > 9 && (aa.substr(aa.length() - 7, string::npos) == ".effect")))
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

float repeat(float f, float a, float b) {
	assert(b > a);
	while (f >= b)
		f -= (b-a);
	while (f < a)
		f += (b-a);
	return f;
}

Vec3 rotate(Vec3 v, Quat q) {
	// Extract the vector part of the quaternion
	Vec3 u(q.x, q.y, q.z);
	// Extract the scalar part of the quaternion
	float s = q.w;
	// Do the math
	return (2.0f * dot(u, v) * u + (s*s - dot(u, u)) * v + 2.0f * s * cross(u, v));
}

//-----------------transform class-------------------
Transform::Transform(SceneObject* sc, Vec3 pos, Quat rot, Vec3 scl) : object(sc), _rotation(rot), scale(scl) {
	Translate(pos);
}

const Vec3 Transform::worldPosition() {
	Vec3 w = position;
	SceneObject* o = object;
	while (o->parent != nullptr) {
		o = o->parent;
		w = w*o->transform.scale + o->transform.position;
	}
	return w;
}

const Vec3 Transform::forward() {
	return rotate(Vec3(0, 0, 1), _rotation);
}

const Vec3 Transform::right() {
	return rotate(Vec3(1, 0, 0), _rotation);
}

const Vec3 Transform::up() {
	return rotate(Vec3(0, 1, 0), _rotation);
}

const Quat Transform::rotation() {
	return _rotation;
}

const Quat Transform::worldRotation() {
	return Quat();
}

void Transform::eulerRotation(Vec3 r) {
	_eulerRotation = r;
	_eulerRotation.x = repeat(_eulerRotation.x, 0, 360);
	_eulerRotation.y = repeat(_eulerRotation.y, 0, 360);
	_eulerRotation.z = repeat(_eulerRotation.z, 0, 360);
	_UpdateQuat();
}

void Transform::_UpdateQuat() {
	_rotation = Quat(deg2rad*_eulerRotation);
}

void Transform::_UpdateEuler() {
	_eulerRotation = Quat2Euler(_rotation);
}

void Transform::_UpdateLMatrix() {
	_localMatrix = Mat4x4(scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0, 0, 1);
	_localMatrix = Mat4x4(0, 0, 0, position.x, 0, 0, 0, position.y, 0, 0, 0, position.z, 0, 0, 0, 1) * Quat2Mat(_rotation) * _localMatrix;
	_UpdateWMatrix(object->parent? object->parent->transform._worldMatrix : Mat4x4());
}

void Transform::_UpdateWMatrix(const Mat4x4& mat) {
	_worldMatrix = _localMatrix*mat;

	for (SceneObject* oo : object->children)
		oo->transform._UpdateWMatrix(_worldMatrix);
}

Transform* Transform::Translate(Vec3 v) {
	position += v;
	return this;
}
Transform* Transform::Rotate(Vec3 v, TransformSpace sp) {
	if (sp == Space_Self) {
		Quat qx = AxisAngle2Quat(Vec3(1, 0, 0), v.x);
		Quat qy = AxisAngle2Quat(Vec3(0, 1, 0), v.y);
		Quat qz = AxisAngle2Quat(Vec3(0, 0, 1), v.z);
		_rotation = qz*qy*qx*_rotation;
	}
	else {

	}
	_UpdateEuler();
	return this;
}
Transform* Transform::Scale(Vec3 v) {
	scale += v;
	return this;
}

//-----------------rendertexture class---------------
RenderTexture::RenderTexture(uint w, uint h, bool depth) : Texture() {
	width = w;
	height = h;
	_texType = TEX_TYPE_RENDERTEXTURE;

	glGenFramebuffers(1, &d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);

	glGenTextures(1, &pointer);
	glGenTextures(1, &d_depthTex);

	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pointer, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (depth) {
		glBindTexture(GL_TEXTURE_2D, d_depthTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, d_depthTex, 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else d_depthTex = 0;

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, DrawBuffers);
	loaded = true;
}

RenderTexture::~RenderTexture() {
	if (d_depthTex > 0) glDeleteTextures(1, &d_depthTex);
	glDeleteFramebuffers(1, &d_fbo);
}

void RenderTexture::Blit(Texture* src, RenderTexture* dst, Material* mat, string texName) {
	mat->SetTexture(texName, src);
	glViewport(0, 0, dst->width, dst->height);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->d_fbo);
	float zero[] = { 0,0,0,0 };
	glClearBufferfv(GL_COLOR, 0, zero);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glDisable(GL_BLEND);

	Vec3 quadPoss[4];
	quadPoss[1].x = 1;
	quadPoss[2].y = 1;
	quadPoss[3].x = 1;
	quadPoss[3].y = 1;
	uint quadIndexes[4] = { 0, 1, 3, 2 };
	mat->ApplyGL(Mat4x4(), Mat4x4());
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &quadPoss[0]);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, &quadPoss[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &quadIndexes[0]);

	glDisableVertexAttribArray(0);
	glUseProgram(0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void RenderTexture::Load(string path) {
	throw runtime_error("RT Load (s) not implemented");
}
void RenderTexture::Load(ifstream& strm) {
	throw runtime_error("RT Load (i) not implemented");
}

bool RenderTexture::Parse(string path) {
	string ss(path + ".meta");
	ofstream str(ss, ios::out | ios::trunc | ios::binary);
	str << "IMR";
	return true;
}

//-----------------texture class---------------------
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
void InvertPNG(vector<byte>& data, uint x, uint y) {
	for (uint a = 0; a <= y*0.5f; a++) {
		for (uint b = 0; b < x; b++) {
			for (uint c = 0; c < 4; c++) {
				byte t = data[(a*x + b)*4 + c];
				data[(a*x + b)*4+c] = data[((y - a - 1)*x + b)*4 + c];
				data[((y - a - 1)*x + b)*4+c] = t;
			}
		}
	}
}

bool LoadPNG(string fileN, uint &x, uint &y, byte& channels, vector<byte>& data) {
	channels = 4;
	uint err = lodepng::decode(data, x, y, fileN.c_str());
	if (err){
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

	ifstream strm(fileN.c_str(), ios::in | ios::binary);

	if (!strm.is_open()){
		printf("Image could not be opened\n");
		return false;
	}
	strm.read(header, 54);
	if (strm.bad()){ // If not 54 bytes read : problem
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
	strm.read(*(char**)data, imageSize);
	return true;
}

Texture::Texture(const string& path) : Texture(path, true, TEX_FILTER_TRILINEAR, 5) {}
Texture::Texture(const string& path, bool mipmap) : Texture(path, mipmap, mipmap? TEX_FILTER_TRILINEAR : TEX_FILTER_BILINEAR, 5) {}
Texture::Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso) : AssetObject(ASSETTYPE_TEXTURE), _mipmap(mipmap), _filter(filter), _aniso(aniso) {
	string sss = path.substr(path.find_last_of('.'), string::npos);
	byte *data;
	vector<byte> dataV;
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
	//*
	else if (sss == ".png") {
		if (!LoadPNG(path, width, height, chn, dataV)) {
			cout << "load png failed! " << path << endl;
			return;
		}
		data = &dataV[0];
	}
	//*/
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
	if (dataV.size() == 0) delete[](data);
	loaded = true;
	//cout << "image loaded: " << width << "x" << height << endl;
}

Texture::Texture(int i, Editor* e) : AssetObject(ASSETTYPE_TEXTURE) {
	string p = e->projectFolder + "Assets\\" + e->normalAssets[ASSETTYPE_TEXTURE][i] + ".meta";
	ifstream strm(p, ios::in | ios::binary);
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

Texture::Texture(ifstream& strm, uint offset) : AssetObject(ASSETTYPE_TEXTURE) {
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

TEX_TYPE Texture::_ReadStrm(Texture* tex, ifstream& strm, byte& chn, GLenum& rgb, GLenum& rgba) {
	vector<char> hd(4);
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
	vector<byte> dataV;
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
	else if (sss == ".png") {
		if (!LoadPNG(path, width, height, chn, dataV)) {
			cout << "load png failed! " << path << endl;
			return false;
		}
		data = &dataV[0];
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
	iStrm.close();
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
	if (dataV.size() == 0) delete[](data);
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

bool Texture::DrawPreview(uint x, uint y, uint w, uint h) {
	Engine::DrawTexture(x, y, w, h, this, DrawTex_Fit);
	return true;
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
	auto data = hdr2float(data2, width, height);
	delete[](data2);

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &data[0]);

	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;
	while (mips < 6 && height > 16) {
		//cout << "Downsampling " << mips << endl;
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
	//cout << "HDR Image loaded: " << width << "x" << height << endl;
}

//do not generate mipmaps in editor < fuck you
Background::Background(int i, Editor* editor) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
	string path = editor->projectFolder + "Assets\\" + editor->normalAssets[ASSETTYPE_HDRI][i] + ".meta";
	ifstream strm(path.c_str(), ios::in | ios::binary);
	vector<char> hd(6);
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

	byte* data2 = new byte[width*height*4];
	strm.read((char*)data2, width*height*4);
	auto data = hdr2float(data2, width, height);
	delete[](data2);

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &data[0]);
	
	uint width_1 = width, height_1 = height, width_2, height_2, mips = 0;
	while (mips < 6 && height > 16) {
		//cout << "Downsampling " << mips << endl;
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
	//cout << "HDR Image loaded: " << width << "x" << height << endl;
}

Background::Background(ifstream& strm, uint offset) : width(0), height(0), AssetObject(ASSETTYPE_HDRI) {
	vector<char> hd(6);
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
		cout << "Downsampling " << mips << endl;
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
	//cout << "HDR Image loaded: " << width << "x" << height << endl;
}

/*
vector<float> Background::Downsample(vector<float>& data, uint w, uint h, uint& w2, uint& h2) {
	glViewport(0, 0, w*0.5f, h*0.5f);
	GLuint fbo, tex, inTex, outTex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, inTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, &data[0]);
	glGenTextures(1, &outTex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w*0.5f, h*0.5f, 0, GL_RGB, GL_FLOAT, NULL);
	glGenTextures(1, &inTex);
	glBindTexture(GL_TEXTURE_2D, outTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w*0.5f, h*0.5f, 0, GL_RGB, GL_FLOAT, NULL);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

	glBindTexture(GL_TEXTURE_2D, tex);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, outTex);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, DrawBuffers);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Background", "FB error:" + Status);
		abort();
	}

	GLint scrSzLoc = glGetUniformLocation(Engine::blurProgram, "screenSize");
	GLint inTexLoc = glGetUniformLocation(Engine::blurProgram, "tex");
	GLint isYLoc = glGetUniformLocation(Engine::blurProgram, "isY");

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, Camera::screenRectVerts);
	glUseProgram(Engine::blurProgram);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, Camera::screenRectVerts);

	glUniform2f(scrSzLoc, w*0.5f, h*0.5f);

	//blur x
	glUniform1i(inTexLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inTex);
	glUniform1f(isYLoc, false);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::screenRectIndices);

	//blur y
	glUniform1i(inTexLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1f(isYLoc, true);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::screenRectIndices);

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	glDisableClientState(GL_VERTEX_ARRAY);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glViewport(0, 0, Display::width, Display::height);
}
*/
//*
vector<float> Background::Downsample(vector<float>& data, uint w, uint h, uint& w2, uint& h2) {
	if (w % 2 != 0) w--;
	if (h % 2 != 0) h--;
	w2 = w / 2;
	h2 = h / 2;

	//half the size
	vector<float> hImg(w2*h2*3);
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
	vector<float> xImg(w2*h2*3);
	for (uint x = 0; x < w2; x++) {
		for (uint y = 0; y < h2; y++) {
			for (uint a = 0; a < 21; a++) {
				int xx = x + (a-10);
				if (xx < 0) xx = w2 + xx;
				else if ((uint)xx >= w2) xx -= w2;
				xImg[x * 3 + y * 3 * w2] += (hImg[xx * 3 + y * 3 * w2] * kernal[a]);
				xImg[x * 3 + y * 3 * w2 + 1] += (hImg[xx * 3 + y * 3 * w2 + 1] * kernal[a]);
				xImg[x * 3 + y * 3 * w2 + 2] += (hImg[xx * 3 + y * 3 * w2 + 2] * kernal[a]);
			}
		}
	}

	//blur 20 pixels y
	vector<float> oImg(w2*h2 * 3);
	for (uint x = 0; x < w2; x++) {
		for (uint y = 0; y < h2; y++) {
			for (uint a = 0; a < 21; a++) {
				int yy = y + (a - 10);
				int xx = w2-x-1;
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

CubeMap::CubeMap(ushort size, bool mips, GLenum type, byte dataSize, GLenum format, GLenum dataType) : size(size), AssetObject(ASSETTYPE_HDRI), loaded(false) {
	if (size != 64 && size != 128 && size != 256 && size != 512 && size != 1024 && size != 2048) {
		Debug::Error("Cubemap", "CubeMaps must be sized POT between 64 and 2048! (" + to_string(size) + ")");
		abort();
	}
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pointer);
	//vector<byte> data = vector<byte>(size*size*dataSize, 0);
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
FT_Library Font::_ftlib = nullptr;
GLuint Font::fontProgram = 0;
void Font::Init() {
	int err = FT_Init_FreeType(&_ftlib);
	if (err != FT_Err_Ok) {
		Debug::Error("Font", "Fatal: Initializing freetype failed!");
		runtime_error("Fatal: Initializing freetype failed!");
	}

	string error;
	GLuint vs, fs;
	string frag = "#version 330 core\nin vec2 UV;\nuniform sampler2D sampler;\nuniform vec4 col;\nout vec4 color;void main(){\ncolor = vec4(1, 1, 1, texture(sampler, UV).r)*col;\n}";
	ifstream strm("D:\\fontVert.txt");
	stringstream vert;
	vert << strm.rdbuf();
	if (!ShaderBase::LoadShader(GL_VERTEX_SHADER, vert.str(), vs, &error)) {
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
		vector<char> program_log(info_log_length);
		glGetProgramInfoLog(fontProgram, info_log_length, NULL, &program_log[0]);
		cerr << "Font shader link error" << endl << &program_log[0] << endl;
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
Vec2 Input::mouseDownPos = Vec2(0, 0);

//-------------------Material class--------------
Material::Material() : _shader(-1), AssetObject(ASSETTYPE_MATERIAL) {
	shader = nullptr;// Engine::unlitProgram;
}

Material::Material(ShaderBase * shad) : _shader(-1), AssetObject(ASSETTYPE_MATERIAL) {
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
		else if (v->type == SHADER_SAMPLER)
			l = new MatVal_Tex();
		valNames[v->type].push_back(v->name);
		GLint ii = glGetUniformLocation(shader->pointer, v->name.c_str());
		if (ii > -1) {
			vals[v->type].emplace(ii, l);
			valOrders.push_back(v->type);
			valOrderIds.push_back((byte)(valNames[v->type].size()-1));
			valOrderGLIds.push_back(ii);
		}
		else
			Editor::instance->_Warning("Material", "Shader parameter " + v->name + " not used!");
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
	if (ss != "KTC") {
		cerr << "file not supported" << endl;
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
		if (loc > -1) {
			vals[v->type].emplace(loc, l);
			nMap.emplace(v->name, loc);
			valOrders.push_back(v->type);
			valOrderIds.push_back((byte)(valNames[v->type].size()-1));
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
	stream.close();
}

Material::Material(ifstream& stream, uint offset) : AssetObject(ASSETTYPE_MATERIAL) {
	if (stream.is_open()) {
		stream.seekg(offset);
		char* c = new char[4];
		stream.read(c, 3);
		c[3] = (char)0;
		string ss(c);
		if (ss != "KTC") {
			cerr << "file not supported" << endl;
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
		unordered_map<string, GLint> nMap;
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
	valOrders = vector<SHADER_VARTYPE>();
}

void Material::SetTexture(string name, Texture * texture) {
	SetTexture(glGetUniformLocation(shader->pointer, name.c_str()), texture);
}
void Material::SetTexture(GLint id, Texture * texture) {
	vals[SHADER_SAMPLER][id] = texture;
}

GLuint Material::defTex_Black = 0;
GLuint Material::defTex_Grey = 0;
GLuint Material::defTex_White = 0;
GLuint Material::defTex_Red = 0;
GLuint Material::defTex_Green = 0;
GLuint Material::defTex_Blue = 0;

void Material::LoadOris() {
	vector<byte> data(3);

	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	glGenTextures(1, &defTex_Black);
	glBindTexture(GL_TEXTURE_2D, defTex_Black);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);
	data[0] = 0x80;
	data[1] = 0x80;
	data[2] = 0x80;
	glGenTextures(1, &defTex_Grey);
	glBindTexture(GL_TEXTURE_2D, defTex_Grey);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);
	data[0] = 0xFF;
	data[1] = 0xFF;
	data[2] = 0xFF;
	glGenTextures(1, &defTex_White);
	glBindTexture(GL_TEXTURE_2D, defTex_White);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);
	data[0] = 0xFF;
	data[1] = 0;
	data[2] = 0;
	glGenTextures(1, &defTex_Red);
	glBindTexture(GL_TEXTURE_2D, defTex_Red);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);
	data[0] = 0;
	data[1] = 0xFF;
	data[2] = 0;
	glGenTextures(1, &defTex_Green);
	glBindTexture(GL_TEXTURE_2D, defTex_Green);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);
	data[0] = 0;
	data[1] = 0;
	data[2] = 0xFF;
	glGenTextures(1, &defTex_Blue);
	glBindTexture(GL_TEXTURE_2D, defTex_Blue);
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
	ofstream strm(path.c_str(), ios::out | ios::binary | ios::trunc);
	strm << "KTC";
	ASSETTYPE st;
	ASSETID si = Editor::instance->GetAssetId(shader, st);
	_StreamWriteAsset(Editor::instance, &strm, ASSETTYPE_SHADER, si);
	int i = 0, j = 0;
	SHADER_VARTYPE t = 0;
	long long p1 = strm.tellp();
	strm << "0000";
	//_StreamWrite(&i, &strm, 4);
	for (auto& v : vals[SHADER_INT]) {
		t = SHADER_INT;
		strm << t;
		strm << valNames[SHADER_INT][j] << (char)0;
		int ii(*(int*)v.second);
		_StreamWrite(&ii, &strm, 4);
		i++;
		j++;
	}
	j = 0;
	for (auto& v : vals[SHADER_FLOAT]) {
		t = SHADER_FLOAT;
		strm << t;
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

void Material::ApplyGL(Mat4x4& _mv, Mat4x4& _p) {
	if (shader == nullptr) {
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
			if (tx->tex == nullptr)
				continue;
			glActiveTexture(GL_TEXTURE0 + ti);
			glBindTexture(GL_TEXTURE_2D, tx->tex->pointer);
			glUniform1i(a.first, ti);
		}

	}
}

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
	(*stream) << p << char0;
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

vector<float> hdr2float(byte imagergbe[], int w, int h) {
	vector<float> image(w * h * 3 * sizeof(float));
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
	c = stream.get();
	while (c != 'o') {
		Debug::Message("Object Deserializer", to_string(c) + " " + to_string(stream.tellg()));
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
				obj->AddComponent(SceneScriptResolver::instance->Resolve(stream));
#endif
				break;
			default:
				cout << "unknown component " << (int)c << "!" << endl;
				char cc;
				cc = stream.get();
				while (cc != 'c')
					stream >> cc;
				long long ss = stream.tellg();
				stream.seekg(ss - 1);
				break;
			}
			Debug::Message("Object Deserializer", "2 " + to_string(stream.tellg()));
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
	settings.sky = _GetCache<Background>(t, settings.skyId);

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
	if (o->parent == nullptr){
		auto it = std::find(active->objects.begin(), active->objects.end(), o);
		if (it != active->objects.end()) {
			swap(*it, active->objects.back());
			active->objects.pop_back();
		}
	}
	else {
		SceneObject* p = o->parent;
		p->RemoveChild(o);
	}

	delete(o);
}

shared_ptr<Scene> Scene::active = nullptr;
ifstream* Scene::strm = nullptr;
vector<string> Scene::sceneNames = {};
vector<long> Scene::scenePoss = {};

void Scene::Load(string name) {
	for (uint a = sceneNames.size(); a > 0; a--) {
		if (sceneNames[a-1] == name) {
			Load(a-1);
			return;
		}
	}
}

void Scene::Load(uint i) {
	if (i >= scenePoss.size()) {
		Debug::Error("Scene Loader", "Scene ID (" + to_string(i) + ") out of range!");
		return;
	}
	Debug::Message("Scene Loader", "Loading scene " + to_string(i) + "...");
	active = make_shared<Scene>(*strm, scenePoss[i]);
	active->sceneId = i;
	Debug::Message("Scene Loader", "Loaded scene " + to_string(i) + "(" + sceneNames[i] + ")");
}

void Scene::Save(Editor* e) {
	ofstream sw(e->projectFolder + "Assets\\" + sceneName + ".scene", ios::out);
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
		abort();
	}

	byte numDat = 0, id;
	ushort num;
	ASSETTYPE type;
	uint pos;
	char* c = new char[100];
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
		cout << (int)type << " " << (int)id << " " << pos << ": " << string(c) << endl;
		numDat = max(numDat, id);
		dataLocs[type].push_back(pair<byte, uint>(id - 1, pos));
		dataCaches[type].push_back(nullptr);
		names[type].push_back(c);
	}

	for (int a = 0; a < numDat; a++) {
		string pp = dpath + to_string(a+1);
		streams.push_back(new ifstream(pp.c_str(), ios::in | ios::binary));
		if (streams[a]->is_open())
			Debug::Message("AssetManager", "Streaming data" + to_string(a+1));
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
	long long ppos = strm->tellg();
	Scene::ReadD0();
}

void* AssetManager::CacheFromName(string nm) {
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

void* AssetManager::GetCache(ASSETTYPE t, ASSETID i) {
	if (i == -1)
		return nullptr;
	if (dataCaches[t][i] != nullptr)
		return dataCaches[t][i];
	else
		return GenCache(t, i);
}
void* AssetManager::GenCache(ASSETTYPE t, ASSETID i) {
	ifstream* strm = streams[dataLocs[t][i].first];
	uint off = dataLocs[t][i].second;
	strm->seekg(off);
	uint sz;
	_Strm2Val(*strm, sz);
	off += 4;
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
		Debug::Warning("AssetManager", "No operation suits asset type " + to_string(t) + "!");
		return nullptr;
	}
	return dataCaches[t][i];
}