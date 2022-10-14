#pragma once
class wchar_error
{
private:
	std::wstring _message;
public:
	explicit wchar_error(const std::wstring& message) : _message(message)  {}
	wchar_error(const wchar_t *message) : _message(message) {}
	const wchar_t* what() { return _message.c_str(); }
};

