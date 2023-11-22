#include "ClientManager.h"
#include "Function.h"
#include "Packet_Utility.h"
#include "Client_Define.h"

// TCP Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	ClientManager::GetInstance().Initialize();

	bool endflag = false;

	STATE state = INITE_STATE;

	while (1)
	{
		WaitForSingleObject(hWriteEvent, 1000); // ���� �Ϸ� ��ٸ���

		state = ClientManager::GetInstance().GetState();

		// ���ڿ� ���̰� 0�̸� ������ ����
		if (state != CHATT_OUT_STATE && ClientManager::GetInstance().GetSendBufSize() == 0)
		{
			EnableWindow(hSendButton, FALSE); // ������ ��ư Ȱ��ȭ
			SetEvent(hReadEvent); // �б� �Ϸ� �˸���
			continue;
		}

		switch (state)
		{
		case CHATT_INITE_STATE:
			break;

		case CHATTING_STATE:
			break;

		case CHATT_OUT_STATE:
			endflag = true;
			break;
		}

		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		SetEvent(hReadEvent); // �б� �Ϸ� �˸���

		if (endflag)
		{
			ClientManager::GetInstance().SetIsEnd(true);
			break;
		}
	}

	return 0;
}

DWORD CALLBACK RecvThread(LPVOID _ptr)
{
	PROTOCOL protocol{};
	int size = 0;
	char msg[BUFSIZE] = { 0 };
	int count = 0;

	while (1)
	{
		if (ClientManager::GetInstance().GetState() == CHATT_INITE_STATE)
			continue;

		if (!ClientManager::GetInstance().ClientPackRecv())
		{
			Function::err_display("recv error()");
			return -1;
		}

		protocol = ClientManager::GetInstance().GetProtocol();

		switch (protocol)
		{
		case INTRO:
			break;

		case NICKNAME_EROR:
			ClientManager::GetInstance().DisplayText(hLog, "NickName Set Error\r\n");
			break;

		case NICKNAME_COMPLETE:
			EnableWindow(hName, FALSE); // �̸� ���� �ؽ�Ʈ ��Ȱ��ȭ
			EnableWindow(hNameCheck, FALSE); // �̸� üũ ��ư ��Ȱ��ȭ
			ClientManager::GetInstance().DisplayText(hLog, "NickName Set Complete\r\n");
			break;

		case NICKNAME_LIST:
			ClientManager::GetInstance().UpdateUserList();
			break;

		case CHATT_ENTER:
			memset(msg, 0, BUFSIZE);
			ClientManager::GetInstance().UnPackPacket(msg);
			SendMessage(hUserList, LB_ADDSTRING, 0, (LPARAM)msg);
			ClientManager::GetInstance().DisplayText(hLog, "%s enter this chatroom\r\n", msg);
			break;

		case CHATT_MSG:
			memset(msg, 0, BUFSIZE);
			ClientManager::GetInstance().UnPackPacket(msg);
			ClientManager::GetInstance().DisplayText(hChat, "%s\r\n", msg);
			break;

		case CHATT_OUT:
			memset(msg, 0, BUFSIZE);
			ClientManager::GetInstance().UnPackPacket(msg);
			SendMessage(hUserList, LB_DELETESTRING, 0, (LPARAM)msg);
			ClientManager::GetInstance().DisplayText(hLog, "%s out this chatroom\r\n", msg);
			break;
		}

		if (ClientManager::GetInstance().GetIsEnd())
			break;
	}

	return 0;
}

// ���� ��Ʈ�� ��� �Լ�
void ClientManager::DisplayText(HWND _hWnd, char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256] = { 0 };
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(_hWnd);
	SendMessage(_hWnd, EM_SETSEL, nLength, nLength);
	SendMessage(_hWnd, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

void ClientManager::UpdateUserList()
{
	vector<string> data;
	int count = 0;
	Packet_Utility::UnPackPacket(recvbuf, data, count);

	SendMessage(hUserList, LB_RESETCONTENT, 0, 0);
	for (int i = 0; i < count; i++)
	{
		int pos = (int)SendMessage(hUserList, LB_ADDSTRING, 0,
			(LPARAM)data[i].c_str());
		SendMessage(hUserList, LB_SETITEMDATA, pos, (LPARAM)i);
	}
}

bool ClientManager::Initialize()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	// socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		Function::err_quit("socket()");
		return false;
	}
	return true;
}

bool ClientManager::Connect()
{
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	int retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		Function::err_quit("connect()");
		return false;
	}

	return true;
}

void ClientManager::Destroy()
{
	CloseHandle(hClientMain);
	CloseHandle(hRecvThread);

	closesocket(sock);
	WSACleanup();
}

void ClientManager::WaitAllThread()
{
	WaitForSingleObject(hClientMain, INFINITE);
	WaitForSingleObject(hRecvThread, INFINITE);
}

int ClientManager::DataSendToServer(PROTOCOL _protocol, char* _str)
{
	int size = 0;
	int reusult = 0;
	size = Packet_Utility::PackPacket(sendbuf, _protocol, _str);
	reusult = send(sock, sendbuf, size, 0);

	return reusult;
}

void ClientManager::ClientMainThreadRun()
{
	hClientMain = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
}

void ClientManager::RecvThreadRun()
{
	hRecvThread = CreateThread(NULL, 0, RecvThread, NULL, 0, NULL);
}

bool ClientManager::ClientPackRecv()
{
	if (!Function::PacketRecv(sock, recvbuf))
		return false;

	return true;
}

PROTOCOL ClientManager::GetProtocol()
{
	return Function::GetProtocol(recvbuf);
}

void ClientManager::UnPackPacket(char* _buf)
{
	if (_buf == nullptr)
		return;

	Packet_Utility::UnPackPacket(recvbuf, _buf);
}
