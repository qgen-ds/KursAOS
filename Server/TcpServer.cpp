#include "pch.h"
#include "TcpServer.h"


TcpServer::TcpServer(unsigned short port, size_t ThreadCount, size_t MaxClients, int backlog)
{
	Port = port;
	WorkersCount = ThreadCount;
	Backlog = backlog;
	hWorkers = new HANDLE[ThreadCount];
	if (!(hAcceptEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
	{
		swprintf_s(err, ERR_SIZE, L"%s%i", L"Failed to create event for accepting socket. Code: ", GetLastError());
		throw WCHARException(err);
	}
	if (!(Lock = CreateEvent(NULL, FALSE, TRUE, NULL)))
	{
		swprintf_s(err, ERR_SIZE, L"%s%i", L"Failed to create ClientList lock. Code: ", GetLastError());
		throw WCHARException(err);
	}
	for (size_t i = 0; i < ThreadCount; i++)
	{
		if (!(hWorkers[i] = CreateThread(NULL, 0, TcpServer::ClientObserver, this, CREATE_SUSPENDED, NULL)))
		{
			swprintf_s(err, ERR_SIZE, L"%s%i", L"CreateThread error in main. Code: ", GetLastError());
			throw WCHARException(err);
		}
	}
}

void TcpServer::Init()
{
	sockaddr_in sa;
	DWORD iNum = 1;
	if ((Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{   // в случае неудачи - выдать сообщение и выйти
		swprintf_s(err, ERR_SIZE, L"%s%i", L"socket error. Code: ", WSAGetLastError());
		throw WCHARException(err);
	}
	if (setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&iNum, sizeof(int)) == SOCKET_ERROR) // set SO_REUSEADDR to true
	{
		// в случае неудачи - выдать сообщение и выйти
		swprintf_s(err, ERR_SIZE, L"%s%i", L"setsockopt error. Code: ", WSAGetLastError());
		throw WCHARException(err);
	}
	sa.sin_family = AF_INET; // заполнение структуры данных
	sa.sin_port = htons(Port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(Socket, (sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		// в случае неудачи - выдать сообщение и выйти
		swprintf_s(err, ERR_SIZE, L"%s%i", L"bind error. Code: ", WSAGetLastError());
		throw WCHARException(err);
	}
	if (listen(Socket, Backlog) == SOCKET_ERROR)
	{
		swprintf_s(err, ERR_SIZE, L"%s%i", L"listen error. Code: ", WSAGetLastError());
		throw WCHARException(err);
	}
	if (!(hAcceptor = CreateThread(NULL, 0, TcpServer::AcceptLoop, this, CREATE_SUSPENDED, NULL)))
	{
		swprintf_s(err, ERR_SIZE, L"%s%i", L"CreateThread error in main. Code: ", GetLastError());
		throw WCHARException(err);
	}
}

void TcpServer::Start()
{
	ResumeThread(hAcceptor);
	for (size_t i = 0; i < WorkersCount; i++)
	{
		ResumeThread(hWorkers[i]);
	}
	std::cout << "Server started successfully.\nAccepting connections..." << std::endl;
}

void TcpServer::ClearErr()
{
	_wcsnset_s(err, 0, ERR_SIZE);
}

DWORD CALLBACK TcpServer::ClientObserver(LPVOID p)
{
	TcpServer* pInst = (TcpServer*)p;
	std::vector<WSAEVENT> Events;
	std::vector<WSABUF> IOBuf; // Вектор структур WSABUF
	DWORD Index = 0; // for use with WSAWaitForMultipleEvents
	DWORD iNum = 0;
	char dummybuffer[RECV_SIZE] = { 0 };
	IOBuf.push_back(WSABUF{ RECV_SIZE, dummybuffer }); // Первая структура хранится на стеке
	Events.reserve(pInst->MAX_CL_COUNT + 1);
	Events.push_back(pInst->hAcceptEvent);
	while (true)
	{
		switch (Index = WSAWaitForMultipleEvents(Events.size(), Events.data(), FALSE, INFINITE, FALSE) - WSA_WAIT_EVENT_0)
		{
		case 0:
		{
			// Handle new accepted connection
			HANDLE NewSocketEvent;
			switch (WaitForSingleObject(pInst->Lock, INFINITE)) // Make sure the client list won't be changed while we work
			{
			case WAIT_FAILED:
				swprintf_s(pInst->err, ERR_SIZE, L"%s%i", L"WaitForSingleObject failed. Code: ", GetLastError());
				WCHARException(pInst->err).Show();
				break;
			case WAIT_ABANDONED:
				WCHARException(L"Abandoned VLock").Show();
				break;
			}
			if ((NewSocketEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
			{
				swprintf_s(pInst->err, ERR_SIZE, L"%s%i", L"Failed to create event for new accepted socket. Code: ", WSAGetLastError());
				WCHARException(pInst->err).Show();
				break;
			}
			if (WSAEventSelect(std::next(pInst->ClientList.begin(), Events.size() - 1)->s, NewSocketEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
			{
				swprintf_s(pInst->err, ERR_SIZE, L"%s%i", L"WSAEventSelect for accepted socket error. Code: ", WSAGetLastError());
				WCHARException(pInst->err).Show();
				break;
			}
			Events.push_back(NewSocketEvent);
			SetEvent(pInst->Lock);
			break;
		}
		case WSA_WAIT_FAILED - WSA_WAIT_EVENT_0:
			// Handle the occured error
			swprintf_s(pInst->err, ERR_SIZE, L"%s%i", L"WSAWaitForMultipleEvents failed. Code: ", GetLastError());
			WCHARException(pInst->err).Show();
			break;
		default:
		{
			// Handle client message
			switch (WaitForSingleObject(pInst->Lock, INFINITE)) // Make sure the client list won't be changed while we're working
			{
			case WAIT_FAILED:
				swprintf_s(pInst->err, ERR_SIZE, L"%s%i", L"WaitForSingleObject failed. Code: ", GetLastError());
				WCHARException(pInst->err).Show();
				break;
			case WAIT_ABANDONED:
				WCHARException(L"Abandoned VLock").Show();
				break;
			}
			switch (iNum = recv(std::next(pInst->ClientList.begin(), Index)->s, IOBuf.back().buf, IOBuf.back().len, 0))
			{
			case 0:
				// Handle disconnection
				ResetEvent(Events[Index]);
				continue;
			case SOCKET_ERROR:
			{
				// Handle the occured error
				iNum = WSAGetLastError();
				if (iNum != WSAEWOULDBLOCK)
				{
					swprintf_s(pInst->err, ERR_SIZE, L"%s%i", L"recv failed. Code: ", iNum);
					WCHARException(pInst->err).Show();
				}
				break;
			}
			case RECV_SIZE:
				// Handle full buffer
				break;
			default:
			{
				// Handle incoming data
				pInst->Broadcast(IOBuf[0].buf, IOBuf[0].len);
				break;
			}
			}
			SetEvent(pInst->Lock);
			ZeroMemory(IOBuf[0].buf, IOBuf[0].len);
			break;
		}
		}
		ResetEvent(Events[Index]);
	}
}

void TcpServer::Broadcast(const char* msg, int len)
{
	for (auto it = ClientList.begin(); it != ClientList.end(); it++)
	{
		if (send(it->s, msg, len, 0) == SOCKET_ERROR)		// TODO: Replace with WSASend for gathering output
		{
			swprintf_s(err, ERR_SIZE, L"%s%i", L"send failed. Code: ", WSAGetLastError());
			throw WCHARException(err);
		}
	}
}

DWORD CALLBACK TcpServer::AcceptLoop(LPVOID p)
{
	using namespace std;
	TcpServer* pInst = (TcpServer*)p;
	sockaddr_in sa;
	int iNum = 1;
	while (1) // цикл приема соединений
	{
		ClientInfo ci = { 0 };
		iNum = sizeof(sockaddr_in);
		ci.s = accept(pInst->Socket, (sockaddr*)&sa, &iNum); // принять соединение
		if (ci.s == INVALID_SOCKET)
		{
			cout << "accept failed. Code: " << WSAGetLastError() << endl;
			return 1;
		}
		if (pInst->MAX_CL_COUNT == pInst->ClientList.size())
		{
			cout << "Unable to accept more clients: server is full." << endl;
			shutdown(ci.s, SD_BOTH);
			closesocket(ci.s);
			continue;
		}
		iNum = sizeof(sockaddr_in);
		if (getpeername(ci.s, (sockaddr*)&sa, &iNum) != SOCKET_ERROR) // попытаться получить адрес клиента
		{
			InetNtopW(AF_INET, &sa.sin_addr, ci.addr, ci.CLADDRLEN);
		}
		else
		{
			wcsncpy_s(ci.addr, L"Unknown", ci.CLADDRLEN);
		}
		switch (WaitForSingleObject(pInst->Lock, INFINITE)) // Acquire VLock
		{
		case WAIT_FAILED:
			cout << "WaitForSingleObject failed. Code: " << GetLastError() << endl;
			return 1;
		case WAIT_ABANDONED:
			cout << "WaitForSingleObject abandoned the object. Code: " << GetLastError() << endl;
		}
		pInst->ClientList.push_back(ci);
		SetEvent(pInst->hAcceptEvent);
		SetEvent(pInst->Lock);
	}
}

void TcpServer::Broadcast(std::vector<WSABUF>& IOBuf)
{
	//TODO: Implement I/O for WSASend/Recv
}

TcpServer::~TcpServer()
{
	if (Lock) CloseHandle(Lock);
	if (hAcceptEvent) CloseHandle(hAcceptEvent);
	for (size_t i = 0; i < WorkersCount; i++)
	{
		CloseHandle(hWorkers[i]);
	}
	for (auto it = ClientList.begin(); it != ClientList.end(); it++)
	{
		closesocket(it->s);
	}
	closesocket(Socket);
	delete[] hWorkers;
}