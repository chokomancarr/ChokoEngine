#pragma once

#include "Engine.h"

typedef void (*renderFunc)(void);

#include "scene/scene.h"
#include "scene/comp/component.h"
#include "scene/transform.h"
#include "scene/comp/camera.h"
#include "scene/comp/audiosource.h"
#include "scene/comp/audiolistener.h"
#include "scene/comp/meshfilter.h"
#include "scene/comp/meshrenderer.h"
#include "scene/comp/texturerenderer.h"
#include "scene/comp/voxelrenderer.h"
#include "scene/comp/particlesystem.h"

class Armature;
class ArmatureBone;

#include "scene/comp/skinnedmeshrenderer.h"
#include "scene/comp/arrayrenderer.h"
#include "scene/comp/light.h"
#include "scene/comp/reflquad.h"
#include "scene/comp/reflprobe.h"
#include "scene/comp/ik.h"
#include "scene/comp/animator.h"
#include "scene/comp/armature.h"
#include "scene/comp/scenescript.h"
#include "scene/sceneobject.h"
