#pragma once
#include "Header.h"

class CVideoSocket
{
public:
	char* piAddress;
	char* videoStreamUrl;

	WSADATA   wsaData;

	SOCKET   ClientSocket;
	SOCKADDR_IN  ToServer;
	SOCKADDR_IN  FromServer;

	USHORT   ServerPort;

	CVideoSocket();
	~CVideoSocket();
	
	void InitSocket();
};

