#include "Server_Manager.h"
#include "Packet_Utility.h"
#include "Function.h"

DWORD CALLBACK ProcessClient(LPVOID  _ptr)
{
	_ClientInfo* Client_ptr = (_ClientInfo*)_ptr;

	int size;
	PROTOCOL protocol{};

	bool breakflag = false;
	bool isNicNameSet = true;

	while (1)
	{

		switch (Client_ptr->state)
		{
		case INITE_STATE:
			Client_ptr->state = STATE::INTRO_STATE;
			break;
		case INTRO_STATE:
			size = Packet_Utility::PackPacket(Client_ptr->sendbuf, INTRO, INTRO_MSG);
			if (send(Client_ptr->sock, Client_ptr->sendbuf, size, 0) == SOCKET_ERROR)
			{
				Function::err_display("intro Send()");
				Client_ptr->state = CONNECT_END_STATE;
				break;
			}
			Client_ptr->state = CHATT_INITE_STATE;
			break;
		case CHATT_INITE_STATE:
			if (!Function::PacketRecv(Client_ptr->sock, Client_ptr->recvbuf))
			{
				Client_ptr->state = CONNECT_END_STATE;
				break;
			}

			protocol = Function::GetProtocol(Client_ptr->recvbuf);

			switch (protocol)
			{
			case CHATT_NICKNAME:
				isNicNameSet = Server_Manager::GetInstance().NickNameSetting(Client_ptr);
				break;
			}

			if (isNicNameSet)
			{
				Server_Manager::GetInstance().ChattingEnterProcess(Client_ptr);

				if (Client_ptr->state != CONNECT_END_STATE)
				{
					Client_ptr->state = CHATTING_STATE;
				}
			}

			break;
		case CHATTING_STATE:
			if (!Function::PacketRecv(Client_ptr->sock, Client_ptr->recvbuf))
			{
				Client_ptr->state = CONNECT_END_STATE;
				break;
			}

			protocol = Function::GetProtocol(Client_ptr->recvbuf);

			switch (protocol)
			{
			case CHATT_MSG:
				Server_Manager::GetInstance().ChattingMessageProcess(Client_ptr);
				break;
			case CHATT_OUT:
				Server_Manager::GetInstance().ChattingOutProcess(Client_ptr);
				Client_ptr->state = CONNECT_END_STATE;
				break;
			}
			break;
		case CONNECT_END_STATE:
			Server_Manager::GetInstance().RemoveClient(Client_ptr);
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

bool Server_Manager::Initialize()
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

void Server_Manager::Destroy()
{
	WaitForMultipleObjects(Handle_Count, Handles, TRUE, INFINITE);

	closesocket(listen_sock);
	DeleteCriticalSection(&cs);
	WSACleanup();
}

void Server_Manager::User_Update()
{
	while (1)
	{
		if (Handle_Count >= MAXUSER)
			continue;
		addrlen = sizeof(clientaddr);

		sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (sock == INVALID_SOCKET)
		{
			Function::err_display("accept()");
			continue;
		}

		_ClientInfo* ptr = AddClient(sock, clientaddr);

		HANDLE hThread = CreateThread(NULL, 0, ProcessClient, ptr, 0, nullptr);
		Handles[Handle_Count++] = hThread;
	}
}

_ClientInfo* Server_Manager::AddClient(SOCKET sock, SOCKADDR_IN clientaddr)
{
	printf("\nClient 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clientaddr.sin_addr),
		ntohs(clientaddr.sin_port));

	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));
	ptr->sock = sock;
	memcpy(&(ptr->clientaddr), &clientaddr, sizeof(clientaddr));
	ptr->state = INITE_STATE;
	ptr->chatflag = false;

	EnterCriticalSection(&cs);
	ClientInfo[Client_Count++] = ptr;
	LeaveCriticalSection(&cs);

	return ptr;
}

void Server_Manager::RemoveClient(_ClientInfo* ptr)
{
	closesocket(ptr->sock);

	printf("\nClient 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(ptr->clientaddr.sin_addr),
		ntohs(ptr->clientaddr.sin_port));

	EnterCriticalSection(&cs);

	for (int i = 0; i < Client_Count; i++)
	{
		if (ClientInfo[i] == ptr)
		{
			RemoveNickName(ClientInfo[i]->nickname);
			delete ptr;
			int j;
			for (j = i; j < Client_Count - 1; j++)
			{
				ClientInfo[j] = ClientInfo[j + 1];
			}
			ClientInfo[j] = nullptr;
			break;
		}
	}

	Client_Count--;
	Handle_Count--;

	NickNameUpdate();
	LeaveCriticalSection(&cs);
}

_ClientInfo* Server_Manager::SearchClient(const char* _nick)
{
	EnterCriticalSection(&cs);
	for (int i = 0; i < Client_Count; i++)
	{
		if (!strcmp(ClientInfo[i]->nickname, _nick))
		{
			LeaveCriticalSection(&cs);
			return ClientInfo[i];
		}
	}

	LeaveCriticalSection(&cs);

	return nullptr;
}

bool Server_Manager::NicknameCheck(const char* _nick)
{
	for (int i = 0; i < Nick_Count; i++)
	{
		if (!strcmp(NickNameList[i], _nick))
		{
			return false;
		}
	}

	return true;
}

void Server_Manager::AddNickName(const char* _nick)
{
	char* ptr = new char[strlen(_nick) + 1];
	strcpy(ptr, _nick);

	EnterCriticalSection(&cs);
	NickNameList[Nick_Count++] = ptr;
	LeaveCriticalSection(&cs);
}

void Server_Manager::RemoveNickName(const char* _nick)
{
	for (int i = 0; i < Nick_Count; i++)
	{
		if (!strcmp(NickNameList[i], _nick))
		{
			delete[] NickNameList[i];

			for (int j = i; j < Nick_Count - 1; j++)
			{
				NickNameList[j] = NickNameList[j + 1];
			}
			NickNameList[Nick_Count--] = nullptr;
			break;
		}
	}
}

bool Server_Manager::NickNameSetting(_ClientInfo* _clientinfo)
{
	char message[BUFSIZE] = { 0 };
	char nName[BUFSIZE] = { 0 };
	int size = 0;

	Packet_Utility::UnPackPacket(_clientinfo->recvbuf, nName); // 닉네임 획득

	if (!strcmp(_clientinfo->nickname, nName)) // 현재 닉네임과 곂치는지 확인
	{
		return false;
	}

	EnterCriticalSection(&cs);
	if (!NicknameCheck(nName))
	{
		LeaveCriticalSection(&cs);
		return false;
	}

	RemoveNickName(_clientinfo->nickname);

	strcpy(_clientinfo->nickname, nName);

	for (int i = 0; i < Client_Count; i++)
	{
		int size = 0;
		size = Packet_Utility::PackPacket(ClientInfo[i]->sendbuf, PROTOCOL::NICKNAME_COMPLETE, message);
		send(ClientInfo[i]->sock, ClientInfo[i]->sendbuf, size, 0);
	}

	LeaveCriticalSection(&cs);

	AddNickName(nName);

	return true;
}

void Server_Manager::ChattingEnterProcess(_ClientInfo* _clientinfo)
{
	EnterCriticalSection(&cs);

	NickNameUpdate();

	LeaveCriticalSection(&cs);
}

void Server_Manager::ChattingOutProcess(_ClientInfo* _clientinfo)
{
	char message[BUFSIZE] = { 0 };
	char nName[BUFSIZE] = { 0 };
	int size = 0;

	Packet_Utility::UnPackPacket(_clientinfo->recvbuf, nName); // 닉네임 획득

	EnterCriticalSection(&cs);
	for (int i = 0; i < Client_Count; i++)
	{
		int size = 0;
		size = Packet_Utility::PackPacket(ClientInfo[i]->sendbuf, PROTOCOL::CHATT_OUT, message);
		send(ClientInfo[i]->sock, ClientInfo[i]->sendbuf, size, 0);
	}

	LeaveCriticalSection(&cs);
}

void Server_Manager::ChattingMessageProcess(_ClientInfo* _clientinfo)
{
	char chatText[BUFSIZE] = { 0 };
	char message[BUFSIZE] = { 0 };
	int size = 0;

	strcpy(message, _clientinfo->nickname);
	strcat(message, ": ");

	EnterCriticalSection(&cs);
	Packet_Utility::UnPackPacket(_clientinfo->recvbuf, chatText); // 채팅 내용 획득
	strcat(message, chatText);

	for (int i = 0; i < Client_Count; i++)
	{
		int size = 0;
		size = Packet_Utility::PackPacket(ClientInfo[i]->sendbuf, PROTOCOL::CHATT_MSG, message);
		send(ClientInfo[i]->sock, ClientInfo[i]->sendbuf, size, 0);
	}

	LeaveCriticalSection(&cs);
}

void Server_Manager::NickNameUpdate()
{
	for (int i = 0; i < Nick_Count; i++)
	{
		int size = 0;
		size = Packet_Utility::PackPacket(ClientInfo[i]->sendbuf, PROTOCOL::NICKNAME_LIST, NickNameList, Nick_Count);
		send(ClientInfo[i]->sock, ClientInfo[i]->sendbuf, size, 0);
	}
}
