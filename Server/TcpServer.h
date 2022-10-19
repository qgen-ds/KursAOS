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
	static const size_t RECV_SIZE = 4096; // ������ ������ I/O
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
	SOCKET Socket;																				// �������� ������, �� ������� ����������� ����������
	int Backlog;																				// ������ ������
	HANDLE hInternalEvent;																		// ������� ���������� �����������
	unsigned short Port;																		// ���� ��������� ���������
	HANDLE hAcceptor;																			// ����������� �����
	HANDLE hWorker;																				// ������� �����
	std::list<ClientInfo> ClientList;															// ������ �������� ��������
	id_t LastAvailableID;																		// ��������� ��������� ��� ���������� ID
	std::vector<WSAEVENT> Events;																// ������ ������� ����
	size_t MaxClients;																			// ������������ ����� ��������
	HANDLE Lock;																				// ����� ������ ��������
	void Broadcast(std::vector<WSABUF>& IOBuf);													// ������� �������� ��������� ���� ������������ ��������
	void SendPrivate(id_t R, const ClientInfo& Sender, std::vector<WSABUF>& IOBuf);
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);										// ������� ������������ �������
	static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);											// ������� �������� ����������
	static void ValidatePacket(const ClientInfo& Sender, const wstring& s);						// ������� �������� ���������������� ������
	static void AppendSenderAddr(const ClientInfo& Sender, std::vector<WSABUF>& V, char* buf);	// ������� ���������� � ������������ ������ ������ �����������
	void UpdateID();																			// ������� ���������� LastAvailableID
	void DisconnectGeneric(std::list<ClientInfo>::iterator cl_it, DWORD Index);
	void DisconnectByIndex(DWORD Index);														// ������� ���������� ������������� �� ������� � ������
	void DisconnectByID(id_t ID);																// ������� ���������� ������������� �� ID
	void PrintClientList();
	auto FindByID(id_t ID);
#ifdef _DEBUG
	static void PrintNewClient(const ClientInfo& ci);
	static void PrintMessage(const ClientInfo& Sender, const wstring& s);
#endif // _DEBUG
};