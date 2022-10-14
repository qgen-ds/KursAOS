// Server.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "ClientInfo.h"
#include "../include/WCHARException.h"

constexpr size_t REC_SIZE = 512;
constexpr size_t TXT_SIZE = 100;
constexpr size_t NUM_WORKERS = 4;
constexpr size_t MAX_CL_NUM = 25;

#pragma once
#pragma comment (lib,"wsock32.lib")
#pragma comment (lib, "ws2_32.lib")

std::queue<ClientInfo> SharedQ;
//HANDLE hSharedQMutex; 
CRITICAL_SECTION QLock;
CONDITION_VARIABLE QEmpty;
bool StopRequested;

DWORD WINAPI ClientHandler(LPVOID _In_ p); // функция обслуживания клиента

int main()
{
	using std::string;
	WSADATA wsd; // структура для WSAStartup
	SOCKET s; // основное гнездо - на которое принимаются соединения
	SOCKET sw; // гнездо, на которое перенаправляются соединения
	sockaddr_in sa; // структура данных гнезда
	unsigned short port = 3030;	// порт серверной программы
	int iNum;
	//HANDLE hWorkers[NUM_WORKERS];
	HANDLE hWorker;
	wchar_t txt[TXT_SIZE] = { 0 };
	char addr_str[20] = { 0 };
	using std::cout;
	using std::cin;
	using std::endl;
	cout << "Initializing WinSock..." << endl;
	try
	{
		if ((iNum = WSAStartup(MAKEWORD(2, 2), &wsd)) != 0)
		{
			swprintf_s(txt, TXT_SIZE, L"%s%i", L"WSAStartup error. Code: ", iNum);
			throw WCHARException(txt);
		}
		cout << "Done!" << endl << "Create Socket..." << endl;
		//s = socket(AF_INET, SOCK_STREAM, 0); // создать гнездо
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
		//if (!(hSharedQMutex = CreateMutex(NULL, FALSE, NULL)))
		//{
		//	swprintf_s(txt, TXT_SIZE, L"%s%i", L"CreateThread error. Code: ", GetLastError());
		//	closesocket(s);
		//	WSACleanup();
		//	throw WCHARException(txt);
		//}
		StopRequested = false;
		QEmpty = CONDITION_VARIABLE_INIT;
		InitializeCriticalSection(&QLock);
		if (!(hWorker = CreateThread(NULL, 0, ClientHandler, NULL, 0, NULL)))
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
			sw = accept(s, (sockaddr*)&sa, &iNum); // принять соединение
			if (sw == INVALID_SOCKET)
			{
				cout << "Error: Accept!" << endl;
				closesocket(s);
				WSACleanup();
				return 0;
			}
			iNum = sizeof(sockaddr_in);
			if (getpeername(sw, (sockaddr*)&sa, &iNum) != SOCKET_ERROR) // попытаться получить адрес клиента
			{
				InetNtopW(AF_INET, &sa.sin_addr, ci.addr, ci.CLADDRLEN);
			}
			else
			{
				wcsncpy(ci.addr, L"Unknown", ci.CLADDRLEN);
			}
			//WaitForSingleObject(hSharedQMutex, INFINITE);
			SharedQ.push(ci);
			//ReleaseMutex(hSharedQMutex);
			// запустить в отдельной нити функцию обслуживания клиента
			/*if (!CreateThread(NULL, 0, ClientHandler, (void*)sw, 0, NULL))
			{
				cout << "Error: beginthread!" << endl;
				closesocket(s);
				WSACleanup();
				return 0;
			}*/
		}
	}
	catch (WCHARException e)
	{
		e.Show();
		return 1;
	}
	closesocket(s);
	WSACleanup();
	return 0;
}

DWORD WINAPI ClientHandler(LPVOID _In_ p)
{

}

// функция обслуживания клиента
//DWORD WINAPI ClientHandler(LPVOID _In_ p)
//{
//	//TODO: сделать рабочих в виде пары потоков: один добавляет в очередь, другой ждёт ивенты и занимается обработкой клиента
//	using std::string;
//	using std::cout;
//	using std::cin;
//	using std::endl;
//	std::vector<ClientInfo> ClientV(MAX_CL_NUM);
//	WSAEVENT hEvents[MAX_CL_NUM] = { 0 };
//	//size_t EventCount = 0;
//	int Index = 0; // for use with WSAWaitForMultipleEvents
//	SOCKET s; // гнездо, с которым будет вестись обмен
//	int len; // для служебных целей
//	char buf[REC_SIZE] = { 0 }; // буфер для данных
//	string mes;
//	while (true)
//	{
//		EnterCriticalSection(&QLock);
//		while (SharedQ.empty() && !StopRequested)
//		{
//			// Buffer is empty - sleep so producers can create items.
//			SleepConditionVariableCS(&QEmpty, &QLock, INFINITE);
//		}
//		if (StopRequested && SharedQ.empty())
//		{
//			LeaveCriticalSection(&QLock);
//			break;
//		}
//		//ClientInfo const& tmp = SharedQ.front();
//		if (ClientV.size() < MAX_CL_NUM)
//		{
//			hEvents[ClientV.size()] = WSACreateEvent();
//			ClientV.push_back(SharedQ.front());
//			SharedQ.pop();
//		}
//	}
//	len = recv(s, buf, REC_SIZE, 0); // прием данных от клиента
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