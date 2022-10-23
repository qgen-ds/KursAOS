#include "pch.h"
#include "clientdll.h"

#define TXT_SIZE 100

SOCKET s;

DWORD CALLBACK Recv(LPVOID _In_ p);

bool WINAPI Connect(wchar_t* address, u_short port, RECVPARAM* param)
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
	CreateThread(NULL, 0, Recv, param, 0, NULL);
	return true;
}

void WINAPI Send(wchar_t* packet)
{
	wchar_t txt[TXT_SIZE] = { 0 };
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

char* Join(const std::vector<WSABUF>& IBuf, ULONG* count, bool* MarkForDelete)
{
	char* ret = nullptr;
	if (IBuf.size() == 1)
	{
		*MarkForDelete = false;
		*count = IBuf[0].len;
		return IBuf[0].buf;
	}
	std::for_each(IBuf.begin(), IBuf.end(), [count](const WSABUF& e) { *count += e.len; });
	try
	{
		ret = new char[*count];
	}
	catch (std::bad_alloc e)
	{
		wchar_error(e.what()).Show();
	}
	*count = 0;
	for (auto it = IBuf.begin(); it != IBuf.end(); it++)
	{
		memcpy_s((ret + *count), it->len, it->buf, it->len);
		*count += it->len;
	}
	*MarkForDelete = true;
	return ret;
}

DWORD CALLBACK Recv(LPVOID _In_ p)
{
	RECVPARAM* out = static_cast<RECVPARAM*>(p);
	DWORD iNum = 0;
	const unsigned int RECV_SIZE = 4096;
	char dummybuf[RECV_SIZE] = { 0 };
	std::vector<WSABUF> IBuf({WSABUF{ RECV_SIZE, dummybuf } });
	while (true)
	{
		switch (IBuf.back().len = recv(s, IBuf.back().buf, IBuf.back().len, 0))
		{
		case 0: // Handle disconnection
			Disconnect();
			out->Disconnect();
			return 0;
		case RECV_SIZE: // Handle full buffer
			break;
		case SOCKET_ERROR:
			switch (iNum = WSAGetLastError())
			{
			case WSAEWOULDBLOCK:
				break;
			case WSAEINTR:
				return 0;
			}
			break;
		default: // Handle incoming data
			out->buf.buf = Join(IBuf, &out->buf.len, &out->MarkForDelete);
			out->Notify();
			ZeroMemory(IBuf[0].buf, IBuf[0].len);
			IBuf[0].len = RECV_SIZE;
		}
	}
}

void WINAPI Disconnect()
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

void WINAPI FreeBlock(void* Block)
{
	delete[] Block;
}
