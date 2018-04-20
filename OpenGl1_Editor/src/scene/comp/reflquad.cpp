#include "Engine.h"
#include "Editor.h"

std::vector<GLint> ReflectiveQuad::paramLocs = std::vector<GLint>();

ReflectiveQuad::ReflectiveQuad(Texture* tex) : Component("Reflective Quad", COMP_RFQ, DRAWORDER_SOLID | DRAWORDER_LIGHT), texture(tex), size(1, 1), origin(-0.5f, -0.5f), intensity(1) {

}

void ReflectiveQuad::_SetTex(void* v) {
	ReflectiveQuad* r = (ReflectiveQuad*)v;
	r->texture = _GetCache<Texture>(ASSETTYPE_TEXTURE, r->_texture);
}

#ifdef IS_EDITOR

void ReflectiveQuad::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	Engine::DrawLineW(Vec3(origin.x, origin.y, 0), Vec3(origin.x + size.x, origin.y, 0), yellow(), 2);
	Engine::DrawLineW(Vec3(origin.x, origin.y, 0), Vec3(origin.x, origin.y + size.y, 0), yellow(), 2);
}

void ReflectiveQuad::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Texture", e->font, white());
		e->DrawAssetSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_texture, &_SetTex, this);
		if (_texture == -1) {
			UI::Label(v.r + 2, v.g + pos + 17, 12, "A Texture is required!", e->font, yellow());
			return;
		}
		UI::Label(v.r + 2, v.g + pos + 17, 12, "Intensity", e->font, white());
		Engine::DrawQuad(v.r + v.b * 0.3f, v.g + pos + 17, v.b * 0.3f - 1, 16, grey1());
		UI::Label(v.r + v.b * 0.3f + 2, v.g + pos + 17, 12, std::to_string(intensity), e->font, white());
		intensity = Engine::DrawSliderFill(v.r + v.b*0.6f, v.g + pos + 17, v.b * 0.4f - 1, 16, 0, 5, intensity, grey1(), white());
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "Size", e->font, white());
		Engine::EButton((e->editorLayer == 0), v.r + v.b*0.3f, v.g + pos, v.b*0.35f - 1, 16, Vec4(0.4f, 0.2f, 0.2f, 1));
		UI::Label(v.r + v.b*0.33f, v.g + pos, 12, std::to_string(size.x), e->font, white());
		Engine::EButton((e->editorLayer == 0), v.r + v.b*0.65f, v.g + pos, v.b*0.35f - 1, 16, Vec4(0.2f, 0.4f, 0.2f, 1));
		UI::Label(v.r + v.b*0.67f, v.g + pos, 12, std::to_string(size.y), e->font, white());
	}
	else pos += 17;
}


void ReflectiveQuad::Serialize(Editor* e, std::ofstream* stream) {

}

#endif