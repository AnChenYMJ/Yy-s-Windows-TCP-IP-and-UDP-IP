#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>			//strlen��ͷ�ļ�
#include <WinSock2.h>		//����ͷ�ļ�
#pragma comment(lib, "WS2_32.lib")


int main(void)
{
	//�������
	WORD wsVersion = MAKEWORD(2, 2);
	WSADATA wssockMsg;
	int nRse = WSAStartup(wsVersion, &wssockMsg);
	if (nRse != 0)
	{
		switch (nRse)
		{
		case WSASYSNOTREADY:
			printf("�ײ�������ϵͳ��δ׼���ý�������ͨ��, ϵͳ�������⣬�����µ��ԣ����ws2_32���Ƿ���ڣ�C�̲���ws2_32��");
			break;
		case WSAVERNOTSUPPORTED:
			printf("Ҫʹ�õİ汾��֧��,����汾�����������");
			break;
		case WSAEPROCLIM:
			printf(" �Ѵﵽ��Windows�׽���ʵ��֧�ֵ��������������ƣ���65536���˿�ȫ�����ˡ��볢�Թرղ���Ҫ�������Ϊ��������ṩ�������Դ");
			break;
		case WSAEINPROGRESS:
			printf(" ��ǰ���������ڼ䣬����ĳЩԭ������������᷵������������롣�������������");
			break;
		case WSAEFAULT:
			printf(" lpWSAData����������Чָ�롣����Ա�Լ������⣬��������WSAStartup�Ĳ���2д����");
			break;

		}
	}
	//У��汾
	if (2 != HIBYTE(wssockMsg.wVersion) || 2 != LOBYTE(wssockMsg.wVersion))
	{
		printf("�����汾����");
		WSACleanup();
		return 0;
	}
	//����SOCKET(����˵���)
	int socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketServer == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	//���ӵ�������
	struct sockaddr_in Server;
	Server.sin_family = AF_INET;
	Server.sin_port = htons(22258);
	Server.sin_addr.S_un.S_un_b.s_b1 = 127;	//����IP��ַ�ķ�ʽ1����4�����루��Ϊ�����ϣ�
	Server.sin_addr.S_un.S_un_b.s_b2 = 0;
	Server.sin_addr.S_un.S_un_b.s_b3 = 0;
	Server.sin_addr.S_un.S_un_b.s_b4 = 1;
	//Server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int co = connect(socketServer, (struct sockaddr*)&Server, sizeof(Server));
	if (co == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	printf("������������ \n");
	//������������շ�
	char ca[1500] = {0};
	/*int sen = send(socketServer, "��Ϣ�ѷ���", sizeof("��Ϣ�ѷ���"), 0);
	if (sen == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
	}*/
	while (1)
	{
		////������Ϣ
		//int rev = recv(socketServer, ca, 1499, 0);
		//if (rev == 0)
		//	printf("�Ͽ����ӡ��ͻ�������");
		//else if (rev == SOCKET_ERROR)
		//{
		//	int a = WSAGetLastError();
		//	//���ùر�����⣬��Ϊֻ��һ���ͻ��˶Ͽ�
		//}
		//else
		//	printf("%d   %s\n", rev, ca);


		//������������һ������
		int s = scanf("%s", ca);
		if (ca[0] == '0')	//���� 0 ���˳�ѭ����Ϊ����selectģ���в��������رտͻ���
			break;
		int sen1 = send(socketServer, ca, strlen(ca), 0);
		if (sen1 == SOCKET_ERROR)
		{
			int a = WSAGetLastError();
		}
		
	}
	



	closesocket(socketServer);
	WSACleanup();


	system("pause>0");
	return 0;
}