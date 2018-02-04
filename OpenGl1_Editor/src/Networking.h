#pragma once
#include "Engine.h"
#include <mutex>

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
	static bool InitWsa();

	static WSADATA* wsa;
	static SOCKET socket;
	static sockaddr_in me, other;

	//static std::mutex mutex;
	static std::thread* receivingThread;

	static bool listening;
	static void HostLoop();
};