#include "Water.h"
#include "Editor.h"

/*
void WaterParticle::repos() {
	auto dp = velocity * dt + (force / (is_oxygen? MASS_O : MASS_H)) * dt * dt * 0.5;
	position = position + dp;
}

void WaterParticle::f_bond() {
	dVec3 fold = dVec3(force);

	auto other1 = Water::Get(p1);
	auto other2 = Water::Get(p2);

	dVec3 dir = other1->position - position;
	double dst = glm::length(dir);
	force = K_LINEAR * Normalize(dir) * (dst - BOND_LENGTH);
	if (is_oxygen) {
		dVec3 dir2 = other2->position - position;
		double dst2 = glm::length(dir2);
		force += K_LINEAR * Normalize(dir2) * (dst2 - BOND_LENGTH);

		//H <--dir-- me --dir2--> H
		dVec3 t1 = Normalize(glm::cross(dir2, dir));

		dVec3 rdir1 = Normalize(glm::cross(dir, t1));
		dVec3 rdir2 = Normalize(glm::cross(t1, dir2));

		double angle = acos(Clamp(glm::dot(Normalize(dir), Normalize(dir2)), -1.0, 1.0));
		force -= (rdir1/dst + rdir2/dst2)*K_RADIAL*(angle - BOND_ANGLE);

		velocity += (fold + force) * 0.5 * dt / MASS_O;
		
		energy = 0.5 * K_RADIAL * pow((angle - BOND_ANGLE), 2);
		energy += 0.5 * MASS_O * glm::dot(velocity, velocity);
	}
	else {
		dVec3 dir1 = -dir;
		dVec3 dir2 = other2->position - other1->position;
		//me <--dir1-- O --dir2--> H
		dVec3 t1 = Normalize(glm::cross(dir2, dir1));
		dVec3 rdir = Normalize(glm::cross(dir1, t1));

		double angle = acos(Clamp(glm::dot(Normalize(dir1), Normalize(dir2)), -1.0, 1.0));
		force += (rdir/dst)*K_RADIAL*(angle - BOND_ANGLE);

		auto dv = (fold + force) * 0.5 * dt / MASS_H;
		velocity = velocity + dv;

		energy = 0.5 * K_LINEAR * pow(dst - BOND_LENGTH, 2);
		energy += 0.5 * MASS_H * glm::dot(velocity, velocity);
	}
}
*/

Water* Water::me = 0;

#define _rand (rand()%100 - 50)*0.02f

struct iVec4 {
	iVec4() : x(0), y(0), z(0), w(0) {}

	int x, y, z, w;
};

Water::Water(string path, uint _c, float d, float t): particlecount((uint)pow(_c, 3) * 4 * 3), threads((uint)ceilf(particlecount / 64.0f)), dens(d), temp(t) {
	me = this;

	frb = new ComputeBuffer<Vec4>(particlecount);

	Vec4* pts = new Vec4[particlecount];
	iVec4* ios = new iVec4[particlecount];

	uint i = 0;
	wall = pow(particlecount / d, 0.333f);
	const float ds = wall / _c;
	const float dds = ds / 4;
	for (uint a = 0; a < _c; a++) {
		for (uint b = 0; b < _c; b++) {
			for (uint c = 0; c < _c; c++) {
				Vec4 bp = Vec4(a, b, c, 0)*ds;
				Vec4 pss[] = { bp + Vec4(dds, dds, dds, 0),
					bp + Vec4(dds * 3, dds * 3, dds, 0),
					bp + Vec4(dds * 3, dds, dds * 3, 0),
					bp + Vec4(dds, dds * 3, dds * 3, 0) };
				for (uint d = 0; d < 4; d++) {
					pts[i] = pss[d];
					ios[i].x = 1;
					ios[i].y = i + 1;
					ios[i].z = i + 2;

					Vec3 d1 = Normalize(Vec3(_rand, _rand, _rand));
					Vec3 d2 = Normalize(Vec3(_rand, _rand, _rand));

					pts[i + 1] = pss[d] + Vec4(d1 * BOND_LENGTH, 0);
					ios[i + 1].y = i;
					ios[i + 1].z = i + 2;
					Vec3 d3 = Normalize(cross(d1, d2));
					Vec3 d4 = d1 * cos(BOND_ANGLE) + d3 * sin(BOND_ANGLE);
					pts[i + 2] = pss[d] + Vec4(d4 * BOND_LENGTH, 0);
					ios[i + 2].y = i;
					ios[i + 2].z = i + 1;

					i += 3;
				}
			}
		}
	}
	psb = new ComputeBuffer<Vec4>(particlecount, pts);
	iob = new ComputeBuffer<iVec4>(particlecount, ios);

	Vec4* vls = new Vec4[particlecount];
	Vec4 tot;
	for (uint a = 0; a < particlecount; a++) {
		vls[a] = Vec4(Random::Value() * 2 - 1, Random::Value() * 2 - 1, Random::Value() * 2 - 1, 0);
		tot += vls[a];
	}
	tot /= (float)particlecount;
	float myt = 0;
	for (uint a = 0; a < particlecount; a++) {
		vls[a] = vls[a] / tot;
		myt += pow(vls[a].x, 2) + pow(vls[a].y, 2) + pow(vls[a].z, 2);
	}
	myt /= (particlecount * 3);
	for (uint a = 0; a < particlecount; a++) {
		vls[a] = vls[a] * sqrt(temp / myt);
	}
	vlb = new ComputeBuffer<Vec4>(particlecount, vls);

	float* prm = new float[4];
	prm[0] = wall;
	prm[1] = temp;
	prm[2] = dens;
	prm[3] = particlecount + 0.1f;
	prb = new ComputeBuffer<float>(4, prm);

	shader = new ComputeShader(IO::GetText(path));
	shader->SetBuffer(3, frb);
	shader->SetBuffer(4, psb);
	shader->SetBuffer(5, vlb);
	shader->SetBuffer(6, iob);
	shader->SetBuffer(7, prb);
}

double ta = 0, to = 0;

void Water::Update() {

	//GLint bufmask = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
	//for (uint n = 0; n < 5; n++) {
		shader->Dispatch(threads, 1, 1);
	//}
}

void Water::Draw() {
	glBindBuffer(GL_ARRAY_BUFFER, psb->pointer);
	glUseProgram(0);

	glTranslatef(-wall / 2, -wall / 2, -wall / 2);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPointSize(4);
	glEnableClientState(GL_VERTEX_ARRAY);
	//glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, 16, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glColorPointer(4, GL_FLOAT, 0, MD::me->colors);
	glDrawArrays(GL_POINTS, 0, particlecount);
	glDisableClientState(GL_VERTEX_ARRAY);
	//glDisableClientState(GL_COLOR_ARRAY);
}