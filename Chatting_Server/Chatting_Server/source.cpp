#pragma comment(lib, "ws2_32.lib")

#include "Packet_Utility.h"
#include "Function.h"
#include "Server_Manager.h"


int main(int argc, char **argv)
{
	bool isRun = true;
	isRun = Server_Manager::GetInstance().Initialize();

	if (isRun)
	{
		Server_Manager::GetInstance().User_Update();

		Server_Manager::GetInstance().Destroy();
	}

	return 0;
}