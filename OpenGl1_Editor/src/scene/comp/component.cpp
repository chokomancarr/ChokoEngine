#include "Engine.h"

Component::Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder, SceneObject* o, std::vector<COMPONENT_TYPE> dep)
	: Object(name), componentType(t), active(true), drawOrder(drawOrder), _expanded(true), dependancies(dep) {
	for (COMPONENT_TYPE t : dependancies) {
		dependacyPointers.push_back(rComponent());
	}
	if (o) object(o);
}

COMPONENT_TYPE Component::Name2Type(string nm) {
	static const string names[] = {
		"Camera",
		"MeshFilter",
		"MeshRenderer",
		"TextureRenderer",
		"SkinnedMeshRenderer",
		"ParticleSystem",
		"Light",
		"ReflectiveQuad",
		"RenderProbe",
		"Armature",
		"Animator",
		"InverseKinematics",
		"Script"
	};
	static const COMPONENT_TYPE types[] = {
		COMP_CAM,
		COMP_MFT,
		COMP_MRD,
		COMP_TRD,
		COMP_SRD,
		COMP_PST,
		COMP_LHT,
		COMP_RFQ,
		COMP_RDP,
		COMP_ARM,
		COMP_ANM,
		COMP_INK,
		COMP_SCR
	};

	for (uint i = 0; i < sizeof(types); i++) {
		if ((nm) == names[i]) return types[i];
	}
	return COMP_UNDEF;
}