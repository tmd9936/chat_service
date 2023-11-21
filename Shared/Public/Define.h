#pragma once

#include <stdio.h>
#include <string>
#include <winsock2.h>
#include <stdlib.h>
#include <commctrl.h>
#include <vector>
#include <unordered_map>

typedef unsigned int ClientID;

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 4096
#define NICKNAMESIZE 255

#define MAXUSER   10
#define NODATA   -1

#define CLIENT_ID_MAX 100000

#define INTRO_MSG "채팅프로그램입니다. 닉네임을 입력하세요"
#define NICKNAME_ERROR_MSG "이미있는 닉네임입니다."