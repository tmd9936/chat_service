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

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);

// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID);
DWORD CALLBACK RecvThread(LPVOID);

void UpdateUserList(char*);

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

	WaitForSingleObject(hClientMain, INFINITE);

	// �̺�Ʈ ����
	CloseHandle(hWriteEvent);
	CloseHandle(hReadEvent);

	CloseHandle(hClientMain);
	CloseHandle(hRecvThread);

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
	int retval = 0;
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
		MyInfo->state = CHATT_INITE_STATE;
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			//WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_MESSAGE, chatMessage, BUFSIZE);
			//SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			size = Packet_Utility::PackPacket(buf, PROTOCOL::CHATT_MSG, chatMessage);
			send(MyInfo->sock, buf, size, 0);
			SetWindowText(hMessage, "");
			SetFocus(hMessage);			
			return TRUE;
		case IDC_NAMECHECK:
			// connect()
			SOCKADDR_IN serveraddr;
			ZeroMemory(&serveraddr, sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
			serveraddr.sin_port = htons(SERVERPORT);
			retval = connect(MyInfo->sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) Function::err_quit("connect()");
			//(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_NAME, nickname, NICKNAMESIZE);
			//SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			size = Packet_Utility::PackPacket(buf, PROTOCOL::CHATT_NICKNAME, nickname);
			send(MyInfo->sock, buf, size, 0);
			MyInfo->state = CHATTING_STATE;
			return TRUE;
		case IDCANCEL:
			//ä�� ���� �˸���
			size = Packet_Utility::PackPacket(buf, PROTOCOL::CHATT_OUT, nickname);
			send(MyInfo->sock, buf, size, 0);
			EndDialog(hDlg, IDCANCEL);
			MyInfo->state = CHATT_OUT_STATE;
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		size = Packet_Utility::PackPacket(buf, PROTOCOL::CHATT_OUT, nickname);
		send(MyInfo->sock, buf, size, 0);
		MyInfo->state = CHATT_OUT_STATE;
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

// TCP Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	MyInfo->sock = socket(AF_INET, SOCK_STREAM, 0);
	if(MyInfo->sock == INVALID_SOCKET) Function::err_quit("socket()");
	
	
	char msg[BUFSIZE] = { 0 };
	int size;

	bool endflag = false;

	while(1)
	{	
		WaitForSingleObject(hWriteEvent, 1000); // ���� �Ϸ� ��ٸ���

		// ���ڿ� ���̰� 0�̸� ������ ����
		if(MyInfo->state != CHATT_OUT_STATE && strlen(buf) == 0)
		{
			EnableWindow(hSendButton, FALSE); // ������ ��ư Ȱ��ȭ
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

DWORD CALLBACK RecvThread(LPVOID _ptr)
{
	PROTOCOL protocol{};
	int size = 0;	
	char nickname[NICKNAMESIZE] = { 0 };
	char msg[BUFSIZE] = { 0 };
	int count = 0;	

	int a = 0;

	while (1)
	{
		if (MyInfo->state == CHATT_INITE_STATE)
			continue;

		if (!Function::PacketRecv(MyInfo->sock, MyInfo->recvbuf))
		{
			Function::err_display("recv error()");
			return -1;
		}

		protocol= Function::GetProtocol(MyInfo->recvbuf);

		switch (protocol)
		{
		case INTRO:
			a = 0;
			break;

		case NICKNAME_EROR:
			DisplayText(hLog, "NickName Set Error\r\n");
			break;

		case NICKNAME_COMPLETE:
			EnableWindow(hName, FALSE); // �̸� ���� �ؽ�Ʈ ��Ȱ��ȭ
			EnableWindow(hNameCheck, FALSE); // �̸� üũ ��ư ��Ȱ��ȭ
			DisplayText(hLog, "NickName Set Complete\r\n");
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