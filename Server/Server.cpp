// Server.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <WinSock2.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <Windows.h>
#include <ws2tcpip.h>

#define REC_SIZE 512

#pragma once
#pragma comment (lib,"wsock32.lib")
#pragma comment (lib, "ws2_32.lib")


DWORD WINAPI ConnectHandler(LPVOID _In_ ptr); // функция обслуживания клиента

int main()
{
	using std::string;
	WSADATA wsd; // структура для WSAStartup
	SOCKET s; // основное гнездо - на которое принимаются соединения
	SOCKET sw; // гнездо, на которое перенаправляются соединения
	sockaddr_in sa; // структура данных гнезда
	unsigned short port = 3030;	// порт серверной программы
	int a; // для служебных целей
	char addr_str[20] = { 0 };
	using std::cout;
	using std::cin;
	using std::endl;

	cout << "Initializing WinSock..." << endl;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) // инициализировать библиотеку WinSock
	{
		cout << "Error: WSAStartup!" << endl;
		return 1;
	}
	cout << "Done!" << endl << "Create Socket..." << endl;
	s = socket(AF_INET, SOCK_STREAM, 0); // создать гнездо
	if (s == INVALID_SOCKET)
	{   // в случае неудачи - выдать сообщение и выйти
		cout << "Error: socket" << endl;
		WSACleanup();
		return 1;
	}
	cout << "Done!" << endl << "Binding..." << endl;
	sa.sin_family = AF_INET; // заполнение структуры данных
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) // получение локального адреса
	{
		// в случае неудачи - выдать сообщение, закрыть гнездо и выйти
		cout << "Error: bind!" << endl;
		closesocket(s);
		WSACleanup();
		return 1;
	}
	cout << "Done!" << endl << "Listening..." << endl;
	if (listen(s, 5) == SOCKET_ERROR) // перевести гнездо в состояние ожидания
	{
		cout << "Error: Listen!" << endl;
		closesocket(s);
		WSACleanup();
		return 1;
	}
	cout << "Done!" << endl << "Accepting..." << endl;
	while (1) // цикл приема соединений
	{
		a = sizeof(sockaddr_in);
		sw = accept(s, (sockaddr*)&sa, &a); // принять соединение
		if (sw == INVALID_SOCKET)
		{
			cout << "Error: Accept!" << endl;
			closesocket(s);
			WSACleanup();
			return 0;
		}
		a = sizeof(sockaddr_in);
		if (getpeername(sw, (sockaddr*)&sa, &a) != SOCKET_ERROR) // попытаться получить адрес клиента

			cout << inet_ntop(AF_INET, &sa.sin_addr, addr_str, 20) << endl;

		// запустить в отдельной нити функцию обслуживания клиента
		if (!CreateThread(NULL, 0, ConnectHandler, (void*)sw, 0, NULL))
		{
			cout << "Error: beginthread!" << endl;
			closesocket(s);
			WSACleanup();
			return 0;
		}
	}
	closesocket(s);
	WSACleanup();
	return 0;
}

// функция обслуживания клиента
DWORD WINAPI ConnectHandler(LPVOID _In_ ptr)
{
	using std::string;
	using std::cout;
	using std::cin;
	using std::endl;
	SOCKET s; // гнездо, с которым будет вестись обмен
	int len; // для служебных целей
	char buf[REC_SIZE] = { 0 }; // буфер для данных
	string mes;

	s = (SOCKET)ptr; // присвоение номера гнезда

	len = recv(s, buf, REC_SIZE, 0); // прием данных от клиента
	if (len == SOCKET_ERROR) // проверка на ошибку
	{
		cout << "Error: recv!" << endl;
		closesocket(s);
		return 1;
	}

	cout << "Message: " << buf << endl;
	mes = string("Your messeage \"") + buf + "\" was received!";
	if (send(s, mes.c_str(), mes.size(), 0) == SOCKET_ERROR) // отправка данных
	{
		cout << "Error: send!" << endl;
		closesocket(s);
		return 1;
	}

	closesocket(s); // закрытие гнезда
	cout << "Client was served!" << endl; // выдача сообщения
	return 0;
}