#include "Engine.h"
#include "Editor.h"
#include "MD.h"

MD* MD::me = nullptr;

MD::MD(const string& path, uint _c, float d, float t): particlecount((uint)pow(_c, 3)*4), threads((uint)ceilf(particlecount/64.0f)), dens(d), temp(t), tempo(t), pen(1000), pes(std::vector<Vec3>(1000)), per(-2000, 0) {
	me = this;
	frb = new ComputeBuffer<Vec4>(particlecount);

	Vec4* pts = new Vec4[particlecount];
	uint i = 0;
	wall = pow(particlecount / d, 0.333f);
	mwall = wall;
	const float ds = wall / _c;
	const float dds = ds / 4;
	for (uint a = 0; a < _c; a++) {
		for (uint b = 0; b < _c; b++) {
			for (uint c = 0; c < _c; c++) {
				Vec4 bp = Vec4(a, b, c, 0)*ds;
				pts[i] = bp + Vec4(dds, dds, dds, 0);
				pts[i + 1] = bp + Vec4(dds * 3, dds * 3, dds, 0);
				pts[i + 2] = bp + Vec4(dds * 3, dds, dds * 3, 0);
				pts[i + 3] = bp + Vec4(dds, dds * 3, dds * 3, 0);

				i += 4;
			}
		}
	}
	psb = new ComputeBuffer<Vec4>(particlecount, pts);

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

	uint prs[4];
	prs[0] = particlecount;
	((float*)prs)[1] = 1.0f;
	((float*)prs)[2] = wall;
	prb = new ComputeBuffer<uint>(4, prs);

	rdb = new ComputeBuffer<Vec4>(40);

	shader = new ComputeShader(path);
	shader->SetBuffer(3, frb);
	shader->SetBuffer(4, psb);
	shader->SetBuffer(5, vlb);
	shader->SetBuffer(6, prb);
	shader->SetBuffer(7, rdb);

	colors = new Vec4[particlecount];
	for (uint a = 0; a < particlecount; a++) {
		float col = 1 - a * 1.0f / particlecount;
		colors[a] = Vec4(1, col, col, 1);
	}
	for (uint a = 0; a < pen; a++) {
		pes[a].x = 0.5f + a*0.3f/pen;
		pes[a].z = 1;
	}
	for (uint a = 0; a < 40; a++) {
		rdf[a].x = 0.5f + a*0.3f / 40;
		rdf[a].z = 1;
	}
}

void MD::Update() {
	GLint bufmask = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
	for (uint n = 0; n < 5; n++) {
		int* prs = prb->Get<int>();
		prs[0] = particlecount;
		((float*)prs)[1] = sqrt(temp / tempo);
		((float*)prs)[2] = wall;
		pe += ((float*)prs)[3];
		((int*)prs)[3] = 0;
		prb->Set(prs);
		tempo = temp;

		shader->Dispatch(threads, 1, 1);

	}

	pe /= 5;
	pes[pei].y = (pe * 1.5f / 10 / particlecount) + 1;// ((float)prs[3]) / particlecount;
	pei = ++pei % pen;
}

void MD::DrawUI() {
	UI::Label(25, 120, 16, to_string(particlecount) + " particles", Editor::instance->font, white());
	UI::Label(25, 140, 16, "using " + to_string(threads) + " threads (" + to_string((int)(Time::delta*1000/5)) + "ms)", Editor::instance->font, white());
	UI::Label(25, 160, 16, "potential: " + to_string(pe), Editor::instance->font, white());

	temp = Engine::DrawSliderFill(20, 200, 200, 30, 0.1f, 20, temp, grey1(), white());
	UI::Label(25, 202, 20, "T=" + to_string(temp), Editor::instance->font);
	wall = Engine::DrawSliderFill(20, 240, 200, 30, mwall, mwall*4, wall, grey1(), white());
	UI::Label(25, 242, 20, "W=" + to_string(wall), Editor::instance->font);

	glEnableClientState(GL_VERTEX_ARRAY);
	glColor4f(1,1,1,1);
	glLineWidth(2);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glVertexPointer(3, GL_FLOAT, 0, &pes[0]);
	glDrawArrays(GL_LINE_STRIP, 0, pen);

	glVertexPointer(3, GL_FLOAT, 0, &rdf[0]);
	glDrawArrays(GL_LINE_STRIP, 0, 40);

	glDisableClientState(GL_VERTEX_ARRAY);
}