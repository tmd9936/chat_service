#include "ServerManager.h"
#include "Packet_Utility.h"
#include "Function.h"

DWORD CALLBACK ProcessClient(LPVOID  _ptr)
{
	if (_ptr == nullptr)
		return 0;

	ClientInfo* Client_ptr = static_cast<ClientInfo*>(_ptr);
	PROTOCOL protocol{};

	bool breakflag = false;
	bool isNicNameSet = true;

	while (1)
	{

		switch (Client_ptr->GetState())
		{
		case INITE_STATE:
			Client_ptr->SetState(INTRO_STATE);
			break;
		case INTRO_STATE:
			//size = Packet_Utility::PackPacket(Client_ptr->sendbuf, INTRO, INTRO_MSG);
			//if (send(Client_ptr->sock, Client_ptr->sendbuf, size, 0) == SOCKET_ERROR)
			//{
			//	Function::err_display("intro Send()");
			//	Client_ptr->state = CONNECT_END_STATE;
			//	break;
			//}
			//Client_ptr->state = CHATT_INITE_STATE;
			break;
		case CHATT_INITE_STATE:
			if (!Client_ptr->PacketRecv())
			{
				Client_ptr->SetState(CONNECT_END_STATE);
				break;
			}

			protocol = Client_ptr->GetProtocol();

			switch (protocol)
			{
			case CHATT_NICKNAME:
				isNicNameSet = ServerManager::GetInstance().NickNameSetting(Client_ptr);
				break;
			}

			if (isNicNameSet)
			{
				ServerManager::GetInstance().ChattingEnterProcess(Client_ptr);

				if (Client_ptr->GetState() != CONNECT_END_STATE)
				{
					Client_ptr->SetState(CHATTING_STATE);
				}
			}

			break;
		case CHATTING_STATE:
			if (!Client_ptr->PacketRecv())
			{
				Client_ptr->SetState(CONNECT_END_STATE);
				break;
			}

			protocol = Client_ptr->GetProtocol();

			switch (protocol)
			{
			case CHATT_MSG:
				ServerManager::GetInstance().ChattingMessageProcess(Client_ptr);
				break;
			case CHATT_OUT:
				ServerManager::GetInstance().ChattingOutProcess(Client_ptr);
				Client_ptr->SetState(CONNECT_END_STATE);
				break;
			}
			break;
		case CONNECT_END_STATE:
			ServerManager::GetInstance().RemoveClient(Client_ptr);
			breakflag = true;
			break;
		}

		if (breakflag)
		{
			break;
		}

	}

	return 0;
}

bool ServerManager::Initialize()
{
	// 윈속 초기화
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
	int retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
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

		HANDLE hThread = CreateThread(NULL, 0, ProcessClient, ptr, 0, nullptr);
		ptr->SetHandle(hThread);
	}
}

ClientInfo* ServerManager::AddClient(SOCKET sock, SOCKADDR_IN clientaddr)
{
	printf("\nClient 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clientaddr.sin_addr),
		ntohs(clientaddr.sin_port));

	ClientInfo* ptr = new ClientInfo;
	ptr->SetSock(sock);
	ptr->SetClientAddr(clientaddr);
	//memcpy(&(ptr->clientaddr), &clientaddr, sizeof(clientaddr));

	while (1)
	{
		ClientID cID = rand() % CLIENT_ID_MAX;
		
		EnterCriticalSection(&cs);
		if (clients.find(cID) != clients.end())
			continue;
		clients.insert({ cID, ptr });
		LeaveCriticalSection(&cs);

		ptr->SetClientID(cID);
		break;
	}

	return ptr;
}

bool ServerManager::NickNameSetting(ClientInfo* _clientinfo)
{
	char message[BUFSIZE] = { 0 };
	char nName[BUFSIZE] = { 0 };
	int size = 0;

	EnterCriticalSection(&cs);

	_clientinfo->UnPackPacket(nName); // 닉네임 획득

	if (!NickNameCheck(nName))
	{
		LeaveCriticalSection(&cs);
		return false;
	}

	_clientinfo->SetNickName(nName);

	_clientinfo->SendMessageToClient(NICKNAME_COMPLETE, message);

	LeaveCriticalSection(&cs);

	return true;
}

void ServerManager::ChattingEnterProcess(ClientInfo* _clientinfo)
{
	EnterCriticalSection(&cs);

	NickNameUpdate();

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

	_clientinfo->GetNickName(message);
	strcat(message, ": ");

	EnterCriticalSection(&cs);
	_clientinfo->UnPackPacket(chatText); // 채팅 내용 획득
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

	ClientID cID = ptr->GetClientID();
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

void ServerManager::NickNameUpdate()
{
	char* NickNameList[MAXUSER];

	int i = 0;
	for (auto iter = clients.begin(); iter != clients.end(); ++iter)
	{
		iter->second->GetNickName(NickNameList[i++]);
	}

	for (auto iter = clients.begin(); iter != clients.end(); ++iter)
	{
		iter->second->SendMessageToClient(NICKNAME_LIST, NickNameList, clients.size());
	}

}
