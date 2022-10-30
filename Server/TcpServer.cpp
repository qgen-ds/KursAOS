#include "pch.h"
#include "TcpServer.h"
#include <algorithm>
#include "functions.h"

TcpServer::TcpServer(unsigned short port, size_t maxClients, int backlog)
{
	sockaddr_in sa;
	DWORD iNum = 1;
	Port = port;
	Backlog = backlog;
	MaxClients = maxClients;
	ServerStatus = ServerStatuses::Stopped;
	Lock = hAcceptor = hWorker = hInternalEvent = NULL;
	Socket = LastAvailableID = 0;
	Events.reserve(MaxClients + 1);
	DeletedIndex = 0;
	if (ServerStatus != ServerStatuses::Stopped)
		throw std::runtime_error("The server doesn't seem to be stopped.");
	if (!(hInternalEvent || (hInternalEvent = CreateEvent(NULL, TRUE, FALSE, NULL))))
	{
		throw std::runtime_error(string("Failed to create event for accepting socket. Code: ") + std::to_string(GetLastError()));
	}
	if (!(Lock || (Lock = CreateMutex(NULL, FALSE, NULL))))
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
}

void TcpServer::Start()
{
	int iNum = sizeof(sockaddr_in);
	sockaddr_in addr;
	char a[50] = { 0 };
	if(ServerStatus != ServerStatuses::Stopped)
		throw std::runtime_error("Server isn't stopped.");
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

// Abandon all hope ye who enter here
DWORD CALLBACK TcpServer::ClientObserver(LPVOID _In_ p)
{
	TcpServer* const pInst = static_cast<TcpServer*>(p);
	std::list<ClientInfo>::iterator cl;
	std::vector<WSABUF> IOBuf; // Вектор структур WSABUF
	DWORD Index = 0; // for use with WSAWaitForMultipleEvents
	DWORD iNum = 0;
	WSAEVENT NewSocketEvent;
	char dummybuffer[RECV_SIZE] = { 0 };
	IOBuf.push_back(WSABUF{ RECV_SIZE, dummybuffer }); // Первая структура хранится на стеке
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
					return 0;
				}
				// Connect new client
				WFSOINF(pInst->Lock); // Make sure the client list won't be changed while we work
				if ((NewSocketEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
				{
					throw std::runtime_error(string("Failed to create event for new accepted socket. Code: ") + std::to_string(WSAGetLastError()));
				}
				if (WSAEventSelect(pInst->ClientList.crbegin()->s, NewSocketEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
				{
					throw std::runtime_error(string("WSAEventSelect for accepted socket error. Code: ") + std::to_string(WSAGetLastError()));
				}
				pInst->Events.push_back(NewSocketEvent);
				ResetEvent(pInst->Events[0]);
				break;
			}
			case WSA_WAIT_FAILED - WSA_WAIT_EVENT_0: // Handle the occured error
				throw std::runtime_error(string("WSAWaitForMultipleEvents failed. Code: ") + std::to_string(WSAGetLastError()));
			default: // Handle client message
			{
				WFSOINF(pInst->Lock); // Make sure the client list won't be changed while we work
				if (pInst->DeletedIndex == Index)
				{
					pInst->DeletedIndex = 0;
					break;
				}
				cl = std::next(pInst->ClientList.begin(), Index - 1);
				while (true)
				{
					switch (IOBuf.back().len = recv(cl->s, IOBuf.back().buf, IOBuf.back().len, 0))
					{
					case 0: // Handle disconnection
					{
						pInst->DisconnectGeneric(cl, Index);
						break;
					}
					case SOCKET_ERROR: // Handle the occured error
					{
						switch (iNum = WSAGetLastError())
						{
						case WSAECONNRESET: // Handle abrupt connection reset							
							pInst->DisconnectGeneric(cl, Index);
							break;
						case WSAEWOULDBLOCK:
							// Сюда попадаем только в случае кратности
							// входных данных размеру одного буфера,
							// поэтому надо удалить выделенный,
							// но не использованный последний буфер
							delete[] IOBuf.back().buf;
							IOBuf.pop_back();
							pInst->HandleData(IOBuf, *cl);
							break;
						default:
							throw std::runtime_error(string("recv failed. Code: ") + std::to_string(iNum));
						}
						break;
					}
					default: // Handle incoming data
					{
						pInst->HandleData(IOBuf, *cl);
						ResetEvent(pInst->Events[Index]);
						break;
					}
					case RECV_SIZE: // Handle full buffer
						IOBuf.push_back(WSABUF{ RECV_SIZE, new char[RECV_SIZE] });
						continue;
					}
					break; // break inner while
				} // inner while
			} // Handle client message
			} // switch WSAWAitForMultipleObjects
			IOBuf[0].len = RECV_SIZE;
		} // try
		catch (wchar_error& e)
		{
			wcout << e.what()
				<< endl
				<< L'>';
			wcout.clear();
		}
		catch (std::exception& e)
		{
			cout << e.what()
				<< endl
				<< '>';
			cout.clear();
		}
		ReleaseMutex(pInst->Lock);
	} // outer while
}

DWORD CALLBACK TcpServer::AcceptLoop(LPVOID _In_ p)
{
	TcpServer* const pInst = static_cast<TcpServer*>(p);
	sockaddr_in sa;
	int iNum = 1;
	while (1) // цикл приема соединений
	{
		ClientInfo ci = { 0 };
		iNum = sizeof(sockaddr_in);
#pragma warning(suppress: 28193)
		ci.s = accept(pInst->Socket, (sockaddr*)&sa, &iNum); // принять соединение
		if (pInst->ServerStatus == ServerStatuses::RequestedForStop)
			return 0;
		if (ci.s == INVALID_SOCKET)
		{
			throw std::runtime_error(string("accept failed. Code: ") + std::to_string(WSAGetLastError()));
		}
		if (pInst->MaxClients == pInst->ClientList.size())
		{
			// Возможно здесь сделать уведомление клиента о полном сервере
			cout << "Unable to accept more clients: server is full."
				<< endl;
			cout.clear();
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
		ci.ID = pInst->LastAvailableID;
		WFSOINF(pInst->Lock); // Acquire Lock
		pInst->ClientList.push_back(ci);
		pInst->UpdateID();
		SetEvent(pInst->hInternalEvent);
		ReleaseMutex(pInst->Lock);
#ifdef _DEBUG
		PrintNewClient(ci);
#endif // _DEBUG
	}
}

void TcpServer::ValidatePacket(const ClientInfo& Sender, const wstring& s)
{
	woss_t err;
	if (s.substr(s.size() - 2) != L"#&")
	{
		err << L"Error: invalid packet recieved from "
			<< Sender.addr
			<< L" with ID "
			<< Sender.ID
			<< L". Total packet size: "
			<< s.size() * sizeof(wchar_t)
			<< L" bytes."
			<< endl;
		DisconnectByID(Sender.ID);
		throw wchar_error(err.str());
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

void TcpServer::DisconnectGeneric(std::list<ClientInfo>::iterator cl_it, DWORD Index)
{
	WFSOINF(Lock);
	ClientInfo l_cl(*cl_it);
	auto ev_it = std::next(Events.begin(), Index);
	LastAvailableID = cl_it->ID;
	shutdown(cl_it->s, SD_BOTH);
	closesocket(cl_it->s);
	WSACloseEvent(*ev_it);
	ClientList.erase(cl_it);
	Events.erase(ev_it);
	LastAvailableID = min(ClientList.size(), LastAvailableID);
	DeletedIndex = Index;
	ReleaseMutex(Lock);
	wcout << L"Client #" << Index - 1
		<< L" with IP address " << l_cl.addr
		<< L" and ID " << l_cl.ID
		<< L" disconnected." << endl << L'>';
	wcout.clear();
}

void TcpServer::HandleData(std::vector<WSABUF>& IOBuf, const ClientInfo& Sender)
{
	wstring msg = Join<wchar_t>(IOBuf);
	std::future<void> f = std::async(std::launch::async, [&]() {
		ValidatePacket(Sender, msg);
		AppendSenderInfo(Sender, msg);
#ifdef _DEBUG
		PrintMessage(Sender, msg);
#endif // _DEBUG
		if (msg[0] == L'@')
		{
			SendPrivate(Sender, msg);
		}
		else
		{
			Broadcast(msg);
		}
		});
	ZeroMemory(IOBuf[0].buf, IOBuf[0].len);
	if (IOBuf.size() > 1)
	{
		std::for_each(std::next(IOBuf.begin()), IOBuf.end(), [](WSABUF& e) { delete[] e.buf; });
		IOBuf.erase(std::next(IOBuf.begin()), IOBuf.end());
	}
}

void TcpServer::AppendSenderInfo(const ClientInfo& Sender, wstring& msg)
{
	woss_t oss;
	msg.pop_back();
	oss << msg << Sender.addr << L'#' << Sender.ID << L"#&";
	msg = oss.str();
}

void TcpServer::DisconnectByID(id_t ID)
{
	auto it = ClientList.begin();
	size_t i = 1;
	while(it != ClientList.end())
	{
		if (it->ID == ID)
			break;
		it++;
		i++;
	}
	if (it == ClientList.end())
		cout << "No client with that ID found." << endl;
	else
		DisconnectGeneric(it, i);
}

std::list<TcpServer::ClientInfo>::iterator TcpServer::FindByID(id_t ID)
{
	auto it = ClientList.begin();
	while (it != ClientList.end())
	{
		if (it->ID == ID)
			break;
		it++;
	}
	return it;
}

void TcpServer::Broadcast(const wstring& msg)
{
	for (auto it = ClientList.begin(); it != ClientList.end(); it++)
	{
		if (send(it->s, (char*)msg.c_str(), msg.size() * sizeof(wchar_t), 0) == SOCKET_ERROR)
		{
			throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
		}
	}
}

void TcpServer::SendPrivate(const ClientInfo& Sender, wstring& msg)
{
	try
	{
		//Для ПМа сообщение клиенту нужно пересобрать, поэтому разбиваем его по новой
		size_t pos = msg.find(L' ');
		id_t reciever = std::stoul(msg.substr(1, pos));
		// 0 - ID
		// 1 - IP address
		// 2 - Name
		// 3 - Message
		auto Data = Split(msg.erase(msg.size() - 2), L'#', pos + 1);
		auto RIt = FindByID(reciever);
		if (RIt != ClientList.end())
		{
			msg = wstring(L"PM from ") + *std::next(Data.begin(), 2) + L'(' + *std::next(Data.begin()) + L')' + L" (ID " + *Data.begin() + L"): " + *std::next(Data.begin(), 3);
			if (send(RIt->s, (char*)msg.c_str(), msg.size() * sizeof(wchar_t), 0) == SOCKET_ERROR)
			{
				throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
			}
			// Return to sender
			msg = wstring(L"PM sent to ") + *std::next(Data.begin()) + L" (ID " + std::to_wstring(RIt->ID) + L"): " + *std::next(Data.begin(), 3);
			if (send(Sender.s, (char*)msg.c_str(), msg.size() * sizeof(wchar_t), 0) == SOCKET_ERROR)
			{
				throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
			}
		}
		else
		{
			if (send(Sender.s, (char*)L"No client with such ID found.", 29 * sizeof(wchar_t), 0) == SOCKET_ERROR)
			{
				throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
			}
		}
	}
	catch (std::logic_error& e)
	{
		woss_t oss;
		oss << L"Invalid ID argument: " << e.what();
		if (send(Sender.s, (char*)oss.str().c_str(), oss.str().size() * sizeof(wchar_t), 0) == SOCKET_ERROR)
		{
			throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
		}
	}
}

void TcpServer::ParseCommand(const string& cmd)
{
	using namespace std;
	istringstream iss(cmd);
	vector<string> tokens;
	copy(istream_iterator<string>(iss),
		istream_iterator<string>(),
		back_inserter(tokens));
	if (tokens[0] == "list")
	{
		PrintClientList();
	}
	else if (tokens[0] == "kick")
	{
		if (tokens.size() < 2)
		{
			cout << "You must specify Client ID." << endl;
			return;
		}
		DisconnectByID(std::stoul(tokens[1]));
	}
	else
	{
		cout << "Invalid command." << endl;
	}
}

void TcpServer::PrintClientList()
{
	size_t i = 0;
	WFSOINF(Lock); // Acquire Lock
	if (ClientList.empty())
	{
		cout << "No clients connected." << endl;
	}
	else
	{
		for (auto it = ClientList.begin(); it != ClientList.end(); it++, i++)
		{
			wcout << L"Client #" << i
				<< L": address: " << it->addr
				<< L"; ID: " << it->ID
				<< endl;
		}
		wcout.clear();
	}
	ReleaseMutex(Lock);
}

TcpServer::~TcpServer()
{
	const HANDLE arr[] = { hAcceptor, hWorker };
	if (ServerStatus == ServerStatuses::Stopped)
	{
		// Start threads for graceful termination
		ResumeThread(hAcceptor);
		ResumeThread(hWorker);
	}
	ServerStatus = ServerStatuses::RequestedForStop;
	SetEvent(hInternalEvent);
	closesocket(Socket);
	for (size_t i = 1; i < Events.size(); i++)
	{
		WSACloseEvent(Events[i]);
	}
	for (auto it = ClientList.begin(); it != ClientList.end(); it++)
	{
		shutdown(it->s, SD_BOTH);
		closesocket(it->s);
	}
	WaitForMultipleObjects(2, arr, TRUE, INFINITE);
	ReleaseMutex(Lock);
	CloseHandle(hWorker);
	CloseHandle(hAcceptor);
	CloseHandle(Lock);
	CloseHandle(hInternalEvent);
	Events.clear();
	hWorker = hAcceptor = NULL;
	ServerStatus = ServerStatuses::Stopped;
}