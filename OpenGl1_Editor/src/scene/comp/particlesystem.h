#pragma once
#include "SceneObjects.h"

enum ParticleEmissionType {
	ParticleEmission_Cone
};
struct Particle {
	Vec3 position, direction;
	float size, sizeFactor, life;
};
enum ParticleSystemValueType {
	ParticleSystemValue_Constant,
	ParticleSystemValue_Random,
	ParticleSystemValue_Curve,
	ParticleSystemValue_RandomCurve,
};
struct ParticleSystemValue {
	ParticleSystemValueType type;
	float val1, val2;

	float Eval();
};
class ParticleSystem : public Component {
public:
	int particleCount, maxParticles();
	void maxParticles(int);
	ParticleEmissionType emissionType;
	float emissionSize;
	ParticleSystemValue particleLifetime, startingSize, scalingFactor;
	bool worldSpace;

	void Emit(int count);

protected:
	std::vector<Particle> particles;
	int _maxParticles;
	void Update();
	const int _clock = 100;
	const float _dclock = 0.01f;
	double clockOffset;
};
