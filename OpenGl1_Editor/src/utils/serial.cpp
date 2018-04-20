#include "Engine.h"

#ifdef PLATFORM_WIN
std::vector<string> SerialPort::GetNames() {
	HKEY hkey;
	auto res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hkey);
	if (!!res) return std::vector<string>();
	auto ports = IO::GetRegistryKeyValues(hkey);
	RegCloseKey(hkey);
	std::vector<string> rets;
	for (auto& k : ports) rets.push_back(k.second);
	return rets;
}

HANDLE SerialPort::handle = 0;

bool SerialPort::Connect(string port) {
	string path = "\\\\.\\" + port;
	handle = CreateFile(&path[0], GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0); //FILE_FLAG_OVERLAPPED, 0);

	if (handle != INVALID_HANDLE_VALUE) {
		DCB dcbSerialParams = { 0 }; // Initializing DCB structure
		dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
		dcbSerialParams.BaudRate = CBR_9600;  // Setting BaudRate = 9600
		dcbSerialParams.ByteSize = 8;         // Setting ByteSize = 8
		dcbSerialParams.StopBits = ONESTOPBIT;// Setting StopBits = 1
		dcbSerialParams.Parity = NOPARITY;  // Setting Parity = None

		SetCommState(handle, &dcbSerialParams);

		return true;
	}
	else return false;
}

bool SerialPort::Disconnect() {
	CloseHandle(handle);
	return true;
}

bool SerialPort::Read(byte* data, uint count) {
	DWORD dwRead;

	ReadFile(handle, data, count, &dwRead, 0);
	return !!dwRead;
}

bool SerialPort::Write(byte* data, uint count) {
	return false;
}

#endif