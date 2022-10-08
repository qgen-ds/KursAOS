#pragma once

#ifdef CLIENTDLL_EXPORTS
#define CLDLL __declspec(dllexport)
#else
#define CLDLL __declspec(dllimport)
#endif // CLIENTDLL_EXPORTS

extern "C" CLDLL void WINAPI GetLocalIP(wchar_t* dst);
extern "C" CLDLL bool WINAPI Connect(wchar_t* address, u_short port);
extern "C" CLDLL bool WINAPI Send(wchar_t* msg);
extern "C" CLDLL void WINAPI Disonnect();