#include "pch.h"
#ifdef _DEBUG
#include "TcpServer.h"

void TcpServer::PrintMessage(const ClientInfo& ci, const wstring& s)
{
	wstring msg, name;
	size_t pos = 0;
	msg = s.substr(0, pos = s.find(L'#'));
	name = s.substr(pos, s.find(L'#', ++pos) - pos);
	wcout << name
		<< L'('
		<< (wchar_t*)ci.addr
		<< L')'
		<< "(ID: "
		<< ci.ID
		<< L')'
		<< ": "
		<< msg
		<< endl
		<< L'>';
	wcout.clear(); // Clear the failbit just in case
}

void TcpServer::PrintNewClient(const ClientInfo& ci)
{
	wcout << L"Accepted new client from " << ci.addr
		<< L" with ID " << ci.ID
		<< L'.' << endl << L'>';
	wcout.clear();
}

#endif // _DEBUG