#include "Water.h"
#include "Editor.h"

Water* Water::me = nullptr;

std::ofstream* tempOut, *msdOut, *vcfOut;

#define _rand (rand()%100 - 50)*0.02f

Vec4* res, *vel0;

struct iVec4 {
	iVec4() : x(0), y(0), z(0), w(0) {}

	int x, y, z, w;
};

float GetTemp(Vec4* vels) {
	float t = 0;
	for (uint a = 0; a < Water::me->particlecount; a += 3) {
		Vec3 mv = Vec3();
		for (uint b = 0; b < 3; b++) {
			float m = (!b) ? MASS_O : MASS_H;
			mv += *(Vec3*)(vels + a) * m;
		}
		t += (pow(mv.x, 2) + pow(mv.y, 2) + pow(mv.z, 2)) / 2 / MASS_WATER;
	}
	return t / (Water::me->particlecount * 3 * kB);
}

Water::Water(string path, uint _c, float d, float t): particlecount((uint)pow(_c, 3) * 4 * 3), threads((uint)ceilf(particlecount / 64.0f)), dens(d), temp(t) {
	me = this;

	frb = new ComputeBuffer<Vec4>(particlecount);

	Vec4* pts = new Vec4[particlecount];
	iVec4* ios = new iVec4[particlecount];

	uint i = 0;
	wall = pow(particlecount / 3.0f / d, 0.333f);
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
					ios[i + 1].x = 0;
					ios[i + 1].y = i;
					ios[i + 1].z = i + 2;
					Vec3 d3 = Normalize(cross(d1, d2));
					Vec3 d4 = d1 * cos(BOND_ANGLE * 1.1f) + d3 * sin(BOND_ANGLE * 1.1f);
					pts[i + 2] = pss[d] + Vec4(d4 * BOND_LENGTH, 0);
					ios[i + 2].x = 0;
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
	for (uint a = 0; a < particlecount; a += 3) {
		auto rand = Vec4(Random::Value() * 2 - 1, Random::Value() * 2 - 1, Random::Value() * 2 - 1, 0);
		vls[a] = rand;
		vls[a + 1] = rand;
		vls[a + 2] = rand;
		tot += rand * 3.0f;
	}
	tot /= (float)particlecount;
	for (uint a = 0; a < Water::me->particlecount; a += 3) {
		vls[a] -= tot;
	}
	float myt = GetTemp(vls);
	for (uint a = 0; a < particlecount; a++) {
		vls[a] = vls[a] * sqrt(temp / myt);
	}
	vlb = new ComputeBuffer<Vec4>(particlecount, vls);
	
	vel0 = new Vec4[particlecount];
	memcpy(vel0, vls, particlecount*sizeof(Vec4));
	
	float* prm = new float[4];
	prm[0] = wall;
	prm[1] = temp;
	prm[2] = dens;
	prm[3] = particlecount + 0.1f;
	prb = new ComputeBuffer<float>(4, prm);

	oub = new ComputeBuffer<Vec4>(particlecount, nullptr);

	shader = new ComputeShader(IO::GetText(path));
	shader->SetBuffer(3, frb);
	shader->SetBuffer(4, psb);
	shader->SetBuffer(5, vlb);
	shader->SetBuffer(6, iob);
	shader->SetBuffer(7, prb);
	shader->SetBuffer(8, oub);

	colors = new Vec4[particlecount];
	for (uint a = 0; a < particlecount; a+=3) {
		colors[a] = red();
		colors[a + 1] = white();
		colors[a + 2] = white();
	}
	colors[0] = green();
	colors[1] = yellow();
	colors[2] = yellow();

	res = new Vec4[particlecount];

	string tempPath = IO::path + "/temp.txt";
	string msdPath = IO::path + "/msd.txt";
	string vcfPath = IO::path + "/vcf.txt";
	tempOut = new std::ofstream(tempPath, std::ios::out | std::ios::binary);
	msdOut = new std::ofstream(msdPath, std::ios::out | std::ios::binary);
	vcfOut = new std::ofstream(vcfPath, std::ios::out | std::ios::binary);

}

double ta = 0, to = 0;
ulong rec_time = -1;
uint rec_step = 10;

Vec3 _res_meanOffset = Vec3();

uint rdfNum = 0, rdf_step = 0;
uint rdf[100] = {};
uint rdfTot[100] = {};

void Water::Update() {
	shader->Dispatch(threads, 1, 1);

	oub->Get<Vec4>(res);
	Vec4 sum = Vec4();
	for (uint a = 0; a < particlecount; a++) {
		sum += res[a];
	}
	res_pot = sum.x / particlecount;

	vlb->Get<Vec4>(res);
	sum = Vec4();
	res_tmp = GetTemp(res);

	Vec3 dv = Vec3();
	for (uint a = 0; a < particlecount; a++) {
		float m = (a % 3 == 0) ? MASS_O : MASS_H;
		dv += *(Vec3*)(res + a) * m;
	}
	dv /= MASS_WATER * particlecount;

	_res_meanOffset += dv;

	res_msd = glm::length(_res_meanOffset) * dt;
	
	res_vcf = 0;
	for (uint a = 0; a < particlecount; a ++) {
		float m = (a % 3 == 0) ? MASS_O : MASS_H;
		res_vcf += m * glm::dot(res[a], vel0[a]);
	}
	res_vcf /= MASS_WATER * particlecount;

	if (++rec_step >= 10) {
		rec_time++;
		*tempOut << 0.01f * rec_time << " " << res_tmp << "\n";
		*msdOut << 0.01f * rec_time << " " << res_msd << "\n";
		*vcfOut << 0.01f * rec_time << " " << res_vcf << "\n";
		rec_step = 0;
		//if (++rdf_step >= 50) {
			const float dw = wall / 200;
			rdfNum++;
			psb->Get<Vec4>(res);
			for (uint i = 0; i < particlecount; i += 3) {
				Vec3 mypos = *(Vec3*)(res + i);
				for (uint c = 0; c < particlecount; c += 3) {
					if (c != i) {
						Vec3 dp = *(Vec3*)(res + c) - mypos;
						dp /= wall;
						dp -= glm::round(dp);
						dp *= wall;
						float dst = glm::length(dp);
						uint loc = (uint)roundf(dst * 200.0f / wall);
						if (loc >= 100)
							continue;
						rdf[loc]++;
						rdfTot[loc]++;
					}
				}
			}
		if (++rdf_step >= 50) {
			rdf_step = 0;
			
			std::ofstream rdfOut(IO::path + "/rdf.txt", std::ios::out | std::ios::binary);
			for (uint a = 0; a < 100; a++) {
				float val = rdfTot[a] / (4 * PI * pow(wall*a / 2 + dw / 2, 2) * dw);
				rdfOut << (a * dw) << " " << (val * 3 / particlecount / rdfNum) << "\n";
			}
			
			std::cout << "rdf" << std::endl;
		}
	}
}

void Water::Draw() {
	glBindBuffer(GL_ARRAY_BUFFER, psb->pointer);
	glUseProgram(0);

	glTranslatef(-wall / 2, -wall / 2, -wall / 2);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPointSize(6);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, 16, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glColorPointer(4, GL_FLOAT, 0, colors);
	glDrawArrays(GL_POINTS, 0, particlecount);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}