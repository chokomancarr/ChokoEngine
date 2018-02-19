#pragma once
#include "Engine.h"
#include <mutex>

#ifndef PLATFORM_WIN
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define ERROR_SUCCESS 0
#endif

typedef void(*dataReceivedCallback)(uint ip, uint port, byte* data, uint dataCount);

class Net {
public:
	static std::vector<string> MyIp();
	
	static bool Listen(uint port, dataReceivedCallback callback);
	static bool StopListen();

	static bool Send(string ip, uint port, byte* data, uint dataSz);

	//static string IP2String(uint ip);
	//static uint String2IP(const string& s);

	static dataReceivedCallback onDataReceived;

protected:
#ifdef PLATFORM_WIN
	static WSADATA* wsa;
	static SOCKET socket;

	static bool InitWsa();
#else
	static int socket;
#endif
	static sockaddr_in me;
	static sockaddr_in other;

	//static std::mutex mutex;
	static std::thread* receivingThread;

	static bool listening;
	static void HostLoop();
};