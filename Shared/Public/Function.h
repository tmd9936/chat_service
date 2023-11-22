#pragma once
// ���� �Լ� ���� ��� �� ����

#include "Define.h"
#include "Enum.h"
#include "NoCopyObject.h"

class Function : public NoCopyObject
{
public:
	static void err_quit(const char* msg);

	// ���� �Լ� ���� ���
	static void err_display(const char* msg);

	// ����� ���� ������ ���� �Լ�
	static int recvn(SOCKET s, char* buf, int len, int flags);

	static bool PacketRecv(SOCKET _sock, char* _buf);

	static PROTOCOL GetProtocol(const char* _ptr);

	static int bindSocket(SOCKET s, SOCKADDR_IN& addr);
};