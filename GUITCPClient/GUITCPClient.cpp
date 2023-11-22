#pragma comment(lib, "ws2_32")
#include "resource.h"
#include "Packet_Utility.h"
#include "Function.h"
#include "ClientManager.h"

using namespace std;

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

char nickname[NICKNAMESIZE]; // �г��� 
char chatMessage[BUFSIZE]; // ä�� �޼���

HANDLE hReadEvent, hWriteEvent; // �̺�Ʈ
HANDLE hNameEvent; // �̺�Ʈ

HWND hSendButton; // ������ ��ư
HWND hMessage, hLog; // ���� ��Ʈ��
HWND hName; // �̸� ��Ʈ��
HWND hNameCheck; // �̸� üũ ��Ʈ��
HWND hUserList; // ���� ����Ʈ ��Ʈ��
HWND hChat; // ä�� ȭ�� ��Ʈ��

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// �̺�Ʈ ����
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hWriteEvent == NULL) return 1;

	ClientManager::GetInstance().ClientMainThreadRun();

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	ClientManager::GetInstance().WaitClientMain();

	// �̺�Ʈ ����
	CloseHandle(hWriteEvent);
	CloseHandle(hReadEvent);
	
	ClientManager::GetInstance().Destroy();
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
		ClientManager::GetInstance().SetState(CHATT_INITE_STATE);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
			if (ClientManager::GetInstance().GetState() != CHATTING_STATE)
				return TRUE;
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_MESSAGE, chatMessage, BUFSIZE);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			ClientManager::GetInstance().DataSendToServer(PROTOCOL::CHATT_MSG, chatMessage);
			SetWindowText(hMessage, "");
			SetFocus(hMessage);			
			return TRUE;
		case IDC_NAMECHECK:
			// connect()
			if (false == ClientManager::GetInstance().Connect())
				return TRUE;

			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_NAME, nickname, NICKNAMESIZE);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			ClientManager::GetInstance().DataSendToServer(PROTOCOL::CHATT_NICKNAME, nickname);
			ClientManager::GetInstance().SetState(CHATTING_STATE);
			return TRUE;
		case IDCANCEL:
			//ä�� ���� �˸���
			ClientManager::GetInstance().DataSendToServer(PROTOCOL::CHATT_OUT, nickname);
			ClientManager::GetInstance().SetState(CHATT_OUT_STATE);
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		ClientManager::GetInstance().DataSendToServer(PROTOCOL::CHATT_OUT, nickname);
		ClientManager::GetInstance().SetState(CHATT_OUT_STATE);
		EndDialog(hDlg, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}
