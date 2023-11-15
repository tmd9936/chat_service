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

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...);
// 오류 출력 함수
void err_quit(const char*);
void err_display(const char*);
// 사용자 정의 데이터 수신 함수
int recvn(SOCKET , char*, int, int);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID);
DWORD CALLBACK RecvThread(LPVOID);

void UpdateUserList(char*);

PROTOCOL GetProtocol(const char*);
int PackPacket(char*, PROTOCOL, const char*);

void UnPackPacket(const char*, char*, int&);
void UnPackPacket(const char*, char*);
void UnPackPacket(const char*, vector<string>&, int&);

char buf[BUFSIZE+1]; // 데이터 송수신 버퍼
HANDLE hReadEvent, hWriteEvent; // 이벤트
HANDLE hNameEvent; // 이벤트

HWND hSendButton; // 보내기 버튼
HWND hMessage, hLog; // 편집 컨트롤
HWND hName; // 이름 컨트롤
HWND hNameCheck; // 이름 체크 컨트롤
HWND hUserList; // 유저 리스트 컨트롤
HWND hChat; // 채팅 화면 컨트롤

HANDLE hClientMain, hRecvThread;

bool isNickNameOK = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hWriteEvent == NULL) return 1;
		
	// 소켓 통신 스레드 생성	
	// 대화상자 생성

	MyInfo = new _MyInfo;
	memset(MyInfo, 0, sizeof(_MyInfo));

	hClientMain = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);	

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	WaitForSingleObject(hClientMain, 1000);

	// 이벤트 제거
	CloseHandle(hWriteEvent);
	CloseHandle(hReadEvent);
	//CloseHandle(hNameEvent);

	CloseHandle(hClientMain);
	CloseHandle(hRecvThread);

	//CloseHandle(hName);
	//CloseHandle(hNameCheck);

	// closesocket()
	closesocket(MyInfo->sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 대화상자 프로시저
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
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_MESSAGE, buf, BUFSIZE+1);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			SetWindowText(hMessage, "");
			SetFocus(hMessage);			
			return TRUE;
		case IDC_NAMECHECK:
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_NAME, nickname, 1024);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			size = PackPacket(buf, PROTOCOL::CHATT_NICKNAME, nickname);
			send(MyInfo->sock, buf, size, 0);
			return TRUE;
		case IDCANCEL:
			//채팅 종료 알리기
			EndDialog(hDlg, IDCANCEL);
			if (MyInfo)
				MyInfo->state = CHATT_OUT_STATE;
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		if (MyInfo)
			MyInfo->state = CHATT_OUT_STATE;
		PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}

// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE+256];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(hLog);
	SendMessage(hLog, EM_SETSEL, nLength, nLength);
	SendMessage(hLog, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
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

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

	// 윈속 초기화
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
		WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 기다리기

		// 문자열 길이가 0이면 보내지 않음
		if(MyInfo->state!= CHATT_OUT_STATE && strlen(buf) == 0)
		{
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent); // 읽기 완료 알리기
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
		
		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SetEvent(hReadEvent); // 읽기 완료 알리기

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
			DisplayText("NickName Set Error");
			break;

		case NICKNAME_COMPLETE:
			isNickNameOK = true;
			EnableWindow(hName, FALSE); // 이름 에딧 텍스트 비활성화
			EnableWindow(hNameCheck, FALSE); // 이름 체크 버튼 비활성화
			DisplayText("NickName Set Complete");
			break;

		case NICKNAME_LIST:
			UpdateUserList(MyInfo->recvbuf);
			break;

		case CHATT_MSG:
			
			break;
		}


		DisplayText("%s\r\n", msg);	

	}

	return 0;
}

void UpdateUserList(char* _buf)
{
	if (_buf == nullptr)
		return;

	int count = 0;
	//char str[BUFSIZE] = { 0 };
	vector<string> data;
	UnPackPacket(_buf, data, count);

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
