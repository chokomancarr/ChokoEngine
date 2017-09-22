#include "Editor.h"
#include "Engine.h"
#include <sstream>

void EB_Browser::_AddAsset(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Add Asset", { "Script (Header)", "Script (Cpp)", "Shader", "Material", "Camera Effect" }, { &_DoAddAssetH, nullptr, &_DoAddAssetShad, &_DoAddAssetMat, &_DoAddAssetEff }, 0);
}

void EB_Browser::_DoAddAssetH(EditorBlock* b) {
	EB_Browser* eb = (EB_Browser*)b;
	int q = 1;
	string p = eb->currentDir + "NewSceneScript.h";
	while (IO::HasFile(p.c_str())) {
		p = eb->currentDir + "NewSceneScript" + to_string(q++) + ".h";
	}
	std::ofstream strm(p, std::ios::out | std::ios::trunc);
	strm << "#include \"Engine.h\"\r\n\r\nclass ";
	strm << "NewSceneScript" + to_string(q);
	strm << " : public NewSceneScript{\r\n\tvoid Start() override {}\r\n\tvoid Update() override {}\r\n};";
	strm.close();
	eb->Refresh();
}

void EB_Browser::_DoAddAssetMat(EditorBlock* b) {
	EB_Browser* eb = (EB_Browser*)b;
	int q = 1;
	string p = eb->currentDir + "newMaterial.material";
	while (IO::HasFile(p.c_str())) {
		p = eb->currentDir + "newMaterial" + to_string(q++) + ".material";
	}
	Material m = Material();
	m.Save(p);
	eb->Refresh();
}

void EB_Browser::_DoAddAssetShad(EditorBlock* b) {
	b->editor->RegisterMenu(b, "New Shader", { "Standard Shader", "Effect Shader" }, { &_DoAddShadStd, &_DoAddShadEff }, 0);
}

void EB_Browser::_DoAddShadStd(EditorBlock* b) {
	EB_Browser* eb = (EB_Browser*)b;
	int q = 1;
	string p = eb->currentDir + "NewShader.shade";
	while (IO::HasFile(p.c_str())) {
		p = eb->currentDir + "NewShader" + to_string(q++) + ".shade";
	}
	std::ifstream tmp(eb->editor->dataPath + "\\template_StdShader.txt");
	std::ofstream strm(p, std::ios::out | std::ios::trunc);
	strm << tmp.rdbuf();
	strm.close();
	tmp.close();
	b->editor->ReloadAssets(b->editor->projectFolder + "Assets\\", true);
	eb->Refresh();
}

void EB_Browser::_DoAddShadEff(EditorBlock* b) {
	EB_Browser* eb = (EB_Browser*)b;
	int q = 1;
	string p = eb->currentDir + "NewEffectShader.shade";
	while (IO::HasFile(p.c_str())) {
		p = eb->currentDir + "NewEffectShader" + to_string(q++) + ".shade";
	}
	std::ifstream tmp(eb->editor->dataPath + "\\template_EffShader.txt");
	std::ofstream strm(p, std::ios::out | std::ios::trunc);
	strm << tmp.rdbuf();
	strm.close();
	tmp.close();
	b->editor->ReloadAssets(b->editor->projectFolder + "Assets\\", true);
	eb->Refresh();
}

void EB_Browser::_DoAddAssetEff(EditorBlock* b) {
	EB_Browser* eb = (EB_Browser*)b;
	int q = 1;
	string p = eb->currentDir + "newEffect.effect";
	while (IO::HasFile(p.c_str())) {
		p = eb->currentDir + "newEffect" + to_string(q++) + ".effect";
	}
	CameraEffect m = CameraEffect(nullptr);
	m.Save(p);
	eb->Refresh();
}

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
				v->editor->selected->transform.eulerRotation(v->preModVals);
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
void EB_Viewer::_Rotate(EditorBlock* b) {
	if (b->editor->selected != 0) {
		EB_Viewer* v = (EB_Viewer*)b;
		if (v->modifying > 0) {
			switch (v->modifying >> 4) {
			case 1:
				v->editor->selected->transform.position = v->preModVals;
				break;
			case 2:
				v->editor->selected->transform.eulerRotation(v->preModVals);
				break;
			case 3:
				v->editor->selected->transform.scale = v->preModVals;
				break;
			}
		}
		v->modifying = 0x20;
		v->modVal = Vec2();
		v->preModVals = v->editor->selected->transform.eulerRotation();
	}
}
void EB_Viewer::_Scale(EditorBlock* b) {
	if (b->editor->selected != 0) {
		EB_Viewer* v = (EB_Viewer*)b;
		if (v->modifying > 0) {
			switch (v->modifying >> 4) {
			case 1:
				v->editor->selected->transform.position = v->preModVals;
				break;
			case 2:
				v->editor->selected->transform.eulerRotation(v->preModVals);
				break;
			case 3:
				v->editor->selected->transform.scale = v->preModVals;
				break;
			}
		}
		v->modifying = 0x30;
		v->modVal = Vec2();
		v->preModVals = v->editor->selected->transform.scale;
	}
}

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
	else {
		v->selectedShading = 1 - v->selectedShading;
	}
}

void EB_Viewer::_OpenMenuAdd(EditorBlock* b) {
	if (b->editor->activeScene != nullptr)
		b->editor->RegisterMenu(b, "Add Scene Object", { "Empty", "Blender Object", "Camera", "Audio Source" }, { _AddObjectE, _AddObjectBl, _AddObjectCam, _AddObjectAud }, 3);
}
void EB_Viewer::_OpenMenuCom(EditorBlock* b) {
	if (b->editor->selected != nullptr)
		b->editor->RegisterMenu(b, "Add Component", { "Script", "Audio", "Mesh", "Rendering" }, { _AddComScr, _AddComAud, _AddComMesh, _AddComRend }, 3);
}
void EB_Viewer::_OpenMenuW(EditorBlock* b) {
	if (b->editor->activeScene != nullptr)
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
		b->editor->RegisterMenu(b, "Add as", std::vector<string>({ "Independant", "Child", "Parent" }), std::vector<shortcutFunc>({ ((EB_Viewer*)b)->_AddObjAsI, nullptr, nullptr }), 0);
	else
		((EB_Viewer*)b)->_AddObjAsI(b);
}

void EB_Viewer::_AddComScr(EditorBlock* b) {
	std::vector<void*> vals;
	for (uint a = 0; a < b->editor->headerAssets.size(); a++) {
		vals.push_back(new string(b->editor->headerAssets[a]));
	}
	b->editor->RegisterMenu(b, "Add Script", b->editor->headerAssets, ((EB_Viewer*)b)->_DoAddComScr, vals, 0);
}
void EB_Viewer::_AddComAud(EditorBlock* b) {

}
void EB_Viewer::_AddComMesh(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Add Mesh", std::vector<string>({ "Mesh Filter", "Mesh Renderer" }), std::vector<shortcutFunc>({ _D2AddComMft, _D2AddComMrd }), 0);
}
void EB_Viewer::_AddComRend(EditorBlock* b) {
	b->editor->RegisterMenu(b, "Add Rendering", std::vector<string>({ "Camera", "Mesh Renderer", "Light", "Render Probe" }), std::vector<shortcutFunc>({ _D2AddComCam, _D2AddComMrd, _D2AddComLht, _D2AddComRdp }), 0);
}

void AsCh(SceneObject* sc, const string& nm, std::vector<SceneObject*>& os, bool& found) {
	for (SceneObject* oo : os) {
		if (oo->name == nm) {
			oo->AddChild(sc);
			found = true;
			return;
		}
		else {
			AsCh(sc, nm, oo->children, found);
		}
	}
}
void LoadMeshMeta(std::vector<SceneObject*>& os, string& path) {
	for (SceneObject* o : os) {
		string nn(path.substr(Editor::instance->projectFolder.size() + 7, string::npos));
		int r = 0;
		for (string ss : Editor::instance->normalAssets[ASSETTYPE_MESH]) {
			if (ss == (nn + o->name)) {
				MeshFilter* mft = new MeshFilter();
				o->AddComponent(mft)->object->AddComponent(new MeshRenderer());
				mft->SetMesh(r);
				break;
			}
			r++;
		}
		//string anm = path + o->name + ".arma.meta";
		//if (IO::HasFile(anm.c_str())) {
		//	o->AddComponent(new Armature(anm, o));
		//}
		LoadMeshMeta(o->children, path);
	}
}
void EB_Viewer::_DoAddObjectBl(EditorBlock* b, void* v) {
	string name = *((string*)v);
	string path = b->editor->projectFolder + "Assets\\" + name + ".meta";
	name = name.substr(name.find_last_of('\\') + 1, string::npos).substr(0, name.find_last_of('.'));
	SceneObject* o = new SceneObject(name, ((EB_Viewer*)b)->rotCenter, Quat(), Vec3(1,1,1));
	b->editor->activeScene->objects.push_back(o);
	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
	if (file.is_open()) {
		char * c = new char[8];
		file.read(c, 7);
		c[7] = '\0';
		string s(c);
		if (string(c) != "KTM123\n") {
			b->editor->_Error("Add Object", *((string*)v) + " meta data corrupted!0");
			return;
		}
		string d;
		file >> d;
		while (d == "obj" || d == "arm") {
			string nm;
			string prt;
			file >> nm;
			if (nm == "") break;
			std::cout << nm << std::endl;
			char * cc = new char[7];
			cc[6] = '\0';
			file.read(cc, 6);
			string ss;
			Vec3 tr, sc;
			Quat rt;
			if (*(cc + 1) == '\0') {
				ss = string(cc + 2);
			}
			else {
				b->editor->_Error("Add Object", *((string*)v) + " meta data corrupted!1");
				return;
			}
			if (ss == "prt ") {
				file >> prt;
				file.read(cc, 6);
				if (*(cc + 1) == '\0') {
					ss = string(cc + 2);
				}
				else {
					b->editor->_Error("Add Object", *((string*)v) + " meta data corrupted!2");
					return;
				}
			}
			if (ss != "pos ") {
				b->editor->_Error("Add Object", *((string*)v) + " meta data corrupted!3");
				return;
			}
			file >> ss;
			tr.x = stof(ss);
			file >> ss;
			tr.y = stof(ss);
			file >> ss;
			tr.z = stof(ss);

			file.read(cc, 6);
			if (*(cc + 1) == '\0') {
				ss = string(cc + 2);
			}
			else {
				b->editor->_Error("Add Object", *((string*)v) + " meta data corrupted!4");
				return;
			}
			if (ss != "rot ") {
				b->editor->_Error("Add Object", *((string*)v) + " meta data corrupted!5");
				return;
			}
			file >> ss;
			rt.w = stof(ss);
			file >> ss;
			rt.x = stof(ss);
			file >> ss;
			rt.y = stof(ss);
			file >> ss;
			rt.z = stof(ss);

			file.read(cc, 6);
			if (*(cc + 1) == '\0') {
				ss = string(cc + 2);
			}
			else {
				b->editor->_Error("Add Object", *((string*)v) + " meta data corrupted!6");
				return;
			}
			if (ss != "scl ") {
				b->editor->_Error("Add Object", *((string*)v) + " meta data corrupted!7");
				return;
			}
			file >> ss;
			sc.x = stof(ss);
			file >> ss;
			sc.y = stof(ss);
			file >> ss;
			sc.z = stof(ss);
			if (d == "obj") {
				if (prt == "")
					o->AddChild(new SceneObject(nm, tr, rt, sc));
				else {
					bool found;
					AsCh(new SceneObject(nm, tr, rt, sc), prt, o->children, found);
				}
			}
			else {
				auto so = new SceneObject(nm, tr, rt, sc);
				o->AddChild(so);
				//std::ifstream astrm(path.substr(0, path.size() - 11) + "_blend\\" + nm + ".arma.meta", std::ios::in | sd::ios::bin);
				so->AddComponent(new Armature(path.substr(0, path.size() - 11) + "_blend\\" + nm + ".arma.meta", so));
			}
			file >> d;
		}
		file.close();

		LoadMeshMeta(o->children, path.substr(0, path.size()-11) + "_blend\\");
	}
	else {
		b->editor->_Error("Add Object", "failed to open " + *((string*)v) + " meta file!");
	}
}
void EB_Viewer::_DoAddComScr(EditorBlock* b, void* v) {
	b->editor->selected->AddComponent(new SceneScript(b->editor, b->editor->GetAssetInfoH(*(string*)v)));
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
void EB_Viewer::_D2AddComLht(EditorBlock* b) {
	b->editor->selected->AddComponent(new Light());
}
void EB_Viewer::_D2AddComRdp(EditorBlock* b) {
	b->editor->selected->AddComponent(new ReflectionProbe());
}

void EB_Viewer::_AddObjectE(EditorBlock* b) {
	preAddType = 0;
	DoPreAdd(b);
}
void EB_Viewer::_AddObjectBl(EditorBlock* b) {
	std::vector<void*> vals;
	for (uint a = 0; a < b->editor->blendAssets.size(); a++) {
		vals.push_back(new string(b->editor->blendAssets[a]));
	}
	b->editor->RegisterMenu(b, "Add Blender Object", b->editor->blendAssets, ((EB_Viewer*)b)->_DoAddObjectBl, vals, 0);
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
		b->editor->activeScene->objects.push_back(new SceneObject("Empty Object"));
		b->editor->selected = b->editor->activeScene->objects[b->editor->activeScene->objects.size() - 1];
		break;
	case 5:
		b->editor->activeScene->objects.push_back(new SceneObject("Camera"));
		b->editor->selected = b->editor->activeScene->objects[b->editor->activeScene->objects.size() - 1]->AddComponent(new Camera())->object;
		break;
	case 10:
		b->editor->activeScene->objects.push_back(new SceneObject("Audio Source"));
		b->editor->selected = b->editor->activeScene->objects[b->editor->activeScene->objects.size() - 1]->AddComponent(new Camera())->object;
		break;
	}
	return b->editor->selected;
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
void EB_Viewer::_ViewCam(EditorBlock* b) {
	if (b->editor->selected != nullptr) {
		((EB_Viewer*)b)->seeingCamera = (Camera*)b->editor->selected->GetComponent(COMP_CAM);
	}
}
void EB_Viewer::_SnapCenter(EditorBlock* b) {
	if (b->editor->selected == nullptr) {
		((EB_Viewer*)b)->rotCenter = Vec3();
	}
	else {
		((EB_Viewer*)b)->rotCenter = b->editor->selected->transform.worldPosition();
	}
}

void EB_Viewer::_TogglePersp(EditorBlock* b) {
	((EB_Viewer*)b)->persp = !((EB_Viewer*)b)->persp;
}
void EB_Viewer::_Escape(EditorBlock* b) {
	((EB_Viewer*)b)->OnMousePress(1);
}


void EB_AnimEditor::_AddState(EditorBlock* eb) {
	EB_AnimEditor* b = (EB_AnimEditor*)eb;
	b->editor->RegisterMenu(b, "Add State", { "Single", "Blend Tree" }, { &_AddEmpty, &_AddBlend }, 0);
}

void EB_AnimEditor::_SetAnim(void* v) {
	EB_AnimEditor* b = (EB_AnimEditor*)v;
	b->editingAnim = _GetCache<Animator>(ASSETTYPE_ANIMATOR, b->_editingAnim);
}

void EB_AnimEditor::_AddEmpty(EditorBlock* v) {
	EB_AnimEditor* b = (EB_AnimEditor*)v;
	if (b->editingAnim != nullptr)
		b->editingAnim->states.push_back(new Anim_State(Vec2()));
}

void EB_AnimEditor::_AddBlend(EditorBlock* v) {
	EB_AnimEditor* b = (EB_AnimEditor*)v;
	if (b->editingAnim != nullptr)
		b->editingAnim->states.push_back(new Anim_State(Vec2(), true));
}

void EB_Previewer::_ToggleBuffers(EditorBlock* v) {
	EB_Previewer* b = (EB_Previewer*)v;
	b->showBuffers = !b->showBuffers;
	b->showLumi = true;
}
void EB_Previewer::_ToggleLumi(EditorBlock* v) {
	EB_Previewer* b = (EB_Previewer*)v;
	if (!b->showBuffers)
		b->showLumi = !b->showLumi;
}

void Editor::DeleteActive(Editor* e) {
	if (e->selected)
		e->RegisterMenu(e->blocks[0], "Confirm?", { "Delete" }, { &DoDeleteActive }, 0);
}
void Editor::DoDeleteActive(EditorBlock* b) {

}

void Editor::Maximize(Editor* e) {
	e->hasMaximize = !e->hasMaximize;
	if (e->hasMaximize) {
		for (EditorBlock* b : e->blocks)
		{
			Vec4 v = Vec4(Display::width*e->xPoss[b->x1], Display::height*e->yPoss[b->y1], Display::width*e->xPoss[b->x2], Display::height*e->yPoss[b->y2]);
			b->maximize = (Rect(v.r, v.g, v.b - v.r, v.a - v.g).Inside(Input::mousePos));
		}
	}
	else {
		for (EditorBlock* b : e->blocks)
		{
			b->maximize = false;
		}
	}
}

void GetSceneFiles(string path, string sub, std::vector<string>& list) {
	for (string s : IO::GetFiles(path + sub, ".scene")) {
		string ss(s.substr(path.size(), string::npos));
		list.push_back(ss.substr(0, ss.find_last_of('\\')) + ss.substr(ss.find_last_of('\\')+1, string::npos));
	}
	std::vector<string> folders;
	IO::GetFolders(path + sub, &folders);
	for (string f : folders) {
		if (f != "." && f != "..")
		GetSceneFiles(path, sub + f + "\\", list);
	}
}

void Editor::OpenScene(Editor* e) {
	std::vector<string> scenes;
	GetSceneFiles(e->projectFolder + "Assets\\", "", scenes);
	std::vector<void*> v;
	for (string s : scenes) {
		v.push_back(new string(s));
	}
	e->RegisterMenu(nullptr, "Open Scene", scenes, &DoOpenScene, v, 0);
}

void Editor::DoOpenScene(EditorBlock* b, void* v) {
	//if (Editor::instance->sceneLoaded)
	//	delete(&Editor::instance->activeScene);
	if (Scene::active != nullptr) Scene::active->Unload();
	string nm = Editor::instance->projectFolder + "Assets\\" + *(string*)v;
	std::ifstream s(nm.c_str(), std::ios::binary | std::ios::in);
	Editor::instance->activeScene = std::make_shared<Scene>(s, 0);
	Scene::active = Editor::instance->activeScene;
}