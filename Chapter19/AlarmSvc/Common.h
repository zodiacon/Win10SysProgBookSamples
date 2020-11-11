#pragma once

enum class MessageType {
	AddAlarm,
	RemoveAlarm,
};

struct AlarmMessage {
	MessageType Type;
	FILETIME Time;
};
