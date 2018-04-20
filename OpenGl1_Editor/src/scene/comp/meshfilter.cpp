#include "Engine.h"
#include "Editor.h"

MeshFilter::MeshFilter() : Component("Mesh Filter", COMP_MFT, DRAWORDER_NONE), _mesh(-1) {

}

MeshFilter::MeshFilter(std::ifstream& stream, SceneObject* o, long pos) : Component("Mesh Filter", COMP_MFT, DRAWORDER_NONE, o), _mesh(-1) {
	if (pos >= 0)
		stream.seekg(pos);
	ASSETTYPE t;
	_Strm2Asset(stream, Editor::instance, t, _mesh);
	if (_mesh >= 0)
		mesh(_GetCache<Mesh>(ASSETTYPE_MESH, _mesh));
	object->Refresh();
}

#ifdef IS_EDITOR
void MeshFilter::SetMesh(int i) {
	_mesh = i;
	if (i >= 0) {
		mesh(_GetCache<Mesh>(ASSETTYPE_MESH, i));
	}
	else
		mesh.clear();
	object->Refresh();
}

void MeshFilter::_UpdateMesh(void* i) {
	MeshFilter* mf = (MeshFilter*)i;
	if (mf->_mesh != -1) {
		mf->mesh(_GetCache<Mesh>(ASSETTYPE_MESH, mf->_mesh));
	}
	else
		mf->mesh.clear();
	mf->object->Refresh();
}

void MeshFilter::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	//MeshFilter* mft = (MeshFilter*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		UI::Label(v.r + 2, v.g + pos + 20, 12, "Mesh", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_MESH, 12, e->font, &_mesh, &_UpdateMesh, this);
		pos += 34;
		UI::Label(v.r + 2, v.g + pos, 12, "Show Bounding Box", e->font, white());
		showBoundingBox = Engine::Toggle(v.r + v.b*0.3f, v.g + pos, 12, e->tex_checkbox, showBoundingBox, white(), ORIENT_HORIZONTAL);
		pos += 17;
	}
	else pos += 17;
}

void MeshFilter::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_MESH, _mesh);
}
#endif