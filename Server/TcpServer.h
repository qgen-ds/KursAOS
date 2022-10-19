#pragma once
#include "pch.h"
#include <list>

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
	void Init();
	void Start();
	void Stop();
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
		Initialised,
		Running,
		RequestedForStop
	} ServerStatus;
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
	void Broadcast(std::vector<WSABUF>& IOBuf);													// функция рассылки сообщений всем подключённым клиентам
	void SendPrivate(id_t R, const ClientInfo& Sender, std::vector<WSABUF>& IOBuf);
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);										// функция обслуживания клиента
	static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);											// функция принятия соединений
	static void ValidatePacket(const ClientInfo& Sender, const wstring& s);						// Функция проверки действительности пакета
	static void AppendSenderAddr(const ClientInfo& Sender, std::vector<WSABUF>& V, char* buf);	// Функция добавления к рассылаемому пакету адреса отправителя
	void UpdateID();																			// Функция обновления LastAvailableID
	void DisconnectGeneric(std::list<ClientInfo>::iterator cl_it, DWORD Index);
	void DisconnectByIndex(DWORD Index);														// Функция отключения пользоователя по индексу в списке
	void DisconnectByID(id_t ID);																// Функция отключения пользоователя по ID
	void PrintClientList();
	auto FindByID(id_t ID);
#ifdef _DEBUG
	static void PrintNewClient(const ClientInfo& ci);
	static void PrintMessage(const ClientInfo& Sender, const wstring& s);
#endif // _DEBUG
};