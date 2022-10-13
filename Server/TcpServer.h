#pragma once
#include "pch.h"
#include "ClientInfo.h"

class TcpServer
{
public:
	static const size_t ERR_SIZE = 100;
	TcpServer(unsigned short port = 3030, size_t ThreadCount = 1, size_t MaxClients = 25, int backlog = 5);
	void Init();
	void Start();
	//void StartAcceptLoop();
	void ClearErr();
	~TcpServer();
private:
	SOCKET Socket;											// основное гнездо, на которое принимаются соединения
	int Backlog;											// Бэклог сокета
	HANDLE hAcceptEvent;									// Событие, уведомляемое при принятии нового подключения
	unsigned short Port;									// порт серверной программы
	HANDLE hAcceptor;										// Принимающий поток
	HANDLE* hWorkers;										// Массив рабочих потоков
	size_t WorkersCount;									// Длина массива
	std::list<ClientInfo> ClientList;						// Список принятых клиентов
	size_t MAX_CL_COUNT;									// Максимальное число клиентов
	HANDLE Lock;											// Замок списка клиентов
	wchar_t err[ERR_SIZE] = {0};							// Буфер для сообщений об ошибках
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);	// функция обслуживания клиента
	//static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);		// функция принятия соединений
	void StartAcceptLoop();
	void Broadcast(const char* msg, int len);				// функция рассылки сообщений всем подключённым клиентам
	void Broadcast(std::vector<WSABUF>& IOBuf);				// функция рассылки сообщений всем подключённым клиентам
};

