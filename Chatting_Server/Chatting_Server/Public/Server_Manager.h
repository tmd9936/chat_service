#pragma once

#include "Define.h"
#include "Enum.h"
#include "NoCopyObject.h"

struct _ClientInfo
{
	SOCKET sock;
	SOCKADDR_IN clientaddr;
	STATE state;
	bool chatflag;
	char  nickname[NICKNAMESIZE];
	char  sendbuf[BUFSIZE];
	char  recvbuf[BUFSIZE];

};

class Server_Manager : public NoCopyObject
{
public:
	static Server_Manager& GetInstance()
	{
		static Server_Manager sm;
		return sm;
	}

public:
	bool	Initialize();
	void	Destroy();
	void	User_Update();

public:
	_ClientInfo* AddClient(SOCKET sock, SOCKADDR_IN clientaddr);
	void RemoveClient(_ClientInfo* ptr);
	_ClientInfo* SearchClient(const char* _nick);
	bool NicknameCheck(const char* _nick);
	void AddNickName(const char* _nick);
	void RemoveNickName(const char* _nick);
	bool NickNameSetting(_ClientInfo* _clientinfo);
	void ChattingEnterProcess(_ClientInfo* _clientinfo);
	void ChattingOutProcess(_ClientInfo* _clientinfo);
	void ChattingMessageProcess(_ClientInfo* _clientinfo);
	void NickNameUpdate();

private:
	WSADATA wsa = {};
	SOCKET listen_sock = {};
	SOCKADDR_IN serveraddr = {};

	SOCKET sock = {};
	SOCKADDR_IN clientaddr = {};
	int addrlen = { 0 };

private:
	CRITICAL_SECTION cs;
	HANDLE Handles[MAXUSER] = {nullptr};
	int Handle_Count = { 0 };

private:
	_ClientInfo* ClientInfo[MAXUSER];
	char* NickNameList[MAXUSER];

	int Client_Count = { 0 };
	int Nick_Count = { 0 };

};

