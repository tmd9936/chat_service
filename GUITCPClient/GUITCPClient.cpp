#pragma comment(lib, "ws2_32")
#include "resource.h"
#include "Packet_Utility.h"
#include "Function.h"

using namespace std;

struct _MyInfo
{
	SOCKET sock;
	STATE state;
	char sendbuf[BUFSIZE];
	char recvbuf[BUFSIZE];
}*MyInfo;

char nickname[NICKNAMESIZE];

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...);

// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID);
DWORD CALLBACK RecvThread(LPVOID);

void UpdateUserList(char*);

char buf[BUFSIZE+1]; // 데이터 송수신 버퍼
char chatMessage[BUFSIZE]; // 채팅 메세진

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

	CloseHandle(hName);
	CloseHandle(hNameCheck);

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
			GetDlgItemText(hDlg, IDC_MESSAGE, chatMessage, BUFSIZE);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			size = Packet_Utility::PackPacket(buf, PROTOCOL::CHATT_MSG, chatMessage);
			send(MyInfo->sock, buf, size, 0);
			SetWindowText(hMessage, "");
			SetFocus(hMessage);			
			return TRUE;
		case IDC_NAMECHECK:
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_NAME, nickname, NICKNAMESIZE);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			size = Packet_Utility::PackPacket(buf, PROTOCOL::CHATT_NICKNAME, nickname);
			send(MyInfo->sock, buf, size, 0);
			return TRUE;
		case IDCANCEL:
			//채팅 종료 알리기
			size = Packet_Utility::PackPacket(buf, PROTOCOL::CHATT_OUT, nickname);
			send(MyInfo->sock, buf, size, 0);
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		size = Packet_Utility::PackPacket(buf, PROTOCOL::CHATT_OUT, nickname);
		send(MyInfo->sock, buf, size, 0);
		PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}

// 편집 컨트롤 출력 함수
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

DWORD CALLBACK RecvThread(LPVOID _ptr)
{
	PROTOCOL protocol{};
	int size = 0;	
	char nickname[NICKNAMESIZE] = { 0 };
	char msg[BUFSIZE] = { 0 };
	int count = 0;	

	while (1)
	{
		if (!Packet_Utility::PacketRecv(MyInfo->sock, MyInfo->recvbuf))
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
				EnableWindow(hName, FALSE); // 이름 에딧 텍스트 비활성화
				EnableWindow(hNameCheck, FALSE); // 이름 체크 버튼 비활성화
				DisplayText(hLog, "NickName Set Complete\r\n");
			}
			else
			{
				memset(msg, 0, BUFSIZE);
				Packet_Utility::UnPackPacket(MyInfo->recvbuf, msg);
				DisplayText(hLog, "%s\r\n", msg);
			}
			break;

		case NICKNAME_LIST:
			UpdateUserList(MyInfo->recvbuf);
			break;

		case CHATT_MSG:
			memset(msg, 0, BUFSIZE);
			Packet_Utility::UnPackPacket(MyInfo->recvbuf, msg);
			DisplayText(hChat, "%s\r\n", msg);
			break;

		case CHATT_OUT:
			memset(msg, 0, BUFSIZE);
			Packet_Utility::UnPackPacket(MyInfo->recvbuf, msg);
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
	Packet_Utility::UnPackPacket(_buf, data, count);

	SendMessage(hUserList, LB_RESETCONTENT, 0, 0);
	for (int i = 0; i < count; i++)
	{
		int pos = (int)SendMessage(hUserList, LB_ADDSTRING, 0,
			(LPARAM)data[i].c_str());
		SendMessage(hUserList, LB_SETITEMDATA, pos, (LPARAM)i);
	}
}