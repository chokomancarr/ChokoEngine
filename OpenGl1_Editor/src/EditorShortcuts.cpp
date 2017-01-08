#include "Editor.h"
#include "Engine.h"

void EB_Viewer::_OpenMenuW(EditorBlock* b) {
	b->editor->RegisterMenu(b, vector<string>({ "(De)Select All (A)", "Dummy", "Dummy" }), vector<shortcutFunc>({ &_SelectAll, nullptr, nullptr }), 2);
}

void EB_Viewer::_OpenMenuChgMani(EditorBlock* b) {
	b->editor->RegisterMenu(b, vector<string>({ "Transform", "Rotate", "Scale" }), vector<shortcutFunc>({ &_TooltipT, &_TooltipR, &_TooltipS }), 0);
}

void EB_Viewer::_SelectAll(EditorBlock* b) {
	b->editor->selected = nullptr;
}
void EB_Viewer::_ViewInvis(EditorBlock* b) {
	((EB_Viewer*)b)->selectedShading = (((EB_Viewer*)b)->selectedShading == 1) ? 0 : 1;
}
void EB_Viewer::_ViewPersp(EditorBlock* b) {
	((EB_Viewer*)b)->persp = !((EB_Viewer*)b)->persp;
}
void EB_Viewer::_TooltipT(EditorBlock* b) {
	((EB_Viewer*)b)->selectedTooltip = 0;
}
void EB_Viewer::_TooltipR(EditorBlock* b) {
	((EB_Viewer*)b)->selectedTooltip = 1;
}
void EB_Viewer::_TooltipS(EditorBlock* b) {
	((EB_Viewer*)b)->selectedTooltip = 2;
}