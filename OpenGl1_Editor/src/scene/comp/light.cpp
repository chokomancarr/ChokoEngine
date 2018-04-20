#include "Engine.h"
#include "Editor.h"

void Light::_SetCookie(void* v) {
	Light* l = (Light*)v;
	l->cookie = _GetCache<Texture>(ASSETTYPE_TEXTURE, l->_cookie);
}
void Light::_SetHsvMap(void* v) {
	Light* l = (Light*)v;
	l->hsvMap = _GetCache<Texture>(ASSETTYPE_TEXTURE, l->_hsvMap);
}

void Light::CalcShadowMatrix() {
	MVP::Switch(true);
	MVP::Clear();
	if (_lightType == LIGHTTYPE_SPOT || _lightType == LIGHTTYPE_POINT) {
		Quat q = glm::inverse(object->transform.rotation());
		MVP::Mul(glm::perspectiveFov(angle * deg2rad, 1024.0f, 1024.0f, minDist, maxDist));
		MVP::Scale(1, 1, -1);
		MVP::Mul(QuatFunc::ToMatrix(q));
		Vec3 pos = -object->transform.position();
		MVP::Translate(pos.x, pos.y, pos.z);
	}
	//else
	//_shadowMatrix = Mat4x4();
}

Light::Light() : Component("Light", COMP_LHT, DRAWORDER_LIGHT), _lightType(LIGHTTYPE_POINT), color(white()), cookie(0), hsvMap(0) {}

Light::Light(std::ifstream& stream, SceneObject* o, long pos) : Light() {
	if (pos >= 0)
		stream.seekg(pos);

	_Strm2Val(stream, _lightType);
	drawShadow = (_lightType & 0xf0) != 0;
	_lightType = LIGHTTYPE(_lightType & 0x0f);
	_Strm2Val(stream, intensity);
	_Strm2Val(stream, minDist);
	_Strm2Val(stream, maxDist);
	_Strm2Val(stream, angle);
	_Strm2Val(stream, shadowBias);
	_Strm2Val(stream, shadowStrength);
	_Strm2Val(stream, color.r);
	_Strm2Val(stream, color.g);
	_Strm2Val(stream, color.b);
	_Strm2Val(stream, color.a);
	_cookie = -1;
}

#ifdef IS_EDITOR

void Light::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	if (ebv->editor->selected != object) return;
	switch (_lightType) {
	case LIGHTTYPE_POINT:
		if (minDist > 0) {
			Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 1, 0), minDist, 32, color, 1);
			Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 0, 1), minDist, 32, color, 1);
			Engine::DrawCircleW(Vec3(), Vec3(0, 1, 0), Vec3(0, 0, 1), minDist, 32, color, 1);
		}
		Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 1, 0), maxDist, 32, Lerp(color, black(), 0.5f), 1);
		Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 0, 1), maxDist, 32, Lerp(color, black(), 0.5f), 1);
		Engine::DrawCircleW(Vec3(), Vec3(0, 1, 0), Vec3(0, 0, 1), maxDist, 32, Lerp(color, black(), 0.5f), 1);
		break;
	case LIGHTTYPE_DIRECTIONAL:
		Engine::DrawLineW(Vec3(0, 0, 0.2f), Vec3(0, 0, 1), color, 1);
		Engine::DrawLineW(Vec3(0, 0, 1), Vec3(0, 0.2f, 0.7f), color, 1);
		Engine::DrawLineW(Vec3(0, 0, 1), Vec3(0, -0.2f, 0.7f), color, 1);
		Engine::DrawLineW(Vec3(0, 0, 1), Vec3(0.2f, 0, 0.7f), color, 1);
		Engine::DrawLineW(Vec3(0, 0, 1), Vec3(-0.2f, 0, 0.7f), color, 1);
		/*
		Engine::DrawLineW(Vec3(0, 0, -0.3f), Vec3(0, 0, -0.5f), color, 1);
		Engine::DrawLineW(Vec3(0, -0.3f, 0), Vec3(0, -0.5f, 0), color, 1);
		Engine::DrawLineW(Vec3(0, 0.3f, 0), Vec3(0, 0.5f, 0), color, 1);
		Engine::DrawLineW(Vec3(-0.3f, 0, 0), Vec3(-0.5f, 0, 0), color, 1);
		Engine::DrawLineW(Vec3(0.3f, 0, 0), Vec3(0.5f, 0, 0), color, 1);
		*/
		Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 1, 0), 0.2f, 12, color, 1);
		Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 0, 1), 0.2f, 12, color, 1);
		Engine::DrawCircleW(Vec3(), Vec3(0, 1, 0), Vec3(0, 0, 1), 0.2f, 12, color, 1);
		break;
	case LIGHTTYPE_SPOT:
		if (ebv->editor->selected == object) {
			Engine::DrawLineWDotted(Vec3(0, 0, 1) * minDist, Vec3(0, 0, 1) * maxDist, white(0.5f, 0.5f), 1, 0.2f, true);
			float rd = minDist*tan(deg2rad*angle*0.5f);
			float rd2 = maxDist*tan(deg2rad*angle*0.5f);
			if (minDist > 0)
				Engine::DrawCircleW(Vec3(0, 0, 1) * minDist, Vec3(1, 0, 0), Vec3(0, 1, 0), rd, 16, color, 1);
			Engine::DrawCircleW(Vec3(0, 0, 1) * maxDist, Vec3(1, 0, 0), Vec3(0, 1, 0), rd2, 32, color, 1);
			Engine::DrawLineW(Vec3(rd, 0, minDist), Vec3(rd2, 0, maxDist), color, 1);
			Engine::DrawLineW(Vec3(-rd, 0, minDist), Vec3(-rd2, 0, maxDist), color, 1);
			Engine::DrawLineW(Vec3(0, rd, minDist), Vec3(0, rd2, maxDist), color, 1);
			Engine::DrawLineW(Vec3(0, -rd, minDist), Vec3(0, -rd2, maxDist), color, 1);
		}
		break;
	}
}

void Light::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		if (Engine::EButton(e->editorLayer == 0, v.r, v.g + pos, v.b * 0.33f - 1, 16, (_lightType == LIGHTTYPE_POINT) ? white(1, 0.5f) : grey1(), "Point", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_POINT;
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.33f, v.g + pos, v.b * 0.34f - 1, 16, (_lightType == LIGHTTYPE_DIRECTIONAL) ? white(1, 0.5f) : grey1(), "Directional", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_DIRECTIONAL;
		if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.67f, v.g + pos, v.b * 0.33f - 1, 16, (_lightType == LIGHTTYPE_SPOT) ? white(1, 0.5f) : grey1(), "Spot", 12, e->font, white()) == MOUSE_RELEASE)
			_lightType = LIGHTTYPE_SPOT;

		UI::Label(v.r + 2, v.g + pos + 17, 12, "Intensity", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, grey1());
		UI::Label(v.r + v.b * 0.3f + 2, v.g + pos + 17, 12, std::to_string(intensity), e->font, white());
		intensity = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f - 1, 16, 0, 20, intensity, grey1(), white());
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "Color", e->font, white());
		e->DrawColorSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), 12, e->font, &color);
		pos += 17;

		switch (_lightType) {
		case LIGHTTYPE_POINT:
			UI::Label(v.r + 2, v.g + pos, 12, "core radius", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(minDist), e->font, white());
			minDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, maxDist, minDist, grey1(), white());
			pos += 17;
			UI::Label(v.r + 2, v.g + pos, 12, "distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(maxDist), e->font, white());
			maxDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 20, maxDist, grey1(), white());
			pos += 17;
			UI::Label(v.r + 2, v.g + pos, 12, "falloff", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(falloff), e->font, white());
			falloff = (LIGHT_FALLOFF)((int)round(Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 20, (float)falloff, grey1(), white())));
			pos += 17;
			break;
		case LIGHTTYPE_DIRECTIONAL:

			break;
		case LIGHTTYPE_SPOT:
			UI::Label(v.r + 2, v.g + pos, 12, "angle", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, std::to_string(angle), e->font, white());
			angle = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 180, angle, grey1(), white());
			pos += 17;
			UI::Label(v.r + 2, v.g + pos, 12, "start distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(minDist), e->font, white());
			minDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0.0001f, maxDist, minDist, grey1(), white());
			pos += 17;
			UI::Label(v.r + 2, v.g + pos, 12, "end distance", e->font, white());
			Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
			UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(maxDist), e->font, white());
			maxDist = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0.0002f, 50, maxDist, grey1(), white());
			pos += 17;
			break;
		}
		UI::Label(v.r + 2, v.g + pos, 12, "cookie", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_cookie, &_SetCookie, this);
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "cookie strength", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
		UI::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, std::to_string(cookieStrength), e->font, white());
		cookieStrength = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, cookieStrength, grey1(), white());
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "square shape", e->font, white());
		square = Engine::Toggle(v.r + v.b * 0.3f, v.g + pos, 12, e->tex_checkbox, square, white(), ORIENT_HORIZONTAL);
		pos += 17;
		if (_lightType != LIGHTTYPE_DIRECTIONAL) {
			if (Engine::EButton(e->editorLayer == 0, v.r, v.g + pos, v.b * 0.5f - 1, 16, (!drawShadow) ? white(1, 0.5f) : grey1(), "No Shadow", 12, e->font, white()) == MOUSE_RELEASE)
				drawShadow = false;
			if (Engine::EButton(e->editorLayer == 0, v.r + v.b * 0.5f, v.g + pos, v.b * 0.5f - 1, 16, (drawShadow) ? white(1, 0.5f) : grey1(), "Shadow", 12, e->font, white()) == MOUSE_RELEASE)
				drawShadow = true;
			pos += 17;
			if (drawShadow) {
				UI::Label(v.r + 2, v.g + pos, 12, "shadow bias", e->font, white());
				Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
				UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(shadowBias), e->font, white());
				shadowBias = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 0.02f, shadowBias, grey1(), white());
				pos += 17;
				UI::Label(v.r + 2, v.g + pos, 12, "shadow strength", e->font, white());
				Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
				UI::Label(v.r + v.b * 0.3f + 2, v.g + pos, 12, std::to_string(shadowStrength), e->font, white());
				shadowStrength = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, shadowStrength, grey1(), white());
				pos += 17;

				UI::Label(v.r + 2, v.g + pos, 12, "Contact Shadows", e->font, white());
				contactShadows = Engine::Toggle(v.r + v.b * 0.3f, v.g + pos, 16, e->tex_checkbox, contactShadows, white(), ORIENT_HORIZONTAL);
				pos += 17;
				if (contactShadows) {
					UI::Label(v.r + 2, v.g + pos, 12, "  samples", e->font, white());
					Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
					UI::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, std::to_string(contactShadowSamples), e->font, white());
					contactShadowSamples = (uint)Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 5, 50, (float)contactShadowSamples, grey1(), white());
					pos += 17;
					UI::Label(v.r + 2, v.g + pos, 12, "  distance", e->font, white());
					Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos, v.b * 0.3f - 1, 16, grey1());
					UI::Label(v.r + v.b * 0.3f, v.g + pos + 2, 12, std::to_string(contactShadowDistance), e->font, white());
					contactShadowDistance = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos, v.b * 0.4f - 1, 16, 0, 1, contactShadowDistance, grey1(), white());
					pos += 17;
				}
			}
		}
		UI::Label(v.r + 2, v.g + pos, 12, "HSV Map", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_hsvMap, &_SetHsvMap, this);

	}
	else pos += 17;
}

void Light::Serialize(Editor* e, std::ofstream* stream) {
	byte b = _lightType;
	if (drawShadow) b |= 0xf0;
	_StreamWrite(&b, stream, 1);
	_StreamWrite(&intensity, stream, 4);
	_StreamWrite(&minDist, stream, 4);
	_StreamWrite(&maxDist, stream, 4);
	_StreamWrite(&angle, stream, 4);
	_StreamWrite(&shadowBias, stream, 4);
	_StreamWrite(&shadowStrength, stream, 4);
	_StreamWrite(&color.r, stream, 4);
	_StreamWrite(&color.g, stream, 4);
	_StreamWrite(&color.b, stream, 4);
	_StreamWrite(&color.a, stream, 4);
}

#endif