#pragma once
#include "SceneObjects.h"

enum SCR_VARTYPE : byte {
	SCR_VAR_UNDEF = 0,
	SCR_VAR_INT, SCR_VAR_UINT,
	SCR_VAR_FLOAT,
	SCR_VAR_V2, SCR_VAR_V3, SCR_VAR_V4,
	SCR_VAR_STRING,
	SCR_VAR_ASSREF = 0x20,
	SCR_VAR_COMPREF = 0x30,
	SCR_VAR_SCR = 0xf0,
	SCR_VAR_COMMENT = 0xff
};

#ifdef IS_EDITOR
class SCR_VARVALS {
public:
	SCR_VARTYPE type;
	string desc;
	void* val;
};
#endif

class SceneScript : public Component {
public:
	~SceneScript();

	virtual void Start() {}
	virtual void Update() {}
	virtual void LateUpdate() {}
	virtual void Paint() {}

#ifdef IS_EDITOR
	static std::vector<string> userClasses;

	static bool Check(string s, Editor* e);
	static void Parse(string s, Editor* e);
	static SCR_VARTYPE String2Type(const string& s);
	static ASSETTYPE String2Asset(const string& s);
	static COMPONENT_TYPE String2Comp(const string& s);
	static int String2Script(const string& s);
#endif

	//bool ReferencingObject(Object* o) override;
	friend class Editor;
	friend class EB_Viewer;
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	_allowshared(SceneScript);
protected:
	SceneScript() : Component("", COMP_SCR, DRAWORDER_NONE) {}

#ifdef IS_EDITOR
	SceneScript(Editor* e, string s);
	SceneScript(std::ifstream& strm, SceneObject* o);

	ASSETID _script;
	std::vector<std::pair<string, SCR_VARVALS>> _vals;

	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override; //we want c to be null if deleted
	void Serialize(Editor* e, std::ofstream* stream) override;
#endif
};
