#pragma once
#include "pch.h"

using std::string;
using std::wstring;
using std::cout;
using std::wcout;
using std::endl;

class TcpServer
{
public:
	static const size_t RECV_SIZE = 4096; // Размер буфера I/O
	TcpServer(unsigned short port = 3030, size_t maxClients = 25, int backlog = 5);
	void Start();
	void ParseCommand(const string& cmd);
	~TcpServer();
private:
	typedef unsigned int id_t;
	struct ClientInfo
	{
		static const size_t CLADDRLEN = 20;
		SOCKET s;
		wchar_t addr[CLADDRLEN];
		id_t ID;
	};
	enum class ServerStatuses
	{
		Stopped,
		Running,
		RequestedForStop
	} ServerStatus;
	DWORD DeletedIndex;																			// Индекс последнего удалённого события
	SOCKET Socket;																				// основное гнездо, на которое принимаются соединения
	int Backlog;																				// Бэклог сокета
	HANDLE hInternalEvent;																		// Событие внутренних уведомлений
	unsigned short Port;																		// порт серверной программы
	HANDLE hAcceptor;																			// Принимающий поток
	HANDLE hWorker;																				// Рабочий поток
	std::list<ClientInfo> ClientList;															// Список принятых клиентов
	id_t LastAvailableID;																		// Последний доступный для назначения ID
	std::vector<WSAEVENT> Events;																// Вектор событий сети
	size_t MaxClients;																			// Максимальное число клиентов
	HANDLE Lock;																				// Замок списка клиентов
	void Broadcast(const wstring& msg);															// Функция рассылки сообщений всем подключённым клиентам
	void SendPrivate(const ClientInfo& Sender, wstring& msg);									// Функция отправки личного сообщения
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);										// Функция обслуживания клиента
	static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);											// Функция принятия соединений
	void ValidatePacket(const ClientInfo& Sender, const wstring& msg);							// Функция проверки действительности пакета
	static void AppendSenderInfo(const ClientInfo& Sender, wstring& msg);						// Функция добавления к рассылаемому пакету адреса отправителя
	void UpdateID();																			// Функция обновления LastAvailableID
	void DisconnectGeneric(std::list<ClientInfo>::iterator cl_it, DWORD Index);
	void HandleData(std::vector<WSABUF>& IOBuf, const ClientInfo& Sender);
	void DisconnectByID(id_t ID);																// Функция отключения пользоователя по ID
	void PrintClientList();																		// Функция вывода на экран списка подключённых клиентов
	std::list<ClientInfo>::iterator FindByID(id_t ID);											// Функция нахождения клиента по его ID
#ifdef _DEBUG
	static void PrintNewClient(const ClientInfo& ci);
	static void PrintMessage(const ClientInfo& ci, const wstring& s);
#endif // _DEBUG
};