#pragma once
#include "Engine.h"

#ifdef PLATFORM_WIN
class SerialPort {
public:
	static std::vector<string> GetNames();
	static bool Connect(string port);
	static bool Disconnect();
	static bool Read(byte* data, uint count);
	static bool Write(byte* data, uint count);

protected:
	static HANDLE handle;
};
#endif