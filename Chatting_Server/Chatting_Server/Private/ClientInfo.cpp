#include "ClientInfo.h"
#include "Function.h"
#include "Packet_Utility.h"

ClientInfo::ClientInfo()
{
}

ClientInfo::~ClientInfo()
{
}

bool ClientInfo::PacketRecv()
{
	return Function::PacketRecv(sock, recvbuf);
}

PROTOCOL ClientInfo::GetProtocol()
{
	return Function::GetProtocol(recvbuf);
}

void ClientInfo::UnPackPacket(char* _str)
{
	if (_str == nullptr)
		return;

	Packet_Utility::UnPackPacket(recvbuf, _str);
}

bool ClientInfo::NickNameSameCheck(const char* _str)
{
	if (_str == nullptr)
		return true;
	
	return !strcmp(nickname, _str);
}

void ClientInfo::CloseSocket()
{
	closesocket(sock);

	printf("\nClient 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(clientaddr.sin_addr),
		ntohs(clientaddr.sin_port));
}

int ClientInfo::SendMessageToClient(PROTOCOL _protocol, const char* message)
{
	int size = 0;
	size = Packet_Utility::PackPacket(sendbuf, _protocol, message);
	int result = send(sock, sendbuf, size, 0);

	return result;
}

int ClientInfo::SendMessageToClient(PROTOCOL _protocol, char** _strlist, int _count)
{
	int size = 0;
	size = Packet_Utility::PackPacket(sendbuf, _protocol, _strlist, _count);
	int result = send(sock, sendbuf, size, 0);

	return result;
}
