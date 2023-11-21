#pragma once
// 소켓 함수 오류 출력 후 종료

#include "Define.h"
#include "Enum.h"
#include "NoCopyObject.h"

class Function : public NoCopyObject
{
public:
	static void err_quit(const char* msg);

	// 소켓 함수 오류 출력
	static void err_display(const char* msg);

	// 사용자 정의 데이터 수신 함수
	static int recvn(SOCKET s, char* buf, int len, int flags);

	static bool PacketRecv(SOCKET _sock, char* _buf);

	static PROTOCOL GetProtocol(const char* _ptr);
};