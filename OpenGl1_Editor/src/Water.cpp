#include "Water.h"

void WaterParticle::f_bond() {
	position += velocity * dt + force * dt * dt * 0.5f;
	Vec3 fold = force;

	auto other1 = Water::Get(p1);
	auto other2 = Water::Get(p2);

	Vec3 dir = other1->position - position;
	force = K_LINEAR * (Normalize(dir) * (glm::length(dir) - BOND_LENGTH));
	if (is_oxygen) {
		Vec3 dir = other1->position - position;
		force += K_LINEAR * (Normalize(dir) * (glm::length(dir) - BOND_LENGTH));
		
		Vec3 dir1 = other1->position - position;
		Vec3 dir2 = other2->position - position;
		//H <--dir1-- me --dir2--> H
		Vec3 t1 = Normalize(glm::cross(dir2, dir1));

		Vec3 rdir1 = Normalize(glm::cross(dir1, t1));
		Vec3 rdir2 = Normalize(glm::cross(t1, dir2));

		float angle = acos(glm::dot(dir1, dir2));
		force -= (rdir1 + rdir2)*K_RADIAL*(angle - BOND_ANGLE);
	}
	else {
		Vec3 dir1 = position - other1->position;
		Vec3 dir2 = other2->position - other1->position;
		//me <--dir1-- O --dir2--> H
		Vec3 t1 = Normalize(glm::cross(dir2, dir1));
		Vec3 rdir = Normalize(glm::cross(dir1, t1));

		float angle = acos(glm::dot(Normalize(dir1), Normalize(dir2)));
		force += rdir*K_RADIAL*(angle - BOND_ANGLE);
	}

	velocity += (is_oxygen ? MASS_O : MASS_H) * 0.5f * (fold + force) * dt;
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
	me->particles[0]->position = Vec3(_ast * 0.1f, -_ast, 0);
	me->particles[2]->is_oxygen = false;
	//h2
	me->particles[1]->p1 = 2;
	me->particles[1]->p2 = 0;
	me->particles[1]->position = Vec3(_ast * 0.1f, _ast, 0);
	me->particles[2]->is_oxygen = false;
	//o
	me->particles[2]->p1 = 0;
	me->particles[2]->p2 = 1;
	me->particles[2]->is_oxygen = true;
}

void Water::Update() {
	for (auto& a : me->particles) {
		a->f_bond();
		Engine::DrawQuad(200 + a->position.x * 1e12f, 200 + a->position.y * 1e12f, 5, 5, white());
	}
}