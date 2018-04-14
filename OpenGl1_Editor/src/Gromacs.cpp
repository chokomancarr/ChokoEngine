#pragma once
#include "Gromacs.h"

std::unordered_map<string, Vec3> Gromacs::_type2color = std::unordered_map<string, Vec3>({
	{ "H", Vec3(0.7f, 0.7f, 0.7f) },
	{ "C", Vec3(0.3f, 0.3f, 1.0f) },
	{ "O", Vec3(1.0f, 0.3f, 0.3f) }
});

Gromacs::Gromacs(const string& file) : frames(), boundingBox(), ubo_positions(0) {
	std::ifstream strm(file, std::ios::binary);
	if (!strm.is_open()) {
		std::cout << "gromacs: cannot open file!" << std::endl;
		return;
	}

	Vec4* poss = 0, *cols = 0;
	char buf[100] = {};
	while (!strm.eof()) {
		strm.getline(buf, 100);
		if (strm.eof()) return;
		frames.push_back(Frame());
		auto& frm = frames.back();
		string s(buf, 100);
		auto tp = string_find(s, "t=");
		if (tp > -1) {
			frm.name = s.substr(0, tp);
			frm.time = std::stof(s.substr(tp + 2));
		}
		else {
			frm.name = s;
			frm.time = 0;
		}
		strm.getline(buf, 100);
		frm.count = std::stoi(string(buf));
		frm.particles.resize(frm.count);

		if (!poss) {
			poss = new Vec4[frm.count]{};
			cols = new Vec4[frm.count]{};
		}
		for (uint i = 0; i < frm.count; i++) {
			auto& prt = frm.particles[i];
			strm.getline(buf, 100);
			prt.residueNumber = std::stoul(string(buf, 5));
			prt.residueName = string(buf + 5, 5);
			prt.atomName = string(buf + 10, 5);
			while (prt.atomName[0] == ' ') prt.atomName = prt.atomName.substr(1);
			prt.atomNumber = std::stoul(string(buf + 15, 5));
			prt.position.x = std::stof(string(buf + 20, 8));
			prt.position.y = std::stof(string(buf + 28, 8));
			prt.position.z = std::stof(string(buf + 36, 8));
			prt.velocity.x = std::stof(string(buf + 44, 8));
			prt.velocity.y = std::stof(string(buf + 52, 8));
			prt.velocity.z = std::stof(string(buf + 60, 8));
			if (!ubo_positions) {
				*(Vec3*)&poss[i] = prt.position;
				*(Vec3*)&cols[i] = _type2color[string(&prt.atomName[0], 1)];
			}
		}
		string bx;
		std::getline(strm, bx);
		auto spl = string_split(bx, ' ', true);
		boundingBox.x = std::stof(spl[0]);
		boundingBox.y = std::stof(spl[1]);
		boundingBox.z = std::stof(spl[2]);

		if (!ubo_positions) {
#ifdef GRO_USE_COMPUTE
			ubo_positions = new ComputeBuffer<Vec4>(frm.count, poss);
			ubo_colors = new ComputeBuffer<Vec4>(frm.count, cols);
#else
			ubo_positions = new ShaderBuffer<Vec4>(frm.count, poss);
			ubo_colors = new ShaderBuffer<Vec4>(frm.count, cols);
#endif
		}
	}
}