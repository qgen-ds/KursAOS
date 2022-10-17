#pragma once

#define CLDLL __declspec(dllexport)

extern "C" CLDLL void WINAPI GetLocalIP(wchar_t* dst);
extern "C" CLDLL bool WINAPI Connect(wchar_t* address, u_short port);
extern "C" CLDLL void WINAPI Send(wchar_t* packet);
extern "C" CLDLL void WINAPI Disonnect();