#include "Engine.h"
#include "Editor.h"

#ifdef IS_EDITOR
bool DrawComponentHeader(Editor* e, Vec4 v, uint pos, Component* c) {
	Engine::DrawQuad(v.r, v.g + pos, v.b - 17, 16, grey2());
	//bool hi = expand;
	if (Engine::EButton((e->editorLayer == 0), v.r, v.g + pos, 16, 16, c->_expanded ? e->tex_collapse : e->tex_expand, white()) == MOUSE_RELEASE) {
		c->_expanded = !c->_expanded;//hi = !expand;
	}
	//UI::Texture(v.r, v.g + pos, 16, 16, c->_expanded ? e->collapse : e->expand);
	if (Engine::EButton(e->editorLayer == 0, v.r + v.b - 16, v.g + pos, 16, 16, e->tex_buttonX, white(1, 0.7f)) == MOUSE_RELEASE) {
		//delete
		c->object->RemoveComponent(get_shared<Component>(c));
		if (!c)
			e->flags |= WAITINGREFRESHFLAG;
		return false;
	}
	UI::Label(v.r + 20, v.g + pos, 12, c->name, e->font, white());
	return c->_expanded;
}
#endif