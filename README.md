# chat_service

소개: Window API GUi를 이용한 기본적인 채팅 프로그램입니다.

## 사용 기술
Window API GUI, MFC, C++, 멀티 스레드, 크리티컬 섹션, 소켓, TCP

## How to build
- C++14을 지원하는 Visual studio IDE 설치

- __#include "afxres.h"__ 가 없을 경우 

```text
제어판-visual studio 변경 클릭 - 수정 클릭 - 최신 v142 빌드 도구용 C++ MFC (x86 및 x64) 를 클릭해서 수정해주면 해결

출처: https://kangjik94.tistory.com/65
```

- 빌드 및 실행하기
git clone을 받은 후 
chat_service/Chatting_Server/Chatting_Server.sln
chat_service/GUITCPClient/GUITCPClient.sln
을 실행합니다.

__64비트__ 환경에서 __Debug__ 모드로 솔루션을 빌드합니다.

chat_service/x64/Debug/Chatting_Server.exe로 __서버를__ 실행합니다.

chat_service/GUITCPClient/Debug/64/GUITCPClient.exe로 10개 이하의 클라이언트를 실행합니다.

## GUITCPClient
- 채팅을 주고 받는 프로그램입니다.
- 닉네임을 입력하고 접속 버튼을 누르면 서버에 접속이 됩니다.

- __GUI__
    - message: 로그 메세지 출력
    - list: 현재 접속중인 유저 목록
    - chat: 채팅 메세지 출력


- __ClientManager__ 
    - 클라이언트 소켓의 초기화 및 서버의 연결을 담당 합니다.
    - 현재 클라이언트의 상태를 가지고 있습니다.
    - 클라이언트의 송수신 버퍼를 관리합니다.

- __ClientMain__
    - 클라이언트의 GUI 이벤트 관리 쓰레드입니다.

- __RecvThread__
    - 클라이언트의 상태 및 프로토콜에 따라 데이터를 송수신하여 처리하는 쓰레드입니다.


## 패킷

__프로토콜__

- __enum PROTOCOL__
 클라이언트와 주고 받는 패킷에 포함되어 있으며 어떤 요청인지 확인하기 위한 정보입니다.

```CPP
enum PROTOCOL
{
	INTRO, // 생성
	CHATT_NICKNAME, // 접속 및닉네임 세팅 요청
	NICKNAME_EROR, // 닉네임 세팅 에러
	NICKNAME_COMPLETE, // 닉네임 세팅 완료 
	NICKNAME_LIST, // 전체 닉네임 리스트 요청
	CHATT_ENTER, // 다른 유저 채팅방 접속
	CHATT_MSG, // 채팅 메세지 수신
	CHATT_OUT // 다른 유저 채팅방 접속 해제

};
```

__패킷 송수신 [Packet_Utility.h]__
- 송신 형태
    - [PROTOCOL][char*] : 프로토콜 정보와 문자열 정보를 전송합니다.
    - [PROTOCOL][listSize][strSize][char* str1][strSize][char* str1]... : 프로토콜 정보와 리스트 형태의 문자열 정보를 전송합니다.

- 수신 형태
    - [PROTOCOL][char*] : 프로토콜과 단일 문자열 형태의 데이터 수신합니다.
    - [PROTOCOL][vector] : 프로토콜과 리스트 형태의 문자열을 vector컨테이너로 수신합니다.

## 클라이언트 상태

상태 패턴을 이용하여 클라이언트의 상태마다 다른 행동을 할 수 있게 구현했습니다.

```CPP
enum STATE
{
	INITE_STATE, // 생성중 
	CHATT_INITE_STATE, // 접속 전
	CHATTING_STATE, // 채팅 가능 상태
	CHATT_OUT_STATE, // 채팅방 나간 상태
	CONNECT_END_STATE // 접속 종료 상태
};
```

클라이언트 상태가 추가될 시 서버에서 쉽게 상태에 따른 행동을 추가 할 수 있게 다음과 같이 함수객체를 이용하여 unordered_map으로 함수를 호출하는 방식으로 구현했습니다.

```CPP
ClientInfo* Client_ptr = static_cast<ClientInfo*>(_ptr);
	PROTOCOL protocol{};

	unordered_map<int, function<bool(ClientInfo* _clientInfo)>> functionMap;

	functionMap.insert({ INITE_STATE,  ClientIniteState() });
	functionMap.insert({ CHATT_INITE_STATE, ChattIniteState() });
	functionMap.insert({ CHATTING_STATE, ChattingState() });
	functionMap.insert({ CONNECT_END_STATE, ConnectEndState() });

	bool canDoflag = true;
	while (canDoflag)
	{
		canDoflag = functionMap[Client_ptr->GetState()](Client_ptr);
	}
```
