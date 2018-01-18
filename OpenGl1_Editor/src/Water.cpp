#include "Water.h"
#include "Editor.h"

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


Water* Water::me = 0;

const WaterParticle* Water::Get(uint i) {
	return me->particles[i];
}

void Water::Init() {
	me = new Water();
	me->particles.push_back(new WaterParticle());
	me->particles.push_back(new WaterParticle());
	me->particles.push_back(new WaterParticle());
	//h1
	me->particles[0]->p1 = 2;
	me->particles[0]->p2 = 1;
	me->particles[0]->position = dVec3(_ast * 1.05f, 0, 0);
	me->particles[2]->is_oxygen = false;
	//h2
	me->particles[1]->p1 = 2;
	me->particles[1]->p2 = 0;
	me->particles[1]->position = dVec3(0, _ast*1.1f, 0);
	me->particles[2]->is_oxygen = false;
	//o
	me->particles[2]->p1 = 0;
	me->particles[2]->p2 = 1;
	me->particles[2]->is_oxygen = true;
}

double ta = 0, to = 0;

void Water::Update() {
	to = ta;
	ta = 0;
	for (auto& a : me->particles) {
		a->repos();
	}
	for (auto& a : me->particles) {
		a->f_bond();
		ta += a->energy;
		Engine::DrawQuad(Display::width*0.5f + (float)a->position.x * 1000, Display::height*0.5f + (float)a->position.y * 1000, 5, 5, white());
	}

	UI::Label(10, 10, 20, "Energy (J/mol):" + to_string(ta), Editor::instance->font, white());
	UI::Label(10, 40, 20, "Energy change (d(J/mol)/dstep):" + to_string((ta-to)*Time::delta), Editor::instance->font, white());
}