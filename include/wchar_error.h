#pragma once
#include "pch.h"
#include <sstream>

class wchar_error
{
private:
	std::wstring _message;
public:
	explicit wchar_error(const std::wstring& message) : _message(message)  {}
	wchar_error(const wchar_t *message) : _message(message) {}
	wchar_error(const char* message)
	{
		std::wostringstream oss;
		oss << message;
		_message = oss.str();
	}
	const wchar_t* what() { return _message.c_str(); }
#ifdef GUI_APP
	void Show()
	{
		MessageBox(NULL, _message.c_str(), NULL, MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
#endif // GUI_APP
};