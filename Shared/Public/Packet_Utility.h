#pragma once

#include "Define.h"
#include "Enum.h"
#include "NoCopyObject.h"

using namespace std;

class Packet_Utility : public NoCopyObject
{
public:
	static void UnPackPacket(const char* _buf, char* _str);
	static int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str1);
	static int PackPacket(char* _buf, PROTOCOL _protocol, char** _strlist, int _count);
	static void UnPackPacket(const char* _buf, vector<string>& vec, int& _count);
};

