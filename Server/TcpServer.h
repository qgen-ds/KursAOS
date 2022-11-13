#pragma once
#include "pch.h"
#include "..\include\Packet.h"

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
	friend struct Packet;
	typedef unsigned int id_t;
	struct ClientInfo
	{
		static const size_t CLADDRLEN = 20;
		SOCKET s;
		wchar_t addr[CLADDRLEN];
		id_t ID;
	};
	struct AP
	{
		TcpServer* pInst;
		HANDLE hEvent;
		ClientInfo* ci;
	};
	// В структуре DP будет информация о клиенте,
	// от которого поступил сигнал,
	// то есть, которого мы сейчас обрабатываем
	struct DP
	{
		TcpServer* pInst;
		HANDLE hEvent;
		std::list<ClientInfo>::iterator cl_it;	// Итератор обрабатываемого клиента
		DWORD Index{};							// Индекс события клиента
	};
	enum class ServerStatuses
	{
		Stopped,
		Running,
		RequestedForStop
	} ServerStatus;
	SOCKET Socket;																				// основное гнездо, на которое принимаются соединения
	int Backlog;																				// Бэклог сокета
	unsigned short Port;																		// порт серверной программы
	HANDLE hObserverEvent;																		// Событие внутренних уведомлений
	HANDLE hAcceptor;																			// Принимающий поток
	HANDLE hObserver;																			// Рабочий поток
	std::list<ClientInfo> ClientList;															// Список принятых клиентов
	id_t LastAvailableID;																		// Последний доступный для назначения ID
	std::vector<WSAEVENT> Events;																// Вектор событий сети
	size_t MaxClients;																			// Максимальное число клиентов
	HANDLE Lock;																				// Замок списка клиентов
	void Broadcast(const ClientInfo& Sender, Packet& p);										// Функция рассылки сообщений всем подключённым клиентам
	void SendPrivate(const ClientInfo& Sender, Packet& p);										// Функция отправки личного сообщения
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);										// Функция обслуживания клиента
	static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);											// Функция принятия соединений
	static void CALLBACK AddClientProc(ULONG_PTR p);
	void ValidatePacket(const ClientInfo& Sender, const string& msg);							// Функция проверки действительности пакета
	void UpdateID();																			// Функция обновления LastAvailableID
	static void CALLBACK DisconnectProc(ULONG_PTR p);
	void DisconnectGeneric(std::list<ClientInfo>::iterator cl_it, DWORD Index);
	void DisconnectByID(id_t ID);																// Функция отключения пользоователя по ID
	void HandleData(std::vector<WSABUF>& IOBuf, const ClientInfo& Sender);
	void PrintClientList();																		// Функция вывода на экран списка подключённых клиентов
	std::list<ClientInfo>::iterator FindByID(id_t ID);											// Функция нахождения клиента по его ID
#ifdef _DEBUG
	static void PrintNewClient(const ClientInfo& ci);
	static void PrintMessage(const ClientInfo& ci, const Packet& p);
#endif // _DEBUG
};