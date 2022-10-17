#pragma once
#include "pch.h"

typedef void (*EVENTRAISER)();

struct RECVPARAM
{
	EVENTRAISER Notify;
	WSABUF buf;
};
