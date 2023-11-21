#pragma comment(lib, "ws2_32.lib")

#include "Packet_Utility.h"
#include "Function.h"
#include "Server_Manager.h"
#include "ServerManager.h"

#include <time.h>


int main(int argc, char **argv)
{
	srand((unsigned int)(time(nullptr)));

	bool isRun = true;
	isRun = ServerManager::GetInstance().Initialize();

	if (isRun)
	{
		ServerManager::GetInstance().User_Update();

		ServerManager::GetInstance().Destroy();
	}

	return 0;
}