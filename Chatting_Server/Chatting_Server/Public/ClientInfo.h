#pragma once

#include "Define.h"
#include "Enum.h"

class ClientInfo
{
public:
	ClientInfo();
	~ClientInfo();

public:
	bool	PacketRecv();
	PROTOCOL GetProtocol();
	bool	NickNameSameCheck(const char* _str);
	void	CloseSocket();

public:
	void	UnPackPacket(char* _str);

public:
	int		SendMessageToClient(PROTOCOL _protocol, const char* message);
	int		SendMessageToClient(PROTOCOL _protocol, char** _strlist, int _count);

public:
	ClientID GetClientID() const {
		return clientID;
	}

	STATE GetState() const {
		return state;
	}

	void GetNickName(char* buf) const {
		if (buf == nullptr)
			return;
		strcpy(buf, nickname);
	}

	HANDLE GetHandle() const {
		return handle;
	}

public:
	void SetClientID(ClientID _clientID) {
		clientID = _clientID;
	}

	void SetSock(const SOCKET& _sock) {
		sock = _sock;
	}

	void SetClientAddr(const SOCKADDR_IN& _addr) {
		clientaddr = _addr;
	}

	void SetHandle(HANDLE _handle) {
		handle = _handle;
	}

	void SetState(STATE _state) {
		state = _state;
	}

	void SetNickName(const char* _str) {
		if (_str == nullptr)
			return;
		strcpy(nickname, _str);
	}

private:
	ClientID clientID = { 0 };
	SOCKET	sock;
	SOCKADDR_IN clientaddr;
	STATE	state = { INITE_STATE };
	bool	chatflag = { false };
	char	nickname[NICKNAMESIZE] = {};
	char	sendbuf[BUFSIZE] = {};
	char	recvbuf[BUFSIZE] = {};
	HANDLE	handle = { nullptr };
};