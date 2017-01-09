#include "Editor.h"
#include "Engine.h"

void EB_Viewer::_OpenMenuW(EditorBlock* b) {
	b->editor->RegisterMenu(b, vector<string>({ "(De)Select All (A)", "Dummy", "Dummy" }), vector<shortcutFunc>({ &_SelectAll, nullptr, nullptr }), 2);
}

void EB_Viewer::_OpenMenuChgMani(EditorBlock* b) {
	b->editor->RegisterMenu(b, vector<string>({ "Transform", "Rotate", "Scale" }), vector<shortcutFunc>({ &_TooltipT, &_TooltipR, &_TooltipS }), 0);
}

void EB_Viewer::_OpenMenuChgOrient(EditorBlock* b) {
	b->editor->RegisterMenu(b, vector<string>({ "Global", "Local", "View" }), vector<shortcutFunc>({ &_OrientG, &_OrientL, &_OrientV }), 0);
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
void EB_Viewer::_OrientG(EditorBlock* b) {
	((EB_Viewer*)b)->selectedOrient = 0;
}
void EB_Viewer::_OrientL(EditorBlock* b) {
	((EB_Viewer*)b)->selectedOrient = 1;
}
void EB_Viewer::_OrientV(EditorBlock* b) {
	((EB_Viewer*)b)->selectedOrient = 2;
}

void EB_Viewer::_ViewFront(EditorBlock* b) {
	((EB_Viewer*)b)->rw = 0;
	((EB_Viewer*)b)->rz = 0;
	((EB_Viewer*)b)->MakeMatrix();
}
void EB_Viewer::_ViewBack(EditorBlock* b) {
	((EB_Viewer*)b)->rw = 0;
	((EB_Viewer*)b)->rz = 180;
	((EB_Viewer*)b)->MakeMatrix();
}
void EB_Viewer::_ViewLeft(EditorBlock* b) {
	((EB_Viewer*)b)->rw = 0;
	((EB_Viewer*)b)->rz = 90;
	((EB_Viewer*)b)->MakeMatrix();
}
void EB_Viewer::_ViewRight(EditorBlock* b) {
	((EB_Viewer*)b)->rw = 0;
	((EB_Viewer*)b)->rz = -90;
	((EB_Viewer*)b)->MakeMatrix();
}
void EB_Viewer::_ViewTop(EditorBlock* b) {
	((EB_Viewer*)b)->rw = 90;
	((EB_Viewer*)b)->rz = 0;
	((EB_Viewer*)b)->MakeMatrix();
}
void EB_Viewer::_ViewBottom(EditorBlock* b) {
	((EB_Viewer*)b)->rw = -90;
	((EB_Viewer*)b)->rz = 0;
	((EB_Viewer*)b)->MakeMatrix();
}