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
	Lock = hAcceptor = hObserver = NULL;
	Socket = LastAvailableID = 0;
	Events.reserve(MaxClients);
	if (!(hObserverEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
	{
		throw std::runtime_error(string("Failed to create observer event. Code: ") + std::to_string(GetLastError()));
	}
	if (!(Lock = CreateMutex(NULL, FALSE, NULL)))
	{
		throw std::runtime_error(string("Failed to create ClientList lock. Code: ") + std::to_string(GetLastError()));
	}
	if (!(hObserver = CreateThread(NULL, 0, TcpServer::ClientObserver, this, CREATE_SUSPENDED, NULL)))
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
	if (ResumeThread(hAcceptor) == (DWORD)-1 || ResumeThread(hObserver) == (DWORD)-1)
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

DWORD CALLBACK TcpServer::ClientObserver(LPVOID _In_ p)
{
	TcpServer* const pInst = static_cast<TcpServer*>(p);
	std::vector<WSABUF> IOBuf; // Вектор структур WSABUF
	std::list<ClientInfo>::iterator cl;
	DWORD iNum = 0, Index = 0;
	char dummybuffer[RECV_SIZE] = { 0 };
	IOBuf.push_back(WSABUF{ RECV_SIZE, dummybuffer }); // Первая структура хранится на стеке
	while (true)
	{
		try
		{
			switch (Index = (pInst->Events.empty() ? SleepEx(INFINITE, TRUE) :
				WSAWaitForMultipleEvents(pInst->Events.size(), pInst->Events.data(), FALSE, INFINITE, TRUE) - WSA_WAIT_EVENT_0))
			{
			case WSA_WAIT_IO_COMPLETION - WSA_WAIT_EVENT_0: // Возврат по тревожному состоянию
			{
				// Остановить сервер
				if (pInst->ServerStatus == ServerStatuses::RequestedForStop)
				{
					return 0;
				}
				continue;
			}
			case WSA_WAIT_FAILED - WSA_WAIT_EVENT_0: // Обработать ошибку
				throw std::runtime_error(string("WSAWaitForMultipleEvents failed. Code: ") + std::to_string(WSAGetLastError()));
			default: // Обработать сообщение от клиента
			{
				WFSOINF(pInst->Lock); // Забираем замок
				//if (pInst->DeletedIndex == Index)
				//{
				//	pInst->DeletedIndex = 0;
				//	break;
				//}
				cl = std::next(pInst->ClientList.begin(), Index);
				while (true)
				{
					switch (IOBuf.back().len = recv(cl->s, IOBuf.back().buf, IOBuf.back().len, 0))
					{
					case 0: // Клиент отключился
					{
						pInst->DisconnectGeneric(cl, Index);
						break;
					}
					case SOCKET_ERROR: // Обработать ошибку
					{
						switch (iNum = WSAGetLastError())
						{
						case WSAECONNRESET: // Обработать аварийное отключение клиента							
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
					default: // Обработать входящие данные
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
	AP ap;
	ap.pInst = pInst;
	if (!(ap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
	{
		return GetLastError();
	}
	while (true) // цикл приема соединений
	{
		ClientInfo ci = { 0 };
		iNum = sizeof(sockaddr_in);
#pragma warning(suppress: 28193)
		ci.s = accept(pInst->Socket, (sockaddr*)&sa, &iNum); // принять соединение
		if (pInst->ServerStatus == ServerStatuses::RequestedForStop)
		{
			CloseHandle(ap.hEvent);
			return 0;
		}
		if (ci.s == INVALID_SOCKET)
		{
			CloseHandle(ap.hEvent);
			return WSAGetLastError();
		}
		if (pInst->MaxClients == pInst->ClientList.size())
		{
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
		ap.ci = &ci;
		QueueUserAPC(AddClientProc, pInst->hObserver, (ULONG_PTR)&ap);
		WFSOINF(ap.hEvent);
	}
}

void CALLBACK TcpServer::AddClientProc(ULONG_PTR p)
{
	WSAEVENT NewSocketEvent;
	AP param(*(AP*)p);
	ClientInfo ci(std::move(*param.ci));
	SetEvent(param.hEvent);
	WFSOINF(param.pInst->Lock);
	ci.ID = param.pInst->LastAvailableID;
	param.pInst->ClientList.push_back(ci);
	param.pInst->UpdateID();
	if ((NewSocketEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
	{
		throw std::runtime_error(string("Failed to create event for new accepted socket. Code: ") + std::to_string(WSAGetLastError()));
	}
	if (WSAEventSelect(param.pInst->ClientList.crbegin()->s, NewSocketEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		throw std::runtime_error(string("WSAEventSelect for accepted socket error. Code: ") + std::to_string(WSAGetLastError()));
	}
	param.pInst->Events.push_back(NewSocketEvent);
	ReleaseMutex(param.pInst->Lock);
#ifdef _DEBUG
	PrintNewClient(ci);
#endif // _DEBUG
} 

void TcpServer::ValidatePacket(const ClientInfo& Sender, const string& s)
{
	woss_t err;
	if (*(int32_t*)(s.c_str() + s.size() - 4) != 0x17171717)
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

void CALLBACK TcpServer::DisconnectProc(ULONG_PTR p)
{
	DP param(*(DP*)(p));
	SetEvent(param.hEvent);
	param.pInst->DisconnectGeneric(param.cl_it, param.Index);
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
	ReleaseMutex(Lock);
	wcout << L"Client #" << Index
		<< L" with IP address " << l_cl.addr
		<< L" and ID " << l_cl.ID
		<< L" disconnected." << endl << L'>';
	wcout.clear();
}

void TcpServer::HandleData(std::vector<WSABUF>& IOBuf, const ClientInfo& Sender)
{
	// объединяем строку в массив байтов
	string msg = Join<char>(IOBuf);
	std::thread([=, Sender = (Sender)] () mutable {
		try
		{
			ValidatePacket(Sender, msg);
			Packet pkt = Packet::FromString(msg);
#ifdef _DEBUG
			PrintMessage(Sender, pkt);
#endif // _DEBUG
			switch(pkt.Code)
			{
			case COMMAND_PRIVATE_MESSAGE:
				SendPrivate(Sender, pkt);
				break;
			case COMMAND_COMMON_MESSAGE:
				Broadcast(Sender, pkt);
				break;
			}
		}
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
		}).detach();
	ZeroMemory(IOBuf[0].buf, IOBuf[0].len);
	if (IOBuf.size() > 1)
	{
		std::for_each(std::next(IOBuf.begin()), IOBuf.end(), [](WSABUF& e) { delete[] e.buf; });
		IOBuf.erase(std::next(IOBuf.begin()), IOBuf.end());
	}
}

void TcpServer::DisconnectByID(id_t ID)
{
	WFSOINF(Lock);
	auto it = ClientList.begin();
	size_t i = 0;
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
	{
		DP dp{ this, hObserverEvent, it, i };
		QueueUserAPC(DisconnectProc, hObserver, (ULONG_PTR)&dp);
		WFSOINF(hObserverEvent);
	}
	ReleaseMutex(Lock);
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

void TcpServer::Broadcast(const ClientInfo& Sender, Packet& p)
{
	static std::vector<WSABUF> buf(6);
	DWORD iNum = 0;
	p.Code = htonl(p.Code);
	p.NameLen = htonl(p.NameLen);
	id_t tmp = htonl(Sender.ID);
	buf[0].buf = (CHAR*)&p.Code;
	buf[0].len = 4;
	buf[1].buf = (CHAR*)&p.NameLen;
	buf[1].len = 4;
	buf[2].buf = (CHAR*)p.Name.c_str();
	buf[2].len = p.Name.size() * sizeof(wchar_t);
	buf[3].buf = (CHAR*)p.Message.c_str();
	buf[3].len = p.Message.size() * sizeof(wchar_t);
	buf[4].buf = (CHAR*)Sender.addr;
	buf[4].len = wcslen(Sender.addr) * sizeof(wchar_t);
	buf[5].buf = (CHAR*)&tmp;
	buf[5].len = sizeof(id_t);
	for (auto it = ClientList.begin(); it != ClientList.end(); it++)
	{
		if (WSASend(it->s, buf.data(), buf.size(), &iNum, 0, NULL, NULL) == SOCKET_ERROR)
		{
			throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
		}
	}
}

void TcpServer::SendPrivate(const ClientInfo& Sender, Packet& p)
{
	static std::vector<WSABUF> buf(6);
	DWORD iNum = 0;
	id_t tmp = htonl(Sender.ID);
	p.Code = htonl(COMMAND_PRIVATE_MESSAGE);
	p.NameLen = htonl(p.NameLen);
	buf[0].buf = (CHAR*)&p.Code;
	buf[0].len = 4;
	buf[1].buf = (CHAR*)p.Message.c_str();
	buf[1].len = p.Message.size() * sizeof(wchar_t);
	buf[2].buf = (CHAR*)Sender.addr;
	buf[2].len = wcslen(Sender.addr) * sizeof(wchar_t);
	buf[3].buf = (CHAR*)&tmp;
	buf[3].len = sizeof(id_t);
	buf[4].buf = (CHAR*)&p.NameLen;
	buf[4].len = 4;
	buf[5].buf = (CHAR*)p.Name.c_str();
	buf[5].len = p.Name.size() * sizeof(wchar_t);
	try
	{
		size_t pos = p.Message.find(L' ');
		id_t reciever = std::stoul(p.Message.substr(0, pos));
		p.Message = p.Message.substr(pos + 1);
		auto RIt = FindByID(reciever);
		if (RIt != ClientList.end())
		{
			if (WSASend(RIt->s, buf.data(), buf.size(), &iNum, 0, NULL, NULL) == SOCKET_ERROR)
			{
				throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
			}
			// Вернуть отправителю
			// Не нужно возвращать имя и его длину
			p.Code = htonl(COMMAND_PM_RETURN);
			tmp = htonl(reciever);
			if (WSASend(Sender.s, buf.data(), buf.size() - 2, &iNum, 0, NULL, NULL) == SOCKET_ERROR)
			{
				throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
			}
			//wstring msg = wstring(L"PM from ") + p.Name + L'(' + Sender.addr + L')' + L" (ID " + std::to_wstring(Sender.ID) + L"): " + p.Message;
			//if (send(RIt->s, (char*)msg.c_str(), msg.size() * sizeof(wchar_t), 0) == SOCKET_ERROR)
			//{
			//	throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
			//}
			//// Вернуть отправителю
			//msg = wstring(L"PM sent to ") + RIt->addr + L" (ID " + std::to_wstring(reciever) + L"): " + p.Message;
			//if (send(Sender.s, (char*)msg.c_str(), msg.size() * sizeof(wchar_t), 0) == SOCKET_ERROR)
			//{
			//	throw std::runtime_error(string("send failed. Code: ") + std::to_string(WSAGetLastError()));
			//}
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
	const HANDLE arr[] = { hAcceptor, hObserver };
	if (ServerStatus == ServerStatuses::Stopped)
	{
		// Запускаем потоки для корректного завершения
		ResumeThread(hAcceptor);
		ResumeThread(hObserver);
	}
	ServerStatus = ServerStatuses::RequestedForStop;
	// Вызываем Sleep для вывода Наблюдателя из спячки
	QueueUserAPC(Sleep, hObserver, 0);
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
	CloseHandle(hObserver);
	CloseHandle(hAcceptor);
	CloseHandle(hObserverEvent);
	CloseHandle(Lock);
	Events.clear();
	hObserver = hAcceptor = NULL;
	ServerStatus = ServerStatuses::Stopped;
}