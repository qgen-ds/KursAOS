#pragma once
#include "pch.h"

inline void WFSOINF(HANDLE Obj)
{
	if (WaitForSingleObject(Obj, INFINITE) != WAIT_OBJECT_0)
	{
		throw std::runtime_error(std::string("WaitForSingleObject failed. Code: ") + std::to_string(GetLastError()));
	}
}

std::wstring Join(const std::vector<WSABUF>& V);