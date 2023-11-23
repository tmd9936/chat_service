#pragma once

#include "ClientInfo.h"

class ClientIniteState
{
public:
	bool operator() (ClientInfo* _clientInfo)
	{
		if (nullptr == _clientInfo)
			return true;

		_clientInfo->SetState(CHATT_INITE_STATE);
		return true;
	}
};

