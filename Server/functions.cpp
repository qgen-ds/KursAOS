#pragma once
#include "pch.h"
#include "functions.h"

using std::string;
using std::wstring;
using std::cout;
using std::wcout;
using std::endl;

wstring Join(const std::vector<WSABUF>& V) {
	wstring ret;
	for (auto it = V.begin(); it != V.end(); it++)
	{
		ret.append((const wchar_t*)it->buf, it->len / sizeof(wchar_t));
	}
	return ret;
}

void PrintMessage(const ClientInfo& Sender, const wstring& s)
{
	wstring msg, name;
	size_t pos = 0;
	msg = s.substr(0, pos = s.find(L'#'));
	name = s.substr(pos, s.find(L'#', ++pos) - pos);
	wcout << name
		<< L'('
		<< (wchar_t*)Sender.addr
		<< L')'
		<< "(ID: "
		<< Sender.ID
		<< L')'
		<< ": "
		<< msg
		<< endl
		<< L'>';
	wcout.clear(); // Clear the failbit just in case
}