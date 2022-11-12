#pragma once
#include <stdint.h>

#ifdef CLDLL
using pktstr_t = wchar_t*;
#else
using pktstr_t = std::wstring;
#endif // CLDLL

struct Packet
{
	int32_t Code{};			// Код команды
	uint32_t NameLen{};		// Длина имени в символах
	pktstr_t Name;			// Имя
	pktstr_t Message;		// Сообщение
};