#pragma once
#include <stdint.h>

#ifdef CLDLL
using pktstr_t = wchar_t*;
#else
using pktstr_t = std::wstring;
#endif // CLDLL

struct Packet
{
	int32_t Code{};			// ��� �������
	uint32_t NameLen{};		// ����� ����� � ��������
	pktstr_t Name;			// ���
	pktstr_t Message;		// ���������
};