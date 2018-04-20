#include "Engine.h"

float ParticleSystemValue::Eval() {
	switch (type) {
	case ParticleSystemValue_Constant:
	case ParticleSystemValue_Curve:
		return val1;
	case ParticleSystemValue_Random:
	case ParticleSystemValue_RandomCurve:
		return Random::Range(val1, val2);
	default:
		return val1;
	}
}

int ParticleSystem::maxParticles() {
	return _maxParticles;
}

void ParticleSystem::maxParticles(int i) {
	particles.resize(i);
	_maxParticles = i;
}

void ParticleSystem::Emit(int n) {
	for (int i = 0; i < n; i++, particleCount++) {
		if (particleCount == maxParticles()) return;

	}
}

void ParticleSystem::Update() {
	clockOffset += Time::delta;
	double c = floor(clockOffset * _clock);
	for (int i = 0; i < c; i++) {

		for (int p = 0; p < particleCount; p++) {

		}

	}
	clockOffset -= c / _clock;
}