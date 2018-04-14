#pragma once
#include "Engine.h"

#define GRO_USE_COMPUTE

//GROMACS molecular data file parser
class Gromacs {
public:
	class Particle {
	public:
		uint residueNumber, atomNumber;
		string residueName, atomName;
		Vec3 position, velocity;
	};
	class Frame {
	public:
		float time;
		string name;
		uint count;
		std::vector<Particle> particles;
	};

	Gromacs(const string& file);
	
	std::vector<Frame> frames;
	Vec3 boundingBox;

	static std::unordered_map<string, Vec3> _type2color;

#ifdef GRO_USE_COMPUTE
	ComputeBuffer<Vec4> 
#else
	ShaderBuffer<Vec4>
#endif
	*ubo_positions, *ubo_colors;
};