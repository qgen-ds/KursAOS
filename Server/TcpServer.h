#pragma once
#include "pch.h"
#include "ClientInfo.h"

class TcpServer
{
public:
	static const size_t RECV_SIZE = 4096; // ������ ������ I/O
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
	SOCKET Socket;											// �������� ������, �� ������� ����������� ����������
	int Backlog;											// ������ ������
	HANDLE hInternalEvent;									// ������� ���������� �����������
	unsigned short Port;									// ���� ��������� ���������
	HANDLE hAcceptor;										// ����������� �����
	HANDLE hWorker;											// ������� �����
	std::list<ClientInfo> ClientList;						// ������ �������� ��������
	std::vector<WSAEVENT> Events;							// ������ ������� ����
	size_t MaxClients;										// ������������ ����� ��������
	HANDLE Lock;											// ����� ������ ��������
	void Broadcast(const char* msg, int len);				// ������� �������� ��������� ���� ������������ ��������
	void Broadcast(std::vector<WSABUF>& IOBuf);				// ������� �������� ��������� ���� ������������ ��������
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);	// ������� ������������ �������
	static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);		// ������� �������� ����������
	void DisconnectByIndex(DWORD Index);
};

