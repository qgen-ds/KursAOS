#pragma once
#include "pch.h"

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
	void Start();
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
		Running,
		RequestedForStop
	} ServerStatus;
	DWORD DeletedIndex;																			// ������ ���������� ��������� �������
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
	void Broadcast(const wstring& msg);															// ������� �������� ��������� ���� ������������ ��������
	void SendPrivate(const ClientInfo& Sender, wstring& msg);									// ������� �������� ������� ���������
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);										// ������� ������������ �������
	static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);											// ������� �������� ����������
	void ValidatePacket(const ClientInfo& Sender, const wstring& msg);							// ������� �������� ���������������� ������
	static void AppendSenderInfo(const ClientInfo& Sender, wstring& msg);						// ������� ���������� � ������������ ������ ������ �����������
	void UpdateID();																			// ������� ���������� LastAvailableID
	void DisconnectGeneric(std::list<ClientInfo>::iterator cl_it, DWORD Index);
	void HandleData(std::vector<WSABUF>& IOBuf, const ClientInfo& Sender);
	void DisconnectByID(id_t ID);																// ������� ���������� ������������� �� ID
	void PrintClientList();																		// ������� ������ �� ����� ������ ������������ ��������
	std::list<ClientInfo>::iterator FindByID(id_t ID);											// ������� ���������� ������� �� ��� ID
#ifdef _DEBUG
	static void PrintNewClient(const ClientInfo& ci);
	static void PrintMessage(const ClientInfo& ci, const wstring& s);
#endif // _DEBUG
};