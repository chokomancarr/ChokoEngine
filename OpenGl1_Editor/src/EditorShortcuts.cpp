#include "Editor.h"
#include "Engine.h"

byte EB_Viewer::preAddType = 0;

void EB_Viewer::_Grab(EditorBlock* b) {
	if (b->editor->selected != 0) {
		EB_Viewer* v = (EB_Viewer*)b;
		if (v->modifying > 0) {
			switch (v->modifying >> 4) {
			case 1:
				v->editor->selected->transform.position = v->preModVals;
				break;
			case 2:
				v->editor->selected->transform.eulerRotation = v->preModVals;
				break;
			case 3:
				v->editor->selected->transform.scale = v->preModVals;
				break;
			}
		}
		v->modifying = 0x10;
		v->modVal = Vec2();
		v->preModVals = v->editor->selected->transform.position;
	}
}
void EB_Viewer::_Rotate(EditorBlock* b) {}
void EB_Viewer::_Scale(EditorBlock* b) {}

void EB_Viewer::_X(EditorBlock* b) {
	EB_Viewer* v = (EB_Viewer*)b;
	if (v->modifying > 0) {
		v->modifying = (v->modifying & 0xf0) | 1;
		v->modAxisDir = Vec3(1, 0, 0);
		v->OnMouseM(Vec2());
	}
}
void EB_Viewer::_Y(EditorBlock* b) {
	EB_Viewer* v = (EB_Viewer*)b;
	if (v->modifying > 0) {
		v->modifying = (v->modifying & 0xf0) | 2;
		v->modAxisDir = Vec3(0, 1, 0);
		v->OnMouseM(Vec2());
	}
}
void EB_Viewer::_Z(EditorBlock* b) {
	EB_Viewer* v = (EB_Viewer*)b;
	if (v->modifying > 0) {
		v->modifying = (v->modifying & 0xf0) | 3;
		v->modAxisDir = Vec3(0, 0, 1);
		v->OnMouseM(Vec2());
	}
}

void EB_Viewer::_OpenMenuAdd(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Add Scene Object", { "Empty", "Blender Object", "Camera", "Audio Source" }, { _AddObjectE, _AddObjectBl, _AddObjectCam, _AddObjectAud }, 3);
}
void EB_Viewer::_OpenMenuCom(EditorBlock* b) {
	if (b->editor->selected != nullptr)
		b->editor->RegisterMenu(b, "Add Component", { "Script", "Audio", "Mesh", "Rendering" }, { _AddComScr, _AddComAud, _AddComMesh, _AddComRend }, 3);
}
void EB_Viewer::_OpenMenuW(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Special Commands", { "(De)Select All (A)", "Dummy", "Dummy" }, { _SelectAll, nullptr, nullptr }, 2);
}

void EB_Viewer::_OpenMenuChgMani(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Change Manipulator Type", { "Transform", "Rotate", "Scale" }, { _TooltipT, _TooltipR, _TooltipS }, 0);
}

void EB_Viewer::_OpenMenuChgOrient(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Change Manipulator Orientation", { "Global", "Local", "View" }, { _OrientG, _OrientL, _OrientV }, 0);
}

void DoPreAdd(EditorBlock* b) {
	if (b->editor->selected != nullptr)
		b->editor->RegisterMenu(b, "Add as", vector<string>({ "Independant", "Child", "Parent" }), vector<shortcutFunc>({ ((EB_Viewer*)b)->_AddObjAsI, nullptr, nullptr }), 0);
	else
		((EB_Viewer*)b)->_AddObjAsI(b);
}

void EB_Viewer::_AddComScr(EditorBlock* b) {
	vector<void*> vals;
	for (int a = 0; a < b->editor->headerAssets.size(); a++) {
		vals.push_back(&b->editor->headerAssets[a]);
	}
	b->editor->RegisterMenu(b, "Add Script", b->editor->headerAssets, ((EB_Viewer*)b)->_DoAddComScr, vals, 0);
}
void EB_Viewer::_AddComAud(EditorBlock* b) {

}
void EB_Viewer::_AddComMesh(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Add Mesh", vector<string>({ "Mesh Filter", "Mesh Renderer" }), vector<shortcutFunc>({ _D2AddComMft, _D2AddComMrd }), 0);
}
void EB_Viewer::_AddComRend(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Add Rendering", vector<string>({ "Camera", "Mesh Renderer" }), vector<shortcutFunc>({ _D2AddComCam, _D2AddComMrd }), 0);
}
void EB_Viewer::_DoAddComScr(EditorBlock* b, void* v) {
	b->editor->selected->AddComponent(new SceneScript(b->editor, *((string*)v)));
}

void EB_Viewer::_D2AddComCam(EditorBlock* b) {
	b->editor->selected->AddComponent(new Camera());
}
void EB_Viewer::_D2AddComMft(EditorBlock* b) {
	b->editor->selected->AddComponent(new MeshFilter());
}
void EB_Viewer::_D2AddComMrd(EditorBlock* b) {
	b->editor->selected->AddComponent(new MeshRenderer());
}

void EB_Viewer::_AddObjectE(EditorBlock* b) {
	preAddType = 0;
	DoPreAdd(b);
}
void EB_Viewer::_AddObjectBl(EditorBlock* b) {
	vector<void*> vals;
	for (int a = 0; a < b->editor->normalAssets[ASSETTYPE_MESH].size(); a++) {
		vals.push_back(&b->editor->normalAssets[ASSETTYPE_MESH][a]);
	}
	b->editor->RegisterMenu(b, "Add Script", b->editor->headerAssets, ((EB_Viewer*)b)->_DoAddComScr, vals, 0);
}
void EB_Viewer::_AddObjectCam(EditorBlock* b) {
	preAddType = 5;
	DoPreAdd(b);
}
void EB_Viewer::_AddObjectAud(EditorBlock* b) {
	preAddType = 10;
	DoPreAdd(b);
}

SceneObject* DoAdd(EditorBlock* b) {
	switch (((EB_Viewer*)b)->preAddType) {
	case 0:
		b->editor->activeScene.objects.push_back(new SceneObject("Empty Object"));
		b->editor->selected = b->editor->activeScene.objects[b->editor->activeScene.objects.size() - 1];
		return b->editor->selected;
	case 5:
		b->editor->activeScene.objects.push_back(new SceneObject("Camera"));
		b->editor->selected = b->editor->activeScene.objects[b->editor->activeScene.objects.size() - 1]->AddComponent(new Camera())->object;
		return b->editor->selected;
	case 10:
		b->editor->activeScene.objects.push_back(new SceneObject("Audio Source"));
		b->editor->selected = b->editor->activeScene.objects[b->editor->activeScene.objects.size() - 1]->AddComponent(new Camera())->object;
		return b->editor->selected;
	}
}

void EB_Viewer::_AddObjAsI(EditorBlock* b) {
	SceneObject* o = DoAdd(b);
}

void EB_Viewer::_SelectAll(EditorBlock* b) {
	if (((EB_Viewer*)b)->modifying == 0)
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


void Editor::DeleteActive(Editor* e) {
	if (e->selected)
		e->RegisterMenu(e->blocks[0], "Confirm?", { "Delete" }, { &DoDeleteActive }, 0);
}
void Editor::DoDeleteActive(EditorBlock* b) {

}