#include "Engine.h"
#ifndef PLATFORM_WIN
#include <sys/time.h>
#endif

long long Time::startMillis = 0;
long long Time::millis = 0;
float Time::time = 0;
float Time::delta = 0;

long long milliseconds() {
#ifdef PLATFORM_WIN
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
	}
	else {
		return GetTickCount64();
	}
#elif defined(PLATFORM_ADR)
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1000LL + now.tv_nsec / 1000000LL;
#endif
}

void Time::Update() {
#ifdef PLATFORM_WIN
	long long millis = milliseconds();
#else
	struct timeval time;
	gettimeofday(&time, 0);
	long long millis = time.tv_sec * 1000 + time.tv_usec / 1000;
#endif
	Time::delta = (millis - Time::millis)*0.001f;
	Time::time = (millis - Time::startMillis)*0.001f;
	Time::millis = millis;
}