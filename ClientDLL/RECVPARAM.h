#pragma once
#include "pch.h"

typedef void (__stdcall *EVENTRAISER)();

struct RECVPARAM
{
	EVENTRAISER Notify;
	EVENTRAISER Disconnect;
	WSABUF buf;
	//BOOL MarkForDelete;
};
