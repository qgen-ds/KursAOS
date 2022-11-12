#pragma once
#include <stdint.h>

struct Packet
{
	int32_t Code;
	uint32_t NameLen;
	wchar_t* Name;
	wchar_t* Message;
};