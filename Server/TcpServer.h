#pragma once
#include "pch.h"
#include "ClientInfo.h"

class TcpServer
{
public:
	static const size_t RECV_SIZE = 4096; // Размер буфера I/O
	TcpServer(unsigned short port = 3030, size_t maxClients = 25, int backlog = 5);
	void Init();
	void Start();
	void Stop();
	~TcpServer();
private:
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
	size_t LastAvailableID;																		// Последний освободившийся ID
	std::vector<WSAEVENT> Events;																// Вектор событий сети
	size_t MaxClients;																			// Максимальное число клиентов
	HANDLE Lock;																				// Замок списка клиентов
	void Broadcast(std::vector<WSABUF>& IOBuf);													// функция рассылки сообщений всем подключённым клиентам
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);										// функция обслуживания клиента
	static void ValidatePacket(const ClientInfo& Sender, const std::wstring& s);				// Функция проверки действительности пакета
	static void AppendSenderAddr(const ClientInfo& Sender, std::vector<WSABUF>& V, char* buf);	// Функция добавления к рассылаемому пакету адреса отправителя
	static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);											// функция принятия соединений
	void DisconnectByIndex(DWORD Index);														// Функция отключения пользоователя по индексу в списке
	void DisconnectByID(size_t ID);																// Функция отключения пользоователя по ID
};