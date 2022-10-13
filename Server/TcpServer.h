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
	SOCKET Socket;											// �������� ������, �� ������� ����������� ����������
	int Backlog;											// ������ ������
	HANDLE hAcceptEvent;									// �������, ������������ ��� �������� ������ �����������
	unsigned short Port;									// ���� ��������� ���������
	HANDLE hAcceptor;										// ����������� �����
	HANDLE* hWorkers;										// ������ ������� �������
	size_t WorkersCount;									// ����� �������
	std::list<ClientInfo> ClientList;						// ������ �������� ��������
	size_t MAX_CL_COUNT;									// ������������ ����� ��������
	HANDLE Lock;											// ����� ������ ��������
	wchar_t err[ERR_SIZE] = {0};							// ����� ��� ��������� �� �������
	static DWORD CALLBACK ClientObserver(LPVOID _In_ p);	// ������� ������������ �������
	//static DWORD CALLBACK AcceptLoop(LPVOID _In_ p);		// ������� �������� ����������
	void StartAcceptLoop();
	void Broadcast(const char* msg, int len);				// ������� �������� ��������� ���� ������������ ��������
	void Broadcast(std::vector<WSABUF>& IOBuf);				// ������� �������� ��������� ���� ������������ ��������
};

