#include "Engine.h"

class TestScript : public SceneScript {
public:
	TestScript() {}

	int foo = 1;
	int boo = 2;
	Vec2 vector2 = Vec2(1, 2);
	Vec3 vector3 = Vec3(1, 2, 3);

	void Start();

};