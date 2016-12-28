#include "Editor.h"
#include "Engine.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <algorithm>
#include <Windows.h>
#include <math.h>

void EB_Viewer::_SelectAll(EditorBlock* b) {
	cout << "select all" << endl;
}
void EB_Viewer::_ViewInvis(EditorBlock* b) {
	cout << "view invis" << endl;
}
void EB_Viewer::_ViewPersp(EditorBlock* b) {
	cout << "view persp" << endl;
	((EB_Viewer*)b)->persp = !((EB_Viewer*)b)->persp;
}
void EB_Viewer::_TooltipT(EditorBlock* b) {
	cout << "tooltip T" << endl;
}
void EB_Viewer::_TooltipR(EditorBlock* b) {
	cout << "tooltip R" << endl;
}
void EB_Viewer::_TooltipS(EditorBlock* b) {
	cout << "tooltip S" << endl;
}