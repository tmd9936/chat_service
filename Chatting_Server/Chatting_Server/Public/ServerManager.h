#pragma once

#include "Define.h"
#include "Enum.h"
#include "NoCopyObject.h"
#include "ClientInfo.h"

class ServerManager : public NoCopyObject
{
public:
	static ServerManager& GetInstance()
	{
		static ServerManager sm;
		return sm;
	}

public:
	bool	Initialize();
	void	Destroy();
	void	User_Update();

public:
	bool NickNameSetting(ClientInfo* _clientinfo);
	void ChattingEnterProcess(ClientInfo* _clientinfo);
	void ChattingOutProcess(ClientInfo* _clientinfo);
	void ChattingMessageProcess(ClientInfo* _clientinfo);
	void RemoveClient(ClientInfo* ptr);

private:
	ClientInfo* AddClient(SOCKET sock, SOCKADDR_IN clientaddr);
	bool NickNameCheck(const char* _nick);
	void NickNameUpdate();

private:
	WSADATA wsa = {};
	SOCKET listen_sock = {};
	SOCKADDR_IN serveraddr = {};

private:
	CRITICAL_SECTION cs;

private:
	unordered_map<ClientID, ClientInfo*> clients;
};

