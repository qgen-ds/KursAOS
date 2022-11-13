#include "pch.h"
#include "clientdll.h"

#define TXT_SIZE 100
#define BUF_SIZE 4096

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

void WINAPI Send(Packet* packet)
{
	wchar_t txt[TXT_SIZE] = { 0 };
	DWORD iNum;
	static std::vector<WSABUF> buf(5);
	packet->Code = htonl(packet->Code);
	packet->NameLen = htonl(packet->NameLen);
	buf[0].buf = (CHAR*)&packet->Code;
	buf[0].len = 4;
	buf[1].buf = (CHAR*)&packet->NameLen;
	buf[1].len = 4;
	buf[2].buf = (CHAR*)packet->Name;
	buf[2].len = wcslen(packet->Name) * sizeof(wchar_t);
	buf[3].buf = (CHAR*)packet->Message;
	buf[3].len = wcslen(packet->Message) * sizeof(wchar_t);
	// Добавить в конец 4 символа ETB как маркер конца пакета
	buf[4].buf = (CHAR*)"\x17\x17\x17\x17";
	buf[4].len = 4;
	try
	{
		if (WSASend(s,buf.data(), buf.size(), &iNum, 0, NULL, NULL) == SOCKET_ERROR)
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
	char dummybuf[BUF_SIZE] = { 0 };
	std::vector<WSABUF> IBuf({WSABUF{ BUF_SIZE, dummybuf } });
	while (true)
	{
		switch (IBuf.back().len = recv(s, IBuf.back().buf, IBuf.back().len, 0))
		{
		case 0: // Handle disconnection
			Disconnect();
			out->OnDisconnect();
			return 0;
		case BUF_SIZE: // Handle full buffer
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
			IBuf[0].len = BUF_SIZE;
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