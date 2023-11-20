#pragma once

#include <stdio.h>
#include <string>
#include <winsock2.h>
#include <stdlib.h>
#include <commctrl.h>
#include <vector>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 4096
#define NICKNAMESIZE 255

enum STATE
{
	INITE_STATE,
	INTRO_STATE,
	CHATT_INITE_STATE,
	CHATTING_STATE,
	CHATT_OUT_STATE,
	CONNECT_END_STATE
};

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