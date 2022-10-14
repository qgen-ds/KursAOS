#pragma once
#include "pch.h"

struct ClientInfo
{
	static const size_t CLADDRLEN = 20;
	SOCKET s;
	wchar_t addr[CLADDRLEN];
	size_t ID;
};