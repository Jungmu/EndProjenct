#include "VideoSocket.h"

CVideoSocket::CVideoSocket() :ServerPort(3333), piAddress("192.168.0.5"), videoStreamUrl("tcp://192.168.0.5:2222")
{

	// 소켓 초기화
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
	{
		printf("WinSock 초기화부분에서 문제 발생.n");
		WSACleanup();
		exit(0);
	}

	memset(&ToServer, 0, sizeof(ToServer));
	memset(&FromServer, 0, sizeof(FromServer));

	ToServer.sin_family = AF_INET;
	// 외부아이피로도 컨트롤 하고 싶었는데 뭐가 문제인지 외부아이피로 포트포워딩을 하면 데이터가 전달이 안된다.
	// 아무래도 포트를 열어주는 방식에 문제가 있는것 같은데 해결을 아직 못했음.
	ToServer.sin_addr.s_addr = inet_addr(piAddress);
	ToServer.sin_port = htons(ServerPort); // 포트번호

	ClientSocket = socket(AF_INET, SOCK_DGRAM, 0);// udp 

	if (ClientSocket == INVALID_SOCKET)
	{
		printf("소켓을 생성할수 없습니다.");
		closesocket(ClientSocket);
		WSACleanup();
		exit(0);
	}
}


CVideoSocket::~CVideoSocket()
{
}

