#include "pch.h"
#include "TcpServer.h"


TcpServer::TcpServer(size_t ThreadCount)
{
	WSADATA wsd;
	_WorkersCount = ThreadCount;
	_hWorkers = new HANDLE[ThreadCount];
}

void TcpServer::Start()
{
}

TcpServer::~TcpServer()
{
	delete[] _hWorkers;
}
