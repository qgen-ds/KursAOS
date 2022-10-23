#pragma once
#include "pch.h"

typedef void (__stdcall *EVENTRAISER)();

struct RECVPARAM
{
	EVENTRAISER Notify;
	EVENTRAISER OnDisconnect;
	WSABUF buf;
	bool MarkForDelete;
};