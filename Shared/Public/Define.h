#pragma once

#include <stdio.h>
#include <string>
#include <winsock2.h>
#include <stdlib.h>
#include <commctrl.h>
#include <vector>
#include <unordered_map>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 4096
#define NICKNAMESIZE 255

#define MAXUSER   10
#define NODATA   -1

#define CLIENT_ID_MAX 100000

#define INTRO_MSG "ä�����α׷��Դϴ�. �г����� �Է��ϼ���"
#define NICKNAME_ERROR_MSG "�̹��ִ� �г����Դϴ�."

using namespace std;