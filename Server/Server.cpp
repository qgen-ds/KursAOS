// Server.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "ClientInfo.h"
#include "TcpServer.h"

#pragma once
#pragma comment (lib,"wsock32.lib")
#pragma comment (lib, "ws2_32.lib")

//std::list<ClientInfo> SharedV;
//HANDLE VLock; 
//HANDLE hSharedQMutex; 
//CRITICAL_SECTION QLock;
//CONDITION_VARIABLE QEmpty;
//bool StopRequested;

void InitWSA();

int main(int argc, char** argv)
{
	//TODO: Cmdline parser
	// общение с клавиатурой
	//
	using std::string;
	using std::cout;
	using std::cin;
	using std::endl;
	int iNum = 0;
	TcpServer* Serv;
	cout << "Initializing WinSock..." << endl;
	try
	{
		InitWSA();
		Serv = new TcpServer();
		Serv->Init();
		Serv->Start();
	}
	catch (WCHARException e)
	{
		e.Show();
		return 1;
	}
	delete Serv;
	WSACleanup();
	return 0;
}

void InitWSA()
{
	WSADATA wsd;
	int iNum;
	wchar_t txt[50] = { 0 };
	if ((iNum = WSAStartup(MAKEWORD(2, 2), &wsd)) != 0)
	{
		swprintf_s(txt, 50, L"%s%i", L"WSAStartup error. Code: ", iNum);
		throw WCHARException(txt);
	}
}