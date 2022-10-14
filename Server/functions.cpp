#pragma once
#include "pch.h"
#include "functions.h"
#include "wchar_error.h"

using std::string;
using std::wstring;
using std::cout;
using std::wcout;
using std::endl;

size_t FindDelim(const wchar_t* str, const wchar_t delim)
{
	size_t ret = 0;
	while (str[ret] != delim)
		ret++;
	return ret;
}

void PrintMessage(const ClientInfo& Sender, const std::vector<WSABUF>& V)
{
	// No need to validate the packet here: validation should
	// be performed before a call to this function
	//TODO: heavy rework
	wstring msg, name;
	size_t pos;
	for (auto it = V.begin(); it != V.end(); it++)
	{
		
		//p = (wchar_t*)(it->buf);
		//msg.append(p, FindDelim(p, L'#'));
	}
	wcout << msg << endl;
	return;
}

void ValidatePacket(const ClientInfo& Sender, const std::vector<WSABUF>& V)
{
	std::wostringstream err;
	size_t size = 0;
	wchar_t* p = (wchar_t*)V.back().buf;
	if (p[(V.back().len - 1) / sizeof(wchar_t)] != L'&')
	{
		for (auto it = V.begin(); it != V.end(); it++)
		{
			size += it->len;
		}
		err << "Error: invalid packet recieved from "
			<< Sender.addr
			<< " with ID . Total packet size: "
			<< size
			<< " bytes."
			<< endl; // Add user ID here
			throw wchar_error(err.str());
	}
}

void AppendSenderAddr(const ClientInfo& Sender, std::vector<WSABUF>& V, char* buf)
{
	wstring s = wstring(Sender.addr) + L"#&";
	wchar_t* p = (wchar_t*)V.back().buf;
	p[(V.back().len - 1) / sizeof(wchar_t)] = L'#';
	V.push_back(WSABUF{ s.size() * sizeof(wchar_t), buf });
	memcpy_s(V.back().buf, V.back().len, s.c_str(), V.back().len);
}