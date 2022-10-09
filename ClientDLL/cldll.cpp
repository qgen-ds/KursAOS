#include "pch.h"
#include "clientdll.h"
#include "WCHARException.h"

#define TXT_SIZE 100

SOCKET s;

void WINAPI GetLocalIP(wchar_t* dst)
{
	sockaddr_in sa;
	int sa_len = sizeof(sa);
	wchar_t txt[TXT_SIZE] = { 0 };
	try
	{
		if (getsockname(s, (sockaddr*)&sa, &sa_len) == SOCKET_ERROR)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"Failed to get the socket name. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		}
		if (!InetNtopW(AF_INET, &sa.sin_addr, dst, TXT_SIZE))
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"Failed to convert the socket name to a valid IPv4 string. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		}
	}
	catch (WCHARException e)
	{
		e.Show();
	}
}

bool WINAPI Connect(wchar_t* address, u_short port)
{
	sockaddr_in sa;
	in_addr addr;
	wchar_t txt[TXT_SIZE] = { 0 };
	try
	{
		if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"socket error. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		}
		switch (InetPtonW(AF_INET, address, &addr))
		{
		case -1:
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"InetPtonW error. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		case 0:
			throw WCHARException(L"Address parameter is not a valid IPv4 dotted-decimal string");
		}
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = addr.S_un.S_addr;
		if (connect(s, (sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"An error occurred during connection to destination address. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		}
	}
	catch (WCHARException e)
	{
		e.Show();
		return false;
	}
	return true;
}

void WINAPI Send(wchar_t* packet)
{
	wchar_t txt[TXT_SIZE] = { 0 };
	//TODO: отправка данных
	try
	{
		if (send(s, (char*)packet, wcslen(packet) * sizeof(wchar_t), 0) == SOCKET_ERROR)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"send error. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		}
	}
	catch (WCHARException e)
	{
		e.Show();
	}
}

void WINAPI Disonnect()
{
	wchar_t txt[TXT_SIZE] = { 0 };
	try
	{
		if (shutdown(s, SD_BOTH) == SOCKET_ERROR)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"shutdown error. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		}
		if (closesocket(s) == SOCKET_ERROR)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"closesocket error. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		}
	}
	catch (WCHARException e)
	{
		e.Show();
	}
}