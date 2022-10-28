#pragma once
#include "pch.h"

inline void WFSOINF(HANDLE Obj)
{
	switch (WaitForSingleObject(Obj, INFINITE))
	{
	case WAIT_FAILED:
		throw std::runtime_error(std::string("WaitForSingleObject failed. Code: ") + std::to_string(GetLastError()));
	case WAIT_ABANDONED:
		RaiseException(WAIT_ABANDONED, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}
}

template<typename CharT>
std::basic_string<CharT> Join(const std::vector<WSABUF>& V)
{
	using bs = std::basic_string<CharT>;
	bs ret;
	for (auto it = V.begin(); it != V.end(); it++)
	{
		ret.append((CharT*)it->buf, it->len / sizeof(CharT));
	}
	return ret;
}

template<typename CharT>
std::forward_list<std::basic_string<CharT>> Split(const std::basic_string<CharT>& s, CharT delim, size_t start_pos = 0U)
{
	using bs = std::basic_string<CharT>;
	std::forward_list<bs> L;
	bs l_s;
	size_t cur_pos = 0;
	while (cur_pos < s.size())
	{
		cur_pos = s.find(delim, start_pos);
		l_s = s.substr(start_pos, cur_pos - start_pos);
		start_pos = cur_pos + 1;
		L.push_front(l_s);
	}
	return L;
}