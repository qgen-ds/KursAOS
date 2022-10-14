// Server.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "ClientInfo.h"
#include "TcpServer.h"

void InitWSA();
unsigned short strtous(const char* str);

int main(int argc, char** argv)
{
	using std::string;
	using std::cout;
	using std::cin;
	using std::endl;
	unsigned short port = 3030;
	size_t maxclients = 25;
	int backlog = 5;
	string cmd;
	TcpServer* Serv;
	try
	{
		for (int i = 1; i < argc; i++)
		{

			if (argv[i][0] == '-') // check for an option marker
			{
				if (!strcmp((argv[i] + 1), "port"))
				{
					port = strtous(argv[++i]);
				}
				else if (!strcmp((argv[i] + 1), "maxclients"))
				{
					maxclients = std::strtoul(argv[++i], nullptr, 10);
				}
				else if (!strcmp((argv[i] + 1), "backlog"))
				{
					backlog = std::strtol(argv[++i], nullptr, 10);
				}
			}
		}
		InitWSA();
		Serv = new TcpServer(port, maxclients, backlog);
		Serv->Init();
		Serv->Start();
	}
	catch (std::exception e)
	{
		cout << "Exception in "
			<< __FUNCTION__
			<<  ". Message: "
			<< e.what()
			<< endl
			<< '>';
		return 1;
	}
	while (true)
	{
		cout << ">";
		std::getline(cin, cmd);
		cout << endl;
		cin.clear();
		fflush(stdin);
		if (cmd == "exit")
			break;
		//TODO: разбор команд
	}
	delete Serv;
	WSACleanup();
	return 0;
}

unsigned short strtous(const char* str)
{
	unsigned long ret = std::strtoul(str, nullptr, 10);
	if (errno == ERANGE)
		throw std::range_error(std::string("std::strtoul failed in ") + __FUNCTION__);
	if(ret > USHRT_MAX)
		throw std::range_error("value out of USHORT range");
	return (unsigned short)ret;
}

void InitWSA()
{
	using std::cout;
	using std::endl;
	WSADATA wsd;
	int iNum;
	cout << "Initializing WinSock..."
		 << endl;
	if ((iNum = WSAStartup(MAKEWORD(2, 2), &wsd)) != 0)
	{
		throw std::runtime_error(std::string("WSAStartup error. Code: ") + std::to_string(iNum));
	}
}