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
	//Ͷ��send��Ҫ�õ��ͻ���IP��˿ںţ�������recv���棬recvͶ�ݵĺ����ܻ�ȡ
	
	//�ȴ��ź�Ȼ����з��ദ��
	while (1)
	{
		//�ȴ��ź�
		if (WSA_WAIT_FAILED == WSAWaitForMultipleEvents(1, &wol.hEvent, FALSE, WSA_INFINITE, FALSE))
		{
			continue;
		}
		//��ȡ�¼�������ȡ��Ӧ�¼������ͣ�
		DWORD tr;		//��3���᷵�ط��ͻ���յ����ֽ���������
		DWORD Flags;	//��5��װWSARecv�Ĳ���5
		if (FALSE == WSAGetOverlappedResult(SerVerSocket, &wol, &tr, TRUE, &Flags))
		{
			int a = WSAGetLastError();
			continue;
		}
		//�����źţ���TCP������ıʼǣ�
		WSAResetEvent(wol.hEvent);
		//���ദ��
		if (tr >= 0)
		{
			//��������û�����ݣ�˵����send���߼�©�������ﻹ������recv���ͻ����ûس����ͣ�û���������ݣ�������recv��
			if (0 == ReBuf[0])
			{
				printf("send���ͳɹ����ӳٷ��� \n");
				PostSendTo(&si);	//����ͻ��˷��ͻ��У��س����������ڱʼ����֮�����߼�©����
				PostRecvFrom();		//��������recv������Ĳ��䣬���ͻ����ûس�����������Ǽ��������У���������ӳٷ��Ϳ����ֻ�������
			}
			//�������������ݣ�˵��recv
			else
			{
				printf("%s \n", ReBuf);
				PostSendTo(&si);			//Ͷ��send
				memset(ReBuf, 0, 548);		//��0
				PostRecvFrom();
			}

			//Ҳ������������װ�����ķ���ֵ�ж�
		}
	}


	//�ͷ�socket���ر������
	WSACloseEvent(wol.hEvent);		//����˳�����ҲҪ��������ͷ��¼���������ر��¼�����
	closesocket(SerVerSocket);
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
	int nRse = WSASendTo(SerVerSocket, &recvBuf, 1, &NumberOfBytesRecvd, 0, (struct sockaddr*)se, ilen, &wol, NULL);
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


void study()
{
	//6.1 �ص�IOģ�ͻ���֪ʶ
	{
		/*
		* ����֪ʶ��
		*		���鿴�μ����μ��ܽ�úܺã�������ͼ򵥵Ļع����Ҿ�����Ҫ�ģ�
		*			1.���壺�ص�IO��windows�ṩ��һ���첽��д�ļ��Ļ���
		*					recv��send�ڽ�Э�鴫������ݸ��Ƶ������еĹ��̣���ͬ���������ģ�3��recvҪ�ȵ�1����ɺ���ܽ���ʣ�µ�
		*					���ص�IO���ǽ��ļ���д�Ĺ��̣��ŵ��������ٵ��߳���ȥ�ȴ���ɣ��ﵽ�첽��Ч��
		*					�����ص�IO�Ƕ�C/Sģ�͵�ֱ���Ż�
		*			2.���ʣ�����һ���ṹ�壨WSAOVERLAPPED����socket��
		*			3.�Ա���⣺
		*					�첽ѡ��ģ�Ͱ���Ϣ��socket��һ��Ȼ��ϵͳ����Ϣ���ƴ�����
		*					�¼�ѡ��ģ�Ͱ��¼���socket��һ��Ȼ��ϵͳ���¼����ƴ�����
		*					�ص�IOģ�Ͱ��ص��ṹ��socket��һ��Ȼ��ϵͳ���ص�IO���ƴ�����
		*			�ص�IO������ʵ�ַ�ʽ��
		*					�¼�֪ͨ���ļ���д������ɺ��߳���Ľṹ��ͨ���¼��Ļ�����֪ͨ���߳�
		*					������̣��ص�������
		*			�����߼���
		*					������Ͷ�ݣ�Ȼ��ֳ�������ɻ��ӳ����
		*					�ӳ���ɾ�ѭ�����źţ������߼���TCP����ͬ
		*					��������UDPЭ����ص㣬����ҪͶ��AcceptEx������߼������
		*			ѭ�����źţ�WSAWaitForMultipleEvents����
		*					��������ܺ��ã��ڵȴ�ʱ�Ὣ�����߳���ʱ���𣬲�����CPU��ʱ�䣬����ʱ��ʧ����������Դ
		* ���䣺
		*			socketҪ��WSAsocket�������ص�IO��ʹ�õ�socket��һ���socket����
		*			�¼�����Ͳ��ýṹ���Աwol.hEvent���ɣ������赥������
		*	
		*/
	}
	//6.2 WSASocket�����Ľ���
	{
		/*
		* �˺����Ĳ������ͺܶ࣬��ʦ����35�������������Ĳ�������Щ�����������չ֪ʶ������ǿ��
		*		���鿴�μ���MSDN����⣬�μ��ܽ�ñȽϺ�
		*	������
		*		����1��2��3��Э��� ��ַ���͡��׽������͡�Э�����ͣ��˴�UDP���AF_INET��SOCK_STREAM��IPPROTO_TCP
		*		����4 ��NULL ���˴���ʹ�ã��˲���Ϊ�����׽������ԣ����鿴�μ���MSDN��
		*		����5 ��0 �˲����Ǳ���λ��һ��socket����ID���������һ�β������socket��
		*		����6 ��дWSA_FLAG_OVERLAPPED������һ�����ص�IOģ��ʹ�õ�socket���������͵Ŀμ��н���
		*	����ֵ��
		*		�ɹ����ؿ��õ�socket��������socket����ʹ���Ժ�ǵ��ͷţ��ú���closesocket
		*		ʧ�ܷ���INVALID_SOCKET
		*	
		*	��������̫���ˣ��ǲ�̫ס������TCP�Ŀγ�ѧ���ˣ�������MSDN�Ĳ������ܾͼ򵥵ö�
		*/
	}
	//6.3 WSARecvFrom������⣬�첽������Ϣ
	{
		/*
		*	������Ҫ��WSARecvFrom�Ĳ������뺯����ʹ�ã��μ�������ϸ
		*	����ԭ�Ϳμ�����
		* 
		*	������
		*		����1���ͻ���socket
		*		����2��������Ϣ������Ľṹ�壬������ԱΪ�����鳤�ȣ������ַ����������Ҫ��������Ȼ��ַ
		*		����3�����ղ���2�ĸ���������һ�νӼ�����������
		*		����4�����ճɹ��Ļ�������װ�ųɹ����յ����ֽ���
		*		����5���кü������ͣ�UDP��0���ɣ�������ֱ��д0����Ҫ����������ֵ0��Ȼ��ַ
		*		����6��װ�ͻ��˵�IP��ַ�Ͷ˿ںţ�sockaddr
		*		����7������6�Ĵ�С
		*		����8���ص��ṹ
		*		����9���ص�����ʹ�õģ��¼�֪ͨ��NULL����
		*	����ֵ��
		*		��������������0
		*		��������SOCKET_ERROR������WSA_IO_PENDINGΪ�ӳ����
		* 
		*	�˺�����Ӧ���߼���Ͷ�ݸú�����Ҫ������װ��һ��������Ȼ�����������ڰ�socket��ṹ��Ĳ����Ͷ��
		*	Ͷ�ݺ�����������TCP��ͬ��TCP�ĺ��в�������Ϊ��Ҫ֪���ͻ��˵�socket����UDPû�пͻ���socket���Ͳ�д
		* 
		*	
		*/
	}
	//6.4 WSASendTo�������,������Ϣ
	{
		/*
		*	WSASendTo�����Ĳ�����WSARecvFrom�Ĳ�������һ�����ɿ��μ�����
		* 
		*	ע��㣺recv�У�������Ϣ�����飬ÿ�ν����궼Ҫ��0����0�ķ�ʽ�ܶ࣬��Ҫ���ޣ����磺
		*		1.memset(ReBuf, 0, 548); �ú���������0
		*		2.ReBuf[0] = 0;  ���߸�ͷԪ��һ������ֵ��Ȼ��ʹ�õ�ʱ��ʶ�𼴿�
		*	
		*	Send��Ͷ���߼���recv���߼�����һ��
		*	����send��Ҫ֪���ͻ��˵�IP��˿ںŲ��ܷ�����Ϣ������һ����recv���·����׳��֣���Ϊrecv��Ͷ�ݺ����ܻ�ȡ�ͻ���IP��˿ں�
		*			
		*	���䣺recvĿǰ���õݹ飬�ɸĳ�ѭ����
		*/
	}
	//6.5 ѭ�����ź�
	{
		/*
		* ���β���㣺
		*		�˿�sendto������д��547һ��������ô�ͻ���recvҲҪ��д��547���������10040����
		*		���ڿͻ�����һ��һ�գ����Է�����ҲӦ��Ҫ����ͻ��˵�һ��һ��
		*		�ͻ����ܹ������� ���� �����˵�������if('\0' == Buf[0]) ������Ȼ��continue���ɣ������������д洢����\0 
		* 
		* ����
		*	һ��ʼ��10057�����룬����Ϊ�ҵĴ���socket�Ĳ���д��TCP��Э���ˣ�����Ϊ��ʦ�Ŀμ�����
		*		Ӧ��д��WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP.....�ĲŶԣ���UDPЭ���
		* 
		*	Ϊʲôһ��ʼ�ͻ����ûس��������벻�����أ����ҷ�����Ҳ�޷�Ӧ
		*		��Ϊ��������tr >= 0 ���ͻ���ܵ��ַ����жϣ�reBuf[0] == 0 ���ж��Ƿ��ͻ��ǽ��գ�
		*		����ͻ����ûس�����ô��ʵ�������ǽ���recv�Ŷԣ�����û�н��յ��������ѣ�reBuf[0] == 0����Ӧ�ø��ͻ���һ���������ͻ��˲��ܼ������ͣ�
		*		����Ϊ���ǵ��ж��߼����������ڷ��ദ���жϳ���send���ӳٷ��ͣ���û�и��ͻ��˻������ͻ��˿���recv�Ȼ�����״̬
		*		���������ڷ��ദ���send�߼��У����ͻ��˻�����������Ͷ��recv���ɣ�
		*		ԭ�����߼�����ȷ���յ�recv�����г���send���ӳٷ���
		*/
	}


}
