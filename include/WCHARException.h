#pragma once
#include "pch.h"

class WCHARException
{
	std::wstring _message;
public:
	WCHARException(const std::wstring& message) : _message(message) {}
	const wchar_t* what() { return _message.c_str(); }
	void Show()
	{
		MessageBox(NULL, _message.c_str(), NULL, MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
};