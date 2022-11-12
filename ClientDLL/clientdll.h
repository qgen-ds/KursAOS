#pragma once
#include "RECVPARAM.h"
#include "Packet.h"

#define CLDLL __declspec(dllexport)

extern "C" CLDLL bool WINAPI Connect(wchar_t* address, u_short port, RECVPARAM * param);
extern "C" CLDLL void WINAPI Send(Packet* packet);
extern "C" CLDLL void WINAPI Disconnect();
extern "C" CLDLL void WINAPI FreeBlock(void* Block);