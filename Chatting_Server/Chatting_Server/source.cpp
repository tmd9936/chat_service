#pragma comment(lib, "ws2_32.lib")

#include "Packet_Utility.h"
#include "Function.h"

#define MAXUSER   10
#define NODATA   -1

#define INTRO_MSG "채팅프로그램입니다. 닉네임을 입력하세요"
#define NICKNAME_ERROR_MSG "이미있는 닉네임입니다."

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

DWORD CALLBACK ProcessClient(LPVOID);

_ClientInfo* SearchClient(const char*); //닉네임으로 유저찾기
bool NicknameCheck(const char*);// 닉네임 중복 체크
void MaKeChattMessage(const char* , const char* , char* );
void MakeEnterMessage(const char* , char* );
void MakeExitMessage(const char* , char* );


_ClientInfo* AddClient(SOCKET sock, SOCKADDR_IN clientaddr);
void RemoveClient(_ClientInfo* ptr);

void AddNickName(const char*);
void RemoveNickName(const char*);

bool NickNameSetting(_ClientInfo*);
void ChattingMessageProcess(_ClientInfo*);
void ChattingOutProcess(_ClientInfo*);
void ChattingEnterProcess(_ClientInfo*);

void NickNameUpdate();

_ClientInfo* ClientInfo[MAXUSER];
char* NickNameList[MAXUSER];

int Client_Count = 0;
int Nick_Count = 0;

CRITICAL_SECTION cs;

int main(int argc, char **argv)
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;
	InitializeCriticalSection(&cs);
	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	serveraddr.sin_addr.s_addr =inet_addr(SERVERIP);
	int retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수		
	int addrlen;
	SOCKET sock;
	SOCKADDR_IN clientaddr;	
	
	while (1)
	{
		addrlen = sizeof(clientaddr);

		sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (sock == INVALID_SOCKET)
		{
			err_display("accept()");
			continue;
		}

		_ClientInfo* ptr = AddClient(sock, clientaddr);
		
		HANDLE hThread=CreateThread(NULL, 0, ProcessClient, ptr, 0, nullptr);	
		if (hThread != NULL)
		{
			CloseHandle(hThread);
		}

	}

	closesocket(listen_sock);
	DeleteCriticalSection(&cs);
	WSACleanup();
	return 0;
}


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
				err_display("intro Send()");
				Client_ptr->state = CONNECT_END_STATE;	
				break;
			}
			Client_ptr->state = CHATT_INITE_STATE;
			break;
		case CHATT_INITE_STATE:
			if (!PacketRecv(Client_ptr->sock, Client_ptr->recvbuf))
			{
				Client_ptr->state = CONNECT_END_STATE;
				break;
			}

			protocol = GetProtocol(Client_ptr->recvbuf);

			switch (protocol)
			{
			case CHATT_NICKNAME:
				isNicNameSet = NickNameSetting(Client_ptr);
				break;
			}
			
			if (isNicNameSet)
			{
				ChattingEnterProcess(Client_ptr);

				if (Client_ptr->state != CONNECT_END_STATE)
				{
					Client_ptr->state = CHATTING_STATE;
				}
			}

			break;
		case CHATTING_STATE:
			if (!PacketRecv(Client_ptr->sock, Client_ptr->recvbuf))
			{
				Client_ptr->state = CONNECT_END_STATE;
				break;
			}

			protocol = GetProtocol(Client_ptr->recvbuf);

			switch (protocol)
			{
			case CHATT_MSG:
				
				ChattingMessageProcess(Client_ptr);
				break;
			case CHATT_OUT:
					
				ChattingOutProcess(Client_ptr);
				Client_ptr->state = CONNECT_END_STATE;
				break;
			}
			break;
		case CONNECT_END_STATE:
			RemoveClient(Client_ptr);
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


_ClientInfo* AddClient(SOCKET sock, SOCKADDR_IN clientaddr)
{
	printf("\nClient 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clientaddr.sin_addr),
		ntohs(clientaddr.sin_port));

	EnterCriticalSection(&cs);
	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));
	ptr->sock = sock;
	memcpy(&(ptr->clientaddr), &clientaddr, sizeof(clientaddr));
	ptr->state = INITE_STATE;	
	ptr->chatflag = false;
	ClientInfo[Client_Count++] = ptr;
	
	LeaveCriticalSection(&cs);
	return ptr;
}

void RemoveClient(_ClientInfo* ptr)
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

	NickNameUpdate();
	LeaveCriticalSection(&cs);
}

_ClientInfo* SearchClient(const char* _nick)
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


bool NicknameCheck(const char* _nick)
{
	EnterCriticalSection(&cs);
	for (int i = 0; i < Nick_Count; i++)
	{
		if (!strcmp(NickNameList[i], _nick))
		{
			LeaveCriticalSection(&cs);
			return false;
		}
	}
	LeaveCriticalSection(&cs);

	return true;
}

void AddNickName(const char* _nick)
{
	EnterCriticalSection(&cs);
	char* ptr = new char[strlen(_nick) + 1];
	strcpy(ptr, _nick);
	NickNameList[Nick_Count++] = ptr;
	LeaveCriticalSection(&cs);
}

void RemoveNickName(const char* _nick)
{
	EnterCriticalSection(&cs);
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
	LeaveCriticalSection(&cs);
}

void MaKeChattMessage(const char* _nick, const char* _msg, char* _chattmsg)
{
	sprintf(_chattmsg, "[ %s ] %s", _nick, _msg);
}

void MakeEnterMessage(const char* _nick, char* _msg)
{
	sprintf(_msg, "%s님이 입장하셨습니다.", _nick);
}
void MakeExitMessage(const char* _nick, char* _msg)
{
	sprintf(_msg, "%s님이 퇴장하셨습니다.", _nick);
}


bool NickNameSetting(_ClientInfo* _clientinfo)
{
	EnterCriticalSection(&cs);

	char nName[BUFSIZE] = { 0 };
	int size = 0;
	
	Packet_Utility::UnPackPacket(_clientinfo->recvbuf, nName); // 닉네임 획득

	if (!strcmp(_clientinfo->nickname, nName)) // 현재 닉네임과 곂치는지 확인
	{
		LeaveCriticalSection(&cs);
		return false;
	}

	if (!NicknameCheck(nName) ) // 이미 존재하는 닉네임인지 확인
	{
		LeaveCriticalSection(&cs);
		//size = PackPacket(_clientinfo->sendbuf, PROTOCOL::NICKNAME_EROR, "NickName Set Error");
		//send(_clientinfo->sock, _clientinfo->sendbuf, size, 0);
		return false;
	}

	RemoveNickName(_clientinfo->nickname);

	strcpy(_clientinfo->nickname, nName);
	AddNickName(nName);

	char message[BUFSIZE] = { 0 };
	MakeEnterMessage(nName, message);
	for (int i = 0; i < Client_Count; i++)
	{
		int size = 0;
		size = Packet_Utility::PackPacket(ClientInfo[i]->sendbuf, PROTOCOL::NICKNAME_COMPLETE, message);
		send(ClientInfo[i]->sock, ClientInfo[i]->sendbuf, size, 0);
	}

	LeaveCriticalSection(&cs);	

	return true;
}

void ChattingMessageProcess(_ClientInfo* _clientinfo)
{
	EnterCriticalSection(&cs);

	char chatText[BUFSIZE] = { 0 };
	char message[BUFSIZE] = { 0 };
	int size = 0;

	strcpy(message, _clientinfo->nickname);
	strcat(message, ": ");

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

void ChattingEnterProcess(_ClientInfo* _clientinfo)
{
	EnterCriticalSection(&cs);

	NickNameUpdate();

	LeaveCriticalSection(&cs);
}

void ChattingOutProcess(_ClientInfo* _clientinfo)
{
	EnterCriticalSection(&cs);

	char nName[BUFSIZE] = { 0 };
	int size = 0;

	Packet_Utility::UnPackPacket(_clientinfo->recvbuf, nName); // 닉네임 획득

	char message[BUFSIZE] = { 0 };
	MakeExitMessage(nName, message);

	for (int i = 0; i < Client_Count; i++)
	{
		int size = 0;
		size = Packet_Utility::PackPacket(ClientInfo[i]->sendbuf, PROTOCOL::CHATT_OUT, message);
		send(ClientInfo[i]->sock, ClientInfo[i]->sendbuf, size, 0);
	}

	LeaveCriticalSection(&cs);
}

void NickNameUpdate()
{
	for (int i = 0; i < Nick_Count; i++)
	{
		int size = 0;
		size = Packet_Utility::PackPacket(ClientInfo[i]->sendbuf, PROTOCOL::NICKNAME_LIST, NickNameList, Nick_Count);
		send(ClientInfo[i]->sock, ClientInfo[i]->sendbuf, size, 0);
	}
}