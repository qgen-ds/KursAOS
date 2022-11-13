#include "pch.h"
#ifdef _DEBUG
#include "TcpServer.h"

void TcpServer::PrintMessage(const ClientInfo& ci, const Packet& p)
{
	wcout << p.Name
		<< L'('
		<< ci.addr
		<< L')'
		<< "(ID: "
		<< ci.ID
		<< L')'
		<< L": "
		<< p.Message
		<< endl
		<< '>';
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