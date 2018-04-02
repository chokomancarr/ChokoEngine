#pragma once
#include "Engine.h"

#ifdef PLATFORM_WIN
#include <mmdeviceapi.h>
#include <Audioclient.h>

#define REFTIMES_PER_SEC  10000000LL
#define REFTIMES_PER_MILLISEC  10000LL

#define EXIT_ON_ERROR(hres) if (FAILED(hres)) return false
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

extern const CLSID CLSID_MMDeviceEnumerator;
extern const IID IID_IMMDeviceEnumerator;
extern const IID IID_IAudioClient;
extern const IID IID_IAudioRenderClient;
#endif

typedef bool(*audioRequestPacketCallback) (byte* data, uint count); //datasize = count*channels*sizeof(type)

class AudioEngine {
public:
#ifdef PLATFORM_WIN
	static REFERENCE_TIME requestedSamples, actualSamples;
	static IMMDeviceEnumerator *pEnumerator;
	static IMMDevice *pDevice;
	static IAudioClient *pAudioClient;
	static IAudioRenderClient *pRenderClient;
	static WAVEFORMATEX *pwfx;
	static UINT32 bufferFrameCount;
	static UINT32 numFramesAvailable;
	static UINT32 numFramesPadding;
	static BYTE *pData;
	static DWORD flags, samplesPerSec;
	static WORD sampleSize;

	static bool Init_win();
#elif defined(PLATFORM_ADR)
	static bool Init_adr();
#endif

	static bool alive;
	static audioRequestPacketCallback callback;
	static bool forcestop;
	static std::thread* thread;

	static bool Init();
	static void Start(audioRequestPacketCallback callback), Stop();

	static void _DoPlayStream();
};