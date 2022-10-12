// Server.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "ClientInfo.h"
#include "../include/WCHARException.h"
#include "TcpServer.h"

constexpr size_t RECV_SIZE = 4096;
constexpr size_t TXT_SIZE = 100;
constexpr size_t NUM_WORKERS = 4;
constexpr size_t MAX_CL_COUNT = 25;

#pragma once
#pragma comment (lib,"wsock32.lib")
#pragma comment (lib, "ws2_32.lib")

std::list<ClientInfo> SharedV;
HANDLE VLock; 
//HANDLE hSharedQMutex; 
//CRITICAL_SECTION QLock;
//CONDITION_VARIABLE QEmpty;
//bool StopRequested;

DWORD WINAPI ClientObserver(LPVOID _In_ p); // функция обслуживания клиента
void Broadcast(const char* msg, int len);

int main(int argc, char** argv)
{
	// Create this struct on the heap so
	// we can discard of it right away
	// after successful WSAStartup call
	WSADATA* wsd;
	SOCKET s; // основное гнездо - на которое принимаются соединения
	sockaddr_in sa; // структура данных гнезда
	unsigned short port = 3030;	// порт серверной программы
	//HANDLE hWorkers[NUM_WORKERS];
	HANDLE hWorker;
	HANDLE hEvent;
	wchar_t txt[TXT_SIZE] = { 0 };
	char addr_str[20] = { 0 };
	int iNum = 0;
	using std::string;
	using std::cout;
	using std::cin;
	using std::endl;
	cout << "Initializing WinSock..." << endl;
	try
	{
		if (!(wsd = (WSADATA*)malloc(sizeof(WSADATA))) || (iNum = WSAStartup(MAKEWORD(2, 2), wsd)) != 0)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"WSAStartup error. Code: ", iNum);
			throw WCHARException(txt);
		}
		free(wsd);
		wsd = NULL;
		cout << "Done!" << endl << "Create Socket..." << endl;
		if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		{   // в случае неудачи - выдать сообщение и выйти
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"socket error. Code: ", WSAGetLastError());
			WSACleanup();
			throw WCHARException(txt);
		}
		cout << "Done!" << endl << "Binding..." << endl;
		iNum = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&iNum, sizeof(int)) == SOCKET_ERROR) // получение локального адреса
		{
			// в случае неудачи - выдать сообщение, закрыть гнездо и выйти
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"setsockopt error. Code: ", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			throw WCHARException(txt);
		}
		//if ((hEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
		//{
		//	swprintf_s(txt, TXT_SIZE, L"%s%i", L"Failed to create event for accepting socket. Code: ", WSAGetLastError());
		//	closesocket(s);
		//	WSACleanup();
		//	throw WCHARException(txt);
		//}
		if (!(hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"Failed to create event for accepting socket. Code: ", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			throw WCHARException(txt);
		}
		sa.sin_family = AF_INET; // заполнение структуры данных
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(s, (sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) // получение локального адреса
		{
			// в случае неудачи - выдать сообщение, закрыть гнездо и выйти
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"bind error. Code: ", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			throw WCHARException(txt);
		}
		cout << "Done!" << endl << "Listening..." << endl;
		if (listen(s, 5) == SOCKET_ERROR) // перевести гнездо в состояние ожидания
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"listen error. Code: ", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			throw WCHARException(txt);
		}
		//if (WSAEventSelect(s, hEvent, FD_ACCEPT) == SOCKET_ERROR)
		//{
		//	swprintf_s(txt, TXT_SIZE, L"%s%i", L"WSAEventSelect for accepting socket error. Code: ", WSAGetLastError());
		//	closesocket(s);
		//	WSACleanup();
		//	throw WCHARException(txt);
		//}
		if (!(VLock = CreateEvent(NULL, FALSE, TRUE, NULL)/*CreateMutex(NULL, FALSE, NULL)*/))
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"CreateMutex error. Code: ", GetLastError());
			closesocket(s);
			WSACleanup();
			throw WCHARException(txt);
		}
		//StopRequested = false;
		//QEmpty = CONDITION_VARIABLE_INIT;
		//InitializeCriticalSection(&QLock);
		if (!(hWorker = CreateThread(NULL, 0, ClientObserver, hEvent, 0, NULL)))
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"CreateThread error in main. Code: ", GetLastError());
			closesocket(s);
			WSACleanup();
			throw WCHARException(txt);
		}
		//for (int i = 0; i < NUM_WORKERS; i++)
		//{
		//	if (!(hWorkers[i] = CreateThread(NULL, 0, ClientHandler, (void*)sw, 0, NULL)))
		//	{
		//		swprintf_s(txt, TXT_SIZE, L"%s%i", L"CreateThread error. Code: ", WSAGetLastError());
		//		closesocket(s);
		//		WSACleanup();
		//		throw WCHARException(txt);
		//	}
		//}
		cout << "Done!" << endl << "Accepting..." << endl;
		while (1) // цикл приема соединений
		{
			ClientInfo ci = { 0 };
			iNum = sizeof(sockaddr_in);
			ci.s = accept(s, (sockaddr*)&sa, &iNum); // принять соединение
			if (ci.s == INVALID_SOCKET)
			{
				cout << "Error: Accept!" << endl;
				closesocket(s);
				WSACleanup();
				return 1;
			}
			if (MAX_CL_COUNT == SharedV.size())
			{
				cout << "Unable to accept more clients: server is full." << endl;
				shutdown(ci.s, SD_BOTH);
				closesocket(s);
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
			switch (WaitForSingleObject(VLock, INFINITE)) // Acquire VLock
			{
			case WAIT_FAILED:
				swprintf_s(txt, TXT_SIZE, L"%s%i", L"WaitForSingleObject failed. Code: ", GetLastError());
				throw WCHARException(txt);
			case WAIT_ABANDONED:
				throw WCHARException(L"Abandoned VLock");
			}
			SharedV.push_back(ci);
			SetEvent(hEvent);
			SetEvent(VLock);
			//TODO: отсюда; переложить создание ивента и связку его с сокетом на принимающий данные тред, т.к. мы не хотим ни ждать, пока он освободится от ожидания ивентов, ни менять массив ивентов, не зная его состояния
		}
	}
	catch (WCHARException e)
	{
		e.Show();
		//TerminateThread(hWorker, 1);
		//CloseHandle(hWorker);
		return 1;
	}
	closesocket(s);
	WSACleanup();
	return 0;
}
DWORD WINAPI ClientObserver(LPVOID _In_ p)
{
	//std::map<DWORD, ClientInfo> CM;
	//WSAEVENT Events[MAX_CL_COUNT + 1] = { 0 };
	std::vector<WSAEVENT> Events;
	Events.reserve(MAX_CL_COUNT + 1);
	DWORD Index = 0; // for use with WSAWaitForMultipleEvents
	DWORD iNum = 0;
	//size_t EventCount = 1;
	wchar_t txt[TXT_SIZE] = { 0 };
	char buf[RECV_SIZE] = { 0 };
	Events.push_back(p);
	while (true)
	{
		switch (Index = WSAWaitForMultipleEvents(Events.size(), Events.data(), FALSE, INFINITE, FALSE) - WSA_WAIT_EVENT_0)
		{
		case 0:
		{
			// Handle new accepted connection
			HANDLE NewSocketEvent;
			switch (WaitForSingleObject(VLock, INFINITE)) // Make sure the client list won't be changed while we work
			{
			case WAIT_FAILED:
				swprintf_s(txt, TXT_SIZE, L"%s%i", L"WaitForSingleObject failed. Code: ", GetLastError());
				WCHARException(txt).Show();
				break;
			case WAIT_ABANDONED:
				WCHARException(L"Abandoned VLock").Show();
				break;
			}
			if ((NewSocketEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
			{
				swprintf_s(txt, TXT_SIZE, L"%s%i", L"Failed to create event for new accepted socket. Code: ", WSAGetLastError());
				WCHARException(txt).Show();
				break;
			}
			if (WSAEventSelect(std::next(SharedV.begin(), Events.size() - 1)->s, NewSocketEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
			{
				swprintf_s(txt, TXT_SIZE, L"%s%i", L"WSAEventSelect for accepted socket error. Code: ", WSAGetLastError());
				WCHARException(txt).Show();
				break;
			}
			Events.push_back(NewSocketEvent);
			SetEvent(VLock);
			break;
		}
		case WSA_WAIT_FAILED - WSA_WAIT_EVENT_0:
			// Handle the occured error
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"WSAWaitForMultipleEvents failed. Code: ", GetLastError());
			WCHARException(txt).Show();
			break;
		default:
		{
			// Handle client message
			switch (WaitForSingleObject(VLock, INFINITE)) // Make sure the client list won't be changed while we're working
			{
			case WAIT_FAILED:
				swprintf_s(txt, TXT_SIZE, L"%s%i", L"WaitForSingleObject failed. Code: ", GetLastError());
				WCHARException(txt).Show();
				break;
			case WAIT_ABANDONED:
				WCHARException(L"Abandoned VLock").Show();
				break;
			}
			switch (iNum = recv(std::next(SharedV.begin(), Index)->s, buf, RECV_SIZE, 0))
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
					swprintf_s(txt, TXT_SIZE, L"%s%i", L"recv failed. Code: ", iNum);
					WCHARException(txt).Show();
				}
				break;
			}
			case RECV_SIZE:
				// Handle full buffer
				break;
			default:
			{
				// Handle incoming data
				Broadcast(buf, iNum);
				break;
			}
			}
			SetEvent(VLock);
			ZeroMemory(buf, RECV_SIZE);
			break;
		}
		}
		ResetEvent(Events[Index]);
	}
}

void Broadcast(const char* msg, int len)
{
	wchar_t txt[TXT_SIZE] = { 0 };
	for (auto it = SharedV.begin(); it != SharedV.end(); it++)
	{
		if (send(it->s, msg, len, 0) == SOCKET_ERROR)		// TODO: Replace with WSASend for gathering output
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"send failed. Code: ", WSAGetLastError());
			throw WCHARException(txt);
		}
		//if(WSASend(it->s, )
	}
}

// функция обслуживания клиента
//DWORD WINAPI ClientHandler(LPVOID _In_ p)
//{
//	//TODO: сделать рабочих в виде пары потоков: один добавляет в очередь, другой ждёт ивенты и занимается обработкой клиента
//	using std::string;
//	using std::cout;
//	using std::cin;
//	using std::endl;
//	std::vector<ClientInfo> ClientV(MAX_CL_COUNT);
//	WSAEVENT hEvents[MAX_CL_COUNT] = { 0 };
//	//size_t EventCount = 0;
//	int Index = 0; // for use with WSAWaitForMultipleEvents
//	SOCKET s; // гнездо, с которым будет вестись обмен
//	int len; // для служебных целей
//	char buf[RECV_SIZE] = { 0 }; // буфер для данных
//	string mes;
//	while (true)
//	{
//		EnterCriticalSection(&QLock);
//		while (SharedV.empty() && !StopRequested)
//		{
//			// Buffer is empty - sleep so producers can create items.
//			SleepConditionVariableCS(&QEmpty, &QLock, INFINITE);
//		}
//		if (StopRequested && SharedV.empty())
//		{
//			LeaveCriticalSection(&QLock);
//			break;
//		}
//		//ClientInfo const& tmp = SharedV.front();
//		if (ClientV.size() < MAX_CL_COUNT)
//		{
//			hEvents[ClientV.size()] = WSACreateEvent();
//			ClientV.push_back(SharedV.front());
//			SharedV.pop();
//		}
//	}
//	len = recv(s, buf, RECV_SIZE, 0); // прием данных от клиента
//	if (len == SOCKET_ERROR) // проверка на ошибку
//	{
//		cout << "Error: recv!" << endl;
//		closesocket(s);
//		return 1;
//	}
//
//	std::wcout << L"Message: " << (wchar_t*)buf << endl;
//	mes = string("Your messeage \"") + buf + "\" was received!";
//	if (send(s, mes.c_str(), mes.size(), 0) == SOCKET_ERROR) // отправка данных
//	{
//		cout << "Error: send!" << endl;
//		closesocket(s);
//		return 1;
//	}
//
//	closesocket(s); // закрытие гнезда
//	cout << "Client was served!" << endl; // выдача сообщения
//	return 0;
//}