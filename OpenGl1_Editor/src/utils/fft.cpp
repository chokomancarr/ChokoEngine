#include "Engine.h"

std::vector<std::complex<float>> FFT::Evaluate(const std::vector<float>& vals, FFT_WINDOW window) {
	const auto& sz = vals.size();
	assert(isGoodLength(sz));
	std::vector<std::complex<float>> res(sz);
	for (uint i = 0; i < sz; i++) {
		res[i] = applyWindow(vals[i], i * 1.0f / (sz - 1));
	}
	doFft(&res[0], sz);
	return res;
}

bool FFT::isGoodLength(uint v) {
	for (uint a = 1; a <= 31; a++) {
		if (v == (1 << a)) return true;
	}
	return false;
}

float FFT::applyWindow(float val, float pos) {
	return val;
}

void FFT::doFft(std::complex<float>* v, uint c) {
	if (c >= 2) {
		const uint hc = c / 2;
		separate(v, hc);
		doFft(v, hc);
		doFft(v + hc, hc);
		for (uint i = 0; i < hc; i++) {
			auto e = v[i];
			auto o = v[i + hc];
			auto w = exp(std::complex<float>(0, -2.0f*PI * i / c));
			v[i] = e + w * o;
			v[i + hc] = e - w * o;
		}
	}
}

void FFT::separate(std::complex<float>* v, uint hc) {
	auto t = new std::complex<float>[hc];
	for (uint i = 0; i < hc; i++) {
		t[i] = v[i * 2 + 1];
	}
	for (uint i = 0; i < hc; i++) {
		v[i] = v[i * 2];
	}
	for (uint i = 0; i < hc; i++) {
		v[i + hc] = t[i];
	}
	delete[](t);
}