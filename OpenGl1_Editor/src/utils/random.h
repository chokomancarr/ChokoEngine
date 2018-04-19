#pragma once

/*! RNG System
[av] */
class Random {
public:
	static int seed;
	static float Value(), Range(float min, float max);
};