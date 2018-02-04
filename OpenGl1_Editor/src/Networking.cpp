#include "Networking.h"
#include <Ws2tcpip.h>
#include <IPHlpApi.h>

WSADATA* Net::wsa = 0;
SOCKET Net::socket = SOCKET();
sockaddr_in Net::me = sockaddr_in();
sockaddr_in Net::other = sockaddr_in();
//std::mutex Net::mutex;
std::thread* Net::receivingThread = 0;
bool Net::listening = false;

dataReceivedCallback Net::onDataReceived = 0;

std::vector<string> Net::MyIp() {
	ulong sz = 25000;
	IP_ADAPTER_ADDRESSES* buf = (IP_ADAPTER_ADDRESSES*)malloc(sz);
	std::vector<string> res;
	char cbuf[255];
	if (GetAdaptersAddresses(AF_INET, 0, NULL, buf, &sz) == ERROR_SUCCESS) {
		for (; buf; buf = buf->Next) {
			auto unicast = buf->FirstUnicastAddress;
			for (; unicast; unicast = unicast->Next) {
				if (unicast->Address.lpSockaddr->sa_family == AF_INET)
				{
					sockaddr_in *sa_in = (sockaddr_in *)unicast->Address.lpSockaddr;
					inet_ntop(AF_INET, &(sa_in->sin_addr), cbuf, 255);
					res.push_back(string(cbuf));
				}
			}
		}
	}
	free(buf);
	return res;
}

bool Net::Listen(uint port, dataReceivedCallback callback) {
	if (listening) return false;

	if (!wsa && !InitWsa()) return false;

	if ((socket = ::socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
		return false;
	}

	me.sin_family = AF_INET;
	me.sin_addr.S_un.S_addr = INADDR_ANY;//inet_pton(AF_INET, "127.0.0.1", &me.sin_addr.S_un.S_addr);
	me.sin_port = htons(port);
	if (bind(socket, (sockaddr*)&me, sizeof(me)) == SOCKET_ERROR) {
		return false;
	}

	onDataReceived = callback;

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	listening = true;
	receivingThread = new std::thread(HostLoop);
	return true;
}

bool Net::StopListen() {
	listening = false;
	receivingThread->join();
	delete(receivingThread);
	return true;
}

bool Net::Send(string ip, uint port, byte* data, uint dataSz) {
	if (!wsa && !InitWsa()) return false;
	if ((socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
		return false;
	}

	other.sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &other.sin_addr.S_un.S_addr);
	other.sin_port = htons(port);

	return (sendto(socket, (char*)data, dataSz, 0, (sockaddr*)&other, sizeof(other)) != SOCKET_ERROR);
}

bool Net::InitWsa() {
	wsa = new WSADATA();
	return !WSAStartup(MAKEWORD(2, 2), wsa);
}

void Net::HostLoop() {
	byte buffer[255];
	uint sz;
	int ss = sizeof(other);
	while (listening) {
		//memset(buffer, 0, sz);
		if ((sz = recvfrom(socket, (char*)buffer, 255, 0, (sockaddr*)&other, &ss)) == SOCKET_ERROR) {
			Debug::Warning("Net", "recvfrom failed");
			return;
		}
		if (!!sz && onDataReceived)
			onDataReceived(other.sin_addr.S_un.S_addr, other.sin_port, buffer, sz);
	}
}