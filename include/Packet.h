#pragma once
#include <stdint.h>

using std::string;

struct Packet
{
	int32_t Code{};			// Код команды
	uint32_t NameLen{};		// Длина имени в символах
#ifdef CLDLL
	wchar_t* Name;			// Имя
	wchar_t* Message;		// Сообщение
#else
	std::wstring Name;		// Имя
	std::wstring Message;	// Сообщение
	static Packet FromString(const string& s)
	{
		Packet ret;
		woss_t oss;
		string a = s.substr(8, s.size() - 12);
		a.append(sizeof(wchar_t), '\0');
		ret.Code = ntohl(*(int32_t*)(s.c_str()));
		ret.NameLen = ntohl(*(int32_t*)(s.c_str() + 4));
		oss << (wchar_t*)(a.c_str());
		ret.Name = oss.str().substr(0, ret.NameLen);
		ret.Message = oss.str().substr(ret.NameLen);
		return ret;
	}
#endif // CLDLL
};