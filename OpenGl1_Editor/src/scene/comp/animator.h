#pragma once
#include "SceneObjects.h"

class Animator : public Component {
public:
	Animator(Animation* anim = nullptr);
	~Animator();

	Animation* animation;

	int IdOf(const string& s) {
		return animation ? animation->IdOf(s) : -1;
	}

	Vec4 Get(const string& name) {
		return animation ? animation->Get(name) : Vec4();
	}
	Vec4 Get(uint id) {
		return animation ? animation->Get(id) : Vec4();
	}

	float fps = 24;

	void OnPreLUpdate() override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, std::ofstream* stream);
	friend void Deserialize(std::ifstream& stream, SceneObject* obj);
	friend class Engine;
	_allowshared(Animator);
protected:
	ASSETID _animation = -1;

	static void _SetAnim(void* v);
#ifdef IS_EDITOR
	void DrawEditor(EB_Viewer* ebv, GLuint shader = 0) override {}
	void DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, std::ofstream* stream) override {}
#endif
};
