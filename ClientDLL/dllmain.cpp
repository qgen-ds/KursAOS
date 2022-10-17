// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

#define TXT_SIZE 100

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    WSADATA wsd;
    int ret;
    wchar_t txt[TXT_SIZE] = { 0 };
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        if ((ret = WSAStartup(MAKEWORD(2, 2), &wsd)) != 0)
        {
            swprintf_s(txt, TXT_SIZE, L"%s%i", L"WSAStartup error. Code: ", ret);
            wchar_error(txt).Show();
            return FALSE;
        }
    break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
    {
        if (WSACleanup() == SOCKET_ERROR)
        {
            swprintf_s(txt, TXT_SIZE, L"%s%i", L"WSACleanup error. Code: ", WSAGetLastError());
            MessageBox(NULL, txt, NULL, MB_OK | MB_ICONERROR | MB_TASKMODAL);
        }
    }
    }
    return TRUE;
}