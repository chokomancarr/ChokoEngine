#include "Engine.h"

int Random::seed = 1;

float Random::Value() {
	return ((float)rand()) / ((float)RAND_MAX);
}

float Random::Range(float min, float max) {
	return min + Random::Value()*(max - min);
}