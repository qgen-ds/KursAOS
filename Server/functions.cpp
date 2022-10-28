#pragma once
#include "pch.h"
#include "functions.h"

wstring Join(const std::vector<WSABUF>& V) {
	wstring ret;
	for (auto it = V.begin(); it != V.end(); it++)
	{
		ret.append((wchar_t*)it->buf, it->len / sizeof(wchar_t));
	}
	return ret;
}