#include "Engine.h"
#include "Editor.h"

ReflectionProbe::ReflectionProbe(ushort size) : Component("Reflection Probe", COMP_RDP, DRAWORDER_LIGHT), size(size), map(new CubeMap(size)), updateMode(ReflProbe_UpdateMode_Start), intensity(1), clearType(ReflProbe_Clear_Sky), clearColor(), range(1, 1, 1), softness(0) {

}

ReflectionProbe::ReflectionProbe(std::ifstream& stream, SceneObject* o, long pos) : Component("Reflection Probe", COMP_RDP, DRAWORDER_LIGHT) {
	if (pos >= 0)
		stream.seekg(pos);
	_Strm2Val(stream, size);
	_Strm2Val(stream, updateMode);
	_Strm2Val(stream, intensity);
	_Strm2Val(stream, clearType);
	_Strm2Val(stream, clearColor.r);
	_Strm2Val(stream, clearColor.g);
	_Strm2Val(stream, clearColor.b);
	_Strm2Val(stream, clearColor.a);
	_Strm2Val(stream, range.x);
	_Strm2Val(stream, range.y);
	_Strm2Val(stream, range.z);
	_Strm2Val(stream, softness);

	//map = new CubeMap(size);
}

#ifdef IS_EDITOR

void ReflectionProbe::DrawEditor(EB_Viewer* ebv, GLuint shader) {
	Engine::DrawCircleW(Vec3(), Vec3(1, 0, 0), Vec3(0, 1, 0), 0.2f, 12, Vec4(1, 0.76f, 0.80, 1), 1);
	Engine::DrawCircleW(Vec3(), Vec3(0, 1, 0), Vec3(0, 0, 1), 0.2f, 12, Vec4(1, 0.76f, 0.80, 1), 1);
	Engine::DrawCircleW(Vec3(), Vec3(0, 0, 1), Vec3(1, 0, 0), 0.2f, 12, Vec4(1, 0.76f, 0.80, 1), 1);

	if (ebv->editor->selected == object) {
		Engine::DrawLineWDotted(range*Vec3(1, 1, -1), range*Vec3(1, 1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(1, 1, 1), range*Vec3(-1, 1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, 1, 1), range*Vec3(-1, 1, -1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, 1, -1), range*Vec3(1, 1, -1), white(), 1, 0.1f);

		Engine::DrawLineWDotted(range*Vec3(1, 1, 1), range*Vec3(1, -1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(1, 1, -1), range*Vec3(1, -1, -1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, 1, 1), range*Vec3(-1, -1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, 1, -1), range*Vec3(-1, -1, -1), white(), 1, 0.1f);

		Engine::DrawLineWDotted(range*Vec3(1, -1, -1), range*Vec3(1, -1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(1, -1, 1), range*Vec3(-1, -1, 1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, -1, 1), range*Vec3(-1, -1, -1), white(), 1, 0.1f);
		Engine::DrawLineWDotted(range*Vec3(-1, -1, -1), range*Vec3(1, -1, -1), white(), 1, 0.1f);
	}

	/*
	MVP::Push();
	Vec3 v = object->transform.position;
	Vec3 vv = object->transform.scale;
	Quat vvv = object->transform.rotation;
	MVP::Translate(v.x, v.y, v.z);
	MVP::Scale(vv.x, vv.y, vv.z);
	MVP::Mul(Quat2Mat(vvv)));
	MVP::Pop();
	*/
}

void ReflectionProbe::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	if (DrawComponentHeader(e, v, pos, this)) {
		pos += 17;

		UI::Label(v.r + 2, v.g + pos + 20, 12, "Intensity", e->font, white());
		pos += 17;
	}
	else pos += 17;
}


void ReflectionProbe::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWrite(&size, stream, 2);
	_StreamWrite(&updateMode, stream, 1);
	_StreamWrite(&intensity, stream, 4);
	_StreamWrite(&clearType, stream, 1);
	_StreamWrite(&clearColor.r, stream, 4);
	_StreamWrite(&clearColor.g, stream, 4);
	_StreamWrite(&clearColor.b, stream, 4);
	_StreamWrite(&clearColor.a, stream, 4);
	_StreamWrite(&range.x, stream, 4);
	_StreamWrite(&range.y, stream, 4);
	_StreamWrite(&range.z, stream, 4);
	_StreamWrite(&softness, stream, 4);
}

#endif