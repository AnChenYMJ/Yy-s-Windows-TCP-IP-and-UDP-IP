#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>			//strlen��ͷ�ļ�
#include <WinSock2.h>		//����ͷ�ļ�
#pragma comment(lib, "WS2_32.lib")


int main(void)
{
	//�������
	WORD wsVersion = MAKEWORD(2,2);
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
	//����SOCKET
	int socketserver = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketserver == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//socket�󶨶˿����ַ
	struct sockaddr_in st;
	st.sin_family = AF_INET;	//��ַ����
	st.sin_port = htons(22258);	//�˿�		htonsת���������ֽ���
	st.sin_addr.S_un.S_un_b.s_b1 = 127;	//����IP��ַ�ķ�ʽ1����4�����루��Ϊ�����ϣ�
	st.sin_addr.S_un.S_un_b.s_b2 = 0;
	st.sin_addr.S_un.S_un_b.s_b3 = 0;
	st.sin_addr.S_un.S_un_b.s_b4 = 1;
	//st.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");	//��ַ
	int brse = bind(socketserver, (const struct sockaddr*)&st, sizeof(st));
	if (brse == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(socketserver);
		WSACleanup();
		return 0;
	}
	//��������
	int lise = listen(socketserver, SOMAXCONN);
	if (lise == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(socketserver);
		WSACleanup();
		return 0;
	}
	//�������ӣ��������Ǵ����ͻ���socket��
	struct sockaddr_in sockClient;
	int len = sizeof(sockClient);
	SOCKET socketclient = accept(socketserver, (struct sockaddr *)&sockClient, &len);
	if (socketclient == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		closesocket(socketserver);
		WSACleanup();
		return 0;
	}
	//��ͻ����շ���Ϣ
	char buf[1500] = { 0 };
	while (1)
	{
		//������Ϣ
		int rec = recv(socketclient, buf, 1499, 0);
		if (rec == 0)
			printf("�Ͽ����ӡ��ͻ�������");
		else if (rec == SOCKET_ERROR)
		{
			int a = WSAGetLastError();
			//���ùر�����⣬��Ϊֻ��һ���ͻ��˶Ͽ�
		}
		else
			printf("%d   %s\n", rec, buf);

		//���ͻ��˷�����Ϣ
		int s = scanf("%s", buf);
		int sed = send(socketclient, buf, strlen(buf), 0);
		if (sed == SOCKET_ERROR)
		{
			int a = WSAGetLastError();
		}
	}
	

	//���������
	closesocket(socketserver);
	closesocket(socketclient);
	WSACleanup();


	system("pause>0");
	return 0;
}