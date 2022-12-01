#pragma once
enum class IQAcquireCmdType
{
	RECORD_OPEN = 0,
	RECORD_CLOSE,
	REPLAY_ADJUST,
	REPLAY_START,
	REPLAY_STOP,
	TASK_QUERY,
};

enum class IQAcquireSteamType
{
	IQ_RESULT = 0,
	RECORD_DESCRIPTOR,
};