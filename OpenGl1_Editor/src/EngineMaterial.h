#ifndef ENGINE_MAT_H
#define ENGINE_MAT_H

using namespace std;

class Material {
public:
	uint shader;

	Material(void);
	Material(Shader* shad);
	void SetSampler(string name, Texture* texture);
	void SetSampler(string name, Texture* texture, int id);

};

/*
enum ShaderInputTypes {
	Sampler,
	Int,
	Float,
	Vec2,
	Vec3,
	Color
};

struct MatParams {

};
*/

#endif