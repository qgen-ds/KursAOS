#pragma once
#include "pch.h"

typedef void (__stdcall *EVENTRAISER)();

struct RECVPARAM
{
	EVENTRAISER Notify;
	WSABUF buf;
};
