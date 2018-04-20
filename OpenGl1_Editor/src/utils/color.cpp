#include "Engine.h"
#include "Editor.h"
#include <iomanip>

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
	Vec4 c(Lerp(Lerp(cb, Vec4(1, 1, 1, 1), 1 - s), Vec4(), 1 - v));
	r = (byte)round(c.r * 255);
	g = (byte)round(c.g * 255);
	b = (byte)round(c.b * 255);
}

Vec3 Color::Rgb2Hsv(Vec4 col) {
	Vec3 o = Vec3();
	Rgb2Hsv((byte)col.r, (byte)col.g, (byte)col.b, o.r, o.g, o.b);
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
#ifdef IS_EDITOR
	Engine::DrawQuad(x, y, 270.0f, c.useA ? 335.0f : 318.0f, white(0.8f, 0.1f));
	Vec2 v2 = Engine::DrawSliderFill2D(x + 10, y + 10, 220, 220, Vec2(), Vec2(1, 1), Vec2(1 - c.s, c.v), black(), grey1());
	c.s = 1 - v2.x;
	c.v = v2.y;
	DrawSV(x + 10, y + 10, 220, 220, c.h);
	Engine::DrawCircle(Vec2(x + 10 + 220 * (1 - c.s), y + 10 + 220 * (1 - c.v)), 8, 24, black(), 4);
	Engine::DrawCircle(Vec2(x + 10 + 220 * (1 - c.s), y + 10 + 220 * (1 - c.v)), 8, 24, white(), 2);
	//Engine::DrawQuad(x + 244, y + 10, 15, 220, white());
	c.h = Engine::DrawSliderFillY(x + 244, y + 10, 15, 220, 0, 1, c.h, black(), white());
	DrawH(x + 244, y + 10, 15, 220);
	Engine::DrawLine(Vec3(x + 242, y + 10 + 220 * c.h, 0), Vec3(x + 261, y + 10 + 220 * c.h, 0), white(), 2);

	c.RecalcRGB();

	y += 242;

	UI::Label(x + 10, y, 12, "HEX", Editor::instance->font, white());
	UI::Label(x + 10, y + 18, 12, "R", Editor::instance->font, white());
	UI::Label(x + 10, y + 35, 12, "G", Editor::instance->font, white());
	UI::Label(x + 10, y + 52, 12, "B", Editor::instance->font, white());

	Engine::DrawQuad(x + 40, y - 2, 170, 16, grey2());
	UI::Label(x + 42, y, 12, c.hex(), Editor::instance->font, white());
	c.r = (byte)round(Engine::DrawSliderFill(x + 40, y + 16, 170, 16, 0, 255, c.r, grey2(), red()));
	c.g = (byte)round(Engine::DrawSliderFill(x + 40, y + 33, 170, 16, 0, 255, c.g, grey2(), green()));
	c.b = (byte)round(Engine::DrawSliderFill(x + 40, y + 50, 170, 16, 0, 255, c.b, grey2(), blue()));

	Engine::DrawQuad(x + 212, y + 16, 47, 16, grey2());
	UI::Label(x + 214, y + 18, 12, std::to_string(c.r), Editor::instance->font, white());
	Engine::DrawQuad(x + 212, y + 33, 47, 16, grey2());
	UI::Label(x + 214, y + 35, 12, std::to_string(c.g), Editor::instance->font, white());
	Engine::DrawQuad(x + 212, y + 50, 47, 16, grey2());
	UI::Label(x + 214, y + 52, 12, std::to_string(c.b), Editor::instance->font, white());

	if (c.useA) {
		UI::Label(x + 10, y + 69, 12, "A", Editor::instance->font, white());
		c.a = (byte)Engine::DrawSliderFill(x + 40, y + 67, 170, 16, 0, 255, c.a, grey2(), white());
		Engine::DrawQuad(x + 212, y + 67, 47, 16, grey2());
		UI::Label(x + 214, y + 69, 12, std::to_string(c.a), Editor::instance->font, white());
	}

	c.RecalcHSV();
	//std::cout << std::to_string(c.vec4()) << std::endl << std::to_string(c.hsv()) << std::endl;
#endif
}

Vec4 Color::HueBaseCol(float hue) {
	hue *= 6;
	Vec4 v;
	v.r = Clamp(abs(hue - 3) - 1.0f, 0.0f, 1.0f);
	v.g = 1 - Clamp(abs(hue - 2) - 1.0f, 0.0f, 1.0f);
	v.b = 1 - Clamp(abs(hue - 4) - 1.0f, 0.0f, 1.0f);
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
#ifdef IS_EDITOR
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
#endif
}

void Color::DrawH(float x, float y, float w, float h) {
#ifdef IS_EDITOR
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
#endif
}