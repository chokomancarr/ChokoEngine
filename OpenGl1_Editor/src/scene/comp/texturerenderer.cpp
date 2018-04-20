#include "Engine.h"
#include "Editor.h"

TextureRenderer::TextureRenderer(std::ifstream& stream, SceneObject* o, long pos) : Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY, o) {
	if (pos >= 0)
		stream.seekg(pos);
	ASSETTYPE t;
	_Strm2Asset(stream, Editor::instance, t, _texture);
}

#ifdef IS_EDITOR

void TextureRenderer::DrawInspector(Editor* e, Component* c, Vec4 v, uint& pos) {
	//MeshRenderer* mrd = (MeshRenderer*)c;
	if (DrawComponentHeader(e, v, pos, this)) {
		UI::Label(v.r + 2, v.g + pos + 20, 12, "Texture", e->font, white());
		e->DrawAssetSelector(v.r + v.b * 0.3f, v.g + pos + 17, v.b*0.7f, 16, grey1(), ASSETTYPE_TEXTURE, 12, e->font, &_texture);
		pos += 34;
	}
	else pos += 17;
}

void TextureRenderer::Serialize(Editor* e, std::ofstream* stream) {
	_StreamWriteAsset(e, stream, ASSETTYPE_TEXTURE, _texture);
}

#endif