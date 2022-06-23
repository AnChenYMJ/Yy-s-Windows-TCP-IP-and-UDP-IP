#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//����socket�ŵ�ȫ�֣��������˳��������ã�ע���������ﲻҪ��д�����ˣ����ж�����
SOCKET SerClient;

BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(SerClient);
		WSACleanup();
		break;
	}
	return 0;
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
	//����SOCKET ��UDP�ģ�
	SerClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == SerClient)
	{
		int a = WSAGetLastError();
		printf("����SOCKET error: %d\n", a);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//�����������Ķ˿ںš�IP��ַ�ṹ��
	struct sockaddr_in stServer;
	stServer.sin_family = AF_INET;
	stServer.sin_port = htons(22258);
	stServer.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");


	//�����뷢��
	while (1)
	{
		//sendto������Ϣ
		char str1[548] = {0};
		gets(str1);
		int snum = sendto(SerClient, str1, strlen(str1), 0, (const struct sockaddr*)&stServer, sizeof(stServer));
		if (SOCKET_ERROR == snum)
		{
			int a = WSAGetLastError();
			printf("recvfrom error: %d\n", a);
		}
		//recvfrom������Ϣ
		struct sockaddr stServer;	//����5�����ڼ�¼�Է�socket���ͻ��ˣ��Ķ˿����ַ
		int nlen = sizeof(stServer);//����6����5�ṹ���С
		char cbuf[547] = {0};		//��2���ַ����飬��Ž��յ�����Ϣ
		int rnum = recvfrom(SerClient, cbuf, 547, 0, &stServer, &nlen);
		if (SOCKET_ERROR == rnum)
		{
			int a = WSAGetLastError();
			printf("recvfrom error: %d\n", a);
		}
		printf("  %s\n", cbuf);
	}

	//�ͷ�socket���ر������
	closesocket(SerClient);
	WSACleanup();

	system("pause>0");
	return 0;
}
