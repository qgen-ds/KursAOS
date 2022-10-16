#include "pch.h"
#include "TcpServer.h"
#include <algorithm>
#include "functions.h"

using std::string;
using std::wstring;
using std::cout;
using std::wcout;
using std::endl;

TcpServer::TcpServer(unsigned short port, size_t maxClients, int backlog)
{
	Port = port;
	Backlog = backlog;
	MaxClients = maxClients;
	ServerStatus = ServerStatuses::Stopped;
	Lock = hAcceptor = hWorker = hInternalEvent = NULL;
	Socket = 0;
	LastAvailableID = 0;
	Events.reserve(MaxClients + 1);
}

void TcpServer::Init()
{
	sockaddr_in sa;
	DWORD iNum = 1;
	if (ServerStatus != ServerStatuses::Stopped)
		throw std::runtime_error("The server doesn't seem to be stopped.");
	LastAvailableID = 0;
	if (!(hInternalEvent || (hInternalEvent = CreateEvent(NULL, TRUE, FALSE, NULL))))
	{
		throw std::runtime_error(string("Failed to create event for accepting socket. Code: ") + std::to_string(GetLastError()));
	}
	if (!(Lock || (Lock = CreateEvent(NULL, FALSE, TRUE, NULL))))
	{
		throw std::runtime_error(string("Failed to create ClientList lock. Code: ") + std::to_string(GetLastError()));
	}
	if (!(hWorker = CreateThread(NULL, 0, TcpServer::ClientObserver, this, CREATE_SUSPENDED, NULL)))
	{
		throw std::runtime_error(string("Failed to create worker thread. Code: ") + std::to_string(GetLastError()));
	}
	if ((Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		throw std::runtime_error(string("socket error. Code: ") + std::to_string(WSAGetLastError()));
	}
	if (setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&iNum, sizeof(int)) == SOCKET_ERROR) // set SO_REUSEADDR to true
	{
		throw std::runtime_error(string("setsockopt error. Code: ") + std::to_string(WSAGetLastError()));
	}
	sa.sin_family = AF_INET; // заполнение структуры данных
	sa.sin_port = htons(Port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(Socket, (sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		throw std::runtime_error(string("bind error. Code: ") + std::to_string(WSAGetLastError()));
	}
	if (listen(Socket, Backlog) == SOCKET_ERROR)
	{
		throw std::runtime_error(string("listen error. Code: ") + std::to_string(WSAGetLastError()));
	}
	if (!(hAcceptor = CreateThread(NULL, 0, TcpServer::AcceptLoop, this, CREATE_SUSPENDED, NULL)))
	{
		throw std::runtime_error(string("Failed to create acceptor thread. Code: ") + std::to_string(GetLastError()));
	}
	Events.push_back(hInternalEvent);
	ServerStatus = ServerStatuses::Initialised;
}

void TcpServer::Start()
{
	int iNum = sizeof(sockaddr_in);
	sockaddr_in addr;
	char a[50] = { 0 };
	if(ServerStatus != ServerStatuses::Initialised)
		throw std::runtime_error("The server has not been initialised.");
	getsockname(Socket, (sockaddr*)&addr, &iNum);
	inet_ntop(addr.sin_family, &addr.sin_addr, a, 50);
	if (ResumeThread(hAcceptor) == (DWORD)-1 || ResumeThread(hWorker) == (DWORD)-1)
	{
		throw std::runtime_error(string("Failed to start the server. Code: ") + std::to_string(GetLastError()));
	}
	ServerStatus = ServerStatuses::Running;
	cout << "Server successfully started on port "
		<< Port << " and interface "
		<< (strcmp(a, "0.0.0.0") ? a : "ANY")
		<< ".\nAccepting connections..."
		<< endl;
	cout.clear();
}

void TcpServer::Stop()
{
	const HANDLE arr[] = { hAcceptor, hWorker };
	switch (ServerStatus)
	{
	case ServerStatuses::Stopped:
		return;
	case ServerStatuses::Initialised:
		// Start threads for correct termination
		ResumeThread(hAcceptor);
		ResumeThread(hWorker);
	}
	ServerStatus = ServerStatuses::RequestedForStop;
	SetEvent(hInternalEvent);
	closesocket(Socket);
	for (auto it = ClientList.begin(); it != ClientList.end(); it++)
	{
		shutdown(it->s, SD_BOTH);
		closesocket(it->s);
	}
	WaitForMultipleObjects(2, arr, TRUE, INFINITE);
	// Return the events to their initial states
	ResetEvent(hInternalEvent);
	SetEvent(Lock);
	CloseHandle(hWorker);
	CloseHandle(hAcceptor);
	Events.clear();
	hWorker = hAcceptor = NULL;
	ServerStatus = ServerStatuses::Stopped;
}

// Abandon all hope ye who enter here
DWORD CALLBACK TcpServer::ClientObserver(LPVOID _In_ p)
{
	TcpServer* const pInst = static_cast<TcpServer*>(p);
	wstring msg;
	std::list<ClientInfo>::iterator cl;
	std::vector<WSABUF> IOBuf; // ¬ектор структур WSABUF
	DWORD Index = 0; // for use with WSAWaitForMultipleEvents
	DWORD iNum = 0;
	HANDLE NewSocketEvent;
	char dummybuffer[RECV_SIZE] = { 0 };
	char dummyaddr[RECV_SIZE] = { 0 };
	IOBuf.push_back(WSABUF{ RECV_SIZE, dummybuffer }); // ѕерва€ структура хранитс€ на стеке
	while (true)
	{
		try
		{
			switch (Index = WSAWaitForMultipleEvents(pInst->Events.size(), pInst->Events.data(), FALSE, INFINITE, FALSE) - WSA_WAIT_EVENT_0)
			{
			case 0: // Handle internal event
			{
				// Stop the server
				if (pInst->ServerStatus == ServerStatuses::RequestedForStop)
				{
					for (size_t i = 1; i < pInst->Events.size(); i++)
					{
						WSACloseEvent(pInst->Events[i]);
					}
					return 0;
				}
				// Connect new client
				WFSOINF(pInst->Lock); // Make sure the client list won't be changed while we work
				if ((NewSocketEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
				{
					throw std::runtime_error(string("Failed to create event for new accepted socket. Code: ") + std::to_string(WSAGetLastError()));
				}
				if (WSAEventSelect(std::next(pInst->ClientList.begin(), pInst->Events.size() - 1)->s, NewSocketEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
				{
					throw std::runtime_error(string("WSAEventSelect for accepted socket error. Code: ") + std::to_string(WSAGetLastError()));
				}
				pInst->Events.push_back(NewSocketEvent);
				SetEvent(pInst->Lock);
				ResetEvent(pInst->Events[Index]);
				break;
			}
			case WSA_WAIT_FAILED - WSA_WAIT_EVENT_0: // Handle the occured error
				throw std::runtime_error(string("WSAWaitForMultipleEvents failed. Code: ") + std::to_string(WSAGetLastError()));
			default: // Handle client message
			{
				WFSOINF(pInst->Lock); // Make sure the client list won't be changed while we work
				while (true)
				{
					switch (IOBuf.back().len = recv(std::next(pInst->ClientList.begin(), Index - 1)->s, IOBuf.back().buf, IOBuf.back().len, 0))
					{
					case 0: // Handle disconnection
					{
						pInst->DisconnectByIndex(Index);
						break;
					}
					case SOCKET_ERROR: // Handle the occured error
					{
						switch (iNum = WSAGetLastError())
						{
						case WSAECONNRESET: // Handle abrupt connection reset							
							pInst->DisconnectByIndex(Index);
							break;
						case WSAEWOULDBLOCK:
							// —юда попадаем только в случае кратности
							// входных данных размеру одного буфера,
							// поэтому надо удалить выделенный,
							// но не использованный последний буфер
							delete[] IOBuf.back().buf;
							IOBuf.pop_back();
							break;
						default:
							throw std::runtime_error(string("recv failed. Code: ") + std::to_string(iNum));
						}
						break;
					}
					default: // Handle incoming data
					{						
						msg = Join(IOBuf);
						cl = std::next(pInst->ClientList.begin(), Index - 1);
						ValidatePacket(*cl, msg);
						PrintMessage(*cl, msg);
						AppendSenderAddr(*cl, IOBuf, dummyaddr);
						pInst->Broadcast(IOBuf);
						ZeroMemory(IOBuf[0].buf, IOBuf[0].len);
						ZeroMemory(IOBuf.back().buf, IOBuf.back().len);
						IOBuf.pop_back();
						if (IOBuf.size() > 1)
						{
							std::for_each(std::next(IOBuf.begin()), IOBuf.end(), [](WSABUF& e) { delete[] e.buf; });
							IOBuf.erase(std::next(IOBuf.begin()), IOBuf.end());
						}
						ResetEvent(pInst->Events[Index]);
						break;
					}
					case RECV_SIZE: // Handle full buffer
						IOBuf.push_back(WSABUF{ RECV_SIZE, new char[RECV_SIZE] });
						continue;
					}
					break; // break inner while
				}
			} // inner while
			SetEvent(pInst->Lock);
			IOBuf[0].len = RECV_SIZE;
			} // switch WSAWAitForMultipleObjects
		} // try
		catch (wchar_error e)
		{
			wcout << e.what()
				<< endl
				<< L'>';
			wcout.clear();
			//break;
		}
		catch (std::exception e)
		{
			cout << e.what()
				<< endl
				<< '>';
			cout.clear();
			//break;
		}
	} // outer while
	//return 1;
}

void TcpServer::ValidatePacket(const ClientInfo& Sender, const wstring& s)
{
	std::wostringstream err;
	if (s.back() != L'&')
	{
		err << "Error: invalid packet recieved from "
			<< Sender.addr
			<< " with ID "
			<< Sender.ID
			<< ". Total packet size: "
			<< s.size() * sizeof(wchar_t)
			<< " bytes."
			<< endl;
		throw wchar_error(err.str());
	}
}

void TcpServer::AppendSenderAddr(const ClientInfo& Sender, std::vector<WSABUF>& V, char* buf)
{
	// No need to validate the packet here: validation should
	// be performed before a call to this function
	wstring s = wstring(Sender.addr) + L"#&";
	wchar_t* p = (wchar_t*)V.back().buf;
	p[(V.back().len - 1) / sizeof(wchar_t)] = L'#';
	V.push_back(WSABUF{ s.size() * sizeof(wchar_t), buf });
	memcpy_s(V.back().buf, V.back().len, s.c_str(), V.back().len);
}

DWORD CALLBACK TcpServer::AcceptLoop(LPVOID _In_ p)
{
	using namespace std;
	TcpServer* const pInst = static_cast<TcpServer*>(p);
	sockaddr_in sa;
	int iNum = 1;
	while (1) // цикл приема соединений
	{
		ClientInfo ci = { 0 };
		iNum = sizeof(sockaddr_in);
		ci.s = accept(pInst->Socket, (sockaddr*)&sa, &iNum); // прин€ть соединение
		if (pInst->ServerStatus == ServerStatuses::RequestedForStop)
			return 0;
		if (ci.s == INVALID_SOCKET)
		{
			throw std::runtime_error(string("accept failed. Code: ") + std::to_string(WSAGetLastError()));
		}
		if (pInst->MaxClients == pInst->ClientList.size())
		{
			// ¬озможно здесь сделать уведомление клиента о полном сервере
			cout << "Unable to accept more clients: server is full."
				<< endl;
			cout.clear();
			shutdown(ci.s, SD_BOTH);
			closesocket(ci.s);
			continue;
		}
		iNum = sizeof(sockaddr_in);
		if (getpeername(ci.s, (sockaddr*)&sa, &iNum) != SOCKET_ERROR) // попытатьс€ получить адрес клиента
		{
			InetNtopW(AF_INET, &sa.sin_addr, ci.addr, ci.CLADDRLEN);
		}
		else
		{
			wcsncpy_s(ci.addr, L"Unknown", ci.CLADDRLEN);
		}
		ci.ID = pInst->LastAvailableID;
		WFSOINF(pInst->Lock); // Acquire Lock
		pInst->ClientList.push_back(ci);
		pInst->UpdateID();
		SetEvent(pInst->hInternalEvent);
		SetEvent(pInst->Lock);
	}
}

void TcpServer::UpdateID()
{
	if (ClientList.empty())
		LastAvailableID = 0;
	else if (ClientList.size() == LastAvailableID)
		LastAvailableID++;
	else
		LastAvailableID = max(ClientList.size(), LastAvailableID);
}

void TcpServer::DisconnectByIndex(DWORD Index)
{
	auto cl_it = std::next(ClientList.begin(), Index - 1);
	auto ev_it = std::next(Events.begin(), Index);
	LastAvailableID = cl_it->ID;
	shutdown(cl_it->s, SD_BOTH);
	closesocket(cl_it->s);
	WSACloseEvent(*ev_it);
	ClientList.erase(cl_it);
	Events.erase(ev_it);
	LastAvailableID = min(ClientList.size(), LastAvailableID);
}

void TcpServer::DisconnectByID(size_t ID)
{
	//TODO: найти способ св€зать индекс событи€ и клиента
	// нормальное назначение айдишников юзерам
}

void TcpServer::Broadcast(std::vector<WSABUF>& IOBuf)
{
	DWORD iNum;
	for (auto it = ClientList.begin(); it != ClientList.end(); it++)
	{
		if (WSASend(it->s, IOBuf.data(), IOBuf.size(), &iNum, 0, NULL, NULL) == SOCKET_ERROR)
		{
			throw std::runtime_error(string("WSASend failed.Code: ") + std::to_string(WSAGetLastError()));
		}
	}
}

TcpServer::~TcpServer()
{
	Stop();
	if (Lock) CloseHandle(Lock);
	if (hInternalEvent) CloseHandle(hInternalEvent);
}