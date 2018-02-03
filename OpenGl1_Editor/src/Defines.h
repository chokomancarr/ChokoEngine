#pragma once

#define PLATFORM_WIN

#define CHOKO_LAIT_BUILD

//comment out to not use during runtime
/*
Use the ffmpeg library. If commented out:
  - VideoTexture will not be available.
  - AudioClip will only be able to read PCM and natively supported file-types.
*/
#define FEATURE_AV_CODECS
/*
Use compute shaders. Only available on certain hardware.
On mobile, (OPENGL ES 3.1) is required.
*/
#define FEATURE_COMPUTE_SHADERS


//uncomment out to not use during building
//#define DISABLE_AV_CODECS



// sanity checks
#ifdef DISABLE_AV_CODECS
#undef FEATURE_AV_CODECS
#endif