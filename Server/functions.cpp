#pragma once
#include "pch.h"
#include "functions.h"

using std::string;
using std::wstring;
using std::cout;
using std::wcout;
using std::endl;

wstring Join(const std::vector<WSABUF>& V) {
	wstring ret;
	for (auto it = V.begin(); it != V.end(); it++)
	{
		ret.append((const wchar_t*)it->buf, it->len / sizeof(wchar_t));
	}
	return ret;
}