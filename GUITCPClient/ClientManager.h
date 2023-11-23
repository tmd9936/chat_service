#pragma once

#include "Define.h"
#include "Enum.h"
#include "NoCopyObject.h"

class ClientManager : public NoCopyObject
{
public:
	static ClientManager& GetInstance()
	{
		static ClientManager cm;
		return cm;
	}

public:
	bool	Initialize();
	bool	Connect();
	void	Destroy();
	void	WaitAllThread();
	int		DataSendToServer(PROTOCOL _protocol, char* _str);

public:
	void ClientMainThreadRun();
	void RecvThreadRun();

public:
	bool ClientPackRecv();
	PROTOCOL GetProtocol();
	void	UnPackPacket(char* _buf);

public:
	void SetState(STATE _state) {
		state = _state;
	}

	void SetIsEnd(bool _isEnd) {
		isEnd = _isEnd;
	}

public:
	STATE GetState() const {
		return state;
	}

	size_t	GetSendBufSize() const {
		return strlen(sendbuf);
	}

	bool GetIsEnd() const {
		return isEnd;
	}

public:
	void DisplayText(HWND _hWnd, char* fmt, ...);
	void UpdateUserList();

private:
	SOCKET sock;
	STATE state = { STATE::INITE_STATE };
	char sendbuf[BUFSIZE] = {};
	char recvbuf[BUFSIZE] = {};

private:
	HANDLE hClientMain = { nullptr };
	HANDLE	hRecvThread = { nullptr };

private:
	bool isEnd = { false };
};

