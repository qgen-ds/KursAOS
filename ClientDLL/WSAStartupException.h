#pragma once
#include "WCHARException.h"

class WSAStartupException : public WCHARException
{
public:
	WSAStartupException(const std::wstring& message) : WCHARException(message) {}
};