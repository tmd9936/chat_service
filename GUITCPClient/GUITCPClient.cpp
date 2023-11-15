#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "resource.h"

using namespace std;

#define SERVERIP   "172.30.1.85"
#define SERVERPORT 9000
#define BUFSIZE 4096
#define NICKNAMESIZE 255

enum PROTOCOL
{
	INTRO,
	CHATT_NICKNAME,
	NICKNAME_EROR,
	NICKNAME_COMPLETE,
	NICKNAME_LIST,
	CHATT_MSG,
	CHATT_OUT,

};

enum STATE
{
	INITE_STATE,
	CHATT_INITE_STATE,
	CHATTING_STATE,
	CHATT_OUT_STATE
};

struct _MyInfo
{
	SOCKET sock;
	STATE state;
	char sendbuf[BUFSIZE];
	char recvbuf[BUFSIZE];
}*MyInfo;

char nickname[1024];

bool PacketRecv(SOCKET, char*);

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);
// ���� ��� �Լ�
void err_quit(const char*);
void err_display(const char*);
// ����� ���� ������ ���� �Լ�
int recvn(SOCKET , char*, int, int);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID);
DWORD CALLBACK RecvThread(LPVOID);

void UpdateUserList(char*);

PROTOCOL GetProtocol(const char*);
int PackPacket(char*, PROTOCOL, const char*);

void UnPackPacket(const char*, char*, int&);
void UnPackPacket(const char*, char*);
void UnPackPacket(const char*, vector<string>&, int&);

char buf[BUFSIZE+1]; // ������ �ۼ��� ����
char chatMessage[BUFSIZE]; // ä�� �޼���

HANDLE hReadEvent, hWriteEvent; // �̺�Ʈ
HANDLE hNameEvent; // �̺�Ʈ

HWND hSendButton; // ������ ��ư
HWND hMessage, hLog; // ���� ��Ʈ��
HWND hName; // �̸� ��Ʈ��
HWND hNameCheck; // �̸� üũ ��Ʈ��
HWND hUserList; // ���� ����Ʈ ��Ʈ��
HWND hChat; // ä�� ȭ�� ��Ʈ��

HANDLE hClientMain, hRecvThread;

bool isNickNameOK = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// �̺�Ʈ ����
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hWriteEvent == NULL) return 1;
		
	// ���� ��� ������ ����	
	// ��ȭ���� ����

	MyInfo = new _MyInfo;
	memset(MyInfo, 0, sizeof(_MyInfo));

	hClientMain = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);	

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	WaitForSingleObject(hClientMain, 1000);

	// �̺�Ʈ ����
	CloseHandle(hWriteEvent);
	CloseHandle(hReadEvent);
	//CloseHandle(hNameEvent);

	CloseHandle(hClientMain);
	CloseHandle(hRecvThread);

	//CloseHandle(hName);
	//CloseHandle(hNameCheck);

	// closesocket()
	closesocket(MyInfo->sock);

	// ���� ����
	WSACleanup();
	return 0;
}

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int size = 0;
	switch(uMsg){
	case WM_INITDIALOG:
		hMessage = GetDlgItem(hDlg, IDC_MESSAGE);
		hLog = GetDlgItem(hDlg, IDC_LOG);

		hName = GetDlgItem(hDlg, IDC_NAME);
		hNameCheck = GetDlgItem(hDlg, IDC_NAMECHECK);

		hUserList = GetDlgItem(hDlg, IDC_LIST1);
		hChat = GetDlgItem(hDlg, IDC_CHAT);

		hSendButton = GetDlgItem(hDlg, IDOK);
		SendMessage(hMessage, EM_SETLIMITTEXT, BUFSIZE, 0);	
		hRecvThread = CreateThread(NULL, 0, RecvThread, NULL, 0, NULL);
		MyInfo->state = INITE_STATE;
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
			if (!isNickNameOK)
				return TRUE;
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_MESSAGE, chatMessage, BUFSIZE);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			size = PackPacket(buf, PROTOCOL::CHATT_MSG, chatMessage);
			send(MyInfo->sock, buf, size, 0);
			SetWindowText(hMessage, "");
			SetFocus(hMessage);			
			return TRUE;
		case IDC_NAMECHECK:
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_NAME, nickname, 1024);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			size = PackPacket(buf, PROTOCOL::CHATT_NICKNAME, nickname);
			send(MyInfo->sock, buf, size, 0);
			return TRUE;
		case IDCANCEL:
			//ä�� ���� �˸���
			size = PackPacket(buf, PROTOCOL::CHATT_OUT, nickname);
			send(MyInfo->sock, buf, size, 0);
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		size = PackPacket(buf, PROTOCOL::CHATT_OUT, nickname);
		send(MyInfo->sock, buf, size, 0);
		PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(HWND _hWnd, char *fmt, ...)
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

// ���� �Լ� ���� ��� �� ����
void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(hLog, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText(hLog, "[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// TCP Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	MyInfo->sock = socket(AF_INET, SOCK_STREAM, 0);
	if(MyInfo->sock == INVALID_SOCKET) err_quit("socket()");
	
	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(MyInfo->sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("connect()");
	
	char msg[BUFSIZE] = { 0 };
	int size;

	bool endflag = false;

	while(1)
	{		
		WaitForSingleObject(hWriteEvent, INFINITE); // ���� �Ϸ� ��ٸ���

		// ���ڿ� ���̰� 0�̸� ������ ����
		if(MyInfo->state!= CHATT_OUT_STATE && strlen(buf) == 0)
		{
			EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
			SetEvent(hReadEvent); // �б� �Ϸ� �˸���
			continue;
		}

		switch (MyInfo->state)
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
			break;
		}
	}

	return 0;
}

bool PacketRecv(SOCKET _sock, char* _buf)
{
	int size;

	int retval = recvn(_sock, (char*)&size, sizeof(size), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv error()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	retval = recvn(_sock, _buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv error()");
		return false;

	}
	else if (retval == 0)
	{
		return false;
	}

	return true;
}

DWORD CALLBACK RecvThread(LPVOID _ptr)
{
	PROTOCOL protocol{};
	int size = 0;	
	char nickname[NICKNAMESIZE] = { 0 };
	char msg[BUFSIZE] = { 0 };
	int count = 0;	

	while (1)
	{
		if (!PacketRecv(MyInfo->sock, MyInfo->recvbuf))
		{
			err_display("recv error()");
			return -1;
		}

		protocol=GetProtocol(MyInfo->recvbuf);

		switch (protocol)
		{
		case INTRO:
			
			break;

		case NICKNAME_EROR:
			isNickNameOK = false;
			DisplayText(hLog, "NickName Set Error\r\n");
			break;

		case NICKNAME_COMPLETE:
			if (!isNickNameOK)
			{
				isNickNameOK = true;
				EnableWindow(hName, FALSE); // �̸� ���� �ؽ�Ʈ ��Ȱ��ȭ
				EnableWindow(hNameCheck, FALSE); // �̸� üũ ��ư ��Ȱ��ȭ
				DisplayText(hLog, "NickName Set Complete\r\n");
			}
			else
			{
				memset(msg, 0, BUFSIZE);
				UnPackPacket(MyInfo->recvbuf, msg);
				DisplayText(hLog, "%s\r\n", msg);
			}
			break;

		case NICKNAME_LIST:
			UpdateUserList(MyInfo->recvbuf);
			break;

		case CHATT_MSG:
			memset(msg, 0, BUFSIZE);
			UnPackPacket(MyInfo->recvbuf, msg);
			DisplayText(hChat, "%s\r\n", msg);
			break;

		case CHATT_OUT:
			memset(msg, 0, BUFSIZE);
			UnPackPacket(MyInfo->recvbuf, msg);
			DisplayText(hLog, "%s\r\n", msg);
			break;
		}

	}

	return 0;
}

void UpdateUserList(char* _buf)
{
	if (_buf == nullptr)
		return;

	vector<string> data;
	int count = 0;
	UnPackPacket(_buf, data, count);

	SendMessage(hUserList, LB_RESETCONTENT, 0, 0);
	for (int i = 0; i < count; i++)
	{
		int pos = (int)SendMessage(hUserList, LB_ADDSTRING, 0,
			(LPARAM)data[i].c_str());
		SendMessage(hUserList, LB_SETITEMDATA, pos, (LPARAM)i);
	}
}


PROTOCOL GetProtocol(const char* _ptr)
{
	PROTOCOL protocol;
	memcpy(&protocol, _ptr, sizeof(PROTOCOL));

	return protocol;
}

int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str1)
{
	char* ptr = _buf;
	int size = 0;
	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(size);

	int strsize1 = strlen(_str1);
	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	size = size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	size = size + strsize1;

	ptr = _buf;

	memcpy(ptr, &size, sizeof(size));
	size = size + sizeof(size);
	return size;
}


void UnPackPacket(const char* _buf, char* _str)
{
	int strsize;
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&strsize, ptr, sizeof(strsize));
	ptr = ptr + sizeof(strsize);

	memcpy(_str, ptr, strsize);
	ptr = ptr + strsize;
}


void UnPackPacket(const char* _buf, char* _str, int& _count)
{	
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_count, ptr, sizeof(_count));
	ptr = ptr + sizeof(_count);

	for (int i = 0; i < _count; i++)
	{
		int strsize;
		memcpy(&strsize, ptr, sizeof(strsize));
		ptr = ptr + sizeof(strsize);

		memcpy(_str, ptr, strsize);
		ptr = ptr + strsize;
		_str = _str + strsize;
		strcat(_str, ",");
		_str++;
	}

}

void UnPackPacket(const char* _buf, vector<string>& vec, int& _count)
{
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_count, ptr, sizeof(_count));
	ptr = ptr + sizeof(_count);

	vec.reserve(_count);

	char _str[BUFSIZE] = { 0 };
	for (int i = 0; i < _count; i++)
	{
		int strsize;
		memcpy(&strsize, ptr, sizeof(strsize));
		ptr = ptr + sizeof(strsize);

		memcpy(_str, ptr, strsize);
		ptr = ptr + strsize;
		string name = _str;

		vec.push_back(move(_str));
	}

}
