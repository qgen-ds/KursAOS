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

std::wstring Join(const std::vector<WSABUF>& V);