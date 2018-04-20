#include "Engine.h"
#include "Editor.h"

InverseKinematics::InverseKinematics() : Component("InverseKinematics", COMP_INK) {

}

void InverseKinematics::Apply() {
	auto t = target->transform.position();
	for (byte i = 0; i < iterations; i++) {
		auto o = object();
		for (byte a = 0; a < length; a++) {
			if (!o->parent) return;

			auto p = object->transform.position();
			auto pp = o->parent->transform.position();

			o = o->parent();

			auto x = Normalize(t - pp);
			auto r = Normalize(p - pp);
			auto dt = Clamp(glm::dot(x, r), -1.0f, 1.0f);

			o->transform.rotation(QuatFunc::FromAxisAngle(glm::cross(r, x), rad2deg*acos(dt)) * o->transform.rotation());
		}
	}
}

#ifdef IS_EDITOR

void InverseKinematics::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	if (target) Apply();
}

void InverseKinematics::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Target", e->font, white());
		e->DrawObjectSelector(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, grey1(), 12, e->font, &target);
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Length", e->font, white());
		length = (byte)TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), std::to_string(length), e->font, true, nullptr, white()), 1);
		pos += 17;
		UI::Label(v.r + 2, v.g + pos, 12, "Iterations", e->font, white());
		iterations = (byte)TryParse(UI::EditText(v.r + v.b*0.3f, v.g + pos, v.b*0.7f - 1, 16, 12, grey2(), std::to_string(iterations), e->font, true, nullptr, white()), 1);
	}
	else pos += 17;
}

#endif