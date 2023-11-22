#pragma once

#include "ClientInfo.h"
#include "ServerManager.h"

class ChattingState
{
public:
	bool operator() (ClientInfo* _clientInfo)
	{
		if (nullptr == _clientInfo)
			return true;

		PROTOCOL protocol{};

		if (!_clientInfo->PacketRecv())
		{
			_clientInfo->SetState(CONNECT_END_STATE);
			return true;
		}

		protocol = _clientInfo->GetProtocol();

		switch (protocol)
		{
		case CHATT_MSG:
			ServerManager::GetInstance().ChattingMessageProcess(_clientInfo);
			break;
		case CHATT_OUT:
			ServerManager::GetInstance().ChattingOutProcess(_clientInfo);
			_clientInfo->SetState(CONNECT_END_STATE);
			break;
		}

		return true;
	}
};

