#pragma once

#include "ClientInfo.h"
#include "ServerManager.h"

class ConnectEndState
{
public:
	bool operator() (ClientInfo* _clientInfo)
	{
		if (nullptr == _clientInfo)
			return true;

		ServerManager::GetInstance().RemoveClient(_clientInfo);

		return false;
	}
};

