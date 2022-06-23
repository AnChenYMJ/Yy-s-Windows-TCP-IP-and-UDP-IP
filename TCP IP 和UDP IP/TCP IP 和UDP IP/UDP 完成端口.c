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
HANDLE hPort;		//��ɶ˿ڴ�������

//Ͷ�ݺ�������
int PostRecvFrom();
int PostSendTo(struct sockaddr_in* se);
//�̺߳����������������߳�ʱʹ��
DWORD WINAPI ThreadProc(LPVOID lpParameter);
//�����̺߳���ѭ���ı���
BOOL THread = TRUE;

//����̨����˳���������
BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		THread = FALSE;				//�����̺߳����ڵ�ѭ��
		closesocket(SerVerSocket);
		WSACloseEvent(wol.hEvent);	//ע��رյ��ǽṹ���е��¼�����
		CloseHandle(hPort);			//�ر���ɶ˿ڣ�������ϵͳ����
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

	//������ɶ˿�
	hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (0 == hPort)
	{
		int a = WSAGetLastError();
		printf("��ɶ˿ڴ���ʧ�� error: %d\n", a);
		WSACloseEvent(wol.hEvent);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//����ɶ˿�
	hPort = CreateIoCompletionPort((HANDLE)SerVerSocket, hPort, SerVerSocket, 0);
	if (0 == hPort)
	{
		int a = WSAGetLastError();
		printf("��ɶ˿ڰ�socketʧ�� error: %d\n", a);
		WSACloseEvent(wol.hEvent);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//�����߳�
		//��ȡcpu����
	SYSTEM_INFO gsi;	//����һ���ṹ��
	GetSystemInfo(&gsi);//��ȡ��cpu��һЩ��Ϣ�����ص�����ṹ����
	int cpuNum = gsi.dwNumberOfProcessors;	//�ӽṹ���л�ȡ����
		//��ʼ�����߳�
	for (int i = 0; i < cpuNum; i++)
	{
		DWORD ThreadID;	//�߳�ID��β����
		if (NULL == CreateThread(NULL, 0, &ThreadProc, 0, 0, &ThreadID))
		{
			//�����ǰ����̴߳���ʧ��,��i--����
			int a = WSAGetLastError();
			printf("�����߳�ʧ�� error: %d\n", a);
			i--;
		}	
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
	//Ͷ��send��Ҫ�õ��ͻ���IP��˿ںţ�������recv���棬recvͶ�ݵĺ����ܻ�ȡ
	
	//ѭ����ס�����߳�����
	while (1)
	{
		//ÿ��ִ�У����������̹߳���2000ms
		Sleep(2000);	
	}


	//�ͷ�socket���ر������
	WSACloseEvent(wol.hEvent);		//����˳�����ҲҪ��������ͷ��¼���������ر��¼�����
	closesocket(SerVerSocket);
	CloseHandle(hPort);				//�ر���ɶ˿�
	WSACleanup();

	system("pause>0");
	return 0;
}

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
	int nRse = WSARecvFrom(SerVerSocket, &recvBuf1, 1, &NumberOfBytesRecvd, &Flags, (struct sockaddr*)&si, &nLen, &wol, NULL);
	//���̺߳�����
	if (0 != nRse)
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
	int nRse = WSASendTo(SerVerSocket, &recvBuf, 1, &NumberOfBytesRecvd, 0, (struct sockaddr*)se, ilen, &wol, NULL);
	//��ɶ˿�������ζ����̺߳������������������������������
	if(0 != nRse)
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
//�̺߳����������߳�ʱ�Ĳ���3
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	DWORD NumberOfByte;			//����2�����ջ��߷��͵��ֽ���
	ULONG_PTR CompletionKey;	//����3����ɶ˿ں����Ĳ���3�ᴫ��ֵ�����������
	WSAOVERLAPPED* lpwol;		//����4�����ص��ṹ��ַ�ĵ�ַ������ָ�룬�Ὣ��ǰ֪ͨ���ص��ṹ��ַ���ص��������
	int a = 0;
	//��һ����ѭ����һֱ��֪ͨ����ȡ֪ͨ������
	while (THread)
	{
		//���ʧ����
		if (FALSE == GetQueuedCompletionStatus(hPort, &NumberOfByte, &CompletionKey, &lpwol, INFINITE))
		{
			continue;
		}
		//���ṹ������¼��ÿ�
		WSAResetEvent(lpwol->hEvent);
		//�ɹ��ˣ����з��ദ��
			//�ͻ��˷��ͻس�������������send���ͻ��˳ɹ�
		if (ReBuf[0] == 0)
		{
			printf("send�ص����� ok\n");

		}
			//���յ��˿ͻ�����Ϣ
		else
		{
			printf("%s \n", ReBuf);
			PostSendTo(&si);			//Ͷ��send
			memset(ReBuf, 0, 548);		//��0
			PostRecvFrom();

		}
	}
	return 0;
}

void study()
{
	//7.1 ��ɶ˿�֪ʶ�ع�
	{
		/*
		* ֪ʶ�عˣ�
		*		��ϸ��֪ʶ�㿴�μ�
		*		��ɶ˿�Ҳ��windows��һ�ֻ��ƣ�
		*		��ɶ˿ڵ��ص㣺
		*			1.ģ����Ϣ���У�����һ��֪ͨ���У�ϵͳ������ά��
		*			2.�������Ч�ʵ��̣߳������������������CPU����
		*		������������ƣ��лص������Ļ��ƣ��Ͳ���������ɵ�������·�������߻ص���������·
		*		����ɶ˿�Ҳ��һ���ģ������¼�֪ͨ�����е�������ɣ�������֪ͨ���е�·��
		*	
		*		Ͷ�ݵĺ�����RecvFrom�ȣ������źź�����¼�֪ͨ����֪ͨ�洢��֪ͨ�����У�Ȼ������ȡ�����������߳�����������Щ�¼�
		*			�����˳��Ч�ʣ��߳�����ƿ����
		*		��ɶ˿ھ������Ϊ������еĶ���ͷ
		*		
		* ���벿�֣�
		*		�����Ƕ�UDP�ص�IO�¼�֪ͨ�ĸĶ�
		*		��Ҫ����޸ģ�
		*			������ɶ˿�
		*			����ɶ˿�
		*			�����߳�
		* 
		*			�������е�ѭ����Sleep����ס���˺���ִ��ʱ�����Զ��������̹߳��𣬲�ռ��cpuʱ�䣬���Ч��
		*		
		*/
	}
	//7.2 ������ɶ˿ڲ���
	{
		/*
		*	֪ʶ�㣺
		*		��ɶ˿�Ҳ��windows��һ�ֻ��ƣ���������ר�����ļ�����������
		*		���������ɶ˿ڵĺ�����ͬһ����ֻ�ǲ�����ͬ�����鿴�μ�
		*		�������Ի���Ĭ�ϣ�һ������0
		*		����������ɶ˿�Ҫ�ǵ��ͷţ�ʹ��CloseHandle�����ͷ�
		*		
		*	����ֵ��
		*		�ɹ���	��2ΪNULL�������µ���ɶ˿�
		*				��2��ΪNULL�������Լ�
		*				��1Ϊsocket�����ذ󶨺����ɶ˿�
		*/
	}
	//7.3 �����߳�
	{
		/*
		*	�����̺߳�����
		*		���鿴�μ���������������������඼�н���
		*		����5��һ��������úܴ��õ���࣬���ǽ�����̲߳���ʱ������Ǹ���
		*		�߳�IDҪ�˽�,�߳�ID���߳̾����һ������
		*		����4�Ǹ��̺߳�������3������ֵ�����ã�������ʱ��0����
		*		����ֵ��
		*			�ɹ������߳̾�����ں˶��󣬽�β��Ҫ�ͷţ���CloseHandle������
		*			ʧ�ܷ���NULL
		*			
		*	֪ʶ�㲹�䣺
		*		ջ�ı�����С�ܹ��޸ģ�Ĭ��Ϊ1M
		*		�����߳�ǰҪ�Ȼ�ȡ����߳���
		*		�̺߳��� �� �����̺߳����Ĳ���3 �� ͬʱ���̺߳����ǻص�������һ��
		*		ֻҪ�ǻص�������д�����ǹ̶��ģ���������Լ��������������ֵ��
		*/
	}
	//7.4 �̺߳����ڲ������൱�ڻص������ڲ���
	{
		/*
		*	Ҫ�㣺
		*		��ΪUDP �� TCP��ģ�ͽṹ��һ���ģ�ֻ�Ƕ�Ӧ�����Ĳ�����������
		*		�����Ҿ�û��д̫��ʼǣ������TCP���Ѿ��ǹ�һ���ˣ����ҿμ�д�ĺ���ϸ�����μ�����
		*		���������һ����ֻҪд�˻ص������������̺߳��������Ǿ�һ�����߻ص���������·�ߣ��¼�֪ͨģ�͵�������������߿�����������
		*			��Ϊ������������ɣ�Ȼ���������̺߳���
		*		��Ϊ����������ѭ����ס������Ҫ�رգ����ڿ���̨���ڹرպ����������һ���̺߳�������ѭ���Ŀ��ƣ������true��Ϊfalse
		* 
		*	�̺߳����ڲ���
		*		���Դ�ָ����I / O��ɶ˿ڳ���I / O������ݰ���GetQueuedCompletionStatus����
		* 
		*/
	}
	


}
