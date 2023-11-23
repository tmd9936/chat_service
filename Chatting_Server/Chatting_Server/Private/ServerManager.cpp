#include <functional>

#include "ServerManager.h"
#include "Packet_Utility.h"
#include "Function.h"

#include "ClientIniteState.h"
#include "ChattIniteState.h"
#include "ChattingState.h"
#include "ConnectEndState.h"

DWORD CALLBACK ProcessClientThread(LPVOID  _ptr)
{
	if (_ptr == nullptr)
		return 0;
	
	ClientInfo* Client_ptr = static_cast<ClientInfo*>(_ptr);
	PROTOCOL protocol{};

	unordered_map<int, function<bool(ClientInfo* _clientInfo)>> functionMap;

	functionMap.insert({ INITE_STATE,  ClientIniteState() });
	functionMap.insert({ CHATT_INITE_STATE, ChattIniteState() });
	functionMap.insert({ CHATTING_STATE, ChattingState() });
	functionMap.insert({ CONNECT_END_STATE, ConnectEndState() });

	bool canDoflag = true;
	while (canDoflag)
	{
		canDoflag = functionMap[Client_ptr->GetState()](Client_ptr);
	}

	return 0;
}

bool ServerManager::Initialize()
{
	// ���� �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;
	InitializeCriticalSection(&cs);
	// socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) Function::err_quit("socket()");

	// bind()
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	int retval = Function::bindSocket(listen_sock, serveraddr);
	if (retval == SOCKET_ERROR) Function::err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		Function::err_quit("listen()");
		return false;
	}

	return true;
}

void ServerManager::Destroy()
{
	HANDLE handles[MAXUSER];

	size_t i = 0;
	for (auto iter = clients.begin(); iter != clients.end(); ++iter)
	{
		handles[i++] = iter->second->GetHandle();
	}
	
	WaitForMultipleObjects(clients.size(), handles, TRUE, INFINITE);

	closesocket(listen_sock);
	DeleteCriticalSection(&cs);
	WSACleanup();
}

void ServerManager::User_Update()
{
	SOCKET sock = {};
	SOCKADDR_IN clientaddr = {};
	int addrlen = { 0 };

	while (1)
	{
		if (clients.size() >= MAXUSER)
			continue;

		addrlen = sizeof(clientaddr);

		sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (sock == INVALID_SOCKET)
		{
			Function::err_display("accept()");
			continue;
		}

		ClientInfo* ptr = AddClient(sock, clientaddr);

		HANDLE hThread = CreateThread(NULL, 0, ProcessClientThread, ptr, 0, nullptr);
		ptr->SetHandle(hThread);
	}
}

ClientInfo* ServerManager::AddClient(SOCKET sock, SOCKADDR_IN clientaddr)
{
	printf("\nClient ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(clientaddr.sin_addr),
		ntohs(clientaddr.sin_port));

	ClientInfo* ptr = new ClientInfo;
	ptr->SetSock(sock);
	ptr->SetClientAddr(clientaddr);

	EnterCriticalSection(&cs);
	while (1)
	{
		int cID = rand() % CLIENT_ID_MAX;
		
		if (clients.find(cID) != clients.end())
			continue;
		clients.insert({ cID, ptr });

		ptr->SetClientID(cID);
		
		break;
	}
	LeaveCriticalSection(&cs);

	return ptr;
}

bool ServerManager::NickNameSetting(ClientInfo* _clientinfo)
{
	char nName[BUFSIZE] = { 0 };
	int size = 0;

	EnterCriticalSection(&cs);

	_clientinfo->UnPackPacket(nName); // �г��� ȹ��

	if (!NickNameCheck(nName))
	{
		LeaveCriticalSection(&cs);
		return false;
	}

	_clientinfo->SetNickName(nName);

	_clientinfo->SendMessageToClient(NICKNAME_COMPLETE, nName);

	LeaveCriticalSection(&cs);

	return true;
}

void ServerManager::ChattingEnterProcess(ClientInfo* _clientinfo)
{
	EnterCriticalSection(&cs);

	NickNameUpdate(_clientinfo);

	LeaveCriticalSection(&cs);
}

void ServerManager::ChattingOutProcess(ClientInfo* _clientinfo)
{
	char nName[BUFSIZE] = { 0 };
	int size = 0;

	EnterCriticalSection(&cs);
	_clientinfo->UnPackPacket(nName);

	for (auto iter = clients.begin(); iter != clients.end(); ++iter)
	{
		if (iter->second == _clientinfo)
			continue;
		iter->second->SendMessageToClient(CHATT_OUT, nName);
	}

	LeaveCriticalSection(&cs);
}

void ServerManager::ChattingMessageProcess(ClientInfo* _clientinfo)
{
	char chatText[BUFSIZE] = { 0 };
	char message[BUFSIZE] = { 0 };
	int size = 0;

	_clientinfo->CatNickName(message);
	strcat(message, ": ");

	EnterCriticalSection(&cs);
	_clientinfo->UnPackPacket(chatText); // ä�� ���� ȹ��
	strcat(message, chatText);

	for (auto iter = clients.begin(); iter != clients.end(); ++iter)
	{
		iter->second->SendMessageToClient(CHATT_MSG, message);
	}

	LeaveCriticalSection(&cs);
}

void ServerManager::RemoveClient(ClientInfo* ptr)
{
	ptr->CloseSocket();

	int cID = ptr->GetClientID();
	if (ptr)
	{
		delete ptr;
		ptr = nullptr;
	}
	EnterCriticalSection(&cs);
	
	 clients.erase(cID);

	LeaveCriticalSection(&cs);
}

bool ServerManager::NickNameCheck(const char* _nick)
{
	for (auto iter = clients.begin(); iter != clients.end(); ++iter)
	{
		if ((iter)->second->NickNameSameCheck(_nick))
		{
			return false;
		}
	}

	return true;
}

void ServerManager::NickNameUpdate(ClientInfo* _clientinfo)
{
	char* NickNameList[MAXUSER] = {};

	int len = 0;
	for (auto iter = clients.begin(); iter != clients.end(); ++iter)
	{
		NickNameList[len] = new char[NICKNAMESIZE];
		iter->second->GetNickName(NickNameList[len++]);
	}
	_clientinfo->SendMessageToClient(NICKNAME_LIST, NickNameList, clients.size());

	char nickName[NICKNAMESIZE] = {};
	_clientinfo->GetNickName(nickName);
	for (auto iter = clients.begin(); iter != clients.end(); ++iter)
	{
		if (iter->second == _clientinfo)
			continue;
		iter->second->SendMessageToClient(CHATT_ENTER, nickName);
	}

	for (int i = 0; i < len; ++i)
	{
		delete[] NickNameList[i];
	}
}
