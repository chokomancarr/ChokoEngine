#include "AudioEngine.h"

#ifdef PLATFORM_WIN
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

REFERENCE_TIME AudioEngine::requestedSamples = REFTIMES_PER_SEC;
REFERENCE_TIME AudioEngine::actualSamples;
IMMDeviceEnumerator *AudioEngine::pEnumerator = NULL;
IMMDevice *AudioEngine::pDevice = NULL;
IAudioClient *AudioEngine::pAudioClient = NULL;
IAudioRenderClient *AudioEngine::pRenderClient = NULL;
WAVEFORMATEX *AudioEngine::pwfx = NULL;
UINT32 AudioEngine::bufferFrameCount;
UINT32 AudioEngine::numFramesAvailable;
UINT32 AudioEngine::numFramesPadding;
BYTE *AudioEngine::pData;
DWORD AudioEngine::flags = 0;
DWORD AudioEngine::samplesPerSec = 0;
WORD AudioEngine::sampleSize = 0;

bool AudioEngine::Init_win() {
	HRESULT hr;

	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr);

	hr = pEnumerator->GetDefaultAudioEndpoint(
		eRender, eConsole, &pDevice);
	EXIT_ON_ERROR(hr);

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr);

	hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr);

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		0,
		requestedSamples,
		0,
		pwfx,
		NULL);
	EXIT_ON_ERROR(hr);

	sampleSize = pwfx->wBitsPerSample;

	// Tell the audio source which format to use.
	//hr = pMySource->SetFormat(pwfx);

	// Get the actual size of the allocated buffer.
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr);

	hr = pAudioClient->GetService(
		IID_IAudioRenderClient,
		(void**)&pRenderClient);
	EXIT_ON_ERROR(hr);

	samplesPerSec = pwfx->nSamplesPerSec;

	Debug::Message("AudioEngine", "WASAPI Initialized.");
	return true;
}
#endif

audioRequestPacketCallback AudioEngine::callback = nullptr;
bool AudioEngine::forcestop = false;
std::thread* AudioEngine::thread;

bool AudioEngine::Init() {
#ifdef PLATFORM_WIN
	return Init_win();
#elif defined(PLATFORM_ADR)
	//return Init_adr();
#endif
	return false;
}

void AudioEngine::Start(audioRequestPacketCallback cb) {
	if (thread) return;
	forcestop = false;
	callback = cb;
	thread = new std::thread(_DoPlayStream);
}

void AudioEngine::Stop() {
	forcestop = true;
	thread->join();
	delete(thread);
}

void AudioEngine::_DoPlayStream()
{
#ifdef PLATFORM_WIN
	bufferFrameCount = 1000;

	// Grab the entire buffer for the initial fill operation.
	pRenderClient->GetBuffer(bufferFrameCount, &pData);

	callback(pData, bufferFrameCount);

	pRenderClient->ReleaseBuffer(bufferFrameCount, flags);

	// Calculate the actual duration of the allocated buffer.
	actualSamples = REFTIMES_PER_SEC * bufferFrameCount / samplesPerSec;

	pAudioClient->Start();
	while (!forcestop)
	{
		// Sleep for half the buffer duration.
		Sleep((DWORD)(actualSamples / REFTIMES_PER_MILLISEC / 2));

		// See how much buffer space is available.
		pAudioClient->GetCurrentPadding(&numFramesPadding);

		numFramesAvailable = bufferFrameCount - numFramesPadding;

		// Grab all the available space in the shared buffer.
		pRenderClient->GetBuffer(numFramesAvailable, &pData);

		// Get next 1/2-second of data from the audio source.
		flags = callback(pData, numFramesAvailable)? 0 : AUDCLNT_BUFFERFLAGS_SILENT;

		pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
	}

	// Wait for last data in buffer to play before stopping.
	Sleep((DWORD)(actualSamples / REFTIMES_PER_MILLISEC / 2));

	pAudioClient->Stop();  // Stop playing.
#endif
}