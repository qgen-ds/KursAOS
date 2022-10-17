#include "pch.h"
#include "clientdll.h"

#define TXT_SIZE 100

SOCKET s;

//TODO: перелопатить сетевые операции, чтобы фиксировать аварийные закрытия сокета

int WINAPI Recv();

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
			throw wchar_error(txt);
		}
		if (!InetNtopW(AF_INET, &sa.sin_addr, dst, TXT_SIZE))
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"Failed to convert the socket name to a valid IPv4 string. Code: ", WSAGetLastError());
			throw wchar_error(txt);
		}
	}
	catch (wchar_error e)
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
			throw wchar_error(txt);
		}
		switch (InetPtonW(AF_INET, address, &addr))
		{
		case -1:
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"InetPtonW error. Code: ", WSAGetLastError());
			throw wchar_error(txt);
		case 0:
			throw wchar_error(L"Address parameter is not a valid IPv4 dotted-decimal string");
		}
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = addr.S_un.S_addr;
		if (connect(s, (sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"An error occurred during connection to destination address. Code: ", WSAGetLastError());
			throw wchar_error(txt);
		}
	}
	catch (wchar_error e)
	{
		e.Show();
		return false;
	}
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Recv, NULL, 0, NULL);
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
			throw wchar_error(txt);
		}
	}
	catch (wchar_error e)
	{
		e.Show();
	}
}

char* Join(const std::vector<WSABUF>& IBuf)
{
	return NULL;
}

int CALLBACK Recv()
{
	//TODO: отсюдова, передать полученные данные в форму
	const unsigned int RECV_SIZE = 4096;
	char dummybuf[RECV_SIZE] = { 0 };
	std::vector<WSABUF> IBuf({WSABUF{ RECV_SIZE, dummybuf } });
	while (true)
	{
		switch (IBuf.back().len = recv(s, IBuf.back().buf, IBuf.back().len, 0))
		{
		case 0:
			break;
		case RECV_SIZE:
			break;
		case SOCKET_ERROR:
			break;
		default:
			//wchar_t* p = (wchar_t*)IBuf[0].buf;

			ZeroMemory(IBuf[0].buf, IBuf[0].len);
			break;
		}
	}
	return 0;
}

void WINAPI Disonnect()
{
	wchar_t txt[TXT_SIZE] = { 0 };
	try
	{
		if (shutdown(s, SD_BOTH) == SOCKET_ERROR)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"shutdown error. Code: ", WSAGetLastError());
			throw wchar_error(txt);
		}
		if (closesocket(s) == SOCKET_ERROR)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"closesocket error. Code: ", WSAGetLastError());
			throw wchar_error(txt);
		}
	}
	catch (wchar_error e)
	{
		e.Show();
	}
}