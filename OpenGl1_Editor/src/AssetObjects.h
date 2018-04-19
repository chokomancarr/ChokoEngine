#pragma once
#include "Engine.h"

#pragma region enums

enum Interpolation : byte {
	Interpolation_Ease,
	Interpolation_Linear,
	Interpolation_Before,
	Interpolation_Center,
	Interpolation_After
};

enum AnimKeyType : byte {
	AK_Undef = 0x00,
	AK_BoneLoc = 0x10,
	AK_BoneRot,
	AK_BoneScl,
	AK_Location = 0x20,
	AK_Rotation,
	AK_Scale,
	AK_ShapeKey = 0x30,
};

enum TEX_FILTERING : byte {
	TEX_FILTER_POINT = 0x00,
	TEX_FILTER_BILINEAR,
	TEX_FILTER_TRILINEAR
};

enum TEX_WARPING : byte {
	TEX_WARP_CLAMP,
	TEX_WARP_REPEAT
};

enum TEX_TYPE : byte {
	TEX_TYPE_NORMAL = 0x00,
	TEX_TYPE_RENDERTEXTURE,
	TEX_TYPE_READWRITE,
	TEX_TYPE_UNDEF
};

enum RT_FLAGS : byte {
	RT_FLAG_NONE = 0U,
	RT_FLAG_DEPTH = 1U,
	RT_FLAG_STENCIL = 2U, //doesn't do anything for now
	RT_FLAG_HDR = 4U
};

enum CAM_CLEARTYPE : byte {
	CAM_CLEAR_NONE,
	CAM_CLEAR_COLOR,
	CAM_CLEAR_DEPTH,
	CAM_CLEAR_SKY
};

enum GBUFFERS : byte {
	GBUFFER_DIFFUSE,
	GBUFFER_NORMAL,
	GBUFFER_SPEC_GLOSS,
	GBUFFER_EMISSION_AO,
	GBUFFER_Z,
	GBUFFER_NUM_TEXTURES
};

#pragma endregion

#include "asset/assetmanager.h"
#include "asset/resources.h"
#include "asset/assetobject.h"
#include "asset/shader.h"
#include "asset/material.h"
#include "asset/mesh.h"
#include "asset/animclip.h"
#include "asset/animation.h"
#include "asset/audioclip.h"
#include "asset/img/texture.h"
#include "asset/img/texture3d.h"
#include "asset/img/videotexture.h"
#include "asset/img/rendertexture.h"
#include "asset/img/background.h"
#include "asset/cubemap.h"
#include "asset/cameffect.h"