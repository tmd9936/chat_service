#pragma once

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
	CHATT_OUT

};