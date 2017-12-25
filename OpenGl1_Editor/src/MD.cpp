#include "Engine.h"
#include "Editor.h"
#include "MD.h"

MD* MD::me = nullptr;

MD::MD(uint _c, float d, float t): particlecount(pow(_c, 3)*4), threads((uint)ceil(particlecount/64.0f)), dens(d), temp(t), tempo(t), pen(1000), pes(std::vector<Vec3>(1000)), per(-2000, 0) {
	me = this;
	glGenBuffers(1, &forSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, forSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particlecount * sizeof(Vec4), NULL, GL_DYNAMIC_READ);
	GLint bufmask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	Vec4* frs = (Vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, particlecount * sizeof(Vec4), bufmask);
	for (uint a = 0; a < particlecount; a++) {
		frs[a] = Vec4();
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glGenBuffers(1, &posSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particlecount * sizeof(Vec4), NULL, GL_DYNAMIC_READ);
	Vec4* pts = (Vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, particlecount * sizeof(Vec4), bufmask);
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
				pts[i+1] = bp + Vec4(dds * 3, dds * 3, dds, 0);
				pts[i+2] = bp + Vec4(dds * 3, dds, dds * 3, 0);
				pts[i+3] = bp + Vec4(dds, dds * 3, dds * 3, 0);

				i += 4;
			}
		}
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glGenBuffers(1, &velSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particlecount * sizeof(Vec4), NULL, GL_DYNAMIC_READ);
	Vec4* vls = (Vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, particlecount * sizeof(Vec4), bufmask);
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
	myt /= (particlecount*3);
	for (uint a = 0; a < particlecount; a++) {
		vls[a] = vls[a] * sqrt(temp/myt);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glGenBuffers(1, &prmSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, prmSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * 4, NULL, GL_DYNAMIC_COPY);
	uint* prs = (uint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * 4, bufmask);
	prs[0] = particlecount;
	((float*)prs)[1] = 1.0f;
	((float*)prs)[2] = wall;
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glGenBuffers(1, &rdfSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rdfSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 40 * sizeof(Vec4), NULL, GL_DYNAMIC_READ);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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

void MD::InitProg(string path) {
	computeprog = glCreateProgram();
	GLuint mComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	auto txt = IO::GetText(path);
	std::cout << txt << std::endl;
	auto c_str = txt.c_str();
	char err[500];
	glShaderSource(mComputeShader, 1, &c_str, NULL);
	glCompileShader(mComputeShader);
	int rvalue, length;
	glGetShaderiv(mComputeShader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue)
	{
		glGetShaderInfoLog(mComputeShader, 500, &length, err);
		std::cout << string(err);
	}
	glAttachShader(computeprog, mComputeShader);
	glLinkProgram(computeprog);
	glGetProgramiv(computeprog, GL_LINK_STATUS, &rvalue);
	if (!rvalue)
	{
		glGetProgramInfoLog(computeprog, 500, &length, err);
		std::cout << string(err);
	}
	glDetachShader(computeprog, mComputeShader);
	glDeleteShader(mComputeShader);
}

void MD::Update() {
	GLint bufmask = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
	for (uint n = 0; n < 5; n++) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, prmSSBO);
		uint* prs = (uint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * 4, bufmask);
		prs[0] = particlecount;
		((float*)prs)[1] = sqrt(temp / tempo);
		((float*)prs)[2] = wall;
		pe += ((float*)prs)[3];
		((float*)prs)[3] = 0;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		tempo = temp;

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, forSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, posSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, velSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, prmSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, rdfSSBO);
		glUseProgram(computeprog);
		glDispatchCompute(threads, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);

	}
	auto e = glGetError();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rdfSSBO);
	//bufmask = GL_MAP_READ_BIT;
	Vec4* nrdf = (Vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 40 * 16, bufmask);
	e = glGetError();
	for (uint a = 0; a < 40; a++) {
		rdf[a].y = nrdf[a].x*2/particlecount;
		nrdf[a].x = 0;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	pe /= 5;
	pes[pei].y = (pe * 1.5f / 2000.0f) + 1;// ((float)prs[3]) / particlecount;
	pei = ++pei % pen;
}

void MD::DrawUI() {
	UI::Label(25, 120, 16, to_string(particlecount) + " particles", Editor::instance->font, white());
	UI::Label(25, 140, 16, "using " + to_string(threads) + " threads (" + to_string((int)(Time::delta*1000)) + "ms)", Editor::instance->font, white());
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