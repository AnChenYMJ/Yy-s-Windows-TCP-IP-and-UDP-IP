#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//����socket���¼��ŵ�ȫ�֣��������˳��������ã�ע���������ﲻҪ��д�����ˣ����ж�����
SOCKET SerVerSocket;//socket
WSAOVERLAPPED wol;	//�ص�IO�Ľṹ�壬Ҫ��socket�󶨵��Ǹ�
struct sockaddr_in si;
char ReBuf[548] = { 0 };
//WSABUF recvBuf1;

//Ͷ�ݺ�������
int PostRecvFrom();
int PostSendTo(struct sockaddr_in* se);
//�ص���������
void CALLBACK OnRecv(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
void CALLBACK OnSend(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

//����̨����˳���������
BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(SerVerSocket);
		WSACloseEvent(wol.hEvent);	//ע��رյ��ǽṹ���е��¼�����
		WSACleanup();
		return TRUE;
		break;
	}
	return FALSE;
}

int main(void)
{
	//��ϵͳͶ��һ�����ӣ�����˳��ļ��ӣ�
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
	//�������
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	int nrse = WSAStartup(WSVersion, &wsaData);
	if (0 != nrse)
	{
		int a = WSAGetLastError();
		printf("WSAStratup error: %d\n", a);
		system("pause>0");
		return 0;
	}
	//У��汾
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		int a = WSAGetLastError();
		printf("�����汾����ȷ error: %d\n", a);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//����SOCKET ��UDP���ص�IOʹ�õ�socket��WSASocket���������������ŵ�ȫ�֣��������˳��������ã�
	SerVerSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == SerVerSocket)
	{
		int a = WSAGetLastError();
		printf("����SOCKET error: %d\n", a);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//�󶨶˿ں����ַ
	struct sockaddr_in sd;
	sd.sin_family = AF_INET;
	sd.sin_port = htons(22258);
	sd.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (SOCKET_ERROR == bind(SerVerSocket, (SOCKADDR*)&sd, sizeof(sd)))
	{
		int a = WSAGetLastError();
		printf("bind error: %d\n", a);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//�����¼������ǽṹ���е��¼�����
	wol.hEvent = WSACreateEvent();
	if (WSA_INVALID_EVENT == wol.hEvent)
	{
		int a = WSAGetLastError();
		printf("�����¼����� error: %d\n", a);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//��socket�������
	int wsaes = WSAEventSelect(SerVerSocket, (HANDLE)wol.hEvent, FD_READ | FD_WRITE);
	if (SOCKET_ERROR == wsaes)
	{
		int a = WSAGetLastError();
		printf("���¼���socket��Ͷ�� error: %d\n", a);
		WSACloseEvent(wol.hEvent);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//Ͷ��recv,����0������������Զ���ģ�
	if (0 == PostRecvFrom())
	{
		int a = WSAGetLastError();
		printf("Ͷ�ݺ���PostRecvFrom error: %d\n", a);
		WSACloseEvent(wol.hEvent);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//�ȴ��ź�Ȼ����з��ദ��
	while (1)
	{
		//ѭ�����źţ��������β��Ҫ��TRUE
		int nRse = WSAWaitForMultipleEvents(1, &wol.hEvent, FALSE, WSA_INFINITE, TRUE);
		//������
		if (WSA_WAIT_FAILED == nRse)
		{
			continue;
		}
		//һ��������̳ɹ�ִ���꣬�򷵻����ֵ
		if (WSA_WAIT_IO_COMPLETION == nRse)
		{
			continue;
		}
	}


	//�ͷ�socket���ر������
	WSACloseEvent(wol.hEvent);		//����˳�����ҲҪ��������ͷ��¼���������ر��¼�����
	closesocket(SerVerSocket);
	WSACleanup();

	system("pause>0");
	return 0;
}
//�ص�����
void CALLBACK OnRecv(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	//�����ź�
	WSAResetEvent(lpOverlapped->hEvent);
	//�����벻Ϊ0���������
	if (0 != dwError)
	{
		printf("Recv�ص����� ������\n");
		return;
	}
	//������ջ����ֽ�����Ϊ0,��Ϊ�յ�����Ϣ
	if (0 != cbTransferred)
	{
		printf("%s \n", ReBuf);
		memset(ReBuf, 0, 548);
		PostSendTo(&si);
		PostRecvFrom();
		return;
	}
	//���Ϊ0�������ǿͻ��������˻س�
	else if (0 == cbTransferred)
	{
		printf("%s \n", ReBuf);
		memset(ReBuf, 0, 548);
		PostSendTo(&si);
		PostRecvFrom();
		return;
	}
}
void CALLBACK OnSend(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	printf("send �ص�����success\n");
}
//Ͷ�ݺ����߼���װ
int PostRecvFrom()
{
	//����2��������Ϣ������Ľṹ��
	//char ReBuf[548] = {0};	//������Ҫ��������
	WSABUF recvBuf1;
	recvBuf1.len = 547;		//��һ���ռ��\0
	recvBuf1.buf = ReBuf;
	//����4�����ճɹ��Ļ�������װ�ųɹ����յ����ֽ���
	DWORD NumberOfBytesRecvd = 0;
	//����5���кü������ͣ�UDP��0����,��Ҫ����������ֵ0��Ȼ��ַ
	DWORD Flags = 0;
	//����6��װ�ͻ��˵�IP��ַ�Ͷ˿ں�,����7������6�Ĵ�С
	//struct sockaddr_in si;
	int nLen = sizeof(si);
	//Ͷ��recv����
	int nRse = WSARecvFrom(SerVerSocket, &recvBuf1, 1, &NumberOfBytesRecvd, &Flags, (struct sockaddr*)&si, &nLen, &wol, OnRecv);
	//�������
	if (0 == nRse)
	{
		printf("%s \n", recvBuf1.buf);
		PostSendTo(&si);			//Ͷ��send
		memset(ReBuf, 0, 548);		//��0
		PostRecvFrom();
	}
	//���� �� �ӳ����
	else
	{
		int a = WSAGetLastError();
		//�ӳ����
		if (WSA_IO_PENDING == a)
		{
			//��������1������ӳ����
			return 1;
		}
	}
	//��������0����������
	return 0;
}
int PostSendTo(struct sockaddr_in* se)
{
	//����2��������Ϣ������Ľṹ��
	//char ReBuf[548] = { 0 };	//������Ҫ��������
	WSABUF recvBuf;
	recvBuf.len = 547;		//��һ���ռ��\0
	recvBuf.buf = "ok \n";
	//����4�����ͳɹ��Ļ�������װ�ųɹ����͵��ֽ���
	DWORD NumberOfBytesRecvd = 0;
	//����5���кü������ͣ�UDP��0����,��Ҫ����������ֵ0��Ȼ��ַ
	DWORD Flags = 0;
	//����7������6�Ĵ�С
	int ilen = sizeof(struct sockaddr_in);
	int nRse = WSASendTo(SerVerSocket, &recvBuf, 1, &NumberOfBytesRecvd, 0, (struct sockaddr*)se, ilen, &wol, OnSend);
	//�������
	if (0 == nRse)
	{
		printf("send���ͳɹ����������� \n");
	}
	//���� �� �ӳ����
	else
	{
		int a = WSAGetLastError();
		//�ӳ����
		if (WSA_IO_PENDING == a)
		{
			//��������1������ӳ����
			return 1;
		}
	}
	//��������0����������
	return 0;
}
//�ʼ�
void study()
{
	//6.6 �ص�IO���������
	{
		/*
		*	����������¼�֪ͨ�������뿴�μ������ҶԱȴ������
		* 
		*	����������¼�֪ͨ�Ĵ����������޸ģ�
		*		1.д�ص�������Recv��Send��ʹ�ã��ص���������Ͷ�ݸ�ϵͳ�󣬼�⵽�źţ����Զ����ûص��������������Ƿ��ദ��
		*		2.WSARecvFrom��WSASendTo��Ͷ�ݺ�����β��������NULL�޸�Ϊ����д�õĻص�����
		*		3.�������е�ѭ���ڽ��¼�֪ͨ���߼�ȫɾ����Ȼ����ӵ��źź������߼�����
		*		4.���źź���β����Ҫ�޸ĳ�TRUE��ר���������ʹ�õ�
		* 
		*	֪ʶ�㣺
		*		1.�ԱȻص��������¼�֪ͨ�Ļ�ȡ�¼��������ᷢ�����ߵĲ�����һЩ��һ����
		*			˵���ص�������ʵ�����������ڻ�ȡ�¼��Ĺ��ܣ��������Զ����õ�
		*		2.�ص�������Ҫ�ǵó�ֵ�ź�
		*		3.�ĳ�������̵��߼�֮��PostSendTo���߼��ϣ��Ͳ�����������ˣ�����һֱ�߻ص���������·��
		*		4.�ص���������ʱ��CALLBACK�Ǻ���Լ��
		*		5.�ص������Ĳ������鿴�μ�������ϸ
		* 
		*	�������޸��꣬�ɹ��ܽ���ͨ�ţ�������Ȼ�س�����û�н��
		*		������Recv�Ļص����������һ���߼����ɣ����в�����Ƿ�������
		*
		*/
	}
}
