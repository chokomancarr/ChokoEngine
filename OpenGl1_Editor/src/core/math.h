#pragma once

template <typename T> const T& min(const T& a, const T& b) {
	if (a > b) return b;
	else return a;
}
template <typename T> const T& max(const T& a, const T& b) {
	if (a > b) return a;
	else return b;
}
template <typename T> T Repeat(T t, T a, T b) {
	while (t > b)
		t -= (b - a);
	while (t < a)
		t += (b - a);
	return t;
}
template <typename T> T Clamp(T t, T a, T b) {
	return min(b, max(t, a));
}

template <typename T> T Lerp(T a, T b, float c) {
	if (c < 0) return a;
	else if (c > 1) return b;
	else return a*(1 - c) + b*c;
}
template <typename T> float InverseLerp(T a, T b, T c) {
	return Clamp((float)((c - a) / (b - a)), 0.0f, 1.0f);
}