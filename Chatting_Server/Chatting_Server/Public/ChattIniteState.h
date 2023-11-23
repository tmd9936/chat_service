#pragma once

#include "ClientInfo.h"
#include "ServerManager.h"

class ChattIniteState
{
public:
	bool operator() (ClientInfo* _clientInfo)
	{
		if (nullptr == _clientInfo)
			return true;

		PROTOCOL protocol{};

		bool isNicNameSet = true;

		if (!_clientInfo->PacketRecv())
		{
			_clientInfo->SetState(CONNECT_END_STATE);
			return true;
		}

		protocol = _clientInfo->GetProtocol();

		switch (protocol)
		{
		case CHATT_NICKNAME:
			isNicNameSet = ServerManager::GetInstance().NickNameSetting(_clientInfo);
			break;
		}

		if (isNicNameSet)
		{
			ServerManager::GetInstance().ChattingEnterProcess(_clientInfo);

			if (_clientInfo->GetState() != CONNECT_END_STATE)
			{
				_clientInfo->SetState(CHATTING_STATE);
			}
		}
		return true;
	}

};
