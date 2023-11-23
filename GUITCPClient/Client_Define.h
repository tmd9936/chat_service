#pragma once

#include "Define.h"

extern HANDLE hReadEvent, hWriteEvent; // 이벤트
extern HANDLE hNameEvent; // 이벤트

extern HWND hSendButton; // 보내기 버튼
extern HWND hMessage, hLog; // 편집 컨트롤
extern HWND hName; // 이름 컨트롤
extern HWND hNameCheck; // 이름 체크 컨트롤
extern HWND hUserList; // 유저 리스트 컨트롤
extern HWND hChat; // 채팅 화면 컨트롤