#pragma comment(lib, "ws2_32")
#include "resource.h"
#include "Packet_Utility.h"
#include "Function.h"
#include "ClientManager.h"

using namespace std;

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

char nickname[NICKNAMESIZE]; // 닉네임 
char chatMessage[BUFSIZE]; // 채팅 메세지

HANDLE hReadEvent, hWriteEvent; // 이벤트
HANDLE hNameEvent; // 이벤트

HWND hSendButton; // 보내기 버튼
HWND hMessage, hLog; // 편집 컨트롤
HWND hName; // 이름 컨트롤
HWND hNameCheck; // 이름 체크 컨트롤
HWND hUserList; // 유저 리스트 컨트롤
HWND hChat; // 채팅 화면 컨트롤

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hWriteEvent == NULL) return 1;

	ClientManager::GetInstance().ClientMainThreadRun();

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	ClientManager::GetInstance().WaitAllThread();

	// 이벤트 제거
	CloseHandle(hWriteEvent);
	CloseHandle(hReadEvent);
	
	ClientManager::GetInstance().Destroy();
	return 0;
}

// 대화상자 프로시저
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
		ClientManager::GetInstance().RecvThreadRun();
		ClientManager::GetInstance().SetState(CHATT_INITE_STATE);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
			if (ClientManager::GetInstance().GetState() != CHATTING_STATE)
				return TRUE;
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_MESSAGE, chatMessage, BUFSIZE-5);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			ClientManager::GetInstance().DataSendToServer(PROTOCOL::CHATT_MSG, chatMessage);
			SetWindowText(hMessage, "");
			SetFocus(hMessage);			
			return TRUE;
		case IDC_NAMECHECK:
			// connect()
			if (ClientManager::GetInstance().GetState() != CHATT_INITE_STATE)
				return TRUE;

			if (false == ClientManager::GetInstance().Connect())
				return TRUE;

			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_NAME, nickname, NICKNAMESIZE);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			ClientManager::GetInstance().DataSendToServer(PROTOCOL::CHATT_NICKNAME, nickname);
			ClientManager::GetInstance().SetState(CHATTING_STATE);
			return TRUE;
		case IDCANCEL:
			//채팅 종료 알리기
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
