#pragma once
#include "Engine.h"

long long milliseconds();

/*! Time information
[av] */
class Time {
public:
	static long long startMillis;
	static long long millis;
	static float time;
	static float delta;

	static void Update();
};