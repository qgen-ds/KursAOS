#pragma once
class wchar_error : std::runtime_error
{
private:
	std::wstring _message;
public:
	explicit wchar_error(const std::wstring& message) : _message(message), std::runtime_error("") {}
	wchar_error(const wchar_t *message) : _message(message), std::runtime_error("") {}
	const wchar_t* what() { return _message.c_str(); }
};

